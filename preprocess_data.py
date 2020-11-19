import json
import numpy as np
import scipy.sparse as sp
import scipy.io
import os


home = os.getenv("HOME")
data_path = home + "/gpu_memory_reduction/alzheimer/data"


def print_array_prop(a):
    print("Shape: {}".format(a.shape))
    print("Dtype: {}".format(a.dtype))


def preprocess_flickr():
    flickr_path = data_path + "/flickr"

    path = flickr_path + "/adj_full.npz"
    f = np.load(path)
    adj = sp.csr_matrix((f["data"], f["indices"], f["indptr"]), f["shape"])
    adj = adj.astype(np.float32)
    path = flickr_path + "/adjacency.mtx"
    scipy.io.mmwrite(path, adj)

    path = flickr_path + "/feats.npy"
    features = np.load(path)
    features = features.astype(np.float32)
    path = flickr_path + "/features.npy"
    np.save(path, features)
    [print(x[0:10]) for x in features[0:10]]

    classes = np.zeros((features.shape[0],), dtype=np.int32)
    path = flickr_path + "/class_map.json"
    with open(path) as f:
        class_map = json.load(f)
    for key, item in class_map.items():
        classes[int(key)] = item
    path = flickr_path + "/classes.npy"
    np.save(path, classes)
    [print(x) for x in classes[0:10]]

    path = flickr_path + "/role.json"
    with open(path) as f:
        role = json.load(f)
    train_mask = np.zeros((features.shape[0],), dtype=bool)
    train_mask[np.array(role["tr"])] = True
    path = flickr_path + "/train_mask.npy"
    np.save(path, train_mask)
    [print(x) for x in train_mask[0:10]]
    val_mask = np.zeros((features.shape[0],), dtype=bool)
    val_mask[np.array(role["va"])] = True
    path = flickr_path + "/val_mask.npy"
    np.save(path, val_mask)
    [print(x) for x in val_mask[0:10]]
    test_mask = np.zeros((features.shape[0],), dtype=bool)
    test_mask[np.array(role["te"])] = True
    path = flickr_path + "/test_mask.npy"
    np.save(path, test_mask)
    [print(x) for x in test_mask[0:10]]


def preprocess_reddit():
    reddit_path = data_path + "/reddit"

    path = reddit_path + "/reddit_data.npz"
    data = np.load(path)
    print(data.files)
    
    features = data["feature"]
    features = features.astype(np.float32)
    path = reddit_path + "/features.npy"
    np.save(path, features)
    
    classes = data["label"]
    print("Classes min: {}".format(classes.min()))
    print("Classes max: {}".format(classes.max()))
    return
    classes = classes.astype(np.int32)
    path = reddit_path + "/classes.npy"
    np.save(path, classes)

    path = reddit_path + "/reddit_graph.npz"
    graph = np.load(path)
    print(graph.files)

    shape = graph["shape"]
    row = graph["row"]
    column = graph["col"]
    values = graph["data"]
    adjacency = sp.coo_matrix((values, (row, column)), shape=shape)
    adjacency = sp.csr_matrix(adjacency)
    adjacency = adjacency.astype(np.float32)
    path = reddit_path + "/adjacency.mtx"
    scipy.io.mmwrite(path, adjacency)


if __name__ == "__main__":
    #  preprocess_flickr()
    preprocess_reddit()

