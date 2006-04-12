/*
 *   Copyright (c) International Business Machines Corp., 2000-2002
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or 
 *   (at your option) any later version.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software 
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "jfs.h"
#include "jfs_unicode.h"

static struct utf8_table utf8_table[] =
{
  {0x80,  0x00,   0*6,    0x7F,           0,         /* 1 byte sequence */},
  {0xE0,  0xC0,   1*6,    0x7FF,          0x80,      /* 2 byte sequence */},
  {0xF0,  0xE0,   2*6,    0xFFFF,         0x800,     /* 3 byte sequence */},
  {0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,   /* 4 byte sequence */},
  {0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,  /* 5 byte sequence */},
  {0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000, /* 6 byte sequence */},
  {0,                            /* end of table    */}
};


int 
Unicode_Character_to_UTF8_Character(uint8_t *s, uint16_t wc, int maxlen)
{
  long l;
  int c, nc;
  struct utf8_table *t;
  if (s == 0)
    return 0;

  l = wc;
  nc = 0;
  for (t = utf8_table; t->cmask && maxlen; t++, maxlen--) 
    {
      nc++;
      if (l <= t->lmask) 
	{
	  c = t->shift;
	  *s = t->cval | (l >> c);
	  while (c > 0) 
	    {
	      c -= 6;
	      s++;
	      *s = 0x80 | ((l >> c) & 0x3F);
	    }
	  return nc;
	}
    }
  return -1;
}


int 
Unicode_String_to_UTF8_String(uint8_t *s, const uint16_t *pwcs,
				  int maxlen)
{
  const uint16_t *ip;
  uint8_t *op;
  int size;

  op = s;
  ip = pwcs;
   
  while (*ip && maxlen > 0) 
    {
      if (*ip > 0x7f) 
	{
	  size = Unicode_Character_to_UTF8_Character(op, *ip, maxlen);
	  if (size == -1) 
	    {
	      /* Ignore character and move on */
	      maxlen--;
	    } 
	  else 
	    {
	      op += size;
	      maxlen -= size;
	    }
	} 
      else 
	{
	  *op++ = (uint8_t) *ip;
	  maxlen--;
	}
      ip++;
    }
  return(op - s);
}

