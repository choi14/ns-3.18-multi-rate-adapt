use warnings;
use strict;

my $seedNum = 50;
my $numVal = 1;
my $i = 0;
my $seed = 0;
#my $period = 1000;

foreach my $bound (20, 30){
	foreach my $period (100, 200, 500, 1000){
		foreach my $doppler (0.1){
			foreach my $numNode (1, 10){

				print "Bound: $bound Period: $period Doppler: $doppler NumNode: $numNode\n";
				my $output_filename1="storage_results/output_edr_151125/result_per-"."$period"."-"."$doppler"."-"."$numNode"."-"."$bound".".txt";
				open(OUT1, ">$output_filename1") ||die "Failed opening.\n";
				my $output_filename2="storage_results/output_edr_151125/result_air-"."$period"."-"."$doppler"."-"."$numNode"."-"."$bound".".txt";
				open(OUT2, ">$output_filename2") ||die "Failed opening.\n";
				my $output_filename3="storage_results/output_edr_151125/result_node-"."$period"."-"."$doppler"."-"."$numNode"."-"."$bound".".txt";
				open(OUT3, ">$output_filename3") ||die "Failed opening.\n";

				foreach my $eta (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10){

					my @per_a = (0)x$numVal;	
					my @per_s = (0)x$numVal;	
					my @air_a = (0)x$numVal;	
					my @air_s = (0)x$numVal;	
					my @node_a = (0)x$numVal;	
					my @node_s = (0)x$numVal;	
					my $type = 0;

					foreach my $rho (0.1){
						foreach my $delta (0.1){

							#print "Rho: $rho Delta: $delta\n";
							my @per = (0)x$seedNum;
							my @air = (0)x$seedNum;
							my @nodeRatio = (0)x$seedNum;
							my @stddev = (0)x3;

							my $avgPer = 0;
							my $avgAir = 0;
							my $avgNr = 0;
							my $seedCount = 0;

							#foreach my $seed (0, 10, 20, 30, 40, 50, 60, 70, 80, 90){
							for ($seed = 0; $seed < $seedNum; $seed++){
								my $input_filename= "storage_results/result_edr_151125/edr_"."$eta"."_"."$delta"."_"."$rho"."_"."$seed"."_"."$numNode"."_"."$period"."_"."$doppler"."_"."$bound".".txt";
								open(IN, "$input_filename") ||die "Failed opening $input_filename.\n";

								my @per_seed = (0)x$numNode;
								my $snum = 0;
								my $satisfyNode = 0;
								my $n = 0;
								my $sumPer = 0;

								while(my $ref_txt=<IN>)
								{
									if ($ref_txt =~ /Source/)
									{
										my @ref=split(/Source node sent: /, $ref_txt);
										chomp(@ref);
										$snum = $ref[1];
									}
									elsif ($ref_txt =~ /Node/)
									{
										my @ref1=split(/received: /, $ref_txt);
										chomp(@ref1);
										$per_seed[$n] = 1 - $ref1[1]/$snum;
										$n++;
									}
									elsif ($ref_txt =~ /AirTime/)
									{
										my @ref2=split(/AirTime: /, $ref_txt);
										chomp(@ref2);
										$air[$seedCount] = $ref2[1];
									}
								}

								for ($i = 0; $i < $numNode; $i++)
								{
									if($per_seed[$i] < 0.005)
									{
										$satisfyNode++;
									}
									$sumPer += $per_seed[$i];
								}
								$per[$seedCount] = $sumPer/$numNode; 
								$nodeRatio[$seedCount] = $satisfyNode/$numNode;
								$seedCount++;
							} 
							# calculated for all seeds	

							for($i = 0; $i< $seedNum; $i++)
							{
								$avgPer += $per[$i];
								$avgAir += $air[$i];
								$avgNr += $nodeRatio[$i];
							}

							$avgPer = sprintf"%.3f",$avgPer/$seedNum;
							$avgAir = sprintf"%.3f",$avgAir/$seedNum;
							$avgNr = sprintf"%.3f",$avgNr/$seedNum;

							for($i = 0; $i < $seedNum; $i++)
							{
								$stddev[0] += ($per[$i]-$avgPer)*($per[$i]-$avgPer);
								$stddev[1] += ($air[$i]-$avgAir)*($air[$i]-$avgAir);
								$stddev[2] += ($nodeRatio[$i]-$avgNr)*($nodeRatio[$i]-$avgNr);
							}
							$stddev[0] = sprintf"%.3f",sqrt($stddev[0]/$seedNum);
							$stddev[1] = sprintf"%.3f",sqrt($stddev[1]/$seedNum);
							$stddev[2] = sprintf"%.3f",sqrt($stddev[2]/$seedNum);

							$per_a[$type]=$avgPer;	
							$per_s[$type]=$stddev[0];	
							$air_a[$type]=$avgAir;	
							$air_s[$type]=$stddev[1];	
							$node_a[$type]=$avgNr;	
							$node_s[$type]=$stddev[2];	
							$type++;
						}#delta
					}# rho

					print OUT1 "$eta $per_a[0] $per_s[0]\n";
					print OUT2 "$eta $air_a[0] $air_s[0]\n";
					print OUT3 "$eta $node_a[0] $node_s[0]\n";

					print "---------------------------------------------------------------------------\n";
					print "$eta $per_a[0] $per_s[0]\n";
					print "$eta $air_a[0] $air_s[0]\n";
					print "$eta $node_a[0] $node_s[0]\n";
					print "---------------------------------------------------------------------------\n";

				}# eta
			}#nodeNum
		}#doppler
	}#bound
}#period
