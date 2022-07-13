#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <iostream>
#include <unistd.h>
#include <bits/stdc++.h>

double  Mean(double[], int);
double  Median(double[], int);
static int rank, nprocs;

std::vector<int> convert10tob(int w, int N, int b)
{
	std::vector<int> v(w);
	int i = 0;
	while(N) {
	  v[i++] = (N % b);
	  N /= b;
	}
//	std::reverse(v.begin(), v.end());
	return v;
}

void alltoall_radix_r(int r, char *sendbuf, int sendcount, MPI_Datatype sendtype, char *recvbuf, int recvcount, MPI_Datatype recvtype,  MPI_Comm comm) {
	
    double ts = MPI_Wtime();
	double s = MPI_Wtime();
    int rank, nprocs;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    int typesize;
    MPI_Type_size(sendtype, &typesize);

    int unit_size = sendcount * typesize;
    int w = ceil(log(nprocs) / log(r)); // calculate the number of digits when using r-representation
	int nlpow = pow(r, w-1);
	int d = (pow(r, w) - nprocs) / nlpow; // calculate the number of highest digits

    // local rotation
    std::memcpy(recvbuf, &sendbuf[rank*unit_size], (nprocs - rank)*unit_size);
    std::memcpy(&recvbuf[(nprocs - rank)*unit_size], sendbuf, rank*unit_size);
    double e = MPI_Wtime();
    double first_time = e - s;

    // convert rank to base r representation
    s = MPI_Wtime();
    int* rank_r_reps = (int*) malloc(nprocs * w * sizeof(int));
	for (int i = 0; i < nprocs; i++) {
		std::vector<int> r_rep = convert10tob(w, i, r);
		std::memcpy(&rank_r_reps[i*w], r_rep.data(), w*sizeof(int));
	}

	int sent_blocks[nlpow];
	int di = 0;
	int ci = 0;

	int comm_steps = (r - 1)*w - d;
	int nblocks_perstep[comm_steps];
	int istep = 0;

	char* temp_buffer = (char*)malloc(nlpow * unit_size); // temporary buffer
	e = MPI_Wtime();
	double conv_time = e - s;

	// communication steps = (r - 1)w - d
	double pre_time = 0, comm_time = 0, replace_time = 0;
    for (int x = 0; x < w; x++) {
    	int ze = (x == w - 1)? r - d: r;
    	for (int z = 1; z < ze; z++) {

    		// get the sent data-blocks
    		// copy blocks which need to be sent at this step
    		s = MPI_Wtime();
    		di = 0;
    		ci = 0;
    		for (int i = 0; i < nprocs; i++) {
    			if (rank_r_reps[i*w + x] == z){
    				sent_blocks[di++] = i;
    				memcpy(&temp_buffer[unit_size*ci++], &recvbuf[i*unit_size], unit_size);
    			}
    		}
    		nblocks_perstep[istep++] = di;
    		e = MPI_Wtime();
    		pre_time += e - s;

    		// send and receive
    		s = MPI_Wtime();
    		int distance = z * pow(r, x);
    		int recv_proc = (rank - distance + nprocs) % nprocs; // receive data from rank - 2^step process
    		int send_proc = (rank + distance) % nprocs; // send data from rank + 2^k process
    		long long comm_size = di * unit_size;
    		MPI_Sendrecv(temp_buffer, comm_size, MPI_CHAR, send_proc, 0, sendbuf, comm_size, MPI_CHAR, recv_proc, 0, comm, MPI_STATUS_IGNORE);
    		e = MPI_Wtime();
    		comm_time += e - s;

    		s = MPI_Wtime();
    		// replace with received data
    		for (int i = 0; i < di; i++)
    		{
    			long long offset = sent_blocks[i] * unit_size;
    			memcpy(recvbuf+offset, sendbuf+(i*unit_size), unit_size);
    		}
    		e = MPI_Wtime();
    		replace_time += e - s;
    	}
    }

    free(rank_r_reps);
    free(temp_buffer);

    // local rotation
    s = MPI_Wtime();
	for (int i = 0; i < nprocs; i++)
	{
		int index = (rank - i + nprocs) % nprocs;
		memcpy(&sendbuf[index*unit_size], &recvbuf[i*unit_size], unit_size);
	}
	memcpy(recvbuf, sendbuf, nprocs*unit_size);
	e = MPI_Wtime();
	double second_time = e - s;


    double te = MPI_Wtime();
	double max_time = 0;
	double total_time = te - ts;
	MPI_Allreduce(&total_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, comm);

	if (total_time == max_time) {
		std::cout << "[radixRbruck] " << " [" << nprocs << " " << sendcount << "] " <<  total_time << ", " << first_time << ", " << conv_time << ", "
				<< pre_time << ", " << comm_time << ", " << replace_time << ", " << second_time << std::endl;
	}
}

