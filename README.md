 ZCOMP
=======

(warning - I checked the perl implementations. Seem to work. <br>
The c implementations I didn't recheck yet, they are just a snapshot of an ongoing work.)

```
 Demonstrational implementation of a losless (DE)compression algorithm,
 especially suitable for text files.
 Or other files, as long as the chars are mostly < ASCII 128
 What is given for most text files.
 The compression is not that fast, 
 I have implemented optimized versions in C,
 which do count adjacent bytes (tuples) only once,
 and update the tuples dictionary while compressing.
 However, other compression algorithms still give better results
 in terms of compression ratio and compression speed.

 The good side of this algorithm is the decompression side.

 The simplest implementation given:
```
```perl
 sub decomp{
    for (0..length($_[0])){
        if ((our $o=ord(our $c=substr($_[0],$_,1)))<129 ){
            $_[1].=$c;
        } else {
            decomp( $ct[$o-129],$_[1] );
        }
    }
  }
```

 The advantages of this specialized algorithm are here:

  - Quite constant ratio, the theoretical maximum is at round about 99%
    (for a file consisting only of, e.g, 0's. In the normal usecase
    the ratio is around 52-54%, for most text files.

  - The decompression is lightning fast.
    It could even exceed memory throughput, (memory beeing the bottleneck)
    since on average only 55% of the decompressed data are to be read, 
    the decompression consists of (~)three lines of code(!),
    (the instructions might fit into a single cacheline)
    and the dictionary used for decompression is at 254Bytes,
    fitting into (at least) the L1 Cache. (I'd like to claim 
		the fastest decompression algorithm available. Ok. Not the perl
		version. But one of the C/Assembly implementations)

  - this might be a good choice for embedded targets or
		html/javascript compression (compress once with higher load, 
		decompress often with low usage of resources)

  - The compressed files are searchable, without having to be 
    decompressed.
	

 usage: 
 		cat file.txt | ./comp.pl > file.z

file (meaning blocksize) shouldn't exceed much more than, say, 128kB.
In experiments with the algorithm,
blocksizes > 64kB showed rarely better results in the compression ratio,
strangely the ratio in most cases gets worse and around 55-60%,
but do need asymmetrically more resources when compressing.
(Still within resonable limits, but there simply isn't enough gain)
So, when experimenting, you might want to split input files before compressing.
Or edit the source.

The decompression, however, stays constant in terms of complexity and memory usage,
at O(1). 
The memory usage needs 254 Bytes for the dict, + the stack for the recursive
decompression function. The recursion is (in the "worst" case) at a max
of 127. Which still fits with a bit of fiddling into the redzone 
(the zone, which gcc keeps free on the stack, usable by leaf functions).
There would also be the possibility to limit the recursion to, e.g., 64 or 32;
without having any real loss in the compression ratio.
(memo: relation of recursion, loops, stack, the redzone and optimizations)


The decompression would work with streams,
but for the sake of simplicity 
here's a buffered version
It's only a presentational implementation anyways.
A more optimized version (work in progress) lives within the C source files.
It also hurts having the odd number 129
I can hardly stand this. It's also said,
having lost the option for bit operation optimizations.
But the conversion within perl from text to the "base128"
encoding is quite easier, than doing something else.
So, I leave this at it is.

memo: 
     most possibly it would be more performant
     decompressing the dict partially, so at least ints (32bits)
     can be written at once.
     This algorithm might also be a good choice
     for a SIMD implementation

..:o now I'm on this very problem again.
Just give me a problem, I'm trying to solve it, 
sparing sleep and. never mind.
Ok. So, 64bit - the decompression dict needs 4 regular registers.
When getting rid of this unlucky base128 encoding -
8 bytes can be checked with regular instructions in one ( more exactly in 1/3 ) cycle, 
whether one of the bytes is compressed. (checking for the 8th bit set. e.g. rax & 0x80808080.80808080  )
for a, say, 50 % compression ratio, this means. ok, split the register, split again. this works in parallel. ah me. I'm going to bed now. EXCEEDING memory throughput might be,
well, hard to manage. When counting a memory access of 8 Bytes with, say, 4 full cycles. What could be a realistic number. However, when virtual parallelization kicks in - It seems close to possible.


Using SIMD, oh. 





