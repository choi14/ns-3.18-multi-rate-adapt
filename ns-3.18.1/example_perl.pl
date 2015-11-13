use warnings;
use strict;

# explicit warning
my $undef = undef;
print $undef;

# implicit 
my $undef2;
print $undef2;


#my $i = 0;
my @array = (0)x10;
for (my $i = 0; $i < 10; $i++)
{
	print "$array[$i] ";
}
print "\n";
