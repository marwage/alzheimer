// Copyright Marcel Wagenländer 2020

#include "tensors.hpp"
#include "sage_linear.hpp"
#include "helper.hpp"

#include <string>
#include <iostream>
#include <Python.h>


std::string home = std::getenv("HOME");
std::string dir_path = home + "/gpu_memory_reduction/alzheimer/data";
std::string test_dir_path = dir_path + "/tests";

void save_params(matrix<float> *parameters) {
    std::string path;

    path = test_dir_path + "/self_weight.npy";
    save_npy_matrix(parameters[0], path);
    path = test_dir_path + "/self_bias.npy";
    save_npy_matrix(parameters[1], path);
    path = test_dir_path + "/neigh_weight.npy";
    save_npy_matrix(parameters[2], path);
    path = test_dir_path + "/neigh_bias.npy";
    save_npy_matrix(parameters[3], path);
}

void save_grads(SageLinear::SageLinearGradients *gradients, matrix<float> *weight_gradients) {
    std::string path;

    path = test_dir_path + "/self_grads.npy";
    save_npy_matrix(gradients->self_grads, path);
    path = test_dir_path + "/neigh_grads.npy";
    save_npy_matrix(gradients->neigh_grads, path);

    path = test_dir_path + "/self_weight_grads.npy";
    save_npy_matrix(weight_gradients[0], path);
    path = test_dir_path + "/self_bias_grads.npy";
    save_npy_matrix(weight_gradients[1], path);
    path = test_dir_path + "/neigh_weight_grads.npy";
    save_npy_matrix(weight_gradients[2], path);
    path = test_dir_path + "/neigh_bias_grads.npy";
    save_npy_matrix(weight_gradients[3], path);
}

matrix<float> gen_rand_matrix(int num_rows, int num_columns) {
    matrix<float> mat;
    mat.rows = num_rows;
    mat.columns = num_columns;
    mat.values = (float *) malloc(mat.rows * mat.columns * sizeof(float));
    for (int i = 0; i < mat.rows * mat.columns; ++i) {
        mat.values[i] = rand();
    }

    return mat;
}

int run_python(std::string module_name, std::string function_name) {
    std::string path_tests = "/home/ubuntu/gpu_memory_reduction/alzheimer/tests";
    int return_value = 0;

    PyObject *pName, *pModule, *pFunc, *pValue;
    Py_Initialize();

    PyRun_SimpleString( ("import sys\nsys.path.append(\"" + path_tests + "\")\n").c_str());
    pName = PyUnicode_DecodeFSDefault(module_name.c_str());
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, function_name.c_str());
        if (pFunc && PyCallable_Check(pFunc)) {
            pValue = PyObject_CallObject(pFunc, NULL);
            if (pValue != NULL) {
                return_value = (int) PyLong_AsLong(pValue);
                Py_DECREF(pValue);
            }
        } else {
            if (PyErr_Occurred())
                PyErr_Print();
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    } else {
        PyErr_Print();
    }
    if (Py_FinalizeEx() < 0) {
        std::cout << "Error in Py_FinalizeEx" << std::endl;
    }

    std::cout << "Return value " << return_value << std::endl;
    return return_value;
}