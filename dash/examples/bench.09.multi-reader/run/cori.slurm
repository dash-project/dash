#!/bin/bash -l
#
#SBATCH -p regular
#SBATCH -N NNODES
#SBATCH -t 00:05:00

srun --ntasks-per-node=32 -N NNODES ../multi-reader

