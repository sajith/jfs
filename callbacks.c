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

#include "jfs.h"

error_t
pager_unlock_page (struct user_pager_info *pager, vm_offset_t page)
{
  return EROFS; 
}

error_t
diskfs_set_translator (struct node *np, const char *name, unsigned namelen,
		       struct protid *cred)
{
  return EROFS;  
}

error_t
diskfs_get_translator (struct node *NP,
		       char **NAMEP, u_int *NAMELEN)
{
  return EOPNOTSUPP;  
}

/* called when the ports library wants to drop weak refernces. */
/* okay to leave unimplemented, see hurd/pager.h */
void
pager_dropweak (struct user_pager_info *p __attribute__ ((unused)))
{ 
}

void
diskfs_try_dropping_softrefs (struct node *np)
{
  /* drop_pager_softrefs (np);  */
}

int
diskfs_dirempty (struct node *DP, struct protid *CRED)
{
  abort ();  
}


error_t
diskfs_grow (struct node *NP, off_t SIZE,
	     struct protid *CRED)
{
  return EROFS; 
}

error_t
diskfs_reload_global_state ()
{
  return 0; 
}

error_t
diskfs_alloc_node (struct node *DP, mode_t MODE,
          struct node **NP)
{  
  return EROFS; 
}

void
diskfs_file_update (struct node *NP, int WAIT)
{
}

error_t
diskfs_dirremove_hard (struct node *DP,
          struct dirstat *DS)
{
  abort ();   
}

error_t
diskfs_drop_dirstat (struct node *dp, struct dirstat *ds)
{
  return 0; 
}

void
diskfs_new_hardrefs (struct node *np)
{
  /* allow_pager_softrefs (np);   */
}

const size_t diskfs_dirstat_size = 0;   

void
diskfs_null_dirstat (struct dirstat *DS)
{
}

vm_prot_t
diskfs_max_user_pager_prot ()
{
  return VM_PROT_READ | VM_PROT_EXECUTE;   
}

void
diskfs_write_disknode (struct node *NP, int WAIT)
{
}

error_t
diskfs_node_iterate (error_t (*fn) (struct node *NP))
{
  return 0; 
}

void
diskfs_free_node (struct node *no, mode_t mode)
{
  abort ();  
}

error_t
diskfs_set_hypermetadata (int WAIT, int CLEAN)
{
   return 0; 
}

void
diskfs_lost_hardrefs (struct node *NP)
{
}

error_t 
diskfs_direnter_hard (struct node *dp, const char *name,
		      struct node *np, struct dirstat *ds,
		      struct protid *cred)
{
  abort (); 
}

error_t
diskfs_dirrewrite_hard (struct node *DP,
          struct node *NP, struct dirstat *DS)
{
  abort (); 
}

error_t
diskfs_truncate (struct node *NP, off_t SIZE)
{
  return EROFS; 
}

error_t
diskfs_node_reload (struct node *NODE)
{
  return 0; 
}

error_t
diskfs_validate_author_change (struct node *np, uid_t author)
{
  return EROFS; 
}

void
diskfs_shutdown_pager ()
{
}
