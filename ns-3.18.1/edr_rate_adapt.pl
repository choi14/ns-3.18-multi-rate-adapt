use warnings;
use strict;

my $seedNum = 100;
my $numVal = 9;
my $i = 0;
my $seed = 0;

foreach my $numNode (1, 10){
	foreach my $period (100, 500, 1000){
		foreach my $doppler (0.1){
			foreach my $bound (20, 30){

				print "Bound: $bound Period: $period Doppler: $doppler NumNode: $numNode\n";
				my $output_filename1="storage_results/output_edr_151201/result_per-"."$numNode"."-"."$period"."-"."$doppler"."-"."$bound".".txt";
				open(OUT1, ">$output_filename1") ||die "Failed opening.\n";
				my $output_filename2="storage_results/output_edr_151201/result_air-"."$numNode"."-"."$period"."-"."$doppler"."-"."$bound".".txt";
				open(OUT2, ">$output_filename2") ||die "Failed opening.\n";
				my $output_filename3="storage_results/output_edr_151201/result_node-"."$numNode"."-"."$period"."-"."$doppler"."-"."$bound".".txt";
				open(OUT3, ">$output_filename3") ||die "Failed opening.\n";

				foreach my $eta (0, 2, 4, 6, 8, 10){

					my @per_a = (0)x$numVal;	
					my @per_s = (0)x$numVal;	
					my @air_a = (0)x$numVal;	
					my @air_s = (0)x$numVal;	
					my @node_a = (0)x$numVal;	
					my @node_s = (0)x$numVal;	
					my $type = 0;

					foreach my $rho (0.1, 0.5, 0.9){
						foreach my $delta (0.1, 0.5, 0.9){

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
								my $input_filename= "storage_results/result_edr_151201/edr_"."$eta"."_"."$delta"."_"."$rho"."_"."$seed"."_"."$numNode"."_"."$period"."_"."$doppler"."_"."$bound".".txt";
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

					print OUT1 "$eta $per_a[0] $per_s[0] $per_a[1] $per_s[1] $per_a[2] $per_s[2] $per_a[3] $per_s[3] $per_a[4] $per_s[4] $per_a[5] $per_s[5] $per_a[6] $per_s[6] $per_a[7] $per_s[7] $per_a[8] $per_s[8]\n";
					print OUT2 "$eta $air_a[0] $air_s[0] $air_a[1] $air_s[1] $air_a[2] $air_s[2] $air_a[3] $air_s[3] $air_a[4] $air_s[4] $air_a[5] $air_s[5] $air_a[6] $air_s[6] $air_a[7] $air_s[7] $air_a[8] $air_s[8]\n";
					print OUT3 "$eta $node_a[0] $node_s[0] $node_a[1] $node_s[1] $node_a[2] $node_s[2] $node_a[3] $node_s[3] $node_a[4] $node_s[4] $node_a[5] $node_s[5] $node_a[6] $node_s[6] $node_a[7] $node_s[7] $node_a[8] $node_s[8]\n";

					print "---------------------------------------------------------------------------\n";
					print "$eta $per_a[0] $per_s[0] $per_a[1] $per_s[1] $per_a[2] $per_s[2] $per_a[3] $per_s[3] $per_a[4] $per_s[4] $per_a[5] $per_s[5] $per_a[6] $per_s[6] $per_a[7] $per_s[7] $per_a[8] $per_s[8]\n";
					print "$eta $air_a[0] $air_s[0] $air_a[1] $air_s[1] $air_a[2] $air_s[2] $air_a[3] $air_s[3] $air_a[4] $air_s[4] $air_a[5] $air_s[5] $air_a[6] $air_s[6] $air_a[7] $air_s[7] $air_a[8] $air_s[8]\n";
					print "$eta $node_a[0] $node_s[0] $node_a[1] $node_s[1] $node_a[2] $node_s[2] $node_a[3] $node_s[3] $node_a[4] $node_s[4] $node_a[5] $node_s[5] $node_a[6] $node_s[6] $node_a[7] $node_s[7] $node_a[8] $node_s[8]\n";
					print "---------------------------------------------------------------------------\n";

				}# eta
			}#nodeNum
		}#doppler
	}#bound
}#period
