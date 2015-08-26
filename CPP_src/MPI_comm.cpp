#include <mpi.h>
#include "MPI_comm.h"
#include "load.h"
#include <vector>
#include "across_segments.h"
using namespace std;

vector<segment *> slice_segments(vector<segment *> segments, int rank, int nprocs){

	int count 	= segments.size()/ nprocs;
	if (count==0){
		count 	= 1;
	}
	int start 	= rank * count;
    int stop 	= min(start + count, int(segments.size()));

    if (rank==(nprocs-1)){
    	stop 	= int(segments.size());
    }
    if (start >= stop){
    	start 	= stop;
    }
    vector<segment *> new_segs(segments.begin() + start, segments.begin()+stop);

	return new_segs;	
}
int get_all_segs_size(vector<segment *> segments, int rank, int nprocs ){
	int SUM 	= segments.size();
	MPI_Status status;
	int sr;
	for (int job=1; job<nprocs; job++){
		MPI_Recv(&sr, 1, MPI_INT, job, 1, MPI_COMM_WORLD,&status);
		SUM+=sr;
	}
	return SUM;

}
void send_seg_size(vector<segment *> segments){
	int SUM 	= segments.size();
	MPI_Send(&SUM, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);	
}

map<int,int> get_all_bidir_sizes(vector<simple_c> fits, int nprocs ){
	MPI_Status status;
	int sr;
	map<int, int> table;
	for (int job=1; job<nprocs; job++){
		MPI_Recv(&sr, 1, MPI_INT, job, 1, MPI_COMM_WORLD,&status);
		table[job] 	= sr;
	}
	return table;
}

void send_bidir_size(vector<simple_c> fits){
	int SUM 	= fits.size();
	MPI_Send(&SUM, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);	
}


map<int, map<int, bidir_preds> > gather_all_simple_c_fits(vector<simple_c> root_simple_fits, 
	map<int,int> bidir_table, int N, int nproc ){ //for root, N expected number
	int count 	= N/nproc;
	vector<simple_c> all_fits ;
	for (int j =0; j < root_simple_fits.size(); j++){
		all_fits.push_back(root_simple_fits[j]);
	}
	MPI_Datatype mystruct;
	int blocklens[4]={1, 1,4,10};
	
	
	MPI_Datatype old_types[4] 	={MPI_DOUBLE, MPI_DOUBLE,MPI_INT, MPI_DOUBLE}; 
	MPI_Aint displacements[4];
	MPI_Aint intex, doublex;

	displacements[0] 	= offsetof(simple_c, ll);
	displacements[1] 	= offsetof(simple_c, noise_ll);
	displacements[2] 	= offsetof(simple_c, IDS);
	displacements[3] 	= offsetof(simple_c, ps);
	
	
	MPI_Type_create_struct( 4, blocklens, displacements, old_types, &mystruct );
	MPI_Type_commit( &mystruct );
	
	MPI_Status status;
	simple_c sr;
	typedef map<int,int>::iterator it_type;
	map<int, map<int, bidir_preds> > G; //map refering to segment ID and specific bidir ID
	map<int, bidir_preds> A;

	typedef map<int, map<int, bidir_preds> >::iterator it_type_2;
	typedef map<int, bidir_preds>::iterator it_type_3;


	int segment_id, bidir_id, Complexity, pred_bidirs_in_merged;
	int total_bidir_preds 	= 0;
	for (it_type cc=bidir_table.begin(); cc!=bidir_table.end(); cc++){
		//cc->first is the job that we are expect it from
		for (int j = 0; j < cc->second ; j++){
			MPI_Recv(&sr, 1, mystruct, cc->first, j, MPI_COMM_WORLD,&status);
			
			segment_id=sr.IDS[0]+cc->first*count , bidir_id=sr.IDS[3], Complexity=sr.IDS[1], pred_bidirs_in_merged=sr.IDS[2];
			if (G.find(segment_id) == G.end()  ){
				G[segment_id] 	= A;
			}
			if (G[segment_id].find(bidir_id) == G[segment_id].end() ){
				G[segment_id][bidir_id] 	= bidir_preds(sr.noise_ll, sr.ps[9]);
			}
			G[segment_id][bidir_id].insert_component(Complexity, sr, sr.ll);
			total_bidir_preds++;
		}
	}
	printf("total number of bidirectional segments: %d\n", total_bidir_preds );
	

	return G;
}

void send_all_simple_c_fits(vector<simple_c> root_simple_fits ){ //slaves
	//need to setup MPI derived datatype for simple_c struct
	MPI_Datatype mystruct;
	int blocklens[4]={1, 1,4,10};
	MPI_Datatype old_types[4] = {MPI_DOUBLE, MPI_DOUBLE,MPI_INT, MPI_DOUBLE}; 
	MPI_Aint displacements[4];
	MPI_Aint intex, doublex;

	displacements[0] 	= offsetof(simple_c, ll);
	displacements[1] 	= offsetof(simple_c, noise_ll);
	displacements[2] 	= offsetof(simple_c, IDS);
	displacements[3] 	= offsetof(simple_c, ps);
	
	

	MPI_Type_create_struct( 4, blocklens, displacements, old_types, &mystruct );
	MPI_Type_commit( &mystruct );
	
	for (int j = 0; j < root_simple_fits.size(); j++){
		MPI_Send(&root_simple_fits[j], 1, mystruct, 0, j, MPI_COMM_WORLD);
	}




}

