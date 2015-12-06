#!/bin/bash

clear

iter_start=$1
iter_end=$2

for seed in $(seq $iter_start $iter_end);
do
	for bound in 20 30
	do
		for EDRtype in 0 1 2 3 4   
		do
			for rxNodeNum in 1 10 
			do
				for period in 100 500 1000 
				do
					for doppler in 0.1 
					do
						for eta in 1 2 3 4 5
						do
							for delta in 0.1 0.9
							do
								for rho in 0.1 0.9
								do
								./waf --run "scratch/test_r2 --EDRtype=$EDRtype --Seed=$seed --RxNodeNum=$rxNodeNum --FeedbackPeriod=$period --Doppler=$doppler --Bound=$bound --Eta=$eta --Delta=$delta --Rho=$rho"
								done
							done
						done
					done
				done
			done
		done
	done
done
