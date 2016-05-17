// HW6: Application - K Means clustering

#ifndef _KMEANS_H_
#define _KMEANS_H_

void kmeans_init_center(int* testset_init_x, int* testset_init_y);
void kmeans_load_data(int bsg_x, int bsg_y, int* testset_x, int* testset_y);
void kmeans_fit(int bsg_x, int bsg_y);

#endif
