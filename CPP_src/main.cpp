#include <mpi.h>
#include "load.h"
#include "model.h"
#include <iostream>
#include "across_segments.h"
#include <limits>
#ifdef USING_ICC
#include <mathimf.h>
#else
#include <math.h>   
#endif
#include <errno.h>
#include "error_stdo_logging.h"
#include <time.h>  
#include <stdio.h>   
#include <chrono>
#include <map>
#include "read_in_parameters.h"
#include "model_selection.h"
#include <thread>
#include "template_matching.h"
#include "MPI_comm.h"
#include "bootstrap.h"
#include "density_profiler.h"
#include <omp.h>
#include "bidir_main.h"
#include "model_main.h"
#include "select_main.h"
#ifdef USING_ICC
#include <aligned_new>
#endif
using namespace std;

int main(int argc, char* argv[]){
	MPI::Init(argc, argv);
	int nprocs		= MPI::COMM_WORLD.Get_size();
	int rank 		= MPI::COMM_WORLD.Get_rank();
    int threads  	= omp_get_max_threads();

	params * P 	= new params();
    read_in_parameters(argv, P, rank );
    if (P->EXIT){
    	if (rank == 0){
    		printf("exiting...\n");
    	}
    	delete P;
    	MPI::Finalize();
    	return 0;
    }
	int job_ID 		=  MPI_comm::get_job_ID(P->p["-log_out"], P->p["-N"], rank, nprocs);
	
    int verbose 	= stoi(P->p["-v"]);
    Log_File * LG 	= new  Log_File(rank, job_ID, P->p["-N"], P->p["-log_out"]);
    if (verbose and rank==0){
		P->display(nprocs,threads);
	}
    if (P->bidir){
	    bidir_run(P, rank, nprocs, job_ID,LG);
	}
	else if (P->model){
		model_run(P, rank, nprocs,0,job_ID,LG);
	}else if (P->select){
		select_run(P, rank, nprocs, job_ID,LG);	
	}
	if (rank == 0){
		load::collect_all_tmp_files(P->p["-log_out"], P->p["-N"], nprocs, job_ID);
	}

	MPI::Finalize();
	
	return 0;
}
