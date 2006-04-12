/*
 *   Copyright (c) Free Software  Foundation, Inc., 2002
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
#include "jfs_dtree.h"

buf_t	 *bCachelist = NULL;
int32_t	 NBUFFER = 8;
int32_t	 nBuffer = 0;

/* forward declaration */
int32_t  xt_Lookup(dinode_t *dip, int64_t xoff, 
		   int64_t *xaddr, int32_t *xlen);
static void bAllocate(void);

void 
get_file(int inum)
{
  int      which_table = FILESYSTEM_I;
  int      i;
  int64_t  address;
  dinode_t inode; 
  int32_t  rc, xlen;
  int64_t  xoff, xaddr;
  char     *buf;
  
  buf = (char *)malloc(10);
  xoff = 0;
  xlen = 5;

  if(find_inode(inum, which_table, &address) ||
     xRead(address, sizeof(dinode_t), (char *)&inode)) 
    {
      jfs_info("inode: error reading inode\n\n");
      return;
    }

  jfs_info ("inum %d \n", inode.di_number);
  rc = xt_Lookup (&inode, xoff, &xaddr, &xlen);
  jfs_info ("address : %d  xlen : %d \n", xaddr, xlen);

  ujfs_read_diskblocks (xaddr*4096, 11, buf);

  for (i=0; i<11; i++)
    jfs_info ("%c\n",buf[i]);

  free (buf);
}

int32_t 
xt_Lookup(dinode_t *dip, int64_t xoff, int64_t *xaddr, int32_t *xlen)
{
  int32_t   rc = 0;
  buf_t	    *bp = NULL;	/* page buffer */
  xtpage_t  *p;	/* page */
  xad_t     *xad;
  int64_t   t64;
  int32_t   cmp = 1, base, index, lim;
  int32_t   t32;
  extern  struct jfs_superblock * jfs_sb;

  p = (xtpage_t *)&dip->di_btroot;
  
 xtSearchPage:
  lim = p->header.nextindex - XTENTRYSTART; 
  
  for (base = XTENTRYSTART; lim; lim >>= 1) 
    {
      index = base + (lim >> 1);
      
      XT_CMP_4(cmp, xoff, &p->xad[index], t64);
      if (cmp == 0) 
	{
	  /*
	   *	search hit
	   */
	  /* search hit - leaf page:
	   * return the entry found
	   */
	  if (p->header.flag & BT_LEAF) 
	    {
	      /* save search result */
	      xad = &p->xad[index];
	      t32 = xoff - offsetXAD(xad);
	      *xaddr = addressXAD(xad) + t32; 
	      *xlen = MIN(*xlen, lengthXAD(xad) - t32);
	      jfs_info("found %s  xoff %d",
		       __FUNCTION__, xoff);
	      jfs_info("xlen  %d \n",*xlen);
	      
	      rc = cmp;
	      goto out;
	    } 
	  /* search hit - internal page:
	   * descend/search its child page 
	   */
	  goto descend;
	}
      if (cmp > 0) 
	{
	  base = index + 1;
	  --lim;
	}
    }

  /*
   *	search miss
   *
   * base is the smallest index with key (Kj) greater than 
   * search key (K) and may be zero or maxentry index.
   */
  /*
   * search miss - leaf page: 
   *
   * return location of entry (base) where new entry with
   * search key K is to be inserted.
   */
  
  /*   jfs_info("not found %s  xoff %d ",__FUNCTION__,xoff); */
  /*   jfs_info("xlen %d \n ",*xlen); */
	
  if (p->header.flag & BT_LEAF) 
    {
      rc = cmp;
      goto out;
    }

  /*
   * search miss - non-leaf page:
   *
   * if base is non-zero, decrement base by one to get the parent
   * entry of the child page to search.
   */

  index = base ? base - 1 : base;

  /* 
   * go down to child page
   */
 descend:
  /* get the child page block number */
    
  t64 = addressXAD(&p->xad[index]);
  
  /* release parent page */
  if (bp)
    bRelease(bp);
        
  /* read child page */
  bRead(t64, PAGESIZE>>jfs_sb->s_l2bsize, &bp);

  p = (xtpage_t *)bp->b_data;
  goto xtSearchPage;

 out:
  /* release current page */
  if (bp)
    bRelease(bp);
  
  return rc;
}


int32_t 
bRead ( int64_t  xaddr,	  /* in bsize */
	int32_t  xlen,	  /* in bsize */
	buf_t    **bpp)
{
  int32_t  rc;
  int64_t  off;	/* offset in byte */
  int32_t  len;	/* length in byte */
  buf_t	   *bp;
  extern struct jfs_superblock *jfs_sb;

  /* allocate buffer */
  if (bCachelist == NULL)
    bAllocate();
  bp = bCachelist;
  bCachelist = bp->b_next;
  
  off = xaddr << jfs_sb->s_l2bsize;
  len = xlen << jfs_sb->s_l2bsize;

  rc = ujfs_read_diskblocks(off,len, (void *)bp->b_data);
  if (rc == 0)
    *bpp = bp;
  else 
    {
      jfs_info("bp read failed \n");
      bRelease(bp);
    }

  return rc;
}

static void 
bAllocate(void)
{
  buf_t	  *bp;
  char	  *buffer;
  int32_t i;
  
  if ((bp = malloc(NBUFFER * sizeof(buf_t))) == NULL) 
    {
      jfs_info("not enough memory \n");
      exit (0);
    }

  if ((buffer = malloc(NBUFFER * PAGESIZE)) == NULL) 
    {
      jfs_info("not enough memory \n");
      exit (0);
    }
  for (i = 0; i < NBUFFER; i++, bp++, buffer += PAGESIZE) 
    {
      bp->b_data = buffer;
      bp->b_next = bCachelist;
      bCachelist = bp;
    }

  nBuffer += NBUFFER;
}

void 
bRelease (buf_t *bp)
{
  /* release buffer */
  bp->b_next = bCachelist;
  bCachelist = bp;
}
