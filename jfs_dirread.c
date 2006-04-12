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

#include <hurd/diskfs.h>

#include "typedefs.h"
#include "jfs_dirread.h"
#include "jfs_unicode.h"
#include "jfs.h"

uint8_t UTF8_Buffer[8 * PATH_MAX];

extern int
Unicode_String_to_UTF8_String(uint8_t *, uint16_t *, int);

error_t
print_direntry( dtslot_t  *slot,
		uint8_t   head_index,char *sname, size_t
		snamelen,uint64_t *gotinum);

void name_inum(dtslot_t  *slot,uint8_t   head_index, char *rname,
	       uint64_t *gotinum)
{

  ldtentry_t  *entry;
  int32_t     len,i;
  UniChar     *n;
  UniChar     *name;
  int32_t     namlen;
  int32_t     next;
  dtslot_t    *s;
  //  jfs_info("name_inum %s %d \n",rname,*gotinum);
  entry = (ldtentry_t *)&(slot[head_index]);
  namlen = entry->namlen;
  name = (UniChar *)malloc(sizeof(UniChar) * (namlen + 1));

  if (name == 0) 
    {
      jfs_info("dirname: malloc error!\n");
      return;
    }

  name[namlen] = 0;
  len = MIN(namlen, DTLHDRDATALEN);
  UniStrncpy(name, entry->name, len);
  next = entry->next;
  n = name + len;
  
  while (next >= 0) 
    {
      s = &(slot[next]);
      namlen -= len;
      len = MIN(namlen, DTSLOTDATALEN);
      UniStrncpy(n, s->name, len);
      next = s->next;
      n += len;
    }
  
  /* Clear the UTF8 conversion buffer. */
  memset(UTF8_Buffer,0,sizeof(UTF8_Buffer));

  /* Convert the name into UTF8 */
  Unicode_String_to_UTF8_String(UTF8_Buffer, name, entry->namlen);
  for(i=0;i<entry->namlen;i++)
    {
      rname[i]=UTF8_Buffer[i];
    }
   rname[entry->namlen]='\0';
   *gotinum=entry->inumber;
   // jfs_info("name_inumc %s : %d  \n",rname,*gotinum);
}

int 
readndir(struct node *dp,int pos,char *name2,uint64_t *rino1)
{
  int64_t inode_address, node_address; 
  dinode_t inode;
  dtroot_t *root;
  char *name;
  /* uint64_t rino; */
  int cpos = 0;
  /* struct dirent userp; */
  /* size_t reclen, namelen;  */
  idtentry_t *node_entry;
  uint8_t *stbl;
  int i;
  unsigned int l2bsize;
  extern struct jfs_superblock *jfs_sb;
  dtpage_t dtree;
 
  l2bsize=jfs_sb->s_l2bsize;    
  name=(char *)malloc(100);
  if(name==NULL) 
    {
      jfs_info("%s : %d no memory \n",__FUNCTION__,__LINE__);
      return 1; 
    }

  if (find_inode(dp->cache_id, 16, &inode_address) ||
      xRead(inode_address, sizeof(dinode_t), (char *)&inode)) 
    {
      jfs_info("directory: error reading inode\n\n");
      return 1;
    }
  
  if ((inode.di_mode & IFMT) != IFDIR) 
    {
      jfs_info("directory: Not a directory! \n");
      return 1;
    }
  root = (dtroot_t *)&(inode.di_btroot);

  if (root->header.flag & BT_LEAF) 
    {
      if (root->header.nextindex == 0) 
	{
	  jfs_info("Empty directory.\n");
	  return 1;
	}
      for (i = 0; i < root->header.nextindex; i++) 
	{
	 if(cpos==pos)
	   {
             name_inum(root->slot, root->header.stbl[i], 
		       (char *)name2, (uint64_t *)rino1);
	     return 0; 
	   }
         cpos++;        
       }

      return 1;
    }

  node_entry = (idtentry_t *)&(root->slot[root->header.stbl[0]]);
 descend:
  node_address = addressPXD(&(node_entry->xd)) << l2bsize;
  if (xRead(node_address, sizeof(dtpage_t), (char *)&dtree)) 
    {
      jfs_info("Directory:  Error reading dtree node\n");
      return 1;
    }
 
  stbl = (uint8_t *)&(dtree.slot[dtree.header.stblindex]);
  if (!(dtree.header.flag & BT_LEAF)) 
   {
     node_entry = (idtentry_t *)&(dtree.slot[stbl[0]]);
     goto descend;   
   }

 next_leaf:
 for (i = 0; i < dtree.header.nextindex; i++) 
   {
     if(cpos==pos)
       {
	 name_inum(dtree.slot,stbl[i],(char *)name2,(uint64_t *)rino1);	 
	 return 0; 
       }  
     cpos++;
   }
 if (dtree.header.next) 
   {
     if (xRead(dtree.header.next << l2bsize, sizeof(dtpage_t),
	       (char *)&dtree)) 
       {
	 jfs_info("directory: Error reading leaf node\n");
	 return 1;
       }
   
     stbl = (uint8_t *)&(dtree.slot[dtree.header.stblindex]);
     goto next_leaf;
   }
 
 return 1;
}


