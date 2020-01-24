#if 0
mini_printf
mini_fwrites
mini_itodec
mini_itoHEX
#mini_puts
#mini_putc
mini_buf 4096
mini_open
mini_read
mini_write
mini_start
mini_errno
mini_rand
mini_malloc
INCLUDESRC
OPTFLAG
#shrinkelf


return
#endif



// z2 compression and decompression 
// BSD 3-clause, (c) 2019 Michael (misc) Myer
//
//

//   (I'm pretty sure, I didn't "invent" this compression algorithm)
//		So possibly better call it the "z2 implementation".
//		I just don't know the algorithm,
//		and did experience some interesting outcomes while implementing.
//		e.g., changing the length of the dictionary table to more than 2 didn't have
//		the expected effect of a better compression, 
//		only higher complexity of the implementation.
//
//
//
// z2cat decompresses from stdin to stdout
// unbuffered, which is not very effective.
// The decompression speed is limited by the overhead of the syscalls
// for reading/writing stdout.
//
// It's the tradein for having a tiny executable
// ( 289 Bytes, compiled and statically "linked" with gcc )
//
// The compression rate is quite stable around 50% for most text files.
// Memory used when decompressing is 256 Bytes + 
// the stack needed for the recursive function call to decomp(),
// which rarely exceeds 6 recursive calls.
// 
// (The highly theoretical limit is 128 recursions. Which could only happen, 
// when feeding a text file to the compressor, consisting of only blank spaces and 
// a length of 2^128. Which is not possible at todays architectures,
// since this would be 3.4 * 10^38 Bytes.
// 
// So, if given a blocksize of 1MB, the max. recursion would be 20, (2^20 = 1MB)
// when the block consists of nothing than blank spaces. (or nulls,..)
// which would get compressed to 1Byte + the table ( 40 Bytes )
//
// At the moment, the implementation has a max recursion of 16,
// given by the maximun blocksize of 2^30 Bit
// Experiments show, however, the best ratio between compression speed
// and compression rate is around 64 kB, although this might vary,
// depending on the input.
//
//
// Compressed file format:
// 	numbers in round brackets () are optional
//	numbers in {} mean, there's a (optional) loop,
//	in order to be able to put more data into the header later
//	without breaking backwards compatibility
//
//
// ==================================
// *1 1 Byte "check number" : 0xC2  
// ---
// {*2} 1[+1] Byte: skip x Bytes. (can be 0: ->goto *3)
//   	if the first (msb) bit is set, clear it, shift 8 bits left,
//   	and use the next byte for the lower 8 Bits of the resulting 
//   	16(15) bit value. This coding is named 1[+1] below.
//
//   	skip the value. repeat, until 0 is read.
//
//	       (skipped bytes, optionally)
// (---)
//	 (goto *2*)
//
// (*2a) - again either 1 Byte 0x0 (goto *3). Or again 1[+1] Byte(s) skip x Bytes.
//	 		0x0 serves also as file check. 
//			(It's unlikely to hit a 0 by accident, with a max recursion of 8. 
//			So if there's no 0 read before eof/the 9th recursion,
//	 		something might be wrong)
//
//  -------------------------------
//  Header ends. Block data begin
//  -------------------------------
//
// *3 1 Byte (table length)/2 : 0x80 ( or less. )
//
// (*4) dictionary table 
//
// *5 1 Byte (char replacement table length). (could be 0)
//
// (*6) char replacement table 
// 	( The compression relies on the chars 128-255 being unused.
//  	which is mostly the case in text files and source code.
//  	There might be however some utf8 chars, or e.g. german umlauts.
//  	These can be replaced with the normally unused chars 1-9,11-31.
//  	The char replacement table holds the according replacements)
//
// *7 1[+1] Byte offset of compressed data start. (->*12, if not 0)
//	 		(possibility to ignore/omit len and crc block by passing 0)
//
// (*8) 2 Bytes (compressed block length in Bytes)
//      if the first (most significant) byte is set, clear it,
//      read the following two Bytes (*9) as bsd checksum
//      if the second bit is set, clear it,
//      shift left 16 bits, add the next 2 Bytes to the block len.
//
// ((*9)) 2 Bytes: BSD-checksum of the compressed block
//
// (*10) 2 Bytes uncompressed data block len
// 			1st Bit (msb) set: BSD-checksum of uncompressed data present in *11
//   	  2nd Bit (msb) set: clear it, shift left 16 Bits, 
// 	    add next two bytes to the uncompressed data block len.
//
// ((*11)) 2 Bytes: BSD-checksum of the uncompressed block
//
// (*12) Byte 0 (or 1[+1] skip again, until 0. same as *2)
// 
// *13 compressed data.
//
// *14 1 Byte (selection byte): 
//   				0x0 = EOF
//   				0x2 = -> start again at (*2) 
//   				0x3 = start again with table length (->*3)
//   				0x7 = start again at "next data block" length (*7)
//    
// BSD 3clause, Michael (misc) Myer, (c) 2019

