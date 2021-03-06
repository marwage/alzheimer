// Copyright 2020 Marcel Wagenländer

#ifndef ALZHEIMER_MEMORY_H
#define ALZHEIMER_MEMORY_H

#include <cuda_runtime.h>
#include <future>
#include <string>


long get_allocated_memory();

void print_allocated_memory(std::string name);

void log_allocated_memory(std::string name);

#endif//ALZHEIMER_MEMORY_H
