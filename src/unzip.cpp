// NOTE: Jikes incorporates compression code from the Info-ZIP
// group. There are no extra charges or costs due to the use of
// this code, and the original compression sources are freely
// available from http://www.cdrom/com/pub/infozip/ or
// ftp://ftp.cdrom.com/pub/infozip/ on the Internet.
// The sole use by Jikes of this compression code is contained in the
// files unzip.h and unzip.cpp, which are based on Info-ZIP's inflate.c and
// associated header files.
//

//
// You can do whatever you like with this source file, though I would
// prefer that if you modify it and redistribute it that you include
// comments to that effect with your name and the date.  Thank you.
// The abbreviated History list below includes the work of the
// following:
// M. Adler, G. Roelofs, J-l. Failly, J. Bush, C. Ghisler, A. Verheijen,
// P. Kienitz, C. Spieler, S. Maxwell, J. Altman
// Only the first and last entries from the original inflate.c are
// reproduced here.
//

//
// History:
// vers    date          who           what
// ----  ---------  --------------  ------------------------------------
//  a    ~~ Feb 92  M. Adler        used full (large, one-step) lookup table
//  ...
//  c16  20 Apr 97  J. Altman       added memzero(v[]) in huft_build()
//

#include "unzip.h"

#ifdef HAVE_JIKES_NAMESPACE
namespace Jikes { // Open namespace Jikes block
#endif

unsigned long Unzip::global_bb;                         /* bit buffer */
unsigned Unzip::global_bk;                    /* bits in bit buffer */

unsigned Unzip::global_wp;  /* current position in slide */
unsigned Unzip::global_hufts; /* huff memory usage */
unsigned char Unzip::slide_buffer[32768];
struct huft *Unzip::global_fixed_tl;    /* inflate static */
struct huft *Unzip::global_fixed_td;    /* inflate static */
int Unzip::global_fixed_bl,
    Unzip::global_fixed_bd;
#ifdef UNIX_FILE_SYSTEM
    FILE *Unzip::global_file; /* file pointer for zip file */
#elif defined(WIN32_FILE_SYSTEM)
    char *Unzip::global_file; /* file pointer for zip file */
#endif
char *Unzip::global_bufferp; /* current position in output buffer */

/* Tables for deflate from PKZIP's appnote.txt. */
unsigned Unzip::border[] = {    /* Order of the bit length code lengths */
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
unsigned short Unzip::cplens[] = {         /* Copy lengths for literal codes 257..285 */
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
        /* note: see note #13 above about the 258 in this list. */
unsigned short Unzip::cplext[] = {         /* Extra bits for literal codes 257..285 */
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99}; /* 99==invalid */
unsigned short Unzip::cpdist[] = {         /* Copy offsets for distance codes 0..29 */
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577};
unsigned short Unzip::cpdext[] = {         /* Extra bits for distance codes */
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
        12, 12, 13, 13};


/* moved to consts.h (included in unzip.c), resp. funzip.c */
/* And'ing with mask_bits[n] masks the lower n bits */
unsigned short Unzip::mask_bits[] = {
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

int Unzip::lbits = 9;           /* bits in base literal/length lookup table */
int Unzip::dbits = 6;           /* bits in base distance lookup table */

struct huft *fixed_tl = (struct huft *) 0;

int Unzip::huft_build(unsigned *b,unsigned n, unsigned s, unsigned short *d, unsigned short *e, struct huft **t, int *m)
/*unsigned *b             code lengths in bits (all assumed <= BMAX) */
/*unsigned n              number of codes (assumed <= N_MAX) */
/*unsigned s              number of simple-valued codes (0..s-1) */
/* unsigned short *d                  list of base values for non-simple codes */
/*ush *e                  list of extra bits for non-simple codes */
/*struct huft **t         result: starting table */
/*int *m                  maximum lookup bits, returns actual */
/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return zero on success, one if
   the given code set is incomplete (the tables are still built in this
   case), two if the input is invalid (all zero length codes or an
   oversubscribed set of lengths), and three if not enough memory.
   The code with value 256 is special, and the tables are constructed
   so that no bits beyond that code are fetched when that code is
   decoded. */
{
  unsigned a;                   /* counter for codes of length k */
  unsigned c[BMAX+1];           /* bit length count table */
  unsigned el;                  /* length of EOB code (value 256) */
  unsigned f;                   /* i repeats in table every f entries */
  int g;                        /* maximum code length */
  int h;                        /* table level */
  unsigned i;          /* counter, current code */
  unsigned j;          /* counter */
  int k;               /* number of bits in current code */
  int lx[BMAX+1];               /* memory for l[-1..BMAX-1] */
  int *l = lx+1;                /* stack of bits per table */
  unsigned *p;         /* pointer into c[], b[], or v[] */
  struct huft *q;      /* points to current table */
  struct huft r;                /* table entry for structure assignment */
  struct huft *u[BMAX];         /* table stack */
  unsigned v[N_MAX];            /* values in order of bit length */
  int w;               /* bits before this table == (l * h) */
  unsigned x[BMAX+1];           /* bit offsets, then code stack */
  unsigned *xp;                 /* pointer into x */
  int y;                        /* number of dummy codes added */
  unsigned z;                   /* number of entries in current table */


  /* Generate counts for each bit length */
  el = n > 256 ? b[256] : BMAX; /* set length of EOB code, if any */
  memset((char *)c, 0, sizeof(c));
  p = b;
  i = n;
  do {
    c[*p]++;
    p++;               /* assume all entries <= BMAX */
  } while (--i);
  if (c[0] == n)                /* null input--all zero length codes */
  {
    *t = (struct huft *)0;
    *m = 0;
    return 0;
  }


  /* Find minimum and maximum length, bound *m by those */
  for (j = 1; j <= BMAX; j++)
    if (c[j])
      break;
  k = j;                        /* minimum code length */
  if ((unsigned)*m < j)
    *m = j;
  for (i = BMAX; i; i--)
    if (c[i])
      break;
  g = i;                        /* maximum code length */
  if ((unsigned)*m > i)
    *m = i;


  /* Adjust last length count to fill out codes, if needed */
  for (y = 1 << j; j < i; j++, y <<= 1)
    if ((y -= c[j]) < 0)
      return 2;                 /* bad input: more codes than bits */
  if ((y -= c[i]) < 0)
    return 2;
  c[i] += y;


  /* Generate starting offsets into the value table for each length */
  x[1] = j = 0;
  p = c + 1;  xp = x + 2;
  while (--i) {                 /* note that i == g from above */
    *xp++ = (j += *p++);
  }


  /* Make a table of values in order of bit lengths */
  memset((char *) v, 0, sizeof(v));
  p = b;
  i = 0;
  do {
    if ((j = *p++) != 0)
      v[x[j]++] = i;
  } while (++i < n);
  n = x[g];                     /* set n to length of v */


  /* Generate the Huffman codes and for each, make the table entries */
  x[0] = i = 0;                 /* first Huffman code is zero */
  p = v;                        /* grab values in bit order */
  h = -1;                       /* no tables yet--level -1 */
  w = l[-1] = 0;                /* no bits decoded yet */
  u[0] = (struct huft *)0;   /* just to keep compilers happy */
  q = (struct huft *)0;      /* ditto */
  z = 0;                        /* ditto */

  /* go through the bit lengths (k already is bits in shortest code) */
  for (; k <= g; k++)
  {
    a = c[k];
    while (a--)
    {
      /* here i is the Huffman code of length k bits for value *p */
      /* make tables up to required level */
      while (k > w + l[h])
      {
        w += l[h++];            /* add bits already decoded */

        /* compute minimum size table less than or equal to *m bits */
        z = (z = g - w) > (unsigned)*m ? *m : z;        /* upper limit */
        if ((f = 1 << (j = k - w)) > a + 1)     /* try a k-w bit table */
        {                       /* too few codes for k-w bit table */
          f -= a + 1;           /* deduct codes from patterns left */
          xp = c + k;
          while (++j < z)       /* try smaller tables up to z bits */
          {
            if ((f <<= 1) <= *++xp)
              break;            /* enough codes to use up j bits */
            f -= *xp;           /* else deduct codes from patterns */
          }
        }
        if ((unsigned)w + j > el && (unsigned)w < el)
          j = el - w;           /* make EOB code end at table */
        z = 1 << j;             /* table entries for j-bit table */
        l[h] = j;               /* set table size in stack */

        /* allocate and link in new table */
        if ((q = (struct huft *)malloc((z + 1)*sizeof(struct huft))) == (struct huft *)0)
        {
          if (h)
            huft_free(u[0]);
          return 3;             /* not enough memory */
        }
        global_hufts += z + 1;         /* track memory usage */
        *t = q + 1;             /* link to list for huft_free() */
        *(t = &(q->v.t)) = (struct huft *)0;
        u[h] = ++q;             /* table starts after link */

        /* connect to last table, if there is one */
        if (h)
        {
          x[h] = i;             /* save pattern for backing up */
          r.b = (unsigned char)l[h-1];    /* bits to dump before this table */
          r.e = (unsigned char)(16 + j);  /* bits in this table */
          r.v.t = q;            /* pointer to this table */
          j = (i & ((1 << w) - 1)) >> (w - l[h-1]);
          u[h-1][j] = r;        /* connect to last table */
        }
      }

      /* set up table entry in r */
      r.b = (unsigned char)(k - w);
      if (p >= v + n)
        r.e = 99;               /* out of values--invalid code */
      else if (*p < s)
      {
        r.e = (unsigned char)(*p < 256 ? 16 : 15);  /* 256 is end-of-block code */
        r.v.n = (unsigned short)*p++;                /* simple code is just the value */
      }
      else
      {
        r.e = (unsigned char)e[*p - s];   /* non-simple--look up in lists */
        r.v.n = d[*p++ - s];
      }

      /* fill code-like entries with r */
      f = 1 << (k - w);
      for (j = i >> w; j < z; j += f)
        q[j] = r;

      /* backwards increment the k-bit code i */
      for (j = 1 << (k - 1); i & j; j >>= 1)
        i ^= j;
      i ^= j;

      /* backup over finished tables */
      while ((i & ((1 << w) - 1)) != x[h])
        w -= l[--h];            /* don't need to update q */
    }
  }


  /* return actual size of base table */
  *m = l[0];


  /* Return true (1) if we were given an incomplete table */
  return y != 0 && g != 1;
}


int Unzip::huft_free(struct huft *t)
/*struct huft *t          table to free */
/* Free the malloc'ed tables built by huft_build(), which makes a linked
   list of the tables it made, with the links in a dummy first entry of
   each table. */
{
  struct huft *p, *q;


  /* Go through linked list, freeing from the malloced (t[-1]) address. */
  p = t;
  while (p != (struct huft *)0)
  {
    q = (--p)->v.t;
    free(p);
    p = q;
  }
  return 0;
}


int Unzip::inflate_codes(struct huft *tl, struct huft * td, int  bl, int bd)
/*struct huft *tl, *td literal/length and distance decoder tables */
/* int bl, bd;              number of bits decoded by tl[] and td[] */
/* inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. */
{
  unsigned e;  /* table entry flag/number of extra bits */
  unsigned n, d;        /* length and index for copy */
  unsigned w;           /* current window position */
  struct huft *t;       /* pointer to table entry */
  unsigned ml, md;      /* masks for bl and bd bits */
  unsigned long b;       /* bit buffer */
  unsigned k;  /* number of bits in bit buffer */


  /* make local copies of globals */
  b = global_bb;                       /* initialize bit buffer */
  k = global_bk;
  w = global_wp;                       /* initialize window position */


  /* inflate the coded data */
  ml = mask_bits[bl];           /* precompute masks for speed */
  md = mask_bits[bd];
  while (1)                     /* do until end of block */
  {
    NEEDBITS((unsigned)bl)
    if ((e = (t = tl + ((unsigned)b & ml))->e) > 16)
      do {
        if (e == 99)
          return 1;
        DUMPBITS(t->b)
        e -= 16;
        NEEDBITS(e)
      } while ((e = (t = t->v.t + ((unsigned)b & mask_bits[e]))->e) > 16);
    DUMPBITS(t->b)
    if (e == 16)                /* then it's a literal */
    {
      slide_buffer[w++] = (unsigned char)t->v.n;
      if (w == wsize)
      {
        FLUSH(w);
        w = 0;
      }
    }
    else                        /* it's an EOB or a length */
    {
      /* exit if end of block */
      if (e == 15)
        break;

      /* get length of block to copy */
      NEEDBITS(e)
      n = t->v.n + ((unsigned)b & mask_bits[e]);
      DUMPBITS(e);

      /* decode distance of block to copy */
      NEEDBITS((unsigned)bd)
      if ((e = (t = td + ((unsigned)b & md))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((unsigned)b & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      NEEDBITS(e)
      d = w - t->v.n - ((unsigned)b & mask_bits[e]);
      DUMPBITS(e)

      /* do the copy */
      do {
          n -= (e = (e = wsize - ((d &= wsize-1) > w ? d : w)) > n ? n : e);
#ifndef NOMEMCPY
        if (w - d >= e)         /* (this test assumes unsigned comparison) */
        {
          memmove(slide_buffer + w, slide_buffer + d, e);
          w += e;
          d += e;
        }
        else                    /* do it slowly to avoid memcpy() overlap */
#endif /* !NOMEMCPY */
          do {
            slide_buffer[w++] = slide_buffer[d++];
          } while (--e);
        if (w == wsize)
        {
          FLUSH(w);
          w = 0;
        }
      } while (n);
    }
  }


  /* restore the globals from the locals */
  global_wp = w;                       /* restore global window pointer */
  global_bb = b;                       /* restore global bit buffer */
  global_bk = k;


  /* done */
  return 0;
}


int Unzip::inflate_stored()
/* "decompress" an inflated type 0 (stored) block. */
{
  unsigned n;           /* number of bytes in block */
  unsigned w;           /* current window position */
  unsigned long b;       /* bit buffer */
  unsigned k;  /* number of bits in bit buffer */


  /* make local copies of globals */
  Trace((stderr, "\nstored block"));
  b = global_bb;                       /* initialize bit buffer */
  k = global_bk;
  w = global_wp;                       /* initialize window position */


  /* go to byte boundary */
  n = k & 7;
  DUMPBITS(n);


  /* get the length and its complement */
  NEEDBITS(16)
  n = ((unsigned)b & 0xffff);
  DUMPBITS(16)
  NEEDBITS(16)
  if (n != (unsigned)((~b) & 0xffff))
    return 1;                   /* error in compressed data */
  DUMPBITS(16)


  /* read and output the compressed data */
  while (n--)
  {
    NEEDBITS(8)
    slide_buffer[w++] = (unsigned char)b;
    if (w == wsize)
    {
      FLUSH(w);
      w = 0;
    }
    DUMPBITS(8)
  }


  /* restore the globals from the locals */
  global_wp = w;                       /* restore global window pointer */
  global_bb = b;                       /* restore global bit buffer */
  global_bk = k;
  return 0;
}


int Unzip::inflate_fixed()
/* decompress an inflated type 1 (fixed Huffman codes) block.  We should
   either replace this with a custom decoder, or at least precompute the
   Huffman tables. */
{
  /* if first time, set up tables for fixed blocks */
  Trace((stderr, "\nliteral block"));
  if (global_fixed_tl == (struct huft *)0)
  {
    int i;                /* temporary variable */
    unsigned l[288];      /* length list for huft_build */

    /* literal table */
    for (i = 0; i < 144; i++)
      l[i] = 8;
    for (; i < 256; i++)
      l[i] = 9;
    for (; i < 280; i++)
      l[i] = 7;
    for (; i < 288; i++)          /* make a complete, but wrong code set */
      l[i] = 8;
    global_fixed_bl = 7;
    if ((i = huft_build(l, 288, 257, cplens, cplext,
                        &global_fixed_tl, &global_fixed_bl)) != 0)
    {
      global_fixed_tl = (struct huft *)0;
      return i;
    }

    /* distance table */
    for (i = 0; i < 30; i++)      /* make an incomplete code set */
      l[i] = 5;
    global_fixed_bd = 5;
    if ((i = huft_build(l, 30, 0, cpdist, cpdext,
                        &global_fixed_td, &global_fixed_bd)) > 1)
    {
      huft_free(global_fixed_tl);
      global_fixed_tl = (struct huft *)0;
      return i;
    }
  }

  /* decompress until an end-of-block code */
  return inflate_codes(global_fixed_tl, global_fixed_td,
                             global_fixed_bl, global_fixed_bd) != 0;
}



int Unzip::inflate_dynamic()
/* decompress an inflated type 2 (dynamic Huffman codes) block. */
{
  int i;                /* temporary variables */
  unsigned j;
  unsigned l;           /* last length */
  unsigned m;           /* mask for bit lengths table */
  unsigned n;           /* number of lengths to get */
  struct huft *tl;      /* literal/length code table */
  struct huft *td;      /* distance code table */
  int bl;               /* lookup bits for tl */
  int bd;               /* lookup bits for td */
  unsigned nb;          /* number of bit length codes */
  unsigned nl;          /* number of literal/length codes */
  unsigned nd;          /* number of distance codes */
#ifdef PKZIP_BUG_WORKAROUND
  unsigned ll[288+32]; /* literal/length and distance code lengths */
#else
  unsigned ll[286+30]; /* literal/length and distance code lengths */
#endif
  unsigned long b;       /* bit buffer */
  unsigned k;  /* number of bits in bit buffer */


  /* make local bit buffer */
  Trace((stderr, "\ndynamic block"));
  b = global_bb;
  k = global_bk;


  /* read in table lengths */
  NEEDBITS(5)
  nl = 257 + ((unsigned)b & 0x1f);      /* number of literal/length codes */
  DUMPBITS(5)
  NEEDBITS(5)
  nd = 1 + ((unsigned)b & 0x1f);        /* number of distance codes */
  DUMPBITS(5)
  NEEDBITS(4)
  nb = 4 + ((unsigned)b & 0xf);         /* number of bit length codes */
  DUMPBITS(4)
#ifdef PKZIP_BUG_WORKAROUND
  if (nl > 288 || nd > 32)
#else
  if (nl > 286 || nd > 30)
#endif
    return 1;                   /* bad lengths */


  /* read in bit-length-code lengths */
  for (j = 0; j < nb; j++)
  {
    NEEDBITS(3)
    ll[border[j]] = (unsigned)b & 7;
    DUMPBITS(3)
  }
  for (; j < 19; j++)
    ll[border[j]] = 0;


  /* build decoding table for trees--single level, 7 bit lookup */
  bl = 7;
  i = huft_build(ll, 19, 19, 0, 0, &tl, &bl);
  if (bl == 0)                        /* no bit lengths */
    i = 1;
  if (i)
  {
    if (i == 1)
      huft_free(tl);
    return i;                   /* incomplete code set */
  }


  /* read in literal and distance code lengths */
  n = nl + nd;
  m = mask_bits[bl];
  i = l = 0;
  while ((unsigned)i < n)
  {
    NEEDBITS((unsigned)bl)
    j = (td = tl + ((unsigned)b & m))->b;
    DUMPBITS(j)
    j = td->v.n;
    if (j < 16)                 /* length of code in bits (0..15) */
      ll[i++] = l = j;          /* save last length in l */
    else if (j == 16)           /* repeat last length 3 to 6 times */
    {
      NEEDBITS(2)
      j = 3 + ((unsigned)b & 3);
      DUMPBITS(2)
      if ((unsigned)i + j > n)
        return 1;
      while (j--)
        ll[i++] = l;
    }
    else if (j == 17)           /* 3 to 10 zero length codes */
    {
      NEEDBITS(3)
      j = 3 + ((unsigned)b & 7);
      DUMPBITS(3)
      if ((unsigned)i + j > n)
        return 1;
      while (j--)
        ll[i++] = 0;
      l = 0;
    }
    else                        /* j == 18: 11 to 138 zero length codes */
    {
      NEEDBITS(7)
      j = 11 + ((unsigned)b & 0x7f);
      DUMPBITS(7)
      if ((unsigned)i + j > n)
        return 1;
      while (j--)
        ll[i++] = 0;
      l = 0;
    }
  }


  /* free decoding table for trees */
  huft_free(tl);


  /* restore the global bit buffer */
  global_bb = b;
  global_bk = k;


  /* build the decoding tables for literal/length and distance codes */
  bl = lbits;
  i = huft_build(ll, nl, 257, cplens, cplext, &tl, &bl);
  if (bl == 0)                        /* no literals or lengths */
    i = 1;
  if (i)
  {
    if (i == 1) {
      MESSAGE((unsigned char *)"(incomplete l-tree)  ", 21L, 1);
      huft_free(tl);
    }
    return i;                   /* incomplete code set */
  }
  bd = dbits;
  i = huft_build(ll + nl, nd, 0, cpdist, cpdext, &td, &bd);
  if (bd == 0 && nl > 257)    /* lengths but no distances */
  {
    MESSAGE((unsigned char *)"(incomplete d-tree)  ", 21L, 1);
    huft_free(tl);
    return 1;
  }
  if (i == 1) {
#ifdef PKZIP_BUG_WORKAROUND
    i = 0;
#else
    if (!G.qflag)
      MESSAGE((unsigned char *)"(incomplete d-tree)  ", 21L, 1);
    huft_free(td);
#endif
  }
  if (i)
  {
    huft_free(tl);
    return i;
  }


  /* decompress until an end-of-block code */
  if (inflate_codes(tl, td, bl, bd))
    return 1;


  /* free the decoding tables, return */
  huft_free(tl);
  huft_free(td);
  return 0;
}



int Unzip::inflate_block(int *e)
/*  int *e                last block flag */
/* decompress an inflated block */
{
  unsigned t;           /* block type */
  unsigned long b;       /* bit buffer */
  unsigned k;  /* number of bits in bit buffer */


  /* make local bit buffer */
  b = global_bb;
  k = global_bk;


  /* read in last block bit */
  NEEDBITS(1)
  *e = (int)b & 1;
  DUMPBITS(1)


  /* read in block type */
  NEEDBITS(2)
  t = (unsigned)b & 3;
  DUMPBITS(2)


  /* restore the global bit buffer */
  global_bb = b;
  global_bk = k;


  /* inflate that block type */
  if (t == 2)
    return inflate_dynamic();
  if (t == 0)
    return inflate_stored();
  if (t == 1)
    return inflate_fixed();


  /* bad block type */
  return 2;
}


int Unzip::inflate_free()
{
  if (global_fixed_tl != (struct huft *)0)
  {
    huft_free(global_fixed_td);
    huft_free(global_fixed_tl);
    global_fixed_td = global_fixed_tl = (struct huft *)0;
  }
  return 0;
}

#ifdef UNIX_FILE_SYSTEM
    int Unzip::unzip8(FILE * zipfile, char *buffer)
#elif defined(WIN32_FILE_SYSTEM)
    int Unzip::unzip8(char *zipfile, char *buffer)
#endif
/* decompress an inflated entry */
{
  int e;                /* last block flag */
  int r;                /* result code */
  unsigned h;           /* maximum struct huft's malloc'ed */

  /* initialize window, bit buffer */
  global_wp = 0;
  global_bk = 0;
  global_bb = 0;
  global_file = zipfile;
  global_bufferp = buffer;


  /* decompress until the last block */
  h = 0;
  do {
    global_hufts = 0;
    if ((r = inflate_block(&e)) != 0)
      return r;
    if (global_hufts > h)
      h = global_hufts;
  } while (!e);


  /* flush out slide_buffer */
  FLUSH(global_wp);


  /* return success */
  Trace((stderr, "\n%u bytes in Huffman tables (%d/entry)\n",
         h * sizeof(struct huft), sizeof(struct huft)));
  return 0;
}


#ifdef UNIX_FILE_SYSTEM
int Unzip::UncompressFile0(FILE* zipfile, char* buffer, long buffer_length)
{
    SystemFread(buffer, sizeof(char), buffer_length, zipfile);
    return 1;
}

int Unzip::UncompressFile1(FILE*, char*, long) { return 0; }
int Unzip::UncompressFile2(FILE*, char*, long) { return 0; }
int Unzip::UncompressFile3(FILE*, char*, long) { return 0; }
int Unzip::UncompressFile4(FILE*, char*, long) { return 0; }
int Unzip::UncompressFile5(FILE*, char*, long) { return 0; }
int Unzip::UncompressFile6(FILE*, char*, long) { return 0; }
int Unzip::UncompressFile7(FILE*, char*, long) { return 0; }

int Unzip::UncompressFile8(FILE* zipfile, char* buffer, long)
{
    int rc = Unzip::unzip8(zipfile, buffer); /* Use Unzip routine to unpack */
    return (rc == 0);
}

int Unzip::UncompressFile9(FILE*, char*, long) { return 0; }

#elif defined(WIN32_FILE_SYSTEM)
int Unzip::UncompressFile0(char* zipfile_buffer, char* buffer,
                           long buffer_length)
{
    memmove(buffer, zipfile_buffer, buffer_length * sizeof(char));
    return 1;
}

int Unzip::UncompressFile1(char*, char*, long) { return 0; }
int Unzip::UncompressFile2(char*, char*, long) { return 0; }
int Unzip::UncompressFile3(char*, char*, long) { return 0; }
int Unzip::UncompressFile4(char*, char*, long) { return 0; }
int Unzip::UncompressFile5(char*, char*, long) { return 0; }
int Unzip::UncompressFile6(char*, char*, long) { return 0; }
int Unzip::UncompressFile7(char*, char*, long) { return 0; }

int Unzip::UncompressFile8(char* zipfile_buffer, char* buffer, long)
{
    /* Use Unzip routine to unpack */
    int rc = Unzip::unzip8(zipfile_buffer, buffer);
    return (rc == 0);
}

int Unzip::UncompressFile9(char*, char*, long) { return 0; }
#endif // WIN32_FILE_SYSTEM


#ifdef HAVE_JIKES_NAMESPACE
} // Close namespace Jikes block
#endif

