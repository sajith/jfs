/*
 *   Copyright (c) 2002 Sajith T. S. <sajith@gmail.com>
 *                  and John Thomas <johniqin@yahoo.co.in>
 *
 *   Parts of this code were taken from the GNU Hurd, which is:
 *   Copyright (c) 2002 Free Software Foundation, Inc.
 *
 *   Large parts of this file come from jfsutils, which is:
 *   Copyright (c) 2000-2002 International Business Machines Corp. 
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
#include <dirent.h>

#include "jfs.h"
#include "jfs_dinode.h"

struct node *nodehash[INOHSZ];

extern error_t readndir(struct node *dp,int pos,char *name,uint64_t *rino);
static error_t read_node (struct node *np);

error_t 
diskfs_get_directs (struct node *dp, int entry, int n,  char **data, 
		    u_int *datacnt, vm_size_t bufsiz, int *amt)
{
  struct dirent *userp;
  size_t reclen, namelen;
  char *name;
  int pos = 0;
  uint64_t gotinum;
  error_t err = 0;
  char *datap;

  pos = entry;
  datap = *data;

  jfs_info ("%s : inum %d\n",__FUNCTION__,dp->cache_id);
  jfs_info ("entry %d\n", entry);

  name = (char *) malloc(100);
  
  *amt = 0;
  *datacnt = 0;

  while (!err)
    {
      err = readndir(dp, pos, (char *)name, (uint64_t *)&gotinum);
      if(!err)
	{
          userp   = (struct dirent *) datap;
	  namelen = strlen(name);
	  reclen  = sizeof (struct dirent) + namelen;
	  reclen  = (reclen + 3) & ~3; 
	  
	  userp->d_type   = DT_UNKNOWN;  
	  userp->d_reclen = reclen;
	  userp->d_namlen = namelen;
	  
	  bcopy (name, userp->d_name, namelen); 
	  userp->d_name[namelen] = '\0';   
	  userp->d_ino = gotinum; 
	  
	  jfs_info(" pos  %d main name %s \n",pos,name);
	  jfs_info(" pos  %d main gotinum %d \n",pos,gotinum);

	  pos++;
	
	  (*amt)+=1;
	  if( (*amt) >= 30)
	    {
	      /* (*amt)- = 1; */
	      *datacnt = datap - *data;
	      free(name);

              /* jfs_info("return early %d  ",*amt); */
              /* jfs_info("(datap - *data )  %d ",(datap - *data )); */
              /* jfs_info("datacnt %d n %d \n",*datacnt,n); */

	      return 0;
	    }
	  datap = datap + reclen;
	}
    }

  *datacnt = datap - *data;
  free (name);

  jfs_info ("%s() returning.\n", __FUNCTION__);

  return 0; 
}


static error_t
dirscan(uint64_t dinum, char *name, size_t namelen, uint64_t *gotinum)
{
  directory (dinum, name, namelen, (uint64_t *)gotinum); 
  
  if (!strncmp(name, ".", 1))
    return 0;

  else if (*gotinum==-1)
    {
      jfs_info("entry %s not present \n",name);    
      return  ENOENT;
    }

  return 0;  
}

error_t
diskfs_lookup_hard (struct node *dp, const char *name, 
		    enum lookup_type type, struct node **npp, 
		    struct dirstat *ds, struct protid *cred)
{ 
  error_t err = 0;
  uint8_t namelen;
  uint8_t spec_dotdot;
  uint64_t gotinum;

  jfs_info("%s : inum %d name : %s npp %d\n", __FUNCTION__, 
	   dp->cache_id, name, npp);
  
  if ((type == REMOVE) || (type == RENAME))
    {
      assert (npp);
      return EROFS;
    }
  if (npp)
    *npp = 0;

  spec_dotdot = type & SPEC_DOTDOT;
  type &= ~SPEC_DOTDOT;

  namelen = strlen (name);

  /*   This error is constant, but the errors for CREATE and REMOVE  */
  /*   depend on whether NAME exists.  */
  if (type == RENAME)
    return EROFS;

  err = dirscan(dp->cache_id, (char *)name, strlen(name), &gotinum);
  if(!err)
    jfs_info("%s  present %d \n", __FUNCTION__, gotinum);
  
  if (err)
    return err;

  /* Load the inode */
  if (namelen == 2 && name[0] == '.' && name[1] == '.')
    {
      if (dp == diskfs_root_node)
	err = EAGAIN;
      else if (spec_dotdot)
	{
	  assert (type == LOOKUP);
	  diskfs_nput (dp);
	  diskfs_cached_lookup (gotinum,npp);
	}
      else
	{
	  /* We don't have to do the normal rigamarole, because
	     we are permanently read-only, so things are necessarily
	     quiescent.  Just be careful to honor the locking order. */
	  pthread_mutex_unlock (&dp->lock);
	  diskfs_cached_lookup (gotinum,npp);
	  pthread_mutex_lock (&dp->lock);
	}
    }
  else if (namelen == 1 && name[0] == '.')
    {
      *npp = dp;
      diskfs_nref (dp);
    }
  else
    { 
      err=diskfs_cached_lookup (gotinum,npp);
      if(!err) 
	jfs_info("success %s %d inum %d \n", __FUNCTION__,
		 __LINE__, (*npp)->cache_id);      
    }

  return err; 
 }

void
inode_init ()
{
  int n;
  for (n = 0; n < INOHSZ; n++)
    nodehash[n] = 0;
}

