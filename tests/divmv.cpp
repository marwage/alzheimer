// Copyright 2020 Marcel Wagenländer

#include "divmv.h"
#include "cuda_helper.hpp"

#include "catch2/catch.hpp"
#include <cuda_runtime.h>


int test_divmv() {
    float *d_X, *d_y;
    int n = 5;
    int m = 7;

    check_cuda(cudaMalloc((void **) &d_X, n * m * sizeof(float)));
    check_cuda(cudaMalloc((void **) &d_y, n * sizeof(float)));

    float X[n * m];
    for (int i = 0; i < n * m; ++i) {
        X[i] = 1.0;
    }
    float y[n];
    for (int i = 0; i < n; ++i) {
        y[i] = (float) (i + 1);
    }

    float Z[n * m];
    for (int i = 0; i < n * m; ++i) {
        Z[i] = 1.0;
    }

    int idx;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            idx = j * n + i;
            Z[idx] = Z[idx] / y[i];
        }
    }

    check_cuda(cudaMemcpy(d_X, X, n * m * sizeof(float),
                          cudaMemcpyHostToDevice));
    check_cuda(cudaMemcpy(d_y, y, n * sizeof(float),
                          cudaMemcpyHostToDevice));

    div_mat_vec(d_X, d_y, n, m);

    check_cuda(cudaMemcpy(X, d_X, n * m * sizeof(float),
                          cudaMemcpyDeviceToHost));

    check_cuda(cudaFree(d_X));
    check_cuda(cudaFree(d_y));

    int equal = 1;
    for (int i = 0; i < n * m; ++i) {
        if (Z[i] != X[i]) {
            equal = 0;
            break;
        }
    }

    return equal;
}

TEST_CASE("Divide matrix vector", "[divmv]") {
    CHECK(test_divmv());
}