void gather_all_segments(){ //for root

}

void send_all_segments(){ //slaves

}
struct bounds{
public:
	double lower_upper[2];
};
struct bounds2{
public:
	int D[3];
};
map<string , vector<vector<double> > > gather_all_bidir_predicitions(vector<segment *> all, vector<segment *> segments , int rank, int nprocs){
	map<string , vector<vector<double> > > G;
	map<string , vector<vector<double> > > A;
	//insert data from root
	int N 	= all.size();
	int S;
	int count 	= N/ nprocs;
	vector<bounds2> collections;
	vector<bounds2> final_collections;
	
	if (count==0){
		count 	= 1;
	}
	bounds B;
	bounds2 B2;
	MPI_Datatype mystruct;
	MPI_Datatype mystruct2;
	
	int blocklens[1]={2};
	int blocklens2[1]={3};
	
	MPI_Datatype old_types[1] = {MPI_DOUBLE}; 
	MPI_Datatype old_types2[1] = {MPI_INT}; 
	MPI_Aint displacements[1];
	MPI_Aint displacements2[1];
	
	displacements[0] 	= offsetof(bounds, lower_upper);
	displacements2[0] 	= offsetof(bounds2, D);
	
	MPI_Type_create_struct( 1, blocklens, displacements, old_types, &mystruct );
	MPI_Type_commit( &mystruct );
	
	MPI_Type_create_struct( 1, blocklens2, displacements2, old_types2, &mystruct2 );
	MPI_Type_commit( &mystruct2 );
	if (rank == 0){
		for (int i = 0 ; i < segments.size(); i++){
			for (int j = 0; j < segments[i]->bidirectional_bounds.size();j++){

				vector<double> BBB(2);
				BBB[0]=segments[i]->bidirectional_bounds[j][0], BBB[1]=segments[i]->bidirectional_bounds[j][1];
				G[segments[i]->chrom].push_back(BBB);
				bounds2 B2;
				B2.D[0]=i, B2.D[1]=int(BBB[0]),B2.D[2]=int(BBB[1]);
				collections.push_back(B2);
			}
		}
		for (int j =1; j < nprocs; j++){
			int start 	= j * count;
			int stop 	= min(start + count, int(N ));	
			if (j==nprocs-1){
				stop 	= N;
			}
			if (start >= stop){
		    	start 	= stop;
		    }
		    
		 	for (int i = 0; i < (stop-start) ; i++){
				MPI_Recv(&S, 1, MPI_INT, j, i, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
				G[all[ start+i ]->chrom] 	= vector<vector<double> >(S);
				for (int u = 0; u < S; u++ ){
					MPI_Recv(&B, 1, mystruct, j, u, MPI_COMM_WORLD,MPI_STATUS_IGNORE);			
					G[all[ start+i ]->chrom][u] 	= {B.lower_upper[0],B.lower_upper[1]};
					bounds2 B2;
					B2.D[0]=start+i, B2.D[1]=int(B.lower_upper[0]),B2.D[2]=int(B.lower_upper[1]);
					collections.push_back(B2);
				}
			}
		}


	}else  {
		for (int i = 0;  i < segments.size();i++ ){
			int S 	= segments[i]->bidirectional_bounds.size();
			MPI_Send(&S, 1, MPI_INT, 0,i,MPI_COMM_WORLD );
			for (int u=0; u < segments[i]->bidirectional_bounds.size(); u++){				
				bounds B;
				B.lower_upper[0] 		= segments[i]->bidirectional_bounds[u][0];
				B.lower_upper[1] 		= segments[i]->bidirectional_bounds[u][1];				
				MPI_Send(&B, 1, mystruct, 0, u, MPI_COMM_WORLD);
			}
		}
	}
	if (rank==0){
		N 	= collections.size();
		int count 	= N/ nprocs;
		if (count==0){
			count 	= 1;
		}
		for (int j = 1; j < nprocs; j++){
			int start 	= j*count;
			int stop 	= min(start + count, int(N ));	
			if (j==nprocs-1){
				stop 	= N;
			}
			if (start >= stop){
		    	start 	= stop;
		    }
		    
			int S 	= stop-start;
			MPI_Send(&S, 1, MPI_INT, j,1, MPI_COMM_WORLD);

			//okay now get ready! 
			for (int i = 0; i < S;i++){
				MPI_Send(&collections[start+i], 1, mystruct2, j,i,MPI_COMM_WORLD);
			}
		}
		for (int i = 0; i < count;i++){
			final_collections.push_back(collections[i]);
		}
		
	}else{
		MPI_Recv(&S, 1, MPI_INT, 0, 1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);	
		for (int i = 0; i < S; i++){
			MPI_Recv(&B2, 1, mystruct2, 0,i,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			final_collections.push_back(B2);
		}
	}
	for (int i = 0 ; i < final_collections.size(); i++){
		vector <double> BB(2);
		BB[0] = final_collections[i].D[1], BB[1]= final_collections[i].D[2];

		A[all[final_collections[i].D[0]]->chrom ].push_back(BB);
	}
	return A;

}