int main(int argc, char **argv) {

    double t1_b, t2_b;
    int iter = 0;
    // MPI Initial
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
        std::cout << "ERROR: MPI_Init error\n" << std::endl;
    if (MPI_Comm_size(MPI_COMM_WORLD, &nprocs) != MPI_SUCCESS)
    	std::cout << "ERROR: MPI_Comm_size error\n" << std::endl;
    if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS)
    	std::cout << "ERROR: MPI_Comm_rank error\n" << std::endl;
	
	int n = atoi(argv[1]);      //number of elements for each process
  	int max_iter = atoi(argv[2]);   //number of iterations to repeat the procedure
  	int sleep_time = atoi(argv[3]); //Wait time between iterations (microseconds)

  	int n_bytes = n * sizeof(int);
    double alltoall_time[max_iter];
    int *arr = (int*)malloc(n*sizeof(int));

    
    while (iter < max_iter) {

        int* send_buffer = new int[n];
        int* recv_buffer = new int[n];

        for(int i = 0; i < n; i++) {
            send_buffer[i] = rank * 100 * n + i * 100;
        }

        printf("Process %d send\n", rank);
        for (int i=0; i<n; i++) {
            printf("%d ", send_buffer[i]);
        }
        printf("\n");

        MPI_Barrier(MPI_COMM_WORLD);
        t1_b = MPI_Wtime();
        alltoall_radix_r(3, (char*)send_buffer, 1, MPI_INT, (char*)recv_buffer, 1, MPI_INT, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        t2_b = MPI_Wtime();

        MPI_Barrier(MPI_COMM_WORLD);
        for(int i = 0; i < nprocs; i++) {
        	if(i == rank) {
                printf("Rank %d: ", rank);
                // print entire array
                for(int j = 0; j < n; j++) {
                    printf("%d ", recv_buffer[j]);
                }
                printf("\n");
                fflush(stdout);
            }

            MPI_Barrier(MPI_COMM_WORLD);
		}

        delete[] send_buffer;
	    delete[] recv_buffer;
        if (rank == 0) {
            alltoall_time[iter] = ((t2_b - t1_b) * 1000);
        }
        usleep(sleep_time);
        iter++;
    
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
        std::cout << "\nThe size of the data sent: " << n_bytes << " Bytes";
        std::cout << "\nMean of communication times: " << Mean(alltoall_time, max_iter) << "ms";
        std::cout << "\nMedian of communication times: " << Median(alltoall_time, max_iter) << "ms\n";
    } 
    
    
	MPI_Finalize();
	return 0;

}

double Mean(double a[], int n) {
  double sum = 0.0;
  for (int i = 0; i < n; i++)
    sum += a[i];

  return (sum / (double)n);
}

double Median(double a[], int n) {
  std::sort(a, a + n);
  if (n % 2 != 0)
    return a[n / 2];

  return (a[(n - 1) / 2] + a[n / 2]) / 2.0;
}