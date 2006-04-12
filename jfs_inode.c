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

#include <string.h>
#include "jfs.h"
#include "jfs_imap.h"

extern struct jfs_superblock *jfs_sb;

void get_inode(int inum)
{
  int which_table=FILESYSTEM_I;
  int64_t   address;
  dinode_t  inode; 

  if(find_inode(inum, which_table, &address) ||
     xRead(address, sizeof(dinode_t), (char *)&inode)) 
    {
      jfs_info("inode: error reading inode\n\n");
      return;
    }
}


int32_t find_inode( uint32_t  inum,
                    uint32_t  which_table,
                    int64_t   *address)
{
  int32_t  extnum;
  iag_t iag;
  int32_t  iagnum;
  int64_t  map_address;
  int32_t  rc;

  unsigned int l2bsize;

  l2bsize=jfs_sb->s_l2bsize;  

  iagnum = INOTOIAG(inum);
  extnum = (inum & (INOSPERIAG -1)) >> L2INOSPEREXT;

  if (find_iag(iagnum, which_table, &map_address) == 1)
    return 1;

  rc = ujfs_read_diskblocks (map_address, sizeof(iag_t),&iag);
  if (rc) 
    {
      jfs_info("find_inode: Error reading iag\n");
      return 1;
    }

  if (iag.inoext[extnum].len == 0)
    return 1;

  *address = (addressPXD(&iag.inoext[extnum]) << l2bsize) +
             ((inum & (INOSPEREXT-1)) * sizeof(dinode_t));
  *address = (addressPXD(&iag.inoext[extnum]) << l2bsize) +
             ((inum & (INOSPEREXT-1)) * 512);

  return 0;
}


int32_t find_iag( uint32_t  iagnum,
                  uint32_t  which_table,
                  int64_t   *address )
{
  int32_t   base;
  char      buffer[PSIZE];
  int32_t   cmp;
  dinode_t  fileset_inode; 
  uint64_t  fileset_inode_address;
  int64_t   iagblock;
  int32_t   index;
  int32_t   lim;
  xtpage_t  *page;
  int32_t   rc;
  /* dinode_t  *fileinode; */
  /* extern void  *disk_image; */
  unsigned int l2bsize;
  l2bsize = jfs_sb->s_l2bsize;

  if (which_table != FILESYSTEM_I &&
      which_table != AGGREGATE_I &&
      which_table != AGGREGATE_2ND_I) 
    {
      jfs_info( "find_iag: Invalid fileset, %d\n", which_table);
      return 1;
    }

  iagblock = IAGTOLBLK(iagnum, L2PSIZE-l2bsize);

  fileset_inode_address = AGGR_INODE_TABLE_START +
    (which_table * 512); // 53248d

  rc = xRead(fileset_inode_address, sizeof(dinode_t),
             (char *)&fileset_inode);
            
   if (rc) 
     {
       jfs_info("find_inode: Error reading fileset inode\n");
       return 1;
     }

   page = (xtpage_t *)&(fileset_inode.di_btroot);

 descend:
   /* Binary search */

   for (base = XTENTRYSTART,
	  lim = __le16_to_cpu(page->header.nextindex) - XTENTRYSTART;
	lim; lim >>=1) 
     {
       index = base + (lim >> 1);
       XT_CMP(cmp, iagblock, &(page->xad[index]));

       if (cmp == 0) 
	 {
	   /* HIT! */
	   if (page->header.flag & BT_LEAF) {
	     *address = (addressXAD(&(page->xad[index]))
			 + (iagblock - offsetXAD(&(page->xad[index]))))
			   << l2bsize;
	     return 0;
	   } 
	   else 
	     {
	       rc = xRead (addressXAD(&(page->xad[index])) << l2bsize,
			   PSIZE, buffer);
	       if (rc) 
		 {
		   jfs_info("find_iag: Error reading btree node\n");
		   return 1;
		 }
	       page = (xtpage_t *)buffer;
	       
	       goto descend;
	     }
	 } 
       else if (cmp > 0) 
	 {
	   base = index + 1;
	   --lim;
	 }
     }

   if (page->header.flag & BT_INTERNAL ) {
     /* Traverse internal page, it might hit down there
      * If base is non-zero, decrement base by one to get the parent
      * entry of the child page to search.
      */
     index = base ? base - 1 : base;
     
     rc = xRead(addressXAD(&(page->xad[index])) << l2bsize, PSIZE,
		buffer);
     if (rc) 
       {
	 jfs_info("find_iag: Error reading btree node\n");
	 return 1;
       }
     page = (xtpage_t *)buffer;

     goto descend;
   }

   /* Not found! */
   jfs_info("find_iag:  IAG %d not found!\n", iagnum);
   return 1;
}


int32_t xRead( int64_t   address,
               uint32_t  count,
               char      *buffer )
{
  int64_t   block_address;
  char      *block_buffer;
  int64_t   length;
  uint32_t  offset;
  int32_t   bsize;
  bsize = jfs_sb->s_bsize;

  /* fs_info("bsize %d \n",bsize); */
  
  offset = address & (bsize-1);
  length = (offset + count + bsize-1) & ~(bsize - 1);

  if ((offset == 0) & (length == count))
    return ujfs_read_diskblocks(address, count, buffer);

  // jfs_info("%s : %d  \n",__FUNCTION__,__LINE__);
  block_address = address - offset;
  block_buffer = (char *)malloc(length);

  if (block_buffer == 0)
    return 1;
  
  if (ujfs_read_diskblocks (block_address, length, block_buffer)) 
    {
      free (block_buffer);
      return 1;
    }

  memcpy (buffer, block_buffer+offset, count);
  free (block_buffer);
  return 0;
}


uint32_t 
ujfs_read_diskblocks (uint64_t disk_offset, uint32_t disk_count,
                      void *data_buffer)
{
  char *ret;
  extern void *disk_image;

  /* or store_read? */
  ret = memcpy (data_buffer, 
		((char *)disk_image+disk_offset), disk_count);

  if (ret == NULL)
    return 1;

  return 0;
}