// Ideas: Encryption. 
// 	( Mix several encodings. Noone nows exactly, 
// 	how long it takes to break a chiffre. 
//  Password input. mixture of 2factor, colors, pictures, 
//    scriptable dependence on hour,date, moon, whatever,
//    selecting columns with given chars.
//    randomizing (adding some randomness to the current password,
//    needing a "tiny" bruteforce everytime.
//
// ->password feeder. (encrypted memory for comm.?)
// 
//
// BUG in here.
// I leave it. Better restart.

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long uint64_t;
typedef unsigned int uint32_t;

typedef struct {
		union {
				uint64_t l;
				uint32_t t[1];
		} start,sum;
		unsigned int counter;
} tsctimer;


static inline void sync_tsc(void){
  asm volatile("cpuid" : : : "%rax", "%rbx", "%rcx", "%rdx");
}

static inline void inittsc(tsctimer *t){
		t->start.l = 0;
		t->sum.l = 0;
		t->counter = 0;
}

#define gettsctimer(T) tsctimer T; inittsc(&T);

static inline void starttsc(tsctimer *t){
  asm volatile("rdtsc" : "=a" (t->start.t[0]), "=d" (t->start.t[1]) );
	t->counter++;
}

static inline void endtsc(tsctimer *t){
  unsigned a, d;
  asm volatile("rdtsc" : "=a" (a), "=d" (d) );
	t->sum.l += ((( ((uint64_t) d) << 32 ) | (uint64_t)a)) - t->start.l;
}

static inline unsigned int getsumtsci( tsctimer *t ){
		uint i;
		i = (t->sum.t[1] <<27 ) | ( ( t->sum.t[0] >> 5 ) );
		return(i);
}




typedef struct {
		unsigned char *data;
		unsigned char *buf;
		unsigned char ct[256]; // dictionary byte pairs
		unsigned char rt[256]; // char replacement, chars>127
		unsigned int len;
		unsigned int newlen;
		unsigned int tablelen;
		unsigned int rtlen;
} zblock;

// allocate a new zblock structure
// data: input
// buf: output ( for decompression)
// buf should be the same size as data + 540 Bytes, to be sure
zblock* zinit( unsigned char* data, unsigned char* buf, unsigned int len ){
		zblock *z = (zblock*)malloc( sizeof(zblock) );
		z->data = data;
		z->len = len;
		z->buf = buf;
		z->newlen = len;
		for ( unsigned int *i = (unsigned int*)z->ct; i<(unsigned int*)z->ct+256; i++ )
				*i = 0xffffffff;
		
		return(z);
}



static inline unsigned char* decomps2( unsigned char *buf, unsigned char* bufend, unsigned char*ct, unsigned char *data, unsigned char* data2 ){
		if ( *data > 127 ){
			 	buf = decomps2( buf, bufend, ct, ct+(unsigned char)(*data<<1), ct+((unsigned char)(*data<<1)+1) ); 
		} else {
				*buf = *data;
				if ( buf >= bufend )
						return(buf);
				buf++;
		}
		if ( *data2 > 127 ){
				buf = decomps2( buf, bufend, ct, ct+(unsigned char)(*data2<<1), ct+((unsigned char)(*data2<<1)+1) ); 
		} else {
				*buf = *data2;
				if ( buf >= bufend )
						return(buf);
				buf++;
		}
		return( buf );
}

