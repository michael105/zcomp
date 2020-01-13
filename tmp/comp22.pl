#!/bin/perl -w

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
		print "ccomp: $c   -".ord(substr($kk,0,1))."-".ord(substr($kk,1,1)) ."-    x".ord($token)."\n";
	 my $p=0;
	 while( ($p=index($$s,$kk,$p))!=-1 ){
			 substr($$s,$p,2)=$token;
	 }
	 $$dt.=$kk;
}

print "len: ", length($in),"\n";

my $dt="";

# convert chars c > 127 to chr(128).chr(c-128)
# sort of "base128" encoding
$in =~ s/([\x{80}-\x{FF}])/"\x{80}".chr(ord($1)-128)/sge;

print "len, base128: ", length($in),"\n";

# my eyes are bleeding, looking at these odd numbers.
# however, for the sake of simplicity, I keep this at it is
for my $a ( 129..255 ){
		comp( chr($a), \$dt, \$in );
}

print "len: ", length($in),"\n";

open F, ">", "compr2.cm";
print F $dt;
print F $in;
close F;

