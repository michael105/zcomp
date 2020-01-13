#!/bin/perl




open F, "<", "compr2.cm";

my $dt;
read( F, $dt, 256 );
my @ct;

for($a=0; $a <=254; $a+=2 ){
		push @ct, substr( $dt, $a, 2 );
}

print $dt;

my $f;

read( F, $f, 4096 );

#print $in;

my $rec=0;
my $maxrec=0;

sub decomp{
		my $in = shift;
		#my $out = shift;
		#my $p = shift;

		$rec++;
		if($rec>$maxrec){
				$maxrec=$rec;
				print("Maxrec: $maxrec\n");
		}

		my $out = "";
		my $p = 0;

		while( my $c = substr($in,$p,1)){
				if ( ord($c) > 127 ){
						$out .= decomp( $ct[ (ord($c)-128 ) ] );
				}	else {
						$out .= $c;
				}
				$p++;
		}
		$rec--;
		return($out);
}


#print $f;

print "\nxxxxx\n";
print "\nxxxxx\n";
print "\nxxxxx\n";

print( decomp($f) );
#decomp($f);
