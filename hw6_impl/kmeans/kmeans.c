#include "kmeans.h"
#include "bsg_manycore.h"
#include "barrier.h"

// Algorithm parameters
// Note: Should be same to the testset
#define NUM_POINTS 128 // parameter N 
#define NUM_CLUSTERS 3 // parameter K
#define MAX_ITERATION 100
#define MIN_CHANGED_COUNT 4 // Converge condition

// Node parameters
#define KMEANS_CENTRAL_NODE_X 0
#define KMEANS_CENTRAL_NODE_Y 0
#define NUM_TILES (bsg_tiles_X * bsg_tiles_Y)
#define NUM_POINTS_EACH_NODE (NUM_POINTS / NUM_TILES) 

// Variables for sharing and computation ==============
// Iteration & Centers: Updated by the central node
int kmeans_iteration;
int kmeans_centers_x[NUM_CLUSTERS];
int kmeans_centers_y[NUM_CLUSTERS];

// Sum of clusters to update new centers (Each node sends to the central node.)
int kmeans_num_changed[bsg_tiles_X][bsg_tiles_Y];
int kmeans_num_clusters[bsg_tiles_X][bsg_tiles_Y][NUM_CLUSTERS];
int kmeans_sum_clusters_x[bsg_tiles_X][bsg_tiles_Y][NUM_CLUSTERS];
int kmeans_sum_clusters_y[bsg_tiles_X][bsg_tiles_Y][NUM_CLUSTERS];

// Local dataset 
int dataset_idx;
int* kmeans_local_data_x;
int* kmeans_local_data_y;

// Initialize the initial centers
void kmeans_init_center(int* testset_init_x, int* testset_init_y) {
	int c;
	for (c = 0; c < NUM_CLUSTERS; ++c) {
		kmeans_centers_x[c] = testset_init_x[c];
		kmeans_centers_y[c] = testset_init_y[c];
	}
}

// Initialize the data set of each tile
void kmeans_load_data(int bsg_x, int bsg_y, int* testset_x, int* testset_y) {
	int tile_idx;
	tile_idx = bsg_x + bsg_y * bsg_tiles_X;
	dataset_idx = NUM_POINTS_EACH_NODE * tile_idx;

	kmeans_local_data_x = &testset_x[dataset_idx];
	kmeans_local_data_y = &testset_y[dataset_idx];
}

