import os
import numpy as np
import scipy.io
import scipy.sparse as sp
import torch
from helper import (check_close_equal, print_close_equal, write_equal,
                    load_col_major, to_torch, print_small)


def check_graph_conv():
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    all_close = 1.0
    home = os.getenv("HOME")
    dir_path = home + "/gpu_memory_reduction/alzheimer/data"
    flickr_dir_path = dir_path + "/flickr"
    test_dir_path = dir_path + "/tests"

    path = flickr_dir_path + "/adjacency.mtx"
    adj = scipy.io.mmread(path)
    path = flickr_dir_path + "/features.npy"
    features = np.load(path)

    path = test_dir_path + "/activations.npy"
    graph_conv_result = load_col_major(path)

    features_torch = to_torch(features)

    assert (sp.isspmatrix_coo(adj))
    values = adj.data
    indices = np.vstack((adj.row, adj.col))
    indices = torch.LongTensor(indices)
    values = torch.FloatTensor(values)
    shape = adj.shape
    adj_torch = torch.sparse.FloatTensor(indices, values, torch.Size(shape))
    adj_torch = adj_torch.to(device)

    graph_conv_result_torch = torch.sparse.mm(adj_torch, features_torch)

    sum_torch = torch.sparse.sum(adj_torch, dim=-1)
    sum_torch = sum_torch.to_dense()
    for i in range(graph_conv_result_torch.shape[1]):
        graph_conv_result_torch[:, i] = graph_conv_result_torch[:, i] / sum_torch

    true_graph_conv_result = graph_conv_result_torch.detach().cpu().numpy()
    ratio_close, ratio_equal = check_close_equal(graph_conv_result, true_graph_conv_result)
    all_close = all_close * ratio_close
    print_close_equal("Graph convolution", ratio_close, ratio_equal)

    # backpropagation
    path = test_dir_path + "/in_gradients.npy"
    in_gradients = load_col_major(path)
    in_gradients_torch = torch.from_numpy(in_gradients)
    in_gradients_torch = in_gradients_torch.to(device)

    graph_conv_result_torch.backward(in_gradients_torch)

    path = test_dir_path + "/gradients.npy"
    gradients = load_col_major(path)

    true_gradients = features_torch.grad.cpu().numpy()

    ratio_close, ratio_equal = check_close_equal(gradients, true_gradients)
    all_close = all_close * ratio_close
    print_close_equal("Graph convolution gradients", ratio_close, ratio_equal)

    path = test_dir_path + "/value.npy"
    write_equal(all_close, path)


if __name__ == "__main__":
    check_graph_conv()