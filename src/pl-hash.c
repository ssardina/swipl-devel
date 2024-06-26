/*  Part of SWI-Prolog

    Author:	Austin Appleby
    License:	Public domain
    See:	http://murmurhash.googlepages.com/
*/

#ifdef NO_SWIPL
#include <stdint.h>
#define DEBUG(l,g) (void)0
typedef uint64_t word;
#define static_assertion(condition) (void)0
#else
#include "pl-incl.h"
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
The first one is actually  MurmurHashNeutral2().   It  produces the same
hash  as  MurmurHashAligned2()  on  little    endian  machines,  but  is
significantly  slower.  MurmurHashAligned2()  however    is   broken  on
big-endian machines, as it produces different   hashes, depending on the
alignment.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define MIX(h,k,m) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

#if WORDS_BIGENDIAN

unsigned int
MurmurHashAligned2(const void * key, size_t len, unsigned int seed)
{ const unsigned int m = 0x5bd1e995;
  const int r = 24;
  unsigned int h = seed ^ len;
  const unsigned char * data = (const unsigned char *)key;

  while( len >= 4 )
  { unsigned int k;

    k  = data[0];
    k |= data[1] << 8;
    k |= data[2] << 16;
    k |= data[3] << 24;

    MIX(h,k,m);

    data += 4;
    len -= 4;
  }

  switch( len )
  { case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
      h *= m;
  };

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

#else /*WORDS_BIGENDIAN*/

unsigned int
MurmurHashAligned2(const void *key, size_t len, unsigned int seed)
{ const unsigned int m = 0x5bd1e995;
  const int r = 24;
  const unsigned char * data = (const unsigned char *)key;
  unsigned int h = seed ^ (unsigned int)len;
  size_t align = (size_t)(uintptr_t)data & 3;

  DEBUG(0, assert(sizeof(int) == 4));

  if ( align && (len >= 4) )
  { unsigned int t = 0, d = 0;
    int sl, sr;

    switch( align )
    { case 1: t |= data[2] << 16;
      case 2: t |= data[1] << 8;
      case 3: t |= data[0];
    }

    t <<= (8 * align);

    data += 4-align;
    len -= 4-align;

    sl = 8 * (4-(int)align);
    sr = 8 * (int)align;

    while ( len >= 4 )
    { unsigned int k;

      d = *(unsigned int *)data;
      t = (t >> sr) | (d << sl);

      k = t;
      MIX(h,k,m);
      t = d;

      data += 4;
      len -= 4;
    }

    d = 0;

    if ( len >= align )
    { unsigned int k;

      switch( align )
      { case 3: d |= data[2] << 16;
	case 2: d |= data[1] << 8;
	case 1: d |= data[0];
      }

      k = (t >> sr) | (d << sl);
      MIX(h,k,m);

      data += align;
      len -= align;

      switch(len)
      { case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
	h *= m;
      };
    } else
    { switch(len)
      { case 3: d |= data[2] << 16;
	case 2: d |= data[1] << 8;
	case 1: d |= data[0];
	case 0: h ^= (t >> sr) | (d << sl);
	h *= m;
      }
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  } else
  { while( len >= 4 )
    { unsigned int k = *(unsigned int *)data;

      MIX(h,k,m);

      data += 4;
      len -= 4;
    }

    switch(len)
    { case 3: h ^= data[2] << 16;
      case 2: h ^= data[1] << 8;
      case 1: h ^= data[0];
      h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  }
}

#endif /*WORDS_BIGENDIAN*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Optimized version for our Prolog word type
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if WORDS_BIGENDIAN
static int
swap_bytes(unsigned int v)
{ union { unsigned int i;
	  const unsigned char c[4];
	} u;

  u.i = v;
  return u.c[0] | u.c[1]<<8 | u.c[2]<<16 | u.c[3]<<24;
}
#endif

unsigned int
MurmurHashWord(word v, unsigned int seed)
{ const unsigned int m = 0x5bd1e995;
  const int r = 24;
  unsigned int h = seed ^ sizeof(v);
  unsigned int k;

  static_assertion(sizeof(v) == 8);

#if WORDS_BIGENDIAN
  k = swap_bytes((unsigned int)(v>>32));
  MIX(h,k,m);
  k = swap_bytes((unsigned int)v);
#else
  k = v & 0xffffffff;
  MIX(h,k,m);
  k = (unsigned int)(v>>32) & 0xffffffff;
#endif
  MIX(h,k,m);

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  DEBUG(CHK_SECURE, assert(MurmurHashAligned2(&v, sizeof(v), seed) == h));

  return h;
}