// with decomps2 about 15% faster than decomp
unsigned int zunpack( zblock *z ){
		unsigned char *data = z->data;
		unsigned char *sbuf = z->buf;
		do {
				if ( *data > 127 ){
						sbuf = decomps2( sbuf, z->buf+z->newlen, z->ct , z->ct+(unsigned char)(*data<<1), z->ct+((unsigned char)(*data<<1)+1) ); 
				} else {
						*sbuf = *data;
						sbuf++;
				}
				data++;
		} while ( (data<=z->data+z->len) && ( sbuf<=z->buf+z->newlen ) );
		return( sbuf - z->buf );
}



unsigned int z2unpack( zblock *z ){
	z->tablelen = z->data[0]*2;
	z->ct[0] = z->data[1];
	z->data = z->data + 1 + z->tablelen;
	z->newlen = zunpack(z);

	return( z->newlen );
}
#if 0
void z2unpack( z2block *z ){
				if ( z->data & 0x80 ){
						z2unpack( ct[(unsigned char)(data<<1)] );
						z2unpack( ct[((unsigned char)(data<<1)+1)]  );
				} else {
						write( STDOUT_FILENO, &data, 1 );
				}
}
#endif

		unsigned char ct[256];
int comp( unsigned char* data, int len ){// int fd ){

		unsigned short int k[256][256];
		unsigned char prev = 0xFF;

		//memset( k, 0, 256*256 * sizeof(short int) );
		for ( long *l = (long*)(k[0]); l<(long*)(k[255]+255);l++){
				*l= 0; // set k to 0. l+1 means l=l+8 here. or which size a long has.
		}

		// count different pairs
		// 'AAA' count as 1, 'AAAA' as 2
		for ( int b = 0; b<len-1; b++ ){
				unsigned char c1 = data[b];
				int p = b+1;
				if ( (c1 == prev) && ( prev == data[p]) ){
						prev = 0xFF;
				} else {
						k[c1][data[p]] = k[c1][data[p]] + 1;
						if ( c1 == data[p] )
								prev = c1;
						else
								prev=0xff;
				}
		}

		int saved = 3;
		int chr;
		for ( chr = 128; (chr<256) && (saved>2); chr++ ){

				saved=0;
				prev=0xff;
				unsigned short int count = 0;

				for ( int i1 =0; i1<chr; i1++ ){
						for ( int i2 =0; i2<chr; i2++ ){
								if ( (ushort)k[i1][i2] > (ushort)count ){
										count = (ushort)k[i1][i2];
										ct[(chr-128)*2] = i1;
										ct[(chr-128)*2+1] = i2;
								}
						}
				}
				int p = 1;
				int b = 0;
				for ( b = 0; p<len; b++ ){
						if ( (data[b] == ct[(chr-128)*2]) && ( data[p] == ct[(chr-128)*2+1] ) ){
								if ( b>0 ){

										if ( prev != chr ){
												if ( (ushort)k[data[b-1]][chr] < (ushort)((ushort)0-1) )
														(ushort)k[data[b-1]][chr]++;
												prev=data[b-1];
										} else {
												prev=0xff;
										}

										if((ushort)k[data[b-1]][data[b]]>0)
												(ushort)k[data[b-1]][data[b]]--;
								}
								if ( p < len ){

										if ( (ushort)k[data[p]][data[p+1]]  > 0 )
												(ushort)k[data[p]][data[p+1]] --;

										if ( (ushort)k[chr][data[p+1]] < (ushort)((ushort)0-1) )
												(ushort)k[chr][data[p+1]]++;
								}

								p++;
								data[b] = chr;
								saved++;
						}
						//if ( p>(b+1) )
						data[b+1] = data[p]; // copy 
						p++;
				}

				k[ct[(chr-128)*2]][ct[(chr-128)*2+1]] = 0;
				len = len-saved;

				//fprintf(stderr, "diff: %d\n", diff );
				fprintf(stderr, "count: %d =  -%2X-%2X-\nsaved: %d\n\n*********\n", (ushort)count, ct[(chr-128)*2], ct[(chr-128)*2+1], saved);
		}

	//	chr--;

		uchar c = (chr-128);
	 //	write( fd, "\xc2", 1 );
		//write( fd, &c, 1 );
		//write( fd, (char*)ct, c*2 ); 

	//	write( fd, data, len );

		fprintf(stderr,"newlen: %d,chr: %X, table len(*2): %d\n", len,chr,c );
		return(len);
}


