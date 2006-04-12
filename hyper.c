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

#include <hurd/iohelp.h>
#include <hurd/diskfs-pager.h>

#include "typedefs.h"
#include "jfs.h"

struct jfs_superblock *jfs_sb;

void
get_hypermetadata ()
{
  error_t err;
  extern void *disk_image;

  err = diskfs_catch_exception (); 
  jfs_sb = (struct jfs_superblock *) ((char *) disk_image + SUPER1_OFF);

  jfs_info ("jfs_sb->s_magic          %s\n", jfs_sb->s_magic);
  jfs_info ("jfs_sb->l2bsize          %d\n", jfs_sb->s_l2bsize);
  // diskfs_readonly = 1;

  if (strncmp(JFS_MAGIC, jfs_sb->s_magic, 4))
    {
      jfs_info ("bad magic number %#x (should be %#x)",
		 jfs_sb->s_magic, JFS_MAGIC); 
      exit(0);
    }
  else
    {
      jfs_info("jfs filesystem \n");
    }

  /* TODO : Check the state of the filesystem. */
  diskfs_end_catch_exception ();
}
