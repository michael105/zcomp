#!/bin/perl -w
# Implementation of a compression algorithm,
# especially suitable for text files.
# Or other files, as long as the chars are mostly < ASCII 128
# What is the fact for most text files.
# Please have a look into the README for documentation
  
#
# usage: cat file.txt | ./comp.pl > file.z
#
# file shouldn't exceed much more than, say, 128kB.
# I experimented a bit with the algorithm,
# blocksizes > 64kB give rarely better results in the compression ratio,
# but do need exponentially more resources when compressing.
# (Still within resonable limits, but there simply isn't enough gain)
#
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