uint zpack( zblock *z ){

		unsigned short int k[256][256];
		//unsigned char ct[256];
		unsigned char prev = 0xFF;

		//memset( k, 0, 256*256 * sizeof(short int) );
		for ( long *l = (long*)(k[0]); l<(long*)(k[255]+255);l++){
				*l= 0; // set k to 0. l+1 means l=l+8 here. or which size a long has.
		}
		gettsctimer(t1);
		gettsctimer(t2);
		gettsctimer(t3);

		starttsc( &t1 );
		// count different pairs
		// 'AAA' count as 1, 'AAAA' as 2
		for ( uint b = 0; b<z->len-1; b++ ){
				unsigned char c1 = z->data[b];
				int p = b+1;
				if ( (c1 == prev) && ( prev == z->data[p]) ){
						prev = 0xFF;
				} else {
						if ( k[c1][z->data[p]] < (ushort)((ushort)0-1) )
					 			k[c1][z->data[p]] = k[c1][z->data[p]] + 1;
						if ( c1 == z->data[p] )
								prev = c1;
						else
								prev=0xff;
				}
		}

		uint saved = 3;
		uint chr;
		for ( chr = 128; (chr<256) && (saved>2); chr++ ){

				saved=0;
				prev=0xff;
				unsigned short int count = 0;
				for ( int i1 =0; i1<chr; i1++ ){
						for ( int i2 =0; i2<chr; i2++ ){
								if ( (ushort)k[i1][i2] > (ushort)count ){
										count = (ushort)k[i1][i2];
										z->ct[(chr-128)*2] = i1;
										z->ct[(chr-128)*2+1] = i2;
								}
						}
				}
				uint p = 1;
				uint b = 0;
				for ( b = 0; p<z->len; b++ ){
						if ( (z->data[b] == z->ct[(chr-128)*2]) && ( z->data[p] == z->ct[(chr-128)*2+1] ) ){
								if ( b>0 ){

										if ( prev != chr ){
												if ( (ushort)k[z->data[b-1]][chr] < (ushort)((ushort)0-1) )
														(ushort)k[z->data[b-1]][chr]++;
												prev=z->data[b-1];
										} else {
												prev=0xff;
										}

										if((ushort)k[z->data[b-1]][z->data[b]]>0)
												(ushort)k[z->data[b-1]][z->data[b]]--;
								}
								if ( p < z->len ){

										if ( (ushort)k[z->data[p]][z->data[p+1]]  > 0 )
												(ushort)k[z->data[p]][z->data[p+1]] --;

										if ( (ushort)k[chr][z->data[p+1]] < (ushort)((ushort)0-1) )
												(ushort)k[chr][z->data[p+1]]++;
								}

								p++;
								z->data[b] = chr;
								saved++;
						} else {
						//if ( p>(b+1) )
						z->data[b+1] = z->data[p]; // copy 
						p++;
						}
				}

				k[z->ct[(chr-128)*2]][z->ct[(chr-128)*2+1]] = 0;
				z->len = z->len-saved;

				//fprintf(stderr, "diff: %d\n", diff );
				//fprintf(stderr, "a: %X\ncount: %d =  -%2X-%2X-\nsaved: %d\n\n*********\n",a, (ushort)count, ct[(a-128)*2], ct[(a-128)*2+1], saved);
		}
		endtsc(&t1);
	//	chr--;

		unsigned char c = (chr-128); //len of table

		z->tablelen = c;
	 //	write( fd, "\xc2", 1 );
		//write( fd, &c, 1 );
		//write( fd, (char*)ct, c*2 ); 

		//write( fd, data, len );

		fprintf(stderr,"newlen: %d,chr: %X, table len(*2): %d\n", z->len,chr,c );
		//fprintf(stderr,"t1: %u\nt2: %u\nt3: %u\n", getsumtsci(&t1), getsumtsci(&t2), getsumtsci(&t3) );
		//fprintf(stderr,"t4: %u\nt5: %u\n", getsumtsci(&t4), getsumtsci(&t5) );
		return(z->len);
}

