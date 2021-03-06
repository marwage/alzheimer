import numpy as np
import torch


def load_col_major(path):
    mat = np.load(path)
    n, m = mat.shape
    mat = mat.reshape((m, n))
    mat = mat.transpose()

    return mat


def check_isclose(A, B, loose=False):
    if (A.shape == B.shape):
        if not loose:
            is_close = np.isclose(A, B)
        else:
            rtol = 1e-05
            atol = 1e-08
            is_close = np.isclose(A, B, 2 * rtol, 2 * atol)
        ratio_close = is_close.sum() / B.size
    else:
        print(A.shape)
        print(B.shape)
        return 0

    return ratio_close


def check_equal(A, B):
    if (A.shape == B.shape):
        equal = (A == B)
        ratio_equal = equal.sum() / B.size
    else:
        print(A.shape)
        print(B.shape)
        return 0

    return ratio_equal


def check_close_equal(A, B):
    return check_isclose(A, B), check_equal(A, B)


def count_nans(A):
    count = 0
    for i in range(A.shape[0]):
        for j in range(A.shape[1]):
            if np.isnan(A[i, j]):
                count = count + 1
    return count

def check_nans(a, name):
    num_nans = count_nans(a)
    if num_nans > 0:
        print("{} has {} NaNs".format(name, num_nans))

def num_close_rows(A, B):
    is_close = np.isclose(A, B)
    is_close_sum = is_close.sum(axis=1)
    close_rows = is_close_sum == A.shape[1]

    return close_rows.sum()


def print_small(A):
    print(A[0:3, 0:3])


def breakpoint():
    import os, signal
    os.kill(os.getpid(), signal.SIGTRAP)


def print_not_close(A, B):
    is_close = np.isclose(A, B)
    num_rows, num_columns = A.shape
    for i in range(num_rows):
        for j in range(num_columns):
            if not is_close[i, j]:
                print("Coordinate: ({}, {})".format(i, j))
                print("Values: {}, {}; Diff: {}".format(A[i, j], B[i, j], A[i, j] - B[i, j]))


def to_torch(a):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

    a_torch = torch.from_numpy(a)
    a_torch = a_torch.to(device)
    a_torch.requires_grad_()
    a_torch.retain_grad()

    return a_torch


def save_return_value(value, path):
    value_np = np.array([value], dtype=np.int32)
    np.save(path, value_np)


def write_equal(value, path):
    if value == 1.0:
        save_return_value(1, path)
    else:
        save_return_value(0, path)


def write_return(positive, path):
    if positive:
        save_return_value(1, path)
    else:
        save_return_value(0, path)


def print_not_equal(A, B):
    is_equal = (A == B)
    num_rows, num_columns = A.shape
    for i in range(num_rows):
        for j in range(num_columns):
            if not is_equal[i, j]:
                print("Coordinate: ({}, {}); Values: {}, {}; Diff: {}".format(
                    i, j, A[i, j], B[i, j], A[i, j] - B[i, j]))


def print_close_equal(matrix_name, close, equal):
    print("{}: ratio close: {}, ratio equal: {}".format(matrix_name, close, equal))


def update_return(ratio):
    if ratio < 0.98:
        return False
    else:
        return True

