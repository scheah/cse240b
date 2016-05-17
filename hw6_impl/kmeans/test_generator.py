"""
HW6: Application - K Means clustering (CSE 240b, USCD)
Yeseong Kim, Chen Yang, Sebastian Cheah

1. Generate test set of K Means to "kmeans_testset.c" file which will be synthesized
2. The output trace for clusters can be used to plot

USAGE: python test_generator.py [NUM_POINTS NUM_CENTERS] [FILENAME_CLUSTER]
"""

import sys
import numpy as np
import matplotlib.pyplot as plt
from sklearn.datasets import make_blobs
from sklearn.cluster.k_means_ import _init_centroids
from sklearn.cluster import KMeans

def get_clusters_from_file(filename):
	## received I/O device 00000000, addr 00002000, data 00610000 on cycle 0x000000ecb7
	point_cluster_list = []
	for line in open(filename):
		if "## received I/O device 00000000, addr 00002000, data" in line:
			idx = line.index("data") + 5
			p = int(line[idx:idx+4], 16)
			c = int(line[idx+4:idx+8], 16)
			point_cluster_list.append((p,c))

	point_cluster_list = sorted(point_cluster_list, key=lambda x: x[0])
	cluster_list = zip(*point_cluster_list)[1]

	return cluster_list

def scale(v): # To make distinguishable integer
	SCALE_MAX_VALUE = 10000

	# normalize
	v = list(map(lambda x: x - min(v), v))
	v = list(map(lambda x: x / max(v), v))
	v = list(map(lambda x: int(x * SCALE_MAX_VALUE), v))

	return v

# Body ==============================================================================================
n_samples = 128
n_centers = 3
random_state = 170
if len(sys.argv) >= 3:
	n_samples = int(sys.argv[1])
	n_centers = int(sys.argv[2])

cluster_list = None
if len(sys.argv) >= 4:
	cluster_list = get_clusters_from_file(sys.argv[3])

# Generate testset
X, _ = make_blobs(n_samples=n_samples, centers=n_centers, random_state=random_state)

v1 = X[:, 0]
v2 = X[:, 1]

# Scale to integers
v1 = scale(v1)
v2 = scale(v2)
X = np.array(zip(v1, v2))

# Compute initial centers - using KMean++
centers = _init_centroids(X, n_centers, 'k-means++')

# Write file
with open("kmeans_testset.c", "w") as f:
	f.write("int testset_x[" + str(len(v1)) + "];\n");
	f.write("int testset_y[" + str(len(v1)) + "];\n");
	f.write("int testset_initial_centers_x[" + str(len(centers)) + "];\n");
	f.write("int testset_initial_centers_y[" + str(len(centers)) + "];\n");
	f.write("void init_dataset() {\n");

	# Points
	i = 0
	for x, y in zip(v1, v2):
		f.write("testset_x[" + str(i) + "] = " + str(x) + ";\n");
		f.write("testset_y[" + str(i) + "] = " + str(y) + ";\n");
		i += 1

	# Initial centers
	i = 0
	for c in centers:
		f.write("testset_initial_centers_x[" + str(i) + "] = " + str(int(c[0])) + ";\n");
		f.write("testset_initial_centers_y[" + str(i) + "] = " + str(int(c[1])) + ";\n");
		i += 1
	f.write("}\n");


# Show plot for validation
if cluster_list is None:
	cluster_list = KMeans(n_clusters=n_centers, random_state=random_state, init=centers).fit_predict(X)
	print("???")

plt.figure(figsize=(12, 12))
plt.scatter(v1, v2, c=cluster_list)
plt.title("Test set")

plt.show()
	
