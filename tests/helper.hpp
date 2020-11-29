// Copyright 2020 Marcel Wagenländer

#ifndef HELPER_HPP
#define HELPER_HPP

#include "sage_linear.hpp"
#include "tensors.hpp"


void save_params(matrix<float> **parameters);

void save_grads(SageLinearGradients *gradients, matrix<float> **weight_gradients);

int run_python(std::string module_name, std::string function_name);

int read_return_value(std::string path);

void write_value(int value, std::string path);

int num_equal_rows(matrix<float> A, matrix<float> B);

int compare_mat(matrix<float> *mat_a, matrix<float> *mat_b, std::string name);

#endif//HELPER_HPP
