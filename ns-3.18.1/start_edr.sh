#!/bin/bash

clear

iter_start=$1
iter_end=$2

for seed in $(seq $iter_start $iter_end);
do
	for bound in 20 40
	do
		for feedbackType in 3  
		do
			for rxNodeNum in 1 10 
			do
				for period in 100 200 500 1000 
				do
					for doppler in 0.1 
					do
						for eta in 0 0.5 1.0 1.5 2.0
						do
							for delta in 0.1 0.2
							do
								for rho in 0.1 0.2
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
