/*
 *   Copyright (c) 2002 Sajith T. S. <sajith@gmail.com>
 *                  and John Thomas <johniqin@yahoo.co.in>
 *
 *   Parts of this code were taken from the GNU Hurd, which is:
 *   Copyright (c) 2002 Free Software Foundation, Inc.
 *
 *   JFS is copyright by International Business Machines Corp.
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

#ifndef _JFS_H_
#define _JFS_H_

#include <rwlock.h> 

#include "typedefs.h"
#include "jfs_types.h"

#include "jfs_dinode.h" 
#include "jfs_superblock.h"

#define SUPER1_OFF 0x8000  /* JFS primary sblock here */
#define JFS_SUPER_MAGIC 0x3153464a 
#define	INOHSZ	512

#if	((INOHSZ&(INOHSZ-1)) == 0)
#define	INOHASH(ino)	((ino)&(INOHSZ-1))
#else
#define	INOHASH(ino)	(((unsigned)(ino))%INOHSZ)
#endif

struct user_pager_info
{
  struct node *np;
  enum pager_type
    {
      DISK,
      FILE_DATA,
    } type;
    struct pager *p;
};

struct disknode
{
  struct user_pager_info *fileinfo;
  struct node *hnext, **hprevp;
};

extern void create_disk_pager ();
extern void get_hypermetadata ();
int jfs_info (const char *fmt, ...);

struct jfs_superblock *jfs_sb;

#endif /* _JFS_H_ */
