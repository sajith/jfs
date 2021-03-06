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

#ifndef	_H_JFS_IMAP
#define _H_JFS_IMAP

#include "jfs_types.h"

/*
 *	jfs_imap.h: disk inode manager
 */

#define	EXTSPERIAG	128	/* number of disk inode extent per iag  */
#define IMAPBLKNO	0	/* lblkno of dinomap within inode map   */
#define SMAPSZ		4	/* number of words per summary map      */
#define	EXTSPERSUM	32	/* number of extents per summary map entry */
#define	L2EXTSPERSUM	5	/* l2 number of extents per summary map */
#define	PGSPERIEXT	4	/* number of 4K pages per dinode extent */
#define	MAXIAGS		((1<<20)-1)	/* maximum number of iags       */
#define	MAXAG		128	/* maximum number of allocation groups  */

#define AMAPSIZE      512	/* bytes in the IAG allocation maps */
#define SMAPSIZE      16	/* bytes in the IAG summary maps */

/* convert inode number to iag number */
#define	INOTOIAG(ino)	((ino) >> L2INOSPERIAG)

/* convert iag number to logical block number of the iag page */
#define IAGTOLBLK(iagno,l2nbperpg)	(((iagno) + 1) << (l2nbperpg))

/* get the starting block number of the 4K page of an inode extent
 * that contains ino.
 */
#define INOPBLK(pxd,ino,l2nbperpg)    	(addressPXD((pxd)) +		\
	((((ino) & (INOSPEREXT-1)) >> L2INOSPERPAGE) << (l2nbperpg)))

/*
 *	inode allocation map:
 * 
 * inode allocation map consists of 
 * . the inode map control page and
 * . inode allocation group pages (per 4096 inodes)
 * which are addressed by standard JFS xtree.
 */
/*
 *	inode allocation group page (per 4096 inodes of an AG)
 */
typedef struct {
	s64 agstart;		/* 8: starting block of ag              */
	s32 iagnum;		/* 4: inode allocation group number     */
	s32 inofreefwd;		/* 4: ag inode free list forward        */
	s32 inofreeback;	/* 4: ag inode free list back           */
	s32 extfreefwd;		/* 4: ag inode extent free list forward */
	s32 extfreeback;	/* 4: ag inode extent free list back    */
	s32 iagfree;		/* 4: iag free list                     */

	/* summary map: 1 bit per inode extent */
	s32 inosmap[SMAPSZ];	/* 16: sum map of mapwords w/ free inodes;
				 *      note: this indicates free and backed
				 *      inodes, if the extent is not backed the
				 *      value will be 1.  if the extent is
				 *      backed but all inodes are being used the
				 *      value will be 1.  if the extent is
				 *      backed but at least one of the inodes is
				 *      free the value will be 0.
				 */
	s32 extsmap[SMAPSZ];	/* 16: sum map of mapwords w/ free extents */
	s32 nfreeinos;		/* 4: number of free inodes             */
	s32 nfreeexts;		/* 4: number of free extents            */
	/* (72) */
	u8 pad[1976];		/* 1976: pad to 2048 bytes */
	/* allocation bit map: 1 bit per inode (0 - free, 1 - allocated) */
	u32 wmap[EXTSPERIAG];	/* 512: working allocation map  */
	u32 pmap[EXTSPERIAG];	/* 512: persistent allocation map */
	pxd_t inoext[EXTSPERIAG];	/* 1024: inode extent addresses */
} iag_t;			/* (4096) */

/*
 *	per AG control information (in inode map control page)
 */
typedef struct {
	s32 inofree;		/* 4: free inode list anchor            */
	s32 extfree;		/* 4: free extent list anchor           */
	s32 numinos;		/* 4: number of backed inodes           */
	s32 numfree;		/* 4: number of free inodes             */
} iagctl_t;			/* (16) */

/*
 *	per fileset/aggregate inode map control page
 */
typedef struct {
	s32 in_freeiag;		/* 4: free iag list anchor     */
	s32 in_nextiag;		/* 4: next free iag number     */
	s32 in_numinos;		/* 4: num of backed inodes */
	s32 in_numfree;		/* 4: num of free backed inodes */
	s32 in_nbperiext;	/* 4: num of blocks per inode extent */
	s32 in_l2nbperiext;	/* 4: l2 of in_nbperiext */
	s32 in_diskblock;	/* 4: for standalone test driver  */
	s32 in_maxag;		/* 4: for standalone test driver  */
	u8 pad[2016];		/* 2016: pad to 2048 */
	iagctl_t in_agctl[MAXAG];	/* 2048: AG control information */
} dinomap_t;			/* (4096) */

#endif				/* _H_JFS_IMAP */
