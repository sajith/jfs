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

#ifndef JFS_DREAD_H
#define JFS_DREAD_H

#define IFMT       0xF000  /* S_IFMT - mask of file type */
#define IFDIR      0x4000  /* S_IFDIR - directory */
#define BT_LEAF    0x02    /* leaf page */
#define DTLHDRDATALEN 11
#define PATH_MAX      4095 /* # chars in a path name : from linux/limits.h */
#define XTENTRYSTART  2
#define BT_INTERNAL   0x04
#define PAGESIZE      4096

#undef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))

#define L2INOSPEREXT          5
#define    INOSPERIAG          4096
#define    L2INOSPERIAG      12 
#define INOTOIAG(ino)   ((ino) >> L2INOSPERIAG)
#define PSIZE 4096
#define FILESYSTEM_I 16
#define AGGREGATE_I 1
#define AGGREGATE_2ND_I -1
#define INOSPEREXT 32
#define SIZE_OF_MAP_PAGE 4096
#define L2PSIZE 12

#define IAGTOLBLK(iagno,l2nbperpg)   (((iagno) + 1) << (l2nbperpg))

#define SIZE_OF_SUPER      PSIZE
#define SUPER1_OFF 0x8000
#define AIMAP_OFF  (SUPER1_OFF + SIZE_OF_SUPER)
#define AITBL_OFF  (AIMAP_OFF + (SIZE_OF_MAP_PAGE << 1)) 

#define AGGR_INODE_TABLE_START     AITBL_OFF

typedef struct {
	int8_t   next;		/*  1: */
	int8_t   cnt;		/*  1: */
	UniChar  name[15];	/* 30: */
} dtslot_t;	

typedef struct {
	uint32_t  inumber;	/* 4: 4-byte aligned */
	int8_t    next;		/* 1: */
	uint8_t   namlen;	/* 1: */
	UniChar   name[11];	/* 22: 2-byte aligned */
	uint32_t  index;	/* 4: index into dir_table */
} ldtentry_t;			/* (32) */

typedef struct bufhdr {
	struct bufhdr  *b_next;	 /* 4: */
	char   *b_data;	     /* 4: */
} buf_t;		     /* (8) */


#define __le16_to_cpu(x) ((uint16_t)(x))
#define __le24_to_cpu(x) ((uint32_t)(x))
#define __le32_to_cpu(x) ((uint32_t)(x))



#define	addressPXD(pxd)\
	( ((int64_t)((pxd)->addr1)) << 32 | __le32_to_cpu((pxd)->addr2))

#define addressXAD(xad)\
        ( ((int64_t)((xad)->addr1)) << 32 | __le32_to_cpu((xad)->addr2))

#define lengthXAD(xad)  __le24_to_cpu((xad)->len)

#define offsetXAD(xad)\
        ( ((int64_t)((xad)->off1)) << 32 | __le32_to_cpu((xad)->off2))


#define XT_CMP(CMP, K, X) \
{ \
        int64_t offset64 = offsetXAD(X); \
        (CMP) = ((K) >= offset64 + lengthXAD(X)) ? 1 : \
                ((K) < offset64) ? -1 : 0 ; \
}



#define XT_CMP_4(CMP, K, X, OFFSET64)\
{\
	OFFSET64 = offsetXAD(X);\
	(CMP) = ((K) >= OFFSET64 + lengthXAD(X)) ? 1 :\
	      ((K) < OFFSET64) ? -1 : 0;\
}

void get_inode(int inum);

int32_t find_inode( uint32_t  inum,
                    uint32_t  which_table,
                    int64_t   *address);

int32_t find_iag( uint32_t  iagnum,
                  uint32_t  which_table,
                  int64_t   *address );

int32_t xRead( int64_t   address,
               uint32_t  count,
               char      *buffer );

uint32_t
ujfs_read_diskblocks (uint64_t disk_offset, uint32_t disk_count,
                      void *data_buffer);



void directory(uint64_t inum,char *name,size_t namelen,uint64_t *gotinum);
int32_t bRead(    int64_t  xaddr,	  /* in bsize */
	          int32_t  xlen,	  /* in bsize */
	          buf_t    **bpp);
/* static void bAllocate(void); */
void bRelease(buf_t *bp);
void display_file(int inum);

#endif