void directory(uint64_t inum,char *name,size_t namelen,uint64_t *gotinum)
{
  /* char        cmd_line[80]; */
  dtpage_t    dtree;
  int32_t     i;
  dinode_t    inode;
  int64_t     inode_address;
  int64_t     node_address;
  idtentry_t  *node_entry;
  dtroot_t    *root;
  uint8_t     *stbl;
  /* char        *token; */
  uint32_t    which_table = FILESYSTEM_I;
  uint32_t    l2bsize;
  extern struct jfs_superblock *jfs_sb;
  error_t rc=1;

  l2bsize = jfs_sb->s_l2bsize;    

  if (find_inode(inum, which_table, &inode_address) ||
      xRead(inode_address, sizeof(dinode_t), (char *)&inode)) 
    {
      jfs_info("directory: error reading inode\n\n");
      return;
    }

  if ((inode.di_mode & IFMT) != IFDIR) 
    {
      jfs_info("directory: Not a directory! \n");
      return;
    }

  root = (dtroot_t *)&(inode.di_btroot);
  jfs_info ("idotdot = %d\n\n", root->header.idotdot);
 
  if (root->header.flag & BT_LEAF) 
    {
    if (root->header.nextindex == 0) 
      {
	jfs_info("Empty directory.\n");
	return;
      }
    for (i = 0; i < root->header.nextindex; i++) 
      {
	rc = print_direntry(root->slot, root->header.stbl[i],
			    name, namelen, gotinum);
	if(!rc)
	  return;
      }
    return;
    }
  /* Root is not a leaf node, we must descend to the leftmost leaf */
  node_entry = (idtentry_t *)&(root->slot[root->header.stbl[0]]);
 descend:
  node_address = addressPXD(&(node_entry->xd)) << l2bsize;
  if (xRead(node_address, sizeof(dtpage_t), (char *)&dtree)) 
    {
      jfs_info("Directory:  Error reading dtree node\n");
      return;
    }

  stbl = (uint8_t *)&(dtree.slot[dtree.header.stblindex]);
  if (!(dtree.header.flag & BT_LEAF)) 
    {
      node_entry = (idtentry_t *)&(dtree.slot[stbl[0]]);
      goto descend;
    }  

  /* dtree (contained in node) is the left-most leaf node */

 next_leaf:
  for (i = 0; i < dtree.header.nextindex; i++) 
    {
      rc=print_direntry(dtree.slot, stbl[i],name,namelen,gotinum);
      if(!rc)
	return;
    }
  if (dtree.header.next) 
    {
      if (xRead(dtree.header.next << l2bsize, sizeof(dtpage_t),
		(char *)&dtree)) 
	{
	  jfs_info("directory: Error reading leaf node\n");
	  return;
	}
  
      stbl = (uint8_t *)&(dtree.slot[dtree.header.stblindex]);
      goto next_leaf;
    }
  return;
}

error_t
print_direntry (dtslot_t *slot,	uint8_t head_index, 
		char *sname, size_t snamelen, uint64_t *gotinum)
 {
   ldtentry_t  *entry;
   int32_t     len;
   UniChar     *n;
   UniChar     *name;
   int32_t     namlen,i;
   int32_t     next;
   dtslot_t    *s;
   entry = (ldtentry_t *)&(slot[head_index]);
   namlen = entry->namlen;
   name = (UniChar *)malloc(sizeof(UniChar) * (namlen + 1));

   if (name == 0) 
     {
       jfs_info("dirname: malloc error!\n");
       return 1;
     }

   name[namlen] = 0;
   len = MIN(namlen, DTLHDRDATALEN);
   UniStrncpy(name, entry->name, len);
   next = entry -> next;
   n = name + len;

   while (next >= 0) 
     {
       s = &(slot[next]);
       namlen -= len;
       len = MIN(namlen, DTSLOTDATALEN);
       UniStrncpy(n, s->name, len);
       next = s->next;
       n += len;
     }

   /* Clear the UTF8 conversion buffer. */
   memset(UTF8_Buffer,0,sizeof(UTF8_Buffer));

   /* Convert the name into UTF8 */
   Unicode_String_to_UTF8_String(UTF8_Buffer, name, entry->namlen);

   for(i=0;i<snamelen;i++)
     {
       if(UTF8_Buffer[i]==sname[i])
	 continue;
       else 
	 {
	   *gotinum = -1;
	   free(name);
	   return 1;
	 }
     }
       
   *gotinum=entry->inumber;
   free(name);
   return 0;
 }

      
       
