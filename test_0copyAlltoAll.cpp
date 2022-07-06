#include <stdio.h>
#include <iostream>
#include <malloc.h>
#include <time.h>
#include <mpi.h>
#include <unistd.h>
#include <math.h>
#include <bits/stdc++.h>

using namespace std;

double Mean(double[], int);
double Median(double[], int);
void allToAll_index(int *, int, int *, int *, int, int, int);
void allToAll_concat(int *inMsg, int procs, int *id_procs, int *outMsg, int len, int myid);
double logWithBase(double base, double x);
int getrank(int id, int procs, int *id_procs);
void copy(int *A, int *B, int len);
void pack(int *A, int *B, int blklen, int procs, int r, int i, int j, int nblocks);
int mod(int a, int b);

int main(int argc, char **argv)
{
    int i, procs, myid, *outMsg, n, err, max_iter, iter = 0;
    long n_bytes, sleep_time;
    double t1_b, t2_b;
    // initialize MPI_Init
    err = MPI_Init(&argc, &argv);
    if (err != MPI_SUCCESS)
    {
        cout << "\nError initializing MPI.\n";
        MPI_Abort(MPI_COMM_WORLD, err);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    int *id_procs;
    id_procs = (int *)malloc(procs * sizeof(int));
    MPI_Allgather(&myid, 1, MPI_INT, id_procs, 1, MPI_INT, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    if (myid == 0)
    {
        for (i = 0; i < procs; i++)
        {
            printf("%d ", id_procs[i]);
        }
        printf("\n");
    }

    if (myid == 0)
    {
        if (argc < 4)
        {
            cout << "\n Invalid Number of Arguements.\n,\n";
            MPI_Finalize();
            return 0;
        }
    }

    n = atoi(argv[1]);          // number of elements for each process
    max_iter = atoi(argv[2]);   // number of iterations to repeat the procedure
    sleep_time = atoi(argv[3]); // Wait time between iterations (microseconds)

    n_bytes = n * sizeof(int);
    // printf("n is %d\n", n);

    double alltoall_time[max_iter];
    outMsg = (int *)malloc(n * sizeof(int));

    // Generate Data
    for (i = 0; i < n; i++)
    { // generate int data between 0 and 100
        outMsg[i] = myid * 100 * n + i * 100;
    }

    iter = 0;
    int *buffer_recv;
    buffer_recv = (int *)malloc(n * sizeof(int));
    int *inMsg;
    inMsg = (int *)malloc(n * n * sizeof(int));
    while (iter < max_iter)
    {
        //-------------------------------------------------------------------------AllToAll
        MPI_Barrier(MPI_COMM_WORLD);
        t1_b = MPI_Wtime();
        allToAll_concat(inMsg, procs, id_procs, outMsg, n, myid);
        MPI_Barrier(MPI_COMM_WORLD);
        t2_b = MPI_Wtime();
        if (myid == 0)
        {
            alltoall_time[iter] = ((t2_b - t1_b) * 1000);
        }

        printf("Values collected on process %d: ", myid);
        for (int i = 0; i < n * n; i++)
        {
            printf("%d ", inMsg[i]);
        }
        printf("\n");

        usleep(sleep_time);
        iter++;
    }

    if (myid == 0)
    {
        cout << "\nThe size of the data sent: " << n_bytes << " Bytes";
        cout << "\nMean of communication times: " << Mean(alltoall_time, max_iter) << "ms";
        cout << "\nMedian of communication times: " << Median(alltoall_time, max_iter) << "ms\n";
    }

    free(id_procs);
    free(buffer_recv);
    free(inMsg);
    free(outMsg);
    MPI_Finalize();
    return 0;
} // end main

double Mean(double a[], int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        sum += a[i];

    return (sum / (double)n);
}

double Median(double a[], int n) {
    sort(a, a + n);
    if (n % 2 != 0)
        return a[n / 2];

    return (a[(n - 1) / 2] + a[n / 2]) / 2.0;
}
