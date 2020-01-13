#!/bin/perl -w
#



(open F, "<", shift) or die ("Couldn't open arg1");
binmode F;

open OUT, ">", shift or die ("Couldn't open arg2 for writing");
binmode OUT;

my $dt;
read( F, $dt, 254 );

my @ct;

for($a=0; $a <=252; $a+=2 ){
		push @ct, substr( $dt, $a, 2 );
}

# I couldn't resist. 
# It's even faster
# Albite .. perlish ;)
sub deco{
	ord(our $c=substr($_[0],$_,1))<129 
		and	$_[1].=$c or deco($ct[ord($c)-129],$_[1])
			for (0..length($_[0]));
}

# decomp( $in, $out );
sub decomp{
		for (0..length($_[0])){
				if ((our $o=ord(our $c=substr($_[0],$_,1)))<129 ){
						$_[1].=$c;
				} else {
						decomp( $ct[$o-129],$_[1] );
				}
		}
}


my $in;
read(F,$in,2<<21);
print "len: ".length($in)."\n";
my $out="";

deco( $in, $out );

print $out;
print "len: ".length($out)."\n";
