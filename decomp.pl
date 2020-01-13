#!/bin/perl -w
#
# usage: cat file.z | ./decomp.pl > file.txt
#
# More information on the algorithm is in zcomp.pl
# 

binmode STDIN;
binmode STDOUT;

my $dt; # compression dict
read( STDIN, $dt, 254 );

my @ct;

# build an array for the dict.
# Not really neccessary, (a string is already an array)
# but this sourcefile gets easier to read
# and understand
for($a=0; $a <=252; $a+=2 ){
		push @ct, substr( $dt, $a, 2 );
}


# The recursive decompression function
# args: in, out
sub decomp{
		for (0..length($_[0])){
				if ((our $o=ord(our $c=substr($_[0],$_,1)))<129 ){
						$_[1].=$c;
				} else {
						decomp( $ct[$o-129],$_[1] );
				}
		}
}


# Another version,
# I couldn't resist. 
# It's even faster
# Albite .. perlish ;)
sub deco{
	ord(our $c=substr($_[0],$_,1))<129 
		and	$_[1].=$c or deco($ct[ord($c)-129],$_[1])
			for (0..length($_[0]));
}

#
# Read input
# the decompression works with streams,
# but for sake of more simplicity 
# here's a buffered version
# It's only a presentational implementation anyways
# An optimized version lives within the C source files.
# It also hurts having the odd number 129
# I can hardly stand this. It's also said,
# having lost the option for bit operation optimizations.
# But the conversion within perl from text to the "base128"
# encoding is quite easier, than doing something else.
# So, I leave this at it is.
#
# (memo: most possibly it would be more performant
#  decompressing the dict partially, so at least ints (32bits)
#  can be written at once.
#  This algorithm might also be a good choice
#  for SIMD instructions)
#
my $in;
read(STDIN,$in,2<<21);
print STDERR "len compressed: ".length($in)."\n";
my $out="";

# decompress
deco( $in, $out );

# write to stdout
# This howver misses the conversion back from "base128"
# (Please look into comp.pl for an explanation)
print $out;
print STDERR "len uncompressed: ".length($out)."\n";


