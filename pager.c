/*
 *   Copyright (c) 2002 Sajith T. S. <sajith@gmail.com>
 *                  and John Thomas <johniqin@yahoo.co.in>
 *
 *   Parts of this code were taken from the GNU Hurd, which is:
 *   Copyright (c) 2002 Free Software Foundation, Inc.
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

#include <hurd/diskfs.h>
#include <hurd/diskfs-pager.h>
#include <hurd/store.h>
#include <sys/mman.h>
#include <error.h>

#include "jfs.h"

spin_lock_t node2pagelock = SPIN_LOCK_INITIALIZER;
struct port_bucket *pager_bucket;  

/* this function in jfs_fileread.c */
extern int32_t xt_Lookup(dinode_t *, int64_t, int64_t *, int32_t *);

struct pager *
diskfs_get_filemap_pager_struct (struct node *np)
{
  return np->dn->fileinfo->p;
}

mach_port_t
diskfs_get_filemap (struct node *np, vm_prot_t prot)
{
  struct user_pager_info *upi;
  mach_port_t right;
  
  assert (S_ISDIR (np->dn_stat.st_mode)
	  || S_ISREG (np->dn_stat.st_mode)
	  || S_ISLNK (np->dn_stat.st_mode));
  
  spin_lock (&node2pagelock);
  
  do
    if (!np->dn->fileinfo)
      {
	upi = malloc (sizeof (struct user_pager_info));
	upi->type = FILE_DATA;
	upi->np = np;
	diskfs_nref_light (np);
	upi->p = pager_create (upi, pager_bucket, 
			       1, MEMORY_OBJECT_COPY_DELAY, 0);
	if (upi->p == 0)
	  {
	    diskfs_nrele_light (np);
	    free (upi);
	    spin_unlock (&node2pagelock);
	    return MACH_PORT_NULL;
	  }
	np->dn->fileinfo = upi;
	right = pager_get_port (np->dn->fileinfo->p);
	ports_port_deref (np->dn->fileinfo->p);
      }
    else
      {
	/* Because NP->dn->fileinfo->p is not a real reference,
	   this might be nearly deallocated.  If that's so, then
	   the port right will be null.  In that case, clear here
	   and loop.  The deallocation will complete separately. */
	right = pager_get_port (np->dn->fileinfo->p);
	if (right == MACH_PORT_NULL)
	  np->dn->fileinfo = 0;
      }
  while (right == MACH_PORT_NULL);
  
  spin_unlock (&node2pagelock);
  
  mach_port_insert_right (mach_task_self (), right, right, 
			  MACH_MSG_TYPE_MAKE_SEND);
  
  return right;
}

void
pager_clear_user_data (struct user_pager_info *upi)
{
  if (upi->type == FILE_DATA)
    {
      spin_lock (&node2pagelock);
      if (upi->np->dn->fileinfo == upi)
	upi->np->dn->fileinfo = 0;
      spin_unlock (&node2pagelock);
      diskfs_nrele_light (upi->np);
    }
  free (upi);
}

void
drop_pager_softrefs (struct node *np)
{
  struct user_pager_info *upi;

  spin_lock (&node2pagelock);
  upi = np->dn->fileinfo;
  if (upi)
    ports_port_ref (upi->p);
  spin_unlock (&node2pagelock);

  if (upi)
    {
      pager_change_attributes (upi->p, 0, MEMORY_OBJECT_COPY_DELAY, 0);
      ports_port_deref (upi->p);
    }
}

void
allow_pager_softrefs (struct node *np)
{
  struct user_pager_info *upi;

  spin_lock (&node2pagelock);
  upi = np->dn->fileinfo;
  if (upi)
    ports_port_ref (upi->p);
  spin_unlock (&node2pagelock);

  if (upi)
    {
      pager_change_attributes (upi->p, 1, MEMORY_OBJECT_COPY_DELAY, 0);
      ports_port_deref (upi->p);
    }
}
 
void
create_disk_pager (void)
{
  struct user_pager_info *upi = malloc (sizeof (struct user_pager_info));
  extern struct store *store;
  extern void *disk_image;
 
  if (!upi)
    error (1, errno, "Could not create disk pager");

  upi->type = DISK;
  upi->np = 0; 
  pager_bucket = ports_create_bucket ();
  diskfs_start_disk_pager (upi, pager_bucket, 1, 0, store->size,
			   &disk_image);
  upi->p = diskfs_disk_pager;
}


/* Read one page for the pager backing NODE at offset PAGE, into BUF.  This
   may need to read several filesystem blocks to satisfy one page, and tries
   to consolidate the i/o if possible.  */

