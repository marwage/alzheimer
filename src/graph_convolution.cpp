// Copyright 2020 Marcel Wagenländer

#include "graph_convolution.hpp"
#include "cuda_helper.hpp"
#include "divmv.h"


GraphConvolution::GraphConvolution(CudaHelper *helper, sparse_matrix<float> *adjacency, std::string reduction) {
    cuda_helper_ = helper;
    adjacency_ = adjacency;
    reduction_ = reduction;
    if (reduction_.compare("mean") == 0) {
        mean_ = true;
    } else if (reduction_.compare("sum") == 0) {
        mean_ = false;
    } else {
        std::cout << "Reduction not supported" << std::endl;
    }
}

matrix<float> GraphConvolution::forward(matrix<float> B) {
    float *d_A_csr_val;
    int *d_A_csr_row_offsets, *d_A_col_ind;
    check_cuda(cudaMalloc((void **) &d_A_csr_val,
                          adjacency_->nnz * sizeof(float)));
    check_cuda(cudaMalloc((void **) &d_A_csr_row_offsets,
                          (adjacency_->rows + 1) * sizeof(int)));
    check_cuda(cudaMalloc((void **) &d_A_col_ind,
                          adjacency_->nnz * sizeof(int)));
    check_cuda(cudaMemcpy(d_A_csr_val, adjacency_->csr_val,
                          adjacency_->nnz * sizeof(float), cudaMemcpyHostToDevice));
    check_cuda(cudaMemcpy(d_A_csr_row_offsets, adjacency_->csr_row_ptr,
                          (adjacency_->rows + 1) * sizeof(int), cudaMemcpyHostToDevice));
    check_cuda(cudaMemcpy(d_A_col_ind, adjacency_->csr_col_ind,
                          adjacency_->nnz * sizeof(int), cudaMemcpyHostToDevice));
    cusparseSpMatDescr_t A_descr;
    check_cusparse(cusparseCreateCsr(&A_descr, adjacency_->rows,
                                     adjacency_->columns, adjacency_->nnz,
                                     d_A_csr_row_offsets, d_A_col_ind,
                                     d_A_csr_val,
                                     CUSPARSE_INDEX_32I, CUSPARSE_INDEX_32I,
                                     CUSPARSE_INDEX_BASE_ZERO, CUDA_R_32F));

    // create cusparse B
    float *d_B;
    check_cuda(cudaMalloc((void **) &d_B, B.rows * B.columns * sizeof(float)));
    check_cuda(cudaMemcpy(d_B, B.values, B.rows * B.columns * sizeof(float),
                          cudaMemcpyHostToDevice));
    cusparseDnMatDescr_t B_descr;
    check_cusparse(cusparseCreateDnMat(&B_descr, B.rows, B.columns,
                                       B.rows, d_B,
                                       CUDA_R_32F, CUSPARSE_ORDER_COL));

    // create result
    matrix<float> result;
    result.rows = adjacency_->rows;
    result.columns = B.columns;
    result.values = (float *) malloc(result.rows * result.columns * sizeof(float));
    for (int i = 0; i < result.rows * result.columns; ++i) {
        result.values[i] = 0.0f;
    }

    // create cusparse result
    float *d_result;
    check_cuda(cudaMalloc((void **) &d_result, result.rows * result.columns * sizeof(float)));
    check_cuda(cudaMemcpy(d_result, result.values, result.rows * result.columns * sizeof(float),
                          cudaMemcpyHostToDevice));
    cusparseDnMatDescr_t result_descr;
    check_cusparse(cusparseCreateDnMat(&result_descr, result.rows, result.columns,
                                       result.rows, d_result,
                                       CUDA_R_32F, CUSPARSE_ORDER_COL));

    // get buffer size for SpMM
    float alpha = 1.0f;
    float beta = 0.0f;
    size_t buffer_size;
    check_cusparse(cusparseSpMM_bufferSize(cuda_helper_->cusparse_handle,
                                           CUSPARSE_OPERATION_NON_TRANSPOSE, CUSPARSE_OPERATION_NON_TRANSPOSE,
                                           &alpha, A_descr, B_descr, &beta, result_descr,
                                           // CUSPARSE_MM_ALG_DEFAULT is deprecated
                                           // but CUSPARSE_SPMM_ALG_DEFAULT is not working
                                           CUDA_R_32F, CUSPARSE_MM_ALG_DEFAULT,
                                           &buffer_size));
    void *d_buffer;
    check_cuda(cudaMalloc(&d_buffer, buffer_size));

    // compute SpMM
    check_cusparse(cusparseSpMM(cuda_helper_->cusparse_handle,
                                CUSPARSE_OPERATION_NON_TRANSPOSE, CUSPARSE_OPERATION_NON_TRANSPOSE,
                                &alpha, A_descr, B_descr, &beta, result_descr,
                                CUDA_R_32F, CUSPARSE_MM_ALG_DEFAULT,
                                d_buffer));

    check_cuda(cudaFree(d_buffer));

    // apply mean
    if (mean_) {
        matrix<float> ones;
        ones.rows = adjacency_->rows;
        ones.columns = 1;
        ones.values = (float *) malloc(ones.rows * ones.columns * sizeof(float));
        for (int i = 0; i < ones.rows * ones.columns; ++i) {
            ones.values[i] = 1.0;
        }
        float *d_ones;
        check_cuda(cudaMalloc(&d_ones, ones.rows * ones.columns * sizeof(float)));
        check_cuda(cudaMemcpy(d_ones, ones.values, ones.rows * ones.columns * sizeof(float),
                              cudaMemcpyHostToDevice));
        cusparseDnVecDescr_t ones_desc;
        check_cusparse(cusparseCreateDnVec(&ones_desc, ones.rows,
                                           d_ones, CUDA_R_32F));

        sum_.rows = ones.rows;
        sum_.columns = 1;
        sum_.values = (float *) malloc(sum_.rows * sum_.columns * sizeof(float));
        for (int i = 0; i < sum_.rows * sum_.columns; ++i) {
            sum_.values[0] = 0.0;
        }
        float *d_sum;
        check_cuda(cudaMalloc(&d_sum, sum_.rows * sum_.columns * sizeof(float)));
        check_cuda(cudaMemcpy(d_sum, sum_.values, sum_.rows * sum_.columns * sizeof(float),
                              cudaMemcpyHostToDevice));
        cusparseDnVecDescr_t sum_desc;
        check_cusparse(cusparseCreateDnVec(&sum_desc, sum_.rows * sum_.columns,
                                           d_sum, CUDA_R_32F));

        check_cusparse(cusparseSpMV_bufferSize(cuda_helper_->cusparse_handle,
                                               CUSPARSE_OPERATION_NON_TRANSPOSE,
                                               &alpha, A_descr, ones_desc,
                                               &beta, sum_desc,
                                               CUDA_R_32F, CUSPARSE_MV_ALG_DEFAULT, &buffer_size));
        check_cuda(cudaMalloc(&d_buffer, buffer_size));
        check_cusparse(cusparseSpMV(cuda_helper_->cusparse_handle,
                                    CUSPARSE_OPERATION_NON_TRANSPOSE,
                                    &alpha, A_descr, ones_desc,
                                    &beta, sum_desc,
                                    CUDA_R_32F, CUSPARSE_MV_ALG_DEFAULT, d_buffer));

        check_cuda(cudaMemcpy(sum_.values, d_sum,
                              sum_.rows * sum_.columns * sizeof(float),
                              cudaMemcpyDeviceToHost));

        div_mat_vec(d_result, d_sum, result.rows, result.columns);

        // free GPU memory
        check_cuda(cudaFree(d_ones));
        check_cuda(cudaFree(d_sum));

        // free CPU memory
        free(ones.values);
    }// end mean

    // copy result to CPU memory
    check_cuda(cudaMemcpy(result.values, d_result,
                          result.rows * result.columns * sizeof(float),
                          cudaMemcpyDeviceToHost));

    // free memory
    check_cuda(cudaFree(d_A_csr_val));
    check_cuda(cudaFree(d_A_col_ind));
    check_cuda(cudaFree(d_A_csr_row_offsets));
    check_cuda(cudaFree(d_B));
    check_cuda(cudaFree(d_buffer));
    check_cuda(cudaFree(d_result));

    return result;
}

