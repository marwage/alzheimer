// Copyright 2020 Marcel Wagenländer

#ifndef TENSORS_H
#define TENSORS_H

#include <string>

#include "cuda_helper.hpp"


template<typename T>
class Matrix {
public:
    long rows = 0;
    long columns = 0;
    long size_ = 0;
    T *values = 0;
    bool row_major = true;
    Matrix();
    Matrix(long num_rows, long num_columns, bool is_row_major);
    Matrix(long num_rows, long num_columns, T *matrix_values, bool is_row_major);
    ~Matrix();
};

template<typename T>
class SparseMatrix {
public:
    int rows = 0;
    int columns = 0;
    int nnz = 0;
    T *csr_val = NULL;
    int *csr_row_ptr = NULL;
    int *csr_col_ind = NULL;
    SparseMatrix();
    SparseMatrix(int num_rows, int num_columns, int num_nnz);
    SparseMatrix(int num_rows, int num_columns, int num_nnz, T *values, int *row_ptr, int *col_ind);
    ~SparseMatrix();
};

template<typename T>
void print_matrix(Matrix<T> *mat);

template<typename T>
void print_matrix_features(Matrix<T> *mat);

template<typename T>
Matrix<T> load_npy_matrix(std::string path);

template<typename T>
SparseMatrix<T> load_mtx_matrix(std::string path);

template<typename T>
void save_npy_matrix(Matrix<T> *mat, std::string path);

template<typename T>
void save_npy_matrix(Matrix<T> mat, std::string path);

template<typename T>
void save_npy_matrix_no_trans(Matrix<T> mat, std::string path);

template<typename T>
Matrix<T> to_column_major(Matrix<T> *mat);

template<typename T>
Matrix<T> to_row_major(Matrix<T> *mat);

template<typename T>
void to_column_major_inplace(Matrix<T> *mat);

template<typename T>
void to_row_major_inplace(Matrix<T> *mat);

SparseMatrix<float> get_rows(SparseMatrix<float> *mat, int start_row, int end_row);

void print_sparse_matrix(SparseMatrix<float> *mat);

void transpose_csr_matrix(SparseMatrix<float> *mat, CudaHelper *cuda_helper);

long count_nans(Matrix<float> *x);

bool check_nans(Matrix<float> *x, std::string name);

Matrix<float> gen_rand_matrix(long num_rows, long num_columns);

Matrix<float> gen_non_rand_matrix(long num_rows, long num_columns);

#endif
