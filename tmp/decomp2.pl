#!/bin/perl




(open F, "<", shift) or die ("Couldn't open arg1");
binmode F;
my $dt;
read( F, $dt, 254 );

open OUT, ">", shift or die ("Couldn't open arg2 for writing");
binmode OUT;

#my $in;
my $out="";

# problem: when the last char equals 128 ("widechar"),
# ... read 4MB. bigger files WILL produce trouble.
# Since I'm going to rewrite this section,
# I'll leave this, at it is.
# compressing blocks > around 64kB isn't of any use in my experience,
# it just needs exponentially more memory, 
# but doesn't give better compression ratio.
# so.
while ( read( F, my $in, 2 << 21  )){
		for ( $a=252; $a >=0; $a-=2 ){
				my $b = $a/2 + 129;
				my $k1=substr( $dt, $a, 2 );
				my $f = chr($b);
				$in =~ s/$f/$k1/sg;
		}
		# convert the "base128" back to ascii
		#$in =~ s/\x{80}(.)/X/sg;
		if ( $in =~ s/\x{80}$//s ){
				print "oh.\n";
				seek F, 1, -1;
		}
		$in =~ s/\x{80}(.)/chr(ord($1)+128)/sge;
		print OUT $in;
		$out.=$in;
}

#print OUT $out;
close OUT;
