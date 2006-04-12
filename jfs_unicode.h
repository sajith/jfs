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

#ifndef JFS_UNICODE_H
#define JFS_UNICODE_H

/* #include "jfs_dirread.h" */

struct utf8_table
{
  int     cmask;
  int     cval;
  int     shift;
  long    lmask;
  long    lval;
};


static inline UniChar *UniStrncpy(UniChar * ucs1, const UniChar *ucs2,
                                  size_t n)
{
  UniChar *anchor = ucs1;

  while (n-- && *ucs2)    /* Copy the strings */
    *ucs1++ = *ucs2++;

  n++;
  while (n--)             /* Pad with nulls */
    *ucs1++ = 0;
  return anchor;
}

#endif
