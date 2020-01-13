#!/bin/perl -w
#




sub theref{
		my $s=shift;
		print "ref: $s\n";
		return("X");
}


my $s = shift;


$s =~ s/(Y|Z)/theref($1)/ge;


print "s: $s\n";