static error_t
read_node (struct node *np)
{
  error_t err;
  struct stat *st = &np->dn_stat;
  dinode_t di;  
  int64_t   address;
  
  if(find_inode(np->cache_id, 16, &address) ||
      xRead(address, sizeof(dinode_t), (char *)&di)) 
    {
      jfs_info("inode: error reading inode\n\n");
      return 1;
    }
  spin_lock (&diskfs_node_refcnt_lock);
   
  err = diskfs_catch_exception ();
  if (err)
    return err;

  st->st_fstype = JFS_SUPER_MAGIC;
  st->st_fsid = getpid ();	
  st->st_ino = np->cache_id;
  st->st_gen = 0;
  st->st_rdev = 0;
  
  st->st_mode = di.di_mode; /* Note. */
  st->st_nlink = di.di_nlink;
  st->st_uid = di.di_uid;
  st->st_gid = di.di_gid;
  st->st_author = di.di_uid;

  st->st_size = di.di_size;
  st->st_gen = di.di_gen;

  if(S_ISLNK (st->st_mode))
    {
      jfs_info(" link \n");
      st->st_size=0;
    }
  else
    jfs_info("not link \n");
 
  st->st_atim.tv_sec = di.di_atime.tv_sec;
  st->st_mtim.tv_sec = di.di_mtime.tv_sec;
  st->st_ctim.tv_sec = di.di_ctime.tv_sec;

  st->st_atim.tv_nsec = di.di_atime.tv_nsec;
  st->st_mtim.tv_nsec = di.di_mtime.tv_nsec;
  st->st_ctim.tv_nsec = di.di_ctime.tv_nsec;

  st->st_blksize = vm_page_size * 2;
  st->st_blocks = di.di_size / 512 +1;

  st->st_flags = 0;

  np->author_tracks_uid = 1;
 
  diskfs_end_catch_exception ();

  if(S_ISDIR (st->st_mode))
    {
      jfs_info("directory \n");
      st->st_mode = S_IFDIR | 0777;
    }
  else
    {
      jfs_info("not directory \n" );
      st->st_mode = S_IFREG | 0666;
    }  

  /* Allocsize should be zero for anything except directories, files, and
     long symlinks.  These are the only things allowed to have any blocks
     allocated as well, although st_size may be zero for any type (cases
     where st_blocks=0 and st_size>0 include fast symlinks, and, under
     linux, some devices).  */
  /*  np->allocsize = 0; */
  
  /*   np->allocsize = np->dn_stat.st_size; */
  spin_unlock (&diskfs_node_refcnt_lock);
  
  return 0;
}

error_t
diskfs_cached_lookup (ino_t inum, struct node **npp)
{
  error_t err;
  struct node *np;
  struct disknode *dn;

  jfs_info("%s inum  %d \n",__FUNCTION__,inum);

  spin_lock (&diskfs_node_refcnt_lock);

  for (np = nodehash[INOHASH(inum)]; np; np = np->dn->hnext)
    if (np->cache_id == inum)
      {
	*npp = np;
        np->references++;
	spin_unlock (&diskfs_node_refcnt_lock);
	pthread_mutex_lock (&np->lock);	
	jfs_info("%s found in cache \n",__FUNCTION__);
	return 0;   
      }

  *npp=0; 

  /* Format specific data for the new node.  */
  dn = malloc (sizeof (struct disknode));
  if (! dn)
    {
      spin_unlock (&diskfs_node_refcnt_lock);
      jfs_info("not enough memory \n");
      return ENOMEM;
    }

  /* Create the new node.  */
  dn->fileinfo = 0;
  np = diskfs_make_node (dn);
  if (!np)
    {
      free (dn);
      spin_unlock (&diskfs_node_refcnt_lock);
      return ENOMEM;
    }

  /* if(inum!=2)  */
  /*   pthread_mutex_lock (&np->lock); */

  np->cache_id = inum;

  /* Put NP in NODEHASH.  */
  dn->hnext = nodehash[INOHASH(inum)];
  if (dn->hnext)
    dn->hnext->dn->hprevp = &dn->hnext;
  dn->hprevp = &nodehash[INOHASH(inum)];
  nodehash[INOHASH(inum)] = np;

  spin_unlock (&diskfs_node_refcnt_lock);

  /* Get the contents of NP off disk.  */
  /* pthread_mutex_lock (&np->lock); */
  err = read_node (np);
  np->references++;
  *npp = np;
  return err;
}

error_t
diskfs_set_statfs (fsys_statfsbuf_t *st)
{
  extern struct jfs_superblock *jfs_sb;

  bzero (st, sizeof *st);

  st->f_type    = JFS_SUPER_MAGIC;
  st->f_bsize   = jfs_sb->s_bsize;;
  st->f_blocks  = jfs_sb->s_agsize;
  st->f_fsid    = getpid ();
  st->f_namelen = 0;

  return 0;
}

void
diskfs_node_norefs (struct node *np)
{
  *np->dn->hprevp = np->dn->hnext;
  if (np->dn->hnext)
    np->dn->hnext->dn->hprevp = np->dn->hprevp;
  assert (!np->dn->fileinfo);
  free (np->dn);
  free (np);
}

static error_t
read_symlink_hook (struct node *np, char *buf)
{
  /* XXX: Boohoo. */
  /* bcopy (np->dn->link_target, buf, np->dn_stat.st_size); */
  jfs_info("%s \n", __FUNCTION__);
  return EOPNOTSUPP;
}

error_t (*diskfs_read_symlink_hook) (struct node *, char *)
     = read_symlink_hook;
