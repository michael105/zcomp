 ZCOMP
=======

```
 Implementation of a compression algorithm,
 especially suitable for text files.
 Or other files, as long as the chars are mostly < ASCII 128
 What is the fact for most text files.
 The compression is not so fast, 
 I have implemented optimized versions in C,
 which do count adjacent bytes (tuples) only once,
 and update the tuples dictionary while compressing.
 However, other compression algorithms still give better results
 in terms of compression ratio and compression speed.
```

 The advantages of this specialized algorithm are here:

  - Quite constant ratio, the theoretical maximum is at round about 99%
    (for a file consisting only of, e.g, 0's. In the normal usecase
    the ratio is around 50%, for most text files

  - The decompression is lightning fast.
    It can even exceed the memory throughput,
    since in the medium only 50% of the decompressed data are to be read, 
    the decompression consist of three lines of code(!),
    (the instrcutions might fit into a single cacheline)
    and the dictionary used for decompression is at 254Bytes,
    fitting into (at least) the L1 Cache.

  - So this might be of good use for harddisk compression 
    (e.g. the root file system, which is seldom written to,
    but mainly read from), or for embedded targets

  - The compressed files are searchable, without having them 
    decompressed.
	

 usage: 
 		cat file.txt | ./comp.pl > file.z

file shouldn't exceed much more than, say, 128kB.
I experimented a bit with the algorithm,
blocksizes > 64kB give rarely better results in the compression ratio,
but do need exponentially more resources when compressing.
(Still within resonable limits, but there simply isn't enough gain)

The decompression, however, stays constant in terms of complexity and memory usage,
at O(1). 
The memory usage needs 254 Bytes for the dict, + the stack for the recursive
decompression function. The recursion, however, is (in the "worst" case) at a max
of 127. Which still fit with a bit of fiddling into the redzone 
(the zone, which gcc keeps free on the stack, usable by leaf functions).
There's also the possibility to limit the recursion to, e.g., 64 or 32;
without having any real loss.


