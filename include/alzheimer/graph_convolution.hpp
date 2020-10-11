// Copyright 2020 Marcel Wagenländer

#ifndef GRAPH_CONVOLUTION_H
#define GRAPH_CONVOLUTION_H

#include <iostream>

#include "tensors.hpp"
#include "cuda_helper.hpp"


class GraphConvolution {
private:
    CudaHelper *cuda_helper_;
    sparse_matrix<float> *adjacency_;
    std::string reduction_;
    bool mean_;
    matrix<float> sum_;

public:
    GraphConvolution(CudaHelper *helper, sparse_matrix<float> *adjacency_, std::string reduction);
    matrix<float> forward(matrix<float> B);
    matrix<float> backward(matrix<float> in_gradients);
};

#endif
