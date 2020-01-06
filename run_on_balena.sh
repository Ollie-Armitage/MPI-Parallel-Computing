#!/bin/bash
echo "Removing Old Files... "
ssh -o LogLevel=error balena 'rm main && rm ora24-job.err && rm ora24-job.out && rm source_files/main.c && rm source_files/main.h' 2>/dev/null

echo "Uploading Files..."
scp -o LogLevel=error main.c main.h balena:source_files 2>/dev/null
scp -o LogLevel=error job_script.slm balena:job_scripts/job_script.slm 2>/dev/null


echo "Compiling files and running job script."
ssh -o LogLevel=error balena 'module load intel/compiler &&
            module load intel/mpi &&
            mpicc -std=c99 -o main source_files/main.c &&
            sbatch job_scripts/job_script.slm' 2>/dev/null