matrix<float> GraphConvolution::backward(matrix<float> in_gradients) {
    float *d_g;
    check_cuda(cudaMalloc(&d_g, in_gradients.rows * in_gradients.columns * sizeof(float)));
    check_cuda(cudaMemcpy(d_g, in_gradients.values, in_gradients.rows * in_gradients.columns * sizeof(float),
                          cudaMemcpyHostToDevice));

    if (mean_) {
        float *d_sum;
        check_cuda(cudaMalloc(&d_sum, sum_.rows * sum_.columns * sizeof(float)));
        check_cuda(cudaMemcpy(d_sum, sum_.values, sum_.rows * sum_.columns * sizeof(float),
                              cudaMemcpyHostToDevice));

        div_mat_vec(d_g, d_sum, in_gradients.rows, in_gradients.columns);

        check_cuda(cudaFree(d_sum));
    }

    // grad_input = adjacency.T * in_gradients
    // START copy paste from forward
    float *d_A_csr_val;
    int *d_A_csr_row_offsets, *d_A_col_ind;
    check_cuda(cudaMalloc((void **) &d_A_csr_val,
                          adjacency_->nnz * sizeof(float)));
    check_cuda(cudaMalloc((void **) &d_A_csr_row_offsets,
                          (adjacency_->rows + 1) * sizeof(int)));
    check_cuda(cudaMalloc((void **) &d_A_col_ind,
                          adjacency_->nnz * sizeof(int)));
    check_cuda(cudaMemcpy(d_A_csr_val, adjacency_->csr_val,
                          adjacency_->nnz * sizeof(float), cudaMemcpyHostToDevice));
    check_cuda(cudaMemcpy(d_A_csr_row_offsets, adjacency_->csr_row_ptr,
                          (adjacency_->rows + 1) * sizeof(int), cudaMemcpyHostToDevice));
    check_cuda(cudaMemcpy(d_A_col_ind, adjacency_->csr_col_ind,
                          adjacency_->nnz * sizeof(int), cudaMemcpyHostToDevice));
    cusparseSpMatDescr_t A_desc;
    check_cusparse(cusparseCreateCsr(&A_desc, adjacency_->rows,
                                     adjacency_->columns, adjacency_->nnz,
                                     d_A_csr_row_offsets, d_A_col_ind,
                                     d_A_csr_val,
                                     CUSPARSE_INDEX_32I, CUSPARSE_INDEX_32I,
                                     CUSPARSE_INDEX_BASE_ZERO, CUDA_R_32F));
    // END copy paste from forward

    cusparseDnMatDescr_t g_desc;
    check_cusparse(cusparseCreateDnMat(&g_desc, in_gradients.rows, in_gradients.columns,
                                       in_gradients.rows, d_g,
                                       CUDA_R_32F, CUSPARSE_ORDER_COL));

    matrix<float> grad_input;
    grad_input.rows = adjacency_->columns;
    grad_input.columns = in_gradients.columns;
    grad_input.values = reinterpret_cast<float *>(malloc(grad_input.rows * grad_input.columns * sizeof(float)));
    float *d_dinput;
    check_cuda(cudaMalloc((void **) &d_dinput,
                          grad_input.rows * grad_input.columns * sizeof(float)));
    cusparseDnMatDescr_t dinput_desc;
    check_cusparse(cusparseCreateDnMat(&dinput_desc, grad_input.rows, grad_input.columns,
                                       grad_input.rows, d_dinput,
                                       CUDA_R_32F, CUSPARSE_ORDER_COL));

    float alpha = 1.0;
    float beta = 0.0;
    size_t buffer_size;
    check_cusparse(cusparseSpMM_bufferSize(cuda_helper_->cusparse_handle,
                                           CUSPARSE_OPERATION_TRANSPOSE, CUSPARSE_OPERATION_NON_TRANSPOSE,
                                           &alpha, A_desc, g_desc, &beta, dinput_desc,
                                           CUDA_R_32F, CUSPARSE_MM_ALG_DEFAULT,
                                           &buffer_size));
    void *d_buffer;
    check_cuda(cudaMalloc(&d_buffer, buffer_size));

    // compute SpMM
    check_cusparse(cusparseSpMM(cuda_helper_->cusparse_handle,
                                CUSPARSE_OPERATION_TRANSPOSE, CUSPARSE_OPERATION_NON_TRANSPOSE,
                                &alpha, A_desc, g_desc, &beta, dinput_desc,
                                CUDA_R_32F, CUSPARSE_MM_ALG_DEFAULT,
                                d_buffer));

    check_cuda(cudaMemcpy(grad_input.values, d_dinput,
                          grad_input.rows * grad_input.columns * sizeof(float),
                          cudaMemcpyDeviceToHost));

    // clean-up
    check_cuda(cudaFree(d_buffer));
    check_cuda(cudaFree(d_dinput));
    check_cuda(cudaFree(d_A_csr_val));
    check_cuda(cudaFree(d_A_csr_row_offsets));
    check_cuda(cudaFree(d_A_col_ind));
    check_cuda(cudaFree(d_g));

    return grad_input;
}
