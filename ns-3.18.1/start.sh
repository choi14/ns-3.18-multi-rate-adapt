#!/bin/bash

clear

iter_start=$1
iter_end=$2

for seed in $(seq $iter_start $iter_end);
do
	for bound in 20  
	do
		for feedbackType in 0 1 2 3  
		do
			for rxNodeNum in 20 
			do
				for period in 100 200 500 1000 
				do
					for doppler in 0.05 0.1 
					do
						case "$feedbackType" in
							"0")
								for percentile in 0.5 0.75 0.9 0.95 0.99
								do
									./waf --run "scratch/test --FeedbackType=$feedbackType --Seed=$seed --RxNodeNum=$rxNodeNum --FeedbackPeriod=$period --Doppler=$doppler --Percentile=$percentile --Bound=$bound"
								done ;;
							"1")
								for alpha in 0.1 0.3 0.7 0.9
								do
									./waf --run "scratch/test --FeedbackType=$feedbackType --Seed=$seed --RxNodeNum=$rxNodeNum --FeedbackPeriod=$period --Doppler=$doppler --Alpha=$alpha --Bound=$bound"
								done ;;
							"2") 
								for beta in 0.5 1
								do
									./waf --run "scratch/test --FeedbackType=$feedbackType --Seed=$seed --RxNodeNum=$rxNodeNum --FeedbackPeriod=$period --Doppler=$doppler --Beta=$beta --Bound=$bound"
								done ;; 
							"3") 
								./waf --run "scratch/test --FeedbackType=$feedbackType --Seed=$seed --RxNodeNum=$rxNodeNum --FeedbackPeriod=$period --Doppler=$doppler --Bound=$bound"
								;;
						esac
					done
				done
			done
		done
	done
done
