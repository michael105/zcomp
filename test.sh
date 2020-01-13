#!/bin/sh

if [ ! $1 ]; then
		echo usage: testperl.sh filename
		exit 1
fi

cat $1 | ./zcomp.pl | ./zdecomp.pl | diff -sq - $1 

