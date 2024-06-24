# huffman
An implementation of the Huffman code in C18.
## Compatibility
The code should work on any system that supports C18 and the `uint64_t` datatype, including MSb-first as bit numbering is expected. Big-endian and little-endian systems are cross-compatible, except when the decoded file contains only one byte type. The last requirement is a byte consisting of 8 bits.

## Building/Installation
To compile the project, run `make`.  
The Makefile needs to be modified for non linux systems, the resulting executable is in the same directory as the Makefile and is called `huffman`, to install it system wide run `make install`. To remove it, `make uninstall`. Use `make debug` for development , the compiled binary is slower, but includes address sanitiser and debug symbols.

## Usage
```
$ huffman help
Usage:
 huffman [encode|decode] INFILE OUTFILE
 huffman [encode|decode] INFILE
 huffman [encode|decode|help]

Note:
 - may be used for stdin/stdout
 When omitting INFILE/OUTFILE, stdin/stdout is used
```

## Performance
Benchmark on a laptop with an nvme, data is randomly generated:

|| encoding | decoding
|--- | --- | ---
|1 KiB | 0.004s | 0.003s
|1 MiB | 0.020s | 0.031s
|100 MiB | 2.018s | 2.982s
|1 GiB | 10.767s | 15.782s
|10 GiB | 61.474s | 101.383s

## Limitations
The maximum depth for the tree is hardcoded to 64. [This paper](https://tmo.jpl.nasa.gov/progress_report/42-110/110N.PDF) shows that $F_{k+2}$ is the minimum number of symbols required to create a tree of depth $k$, where $F_n$ is the nth Fibonacci number.  
To build a tree of depth 65 (which would fail), you'd need $F_{67}$ bytes. So this Huffman implementation should not fail for up to $F_{67}-1$ bytes, or about $44.94557021$ TiB.    
There is another limit: the frequency per symbol. The frequency of each symbol is stored as `uint64_t`, so each symbol can occur at most $2^{64}-1$ times, which is about $1.844674407\*10^{19}$ or $16'777'216$ TiB.  
The absolute limit that this Huffman implementation can handle is when each symbol frequency is maxed out, so $2^8\*2^64=2^72$, about $4'294'967'296$ TiB.

## The file format
The file format has a simple structure, first the tree is decoded, then comes the encoded data, and the last three bits indicate the number of significant bits in the last byte.  
The tree is stored as described [here](https://stackoverflow.com/a/759766/15833045):  
- Start at root  
- go left, write 0   
    - If leaf, write 1, then symbol, continue  
    - If not leaf, repeat this step  
- go right, write 0  
    - If leaf, write 1, then symbol, continue  
    - If not leaf, repeat this step  

There is a special case when only one type of symbol is encoded, e.g. "AAA". Then the first bit is 1, the second byte is the symbol to decode and the following 8 bytes are the frequency of the symbol as `uint64_t`. Only this scenario isn't cross-compatible between little-endian and big-endian.
