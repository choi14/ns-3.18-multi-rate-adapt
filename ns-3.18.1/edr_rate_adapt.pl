use Switch;

$seedNum = 100;

foreach $bound (20){
	foreach $doppler (0.05, 0.1){
		foreach $n_node (1, 5, 10){
			$output_filename1="storage_results/output_151111/result_avg-"."$doppler"."-"."$n_node"."-"."$bound".".txt";
			open(OUT1, ">$output_filename1") ||die "Failed opening.\n";
			$output_filename2="storage_results/output_151111/result_air-"."$doppler"."-"."$n_node"."-"."$bound".".txt";
			open(OUT2, ">$output_filename2") ||die "Failed opening.\n";
			$output_filename3="storage_results/output_151111/result_node-"."$doppler"."-"."$n_node"."-"."$bound".".txt";
			open(OUT3, ">$output_filename3") ||die "Failed opening.\n";

			foreach $period (100, 200, 500, 1000){
				$avg_a[9] = {0};	
				$avg_s[9] = {0};	
				$air_a[9] = {0};	
				$air_s[9] = {0};	
				$node_a[9] = {0};	
				$node_s[9] = {0};	

				$type = 0;
				my $filename;
				#my $IN;

				for ($type = 0; $type < 9; $type++){
					$per[$seedNum] = {0};
					$air[$seedNum] = {0};
					$nr[$seedNum] = {0};
					$stddev[3] = {0};

					$avgPer = 0;
					$avgAir = 0;
					$avgNr = 0;

					my $filename_prefix;

					switch($type){
						case 0 {$filename_prefix = "storage_results/result2_151109/per_0.5";}
						case 1 {$filename_prefix = "storage_results/result2_151109/per_0.75";}
						case 2 {$filename_prefix = "storage_results/result2_151109/per_0.9";}
						case 3 {$filename_prefix = "storage_results/result2_151109/per_0.95";}
						case 4 {$filename_prefix = "storage_results/result2_151109/per_0.99";}
						case 5 {$filename_prefix = "storage_results/result2_151109/alp_0.5";}
						case 6 {$filename_prefix = "storage_results/result2_151109/bet_0";}
						case 7 {$filename_prefix = "storage_results/result2_151109/bet_0.5";}
						case 8 {$filename_prefix = "storage_results/result2_151109/bet_1";}
					}

					for($seed = 0; $seed < $seedNum; $seed++){
						$input_filename= "$filename_prefix"."_"."$seed"."_"."$n_node"."_"."$period"."_"."$doppler"."_"."$bound".".txt";
						open(IN, "$input_filename") ||die "Failed opening $input_filename.\n";

						$snum = 0;
						$node = 0;
						$per_seed[$n_node] = {0};
						$n = 0;
						$sumPer = 0;

						while($ref_txt=<IN>)
						{

							if ($ref_txt =~ /Source/)
							{
								@ref=split(/Source node sent: /, $ref_txt);
								chomp(@ref);
								$snum = $ref[1];
							}
							elsif ($ref_txt =~ /Node/)
							{
								@ref1=split(/received: /, $ref_txt);
								chomp(@ref1);
								$per_seed[$n] = 1 - $ref1[1]/$snum;
								#print "Per[$n] $per_seed[$n]\n";
								$n++;
							}
							elsif ($ref_txt =~ /AirTime/)
							{
								@ref2=split(/AirTime: /, $ref_txt);
								chomp(@ref2);
								$air[$seed] = $ref2[1];
							}
						}

						for ($a = 0; $a < $n_node; $a++)
						{
							if($per_seed[$a] < 0.2)
							{
								$node++;
							}
							$sumPer += $per_seed[$a];
						}
						$per[$seed] = $sumPer/$n_node; 
						$nr[$seed] = $node/$n_node;
					} # calculated for all seeds	

					for($k = 0; $k < $seedNum; $k++)
					{
						$avgPer += $per[$k];
						$avgAir += $air[$k];
						$avgNr += $nr[$k];
					}

					$avgPer = sprintf"%.3f",$avgPer/$seedNum;
					$avgAir = sprintf"%.3f",$avgAir/$seedNum;
					$avgNr = sprintf"%.3f",$avgNr/$seedNum;


					for($seed = 0; $seed < $seedNum; $seed++)
					{
						$stddev[0] += ($per[$seed]-$avgPer)*($per[$seed]-$avgPer);
						$stddev[1] += ($air[$seed]-$avgAir)*($air[$seed]-$avgAir);
						$stddev[2] += ($nr[$seed]-$avgNr)*($nr[$seed]-$avgNr);
					}
					$stddev[0] = sprintf"%.3f",sqrt($stddev[0]/$seedNum);
					$stddev[1] = sprintf"%.3f",sqrt($stddev[1]/$seedNum);
					$stddev[2] = sprintf"%.3f",sqrt($stddev[2]/$seedNum);

					#print OUT "$avgPer $stddev[0]\n"; 
					#print OUT "$avgAir $stddev[1]\n"; 
					#print OUT "$avgNr $stddev[2]\n";

					$avg_a[$type]=$avgPer;	
					$avg_s[$type]=$stddev[0];	
					$air_a[$type]=$avgAir;	
					$air_s[$type]=$stddev[1];	
					$node_a[$type]=$avgNr;	
					$node_s[$type]=$stddev[2];	
				}#type

				print "$period $avg_a[0] $avg_s[0] $avg_a[1] $avg_s[1] $avg_a[2] $avg_s[2] $avg_a[3] $avg_s[3] $avg_a[4] $avg_s[4] $avg_a[5] $avg_s[5] $avg_a[6] $avg_s[6] $avg_a[7] $avg_s[7] $avg_a[8] $avg_s[8]\n";
				print "$period $air_a[0] $air_s[0] $air_a[1] $air_s[1] $air_a[2] $air_s[2] $air_a[3] $air_s[3] $air_a[4] $air_s[4] $air_a[5] $air_s[5] $air_a[6] $air_s[6] $air_a[7] $air_s[7] $air_a[8] $air_s[8]\n";
				print "$period $node_a[0] $node_s[0] $node_a[1] $node_s[1] $node_a[2] $node_s[2] $node_a[3] $node_s[3] $node_a[4] $node_s[4] $node_a[5] $node_s[5] $node_a[6] $node_s[6] $node_a[7] $node_s[7] $node_a[8] $node_s[8]\n";

				print OUT1 "$period $avg_a[0] $avg_s[0] $avg_a[1] $avg_s[1] $avg_a[2] $avg_s[2] $avg_a[3] $avg_s[3] $avg_a[4] $avg_s[4] $avg_a[5] $avg_s[5] $avg_a[6] $avg_s[6] $avg_a[7] $avg_s[7] $avg_a[8] $avg_s[8]\n";
				print OUT2 "$period $air_a[0] $air_s[0] $air_a[1] $air_s[1] $air_a[2] $air_s[2] $air_a[3] $air_s[3] $air_a[4] $air_s[4] $air_a[5] $air_s[5] $air_a[6] $air_s[6] $air_a[7] $air_s[7] $air_a[8] $air_s[8]\n";
				print OUT3 "$period $node_a[0] $node_s[0] $node_a[1] $node_s[1] $node_a[2] $node_s[2] $node_a[3] $node_s[3] $node_a[4] $node_s[4] $node_a[5] $node_s[5] $node_a[6] $node_s[6] $node_a[7] $node_s[7] $node_a[8] $node_s[8]\n";

			}#period
		}
	}
}

