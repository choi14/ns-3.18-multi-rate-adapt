#!/bin/bash

clear

iter_start=$1
iter_end=$2

for seed in $(seq $iter_start $iter_end)
do
	for feedbackType in 4
	do
		for blockSize in 1 5 10
		do
			for bound in 10 20
			do
				for EDRtype in 1 2   
				do
					for linearTime in 2000
					do
						for rxNodeNum in 10 
						do
							for period in 1000 
							do
								for doppler in 0.1 
								do
									for eta in 1
									do
										for delta in 0.1
										do
											for rho in 0.1 0.9
											do
												./waf --run "scratch/test_r2 --Seed=$seed --FeedbackType=$feedbackType --BlockSize=$blockSize --Bound=$bound --EDRtype=$EDRtype --LinearTime=$linearTime --RxNodeNum=$rxNodeNum --FeedbackPeriod=$period --Doppler=$doppler --Eta=$eta --Delta=$delta --Rho=$rho"
											done
										done
									done
								done
							done
						done
					done
				done
			done
		done
	done
done
