#include <stdio.h>
#include <iostream>
#include <malloc.h>
#include <time.h>
#include <mpi.h>
#include <unistd.h>
#include <math.h>
#include <bits/stdc++.h>

using namespace std;

double  Mean(double[], int);
double  Median(double[], int);
void allToAll_index(int* , int , int *, int *, int , int , int);
void allToAll_concat(int* inMsg, int procs, int *id_procs, int *outMsg, int len, int myid);
double logWithBase(double base, double x);
int getrank(int id, int procs, int *id_procs);
void copy(int* A, int* B, int len);
void pack(int* A, int* B, int blklen, int procs, int r, int i, int j, int nblocks);
int mod(int a, int b);
//void    Print_times(double[], int);

int main(int argc, char **argv)
{
  int i, procs, myid, *arr, n, err, max_iter ,iter = 0;
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
  id_procs = (int*)malloc(procs*sizeof(int));
  MPI_Allgather(&myid, 1, MPI_INT, id_procs, 1, MPI_INT, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);
  if(myid==0){
    for (i = 0; i < procs; i++){ 
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

  n = atoi(argv[1]);      //number of elements for each process
  max_iter = atoi(argv[2]);   //number of iterations to repeat the procedure
  sleep_time = atoi(argv[3]); //Wait time between iterations (microseconds)

  n_bytes = n * sizeof(int);
  //printf("n is %d\n", n);

  double alltoall_time[max_iter];
  arr = (int*)malloc(n*sizeof(int));

  // Generate Data
  //printf("arr values of process %d\n", myid);
  for (i = 0; i < n; i++)
  { // generate random int data between 0 and 100
    arr[i] = myid * 100 * n + i * 100;
    //printf("%d ", arr[i]);
  }
  //printf("\n");
  
  //iteration
  iter = 0;
  int *buffer_recv;
  buffer_recv = (int*)malloc(n*sizeof(int));
  int *inMsg;
  inMsg = (int*)malloc(n*n*sizeof(int));
  while (iter < max_iter)
  {
    //-------------------------------------------------------------------------AllToAll
    MPI_Barrier(MPI_COMM_WORLD);
    t1_b = MPI_Wtime();
    //MPI_Alltoall(arr, 1, MPI_INT, buffer_recv, 1, MPI_INT, MPI_COMM_WORLD);
    //allToAll_index(inMsg, procs, id_procs, arr, 1, 3, myid);
    allToAll_concat(inMsg, procs, id_procs, arr, 5, myid);
    //MPI_Bcast(arr, n, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    t2_b = MPI_Wtime();
    if (myid == 0)
    {
      alltoall_time[iter] = ((t2_b - t1_b) * 1000);
    }

    
    printf("Values collected on process %d: ", myid);
    for(int i=0;i<n*n;i++){
      printf("%d ", inMsg[i]);
    }
    printf("\n");
    //printf("Values collected on process %d: %d, %d, %d.\n", myid, buffer_recv[0], buffer_recv[1], buffer_recv[2]);
    
    usleep(sleep_time);
    iter++;
  } // end iteration

  if (myid == 0)
  {
    cout << "\nThe size of the data sent: " << n_bytes << " Bytes";
    cout << "\nMean of communication times: " << Mean(alltoall_time, max_iter) << "ms";
    cout << "\nMedian of communication times: " << Median(alltoall_time, max_iter) << "ms\n";
    //Print_times(bcast_time, max_iter);
  } 

  free(arr);
  MPI_Finalize();
  return 0;
} // end main

double Mean(double a[], int n){
  double sum = 0.0;
  for (int i = 0; i < n; i++)
    sum += a[i];

  return (sum / (double)n);
}

double Median(double a[], int n){
  sort(a, a + n);
  if (n % 2 != 0)
    return a[n / 2];

  return (a[(n - 1) / 2] + a[n / 2]) / 2.0;
}

void allToAll_index(int* inMsg, int procs, int *id_procs, int *outMsg, int blklen, int r, int myid){
    int h, dest_rank, src_rank;
    int w = (int) floor(logWithBase(r,procs) + 0.5);
    int myrank = getrank(myid, procs, id_procs);
    printf(" myrank: %d, myid: %d \n", myrank, myid);
    int* tmp;
    tmp = (int*) malloc(procs*blklen*sizeof(int));
    copy(outMsg, &tmp[(procs-myrank) * blklen], myrank*blklen);
    copy(&outMsg[myrank*blklen], tmp, (procs-myrank)*blklen);
    /*printf("Values collected on process %d: ", myrank);
    for(int i=0;i<procs;i++){
      printf("%d ", tmp[i]);
    }
    printf("\n");
    */
    int dist = 1;
    for(int i=0;i<w;i++){
        if(i==w-1)
            h = (int) floor(procs/dist);
        else
            h = r;

        for(int j=1;j<h;j++){
            dest_rank = (myrank+j*dist) % procs;
            src_rank = (myrank-j*dist) % procs;
            //pack
            //send e receive
            //unpack
        }
        dist = dist * r;
    }
    for(int i=0;i<procs;i++)
        copy(&tmp[((myrank-i) % procs) * blklen], &inMsg[i * blklen], blklen);
}

double logWithBase(double base, double x) {
    return log(x) / log(base);
}

int getrank(int id, int procs, int *id_procs){
    for(int i=0;i<procs;i++){
        if(id_procs[i]==id)
            return i;
    }
    return -1;
}

void copy(int* A, int* B, int len){
    //B = (int*) malloc(len);
    for(int i=0;i<len;i++){
        B[i] = A[i];
    }
}

void pack(int* A, int* B, int blklen, int procs, int r, int i, int j, int nblocks){
  
}

void allToAll_concat(int* inMsg, int procs, int *id_procs, int *outMsg, int len, int myid){
  MPI_Status status;
  int dest_rank, src_rank;
  int d = (int) floor(logWithBase(2,procs) + 0.5);
  int myrank = getrank(myid, procs, id_procs);
  int* tmp;
  tmp = (int*) malloc(procs*procs*sizeof(int));
  copy(outMsg, tmp, len);
  //printf("Sono il processo %d, e ho copiato\n",myrank);
  int nblk = 1;
  int current_len = len;
  for(int r=0; r<d; r++){
    dest_rank = mod(myrank - nblk, procs);
    src_rank = mod(myrank + nblk, procs);
    //printf("dest_rank: %d, idprocs[dest_rank] = %d - src_rank: %d, idprocs[src_rank] = %d\n",dest_rank, id_procs[dest_rank], src_rank, id_procs[src_rank]);
    MPI_Sendrecv(tmp, current_len, MPI_INT, id_procs[dest_rank], 1, &tmp[current_len], current_len, MPI_INT, id_procs[src_rank], 1, MPI_COMM_WORLD, &status);
    //printf("MPI_Sendrecv(tmp, %d, MPI_INT, %d, 1, &tmp[current_len], %d, MPI_INT, %d, 1, MPI_COMM_WORLD, &status);\n", current_len, id_procs[dest_rank], current_len, id_procs[src_rank]);
    //MPI_Wait(&status);
    nblk = nblk * 2;
    current_len = current_len * 2;
  }
  current_len = len * (procs - nblk);
  dest_rank = mod(myrank - nblk, procs);
  src_rank = mod(myrank + nblk, procs);
  MPI_Sendrecv(tmp, current_len, MPI_INT, id_procs[dest_rank], 1, &tmp[current_len], current_len, MPI_INT, id_procs[src_rank], 1, MPI_COMM_WORLD, &status);
  //MPI_Wait(&status);
  //printf("Sono il processo %d, e ho finito\n",myrank);
  copy(tmp, &inMsg[len * myrank], len * (procs - myrank));
  copy(&tmp[len * (procs - myrank)], inMsg, len * myrank);
  /*for(int i=0;i<n;i++){
      printf("%d ", buffer_recv[i]);
  }
  printf("\n");*/
}

int mod(int a, int b){
   if(b < 0) //you can check for b == 0 separately and do what you want
     return -mod(-a, -b);
   int ret = a % b;
   if(ret < 0)
     ret+=b;
   return ret;
}