// copies first from inbuf to outbuf.
unsigned int zpackbuf( char *inbuf, unsigned int len, char *outbuf, unsigned int buflen ){
				
	return(0);
}



#define LEN 60000

#ifdef MLIB

int main( int argc, char *argv[] ){

		unsigned char data[LEN];

		srand( 123124 );
		union {
				uchar c[LEN];
				int i[LEN/4];
		} u,u2;

		//zblock *z = zinit( u.c, data, LEN );
		zblock z;
		z.data = u.c;
		z.buf = data;
		z.len = LEN;
		z.newlen = LEN;

		for ( int a = 0; a<LEN/4; a++ ){
				u.i[a] = rand();
				for ( int b = 0; b<4; b++ ){
						if ( u.c[a*4+b] > 106 )
								u.c[a*4+b] = 32;
				}
				u2.i[a] = u.i[a];
				//printf("a: %d  rand: %d\n",a, u.i[a] );
		}
		
		for ( int a = 0; a<LEN; a++ ){
				if ( z.data[a] != u2.c[a] ){
						printf("a: %d  XXXX z.buf[a]: %d, u2.c[a]: %d \n",a, z.buf[a], u2.c[a] );
						exit(1);
				}

		}



		printf("len: %d\n", LEN);

		tsctimer t1;
		inittsc(&t1);
		starttsc(&t1);
		printf("timer: %u  %u\n", t1.start.t[1], t1.start.t[0] );
		endtsc( &t1 );
		printf("Elapsed: %u   %u\n", t1.sum.t[1], t1.sum.t[0] );
		printf("Elapsed, getsum: %u\n", getsumtsci(&t1) );

		inittsc(&t1);
		gettsctimer(t2);
		gettsctimer(t3);
		gettsctimer(t4);
		starttsc( &t3 );
		starttsc( &t4 );
		for ( int a = 0; a<1000; a++ ){
				starttsc( &t2 );
				endtsc( &t4 );
				starttsc( &t4 );
				endtsc( &t2 );
		}
		endtsc( &t3 );
		printf("Elapsed: %u   %u\n", t1.sum.t[1], t1.sum.t[0] );
		printf("Elapsed, getsum:\nt1 %u\nt2 %u\nt3 %u\nt4 %u\n", getsumtsci(&t1), getsumtsci(&t2),getsumtsci(&t3), getsumtsci(&t4) );

		int  fd = open( "ti", O_CREAT | O_WRONLY, 0600 );
		write(fd, z.data, z.len );
		close(fd);
		
 

		int r = comp( z.data, z.len );
		z.len = r;
		//int r = zpack( &z );

		printf("z->len: %d\n", z.len);

		//endtsc( &t1 );
		printf("Elapsed: %u   %u\n", t1.sum.t[1], t1.sum.t[0] );
		printf("Elapsed, getsum: %u\n", getsumtsci(&t1) );

		//unsigned char* c = z.buf;
		//z.buf = z.data;
		//z.data = c;
		//z.len = z.newlen;
		fd = open( "to.z", O_CREAT | O_WRONLY, 0600 );

		char cl = 128;
		write(fd, &cl, 1 );
		write(fd, (char*)ct, 256 );
		write(fd, z.data, z.len );
		close(fd);
		
		starttsc( &t1 );
		zunpack(&z);

		fd = open( "tii", O_CREAT | O_WRONLY, 0600 );
		write(fd, z.buf, z.newlen );
		close( fd );
		
		endtsc( &t1 );
		printf("Len: %u\nNewlen: %u\nT: %u\n", z.len, z.newlen, getsumtsci(&t1) );

		for ( int a = 0; a<LEN; a++ ){
				if ( z.buf[a] != u2.c[a] ){
						printf("a: %d  XXXX z.buf[a]: %d, u2.c[a]: %d \n",a, z.buf[a], u2.c[a] );
						exit(1);
				}

		}

		return(0);
}
#endif
