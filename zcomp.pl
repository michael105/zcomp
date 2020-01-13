#!/bin/perl -w
# Implementation of a compression algorithm,
# especially suitable for text files.
# Or other files, as long as the chars are mostly < ASCII 128
# What is the fact for most text files.
# The compression is not so fast, 
# I have implemented optimized versions in C,
# which do count adjacent bytes (tuples) only once,
# and update the tuples dictionary while compressing.
# However, other compression algorithms still give better results,
# in terms of compression ratio and compression speed.
#
# The advantages of this algorithm are somewhere else:
#
#  - Quite constant ratio, the theoretical maximum is at round about 99%
#    (for a file consisting only of, e.g, 0's. In the normal usecase
#    the ratio is around 50%, for most text files
#
#  - The decompression is lightning fast.
#    It can even exceed the memory throughput,
#    since in the medium only 50% of the decompressed data are to be read, 
#    the decompression consist of three lines of code(!),
#    (the instrcutions might fit into a single cacheline)
#    and the dictionary used for decompression is at 254Bytes,
#    fitting into (at least) the L1 Cache.
#
#  - So this might be of good use for harddisk compression 
#    (e.g. the root file system, which is seldom written to,
#    but mainly read from), or for embedded targets
#
#  - The compressed files are searchable, without having them 
#    decompressed.
#	
#
# usage: cat file.txt | ./comp.pl > file.z
#
# file shouldn't exceed much more than, say, 128kB.
# I experimented a bit with the algorithm,
# blocksizes > 64kB give rarely better results in the compression ratio,
# but do need exponentially more resources when compressing.
# (Still within resonable limits, but there simply isn't enough gain)
#
# The decompression, however, stays constant in terms of complexity and memory usage,
# at O(1). 
# The memory usage needs 254 Bytes for the dict, + the stack for the recursive
# decompression function. The recursion, however, is (in the "worst" case) at a max
# of 127. Which still fit with a bit of fiddling into the redzone 
# (the zone, which gcc keeps free on the stack, usable by leaf functions).
# There's also the possibility to limit the recursion to, e.g., 64 or 32;
# without having any real loss.

my $in="";

while (<>){
		$in .= $_;
}

sub comp{
		my $token = shift;
		my $dt = shift;
		my $s = shift;

		my %tupels;
				for my $a ( 0..(length($$s)-2 )){
						$tupels{substr( $$s, $a, 2 )}++;
				}
		my $c = 0;
		my $kk = "";

		foreach my $k ( keys(%tupels) ){
				if ( $tupels{$k} > $c ){
						$c = $tupels{$k};
						$kk = $k;
				}
		}

	 my $p=0;
	 while( ($p=index($$s,$kk,$p))!=-1 ){
			 substr($$s,$p,2)=$token;
	 }
	 $$dt.=$kk;
}


print STDERR "len uncompressed: ", length($in),"\n";

my $dt=""; # dictionary

# convert chars c > 127 to chr(128).chr(c-128)
# sort of "base128" encoding
$in =~ s/([\x{80}-\x{FF}])/"\x{80}".chr(ord($1)-128)/sge;

print STDERR 'len, "base128" encoded: ', length($in),"\n";

# my eyes are bleeding, looking at these odd numbers.
# however, for the sake of simplicity, I keep this at it is
for my $a ( 129..255 ){
		comp( chr($a), \$dt, \$in );
}

print STDERR "len compressed: ", length($in),"\n(+254 Bytes for the dict)\n";

binmode STDOUT;
print $dt;
print $in;


