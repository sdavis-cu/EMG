#Name the job
#PBS -N picking_best_models
### Specify the number of nodes/cores
#PBS -l nodes=1:ppn=1

### Allocate the amount of memory needed
#PBS -l pmem=20gb

### Set your expected walltime
#PBS -l walltime=2:00:00

### Setting to mail when the job is complete
#PBS -e /Users/azofeifa/qsub_errors/                                                                                              
#PBS -o /Users/azofeifa/qsub_stdo/  

### Set your email address
#PBS -m ae
#PBS -M jgazofeifa@gmail.com

###PBS -t 1-10

### Choose your shell 
#PBS -S /bin/sh
### Pass enviroment variables to the job
#PBS -V



### ===================
### what machine?
### ===================
vieques_pando=true ###unix compute clusters
mac=false ###macOS
if [ "$vieques_pando" = true ] ; then ###load modules 
    module load numpy_1.9.2
    module load scipy_0.14.0
    module load matplotlib_1.3.1
fi


src=/Users/azofeifa/Lab/EMG/analysis/parse_parameters.py
root=/Users/azofeifa/
merged_data=${root}Lab/gro_seq_files/HCT116/merged_data_file_1.txt
out_file=${root}Lab/EMG_analysis_files/final_models
penality=100
#diff_threshold=$PBS_ARRAYID
diff_threshold=3
module_path=/Users/azofeifa/Lab/EMG/python_src/
python $src $merged_data $out_file $penality $diff_threshold $module_path


