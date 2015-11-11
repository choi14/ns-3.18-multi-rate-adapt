#!/bin/bash

clear

iter_start=$1
iter_end=$2

for seed in $(seq $iter_start $iter_end);
do
	for bound in 20
	do
		for feedbackType in 3  
		do
			for rxNodeNum in 1 5 10 
			do
				for period in 100 200 500 1000 
				do
					for doppler in 0.05 0.1 
					do
						for eta in 0 0.2 0.4 0.6 0.8 1.0 1.2 1.4 1.6 1.8 2.0
						do
							for delta in 0.1 0.3 0.5 0.7 0.9
							do
								for rho in 0.1 0.3 0.5 0.7 0.9
								do
								./waf --run "scratch/test_r1 --FeedbackType=$feedbackType --Seed=$seed --RxNodeNum=$rxNodeNum --FeedbackPeriod=$period --Doppler=$doppler --Bound=$bound --Eta=$eta --Delta=$delta --Rho=$rho"
								done
							done
						done
					done
				done
			done
		done
	done
done