// Run Kmeans algorithm to fit data
void kmeans_fit(int bsg_x, int bsg_y) {
	int i, c, p, q, dist, point_info;
	int min_dist, min_cluster;

	int num_changed;
	int point_clusters[NUM_POINTS_EACH_NODE];
	int num_cluster[NUM_CLUSTERS];
	int sum_cluster_x[NUM_CLUSTERS];
	int sum_cluster_y[NUM_CLUSTERS];

	while (bsg_volatile_access(kmeans_iteration) < MAX_ITERATION) {
		num_changed = 0;

		// 1. Identify clusters for each data point using the centers
		// & compute the sum of distance for each cluster
		for (c = 0; c < NUM_CLUSTERS; ++c) {
			num_cluster[c] = 0;
			sum_cluster_x[c] = 0;
			sum_cluster_y[c] = 0;
		}

		for (i = 0; i < NUM_POINTS_EACH_NODE; ++i) {
			min_dist = -1;
			min_cluster = 0;
			for (c = 0; c < NUM_CLUSTERS; ++c) {
				dist = (kmeans_local_data_x[i] - kmeans_centers_x[c]) * (kmeans_local_data_x[i] - kmeans_centers_x[c]) + 
					(kmeans_local_data_y[i] - kmeans_centers_y[c]) * (kmeans_local_data_y[i] - kmeans_centers_y[c]);

				if (min_dist == -1 || min_dist > dist) {
					min_dist = dist;
					min_cluster = c;
				}
			}

			if (point_clusters[i] != min_cluster)
				num_changed += 1;

			point_clusters[i] = min_cluster;
			num_cluster[min_cluster] += 1;
			sum_cluster_x[min_cluster] += kmeans_local_data_x[i];
			sum_cluster_y[min_cluster] += kmeans_local_data_y[i];
		}

		// 2. Send sum_clusters to central node
		if (bsg_x != KMEANS_CENTRAL_NODE_X || bsg_y != KMEANS_CENTRAL_NODE_Y) {
			bsg_remote_store(KMEANS_CENTRAL_NODE_X,KMEANS_CENTRAL_NODE_Y,
					&kmeans_num_changed[bsg_x][bsg_y],
					num_changed);

			for (c = 0; c < NUM_CLUSTERS; ++c) {
				bsg_remote_store(KMEANS_CENTRAL_NODE_X,KMEANS_CENTRAL_NODE_Y,
						&kmeans_num_clusters[bsg_x][bsg_y][c],
						num_cluster[c]);

				bsg_remote_store(KMEANS_CENTRAL_NODE_X,KMEANS_CENTRAL_NODE_Y,
						&kmeans_sum_clusters_x[bsg_x][bsg_y][c],
						sum_cluster_x[c]);

				bsg_remote_store(KMEANS_CENTRAL_NODE_X,KMEANS_CENTRAL_NODE_Y,
						&kmeans_sum_clusters_x[bsg_x][bsg_y][c],
						sum_cluster_y[c]);
			}
		}

		barrier(bsg_x, bsg_y);

		// 3. In the central node, compute new centers
		if (bsg_x == KMEANS_CENTRAL_NODE_X && bsg_y == KMEANS_CENTRAL_NODE_Y) {
			for (c = 0; c < NUM_CLUSTERS; ++c) {
				for (p = 0; p < bsg_tiles_X; ++p) {
					for (q = 0; q < bsg_tiles_Y; ++q) {
						if (p == KMEANS_CENTRAL_NODE_X && q == KMEANS_CENTRAL_NODE_Y)
							continue;

						num_cluster[c] += kmeans_num_clusters[p][q][c];
						sum_cluster_x[c] += kmeans_sum_clusters_x[p][q][c];
						sum_cluster_y[c] += kmeans_sum_clusters_y[p][q][c];
					}
				}

				kmeans_centers_x[c] = sum_cluster_x[c] / num_cluster[c];
				kmeans_centers_y[c] = sum_cluster_y[c] / num_cluster[c];

				// Send to other nodes
				for (p = 0; p < bsg_tiles_X; ++p) {
					for (q = 0; q < bsg_tiles_Y; ++q) {
						if (p == KMEANS_CENTRAL_NODE_X && q == KMEANS_CENTRAL_NODE_Y)
							continue;
						bsg_remote_store(p,q, &kmeans_centers_x[c], kmeans_centers_x[c]);
						bsg_remote_store(p,q, &kmeans_centers_y[c], kmeans_centers_y[c]);
					}
				}
			}

			// Update iteration
			kmeans_iteration += 1;

			// Count # of points whose cluster are changed
			for (p = 0; p < bsg_tiles_X; ++p) {
				for (q = 0; q < bsg_tiles_Y; ++q) {
					num_changed += kmeans_num_changed[p][q];
				}

				if (num_changed <= MIN_CHANGED_COUNT) 
					kmeans_iteration = MAX_ITERATION;
			}

			// Send iterations to other tiles
			for (p = 0; p < bsg_tiles_X; ++p) {
				for (q = 0; q < bsg_tiles_Y; ++q) {
					if (p == KMEANS_CENTRAL_NODE_X && q == KMEANS_CENTRAL_NODE_Y)
						continue;
					bsg_remote_store(p,q, &kmeans_iteration, kmeans_iteration);
				}
			}
		}

		barrier(bsg_x, bsg_y);
	}

	// Print to IO
	for (i = 0; i < NUM_POINTS_EACH_NODE; ++i) {
		point_info = ((i + dataset_idx) << 16) + // Data index
			point_clusters[i];
		bsg_remote_ptr_io_store(0,0x2000,point_info);
	}


	// Call barriers multiple times to flush all IO's
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
	barrier(bsg_x, bsg_y);
}