error_t
pager_read_page (struct user_pager_info *upi, vm_offset_t page,
		 vm_address_t *buf, int *writelock)
{
  error_t err;
  daddr_t addr;
  struct node *np = upi->np;
  size_t read = 0;
  size_t overrun = 0;

  int64_t address;
  dinode_t inode;
  int64_t xoff,xaddr;
  int32_t xlen;
  /* extern void *disk_image; */
  
  extern struct store *store;
  xoff = page / 4096;
  xlen = vm_page_size;

  /* This is a read-only medium */
  *writelock = 1;

  if (upi->type == FILE_DATA)
    {
      jfs_info(" %s FILE_DATA  %d \n ", __FUNCTION__, xoff);
      
      if (find_inode (np->cache_id, 16, &address) || 
	 xRead (address, sizeof(dinode_t), (char *)&inode)) 
	{
	  jfs_info("inode: error reading inode\n\n");
	  return 1;
	}

      jfs_info("%s num %d \n", __FUNCTION__, inode.di_number);

      if(xt_Lookup(&inode, xoff, &xaddr, &xlen))
	{
	  jfs_info("error in lookup \n");
	  if (page >= np->dn_stat.st_size)
	    {
	      *buf = (vm_address_t) mmap (0, vm_page_size, 
					  PROT_READ|PROT_WRITE,
					  MAP_ANON, 0, 0);

	      jfs_info("np->dn_stat.st_size : %d  \n",
		       np->dn_stat.st_size);

	      return 0;
	    }
	  return 1;
	}

      jfs_info ("%s address : %d  xlen : %d \n", __FUNCTION__, xaddr, xlen);

      xaddr *= 4096;
      addr = (xaddr >> store->log2_block_size);
             
      if (page >= np->dn_stat.st_size)
	{
	  *buf = (vm_address_t) mmap (0, vm_page_size, PROT_READ|PROT_WRITE,
				      MAP_ANON, 0, 0);
	  jfs_info(" np->dn_stat.st_size : %d  \n" ,np->dn_stat.st_size);
	  return 0;
	}

      if (page + vm_page_size > np->dn_stat.st_size)
	overrun = page + vm_page_size - np->dn_stat.st_size;
    }
  else
    {
      assert (upi->type == DISK);
      addr = page >> store->log2_block_size;
    }

  err = store_read (store, addr, vm_page_size, (void **) buf, &read);
  if (read != vm_page_size)
    return EIO;

  if (overrun)
    bzero ((void *) *buf + vm_page_size - overrun, overrun);
    
  return 0;
}

inline error_t
pager_report_extent (struct user_pager_info *pager,
		     vm_address_t *offset, vm_size_t *size)
{
  *offset = 0;
  *size = pager->np->dn_stat.st_size;
  return 0;
}

void
diskfs_sync_everything (int wait)
{
}

static void
enable_caching ()
{
  error_t enable_cache (void *arg)
    {
      struct pager *p = arg;
      struct user_pager_info *upi = pager_get_upi (p);

      pager_change_attributes (p, 1, MEMORY_OBJECT_COPY_DELAY, 0);

      /* It's possible that we didn't have caching on before, because
	 the user here is the only reference to the underlying node
	 (actually, that's quite likely inside this particular
	 routine), and if that node has no links.  So dinkle the node
	 ref counting scheme here, which will cause caching to be
	 turned off, if that's really necessary.  */
      if (upi->type == FILE_DATA)
	{
	  diskfs_nref (upi->np);
	  diskfs_nrele (upi->np);
	}
      
      return 0;
    }

  ports_bucket_iterate (pager_bucket, enable_cache);
}

static void
block_caching ()
{
  error_t block_cache (void *arg)
    {
      struct pager *p = arg;

      pager_change_attributes (p, 0, MEMORY_OBJECT_COPY_DELAY, 1);
      return 0;
    }

  /* Loop through the pagers and turn off caching one by one,
     synchronously.  That should cause termination of each pager. */
  ports_bucket_iterate (pager_bucket, block_cache);
}

int
diskfs_pager_users ()
{ 
  int npagers = ports_count_bucket (pager_bucket);

  if (npagers <= 1)
    return 0;

  block_caching ();

  /* Give it a second; the kernel doesn't actually shutdown
     immediately.  XXX */
  sleep (1);

  npagers = ports_count_bucket (pager_bucket);
  if (npagers <= 1)
    return 0;

  /* Darn, there are actual honest users.  Turn caching back on,
     and return failure. */
  enable_caching ();

  ports_enable_bucket (pager_bucket);

  return 1;
}

error_t
pager_write_page (struct user_pager_info *pager,
		  vm_offset_t page,
		  vm_address_t buf)
{
  assert (0); 
}

void
pager_notify_evict (struct user_pager_info *pager,
                   vm_offset_t page)
{
  assert (!"unrequested notification on eviction");
}


