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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* #define DEBUG */
/* #define LOGFILE "jfslog.txt" */

#include <argp.h>
#include <argz.h>
#include <cthreads.h>
#include <hurd/diskfs.h>
#include <hurd/store.h>

#include "jfs.h"

#define JFS_NAME_MAX 255
#define JFS_LINK_MAX 65535

struct node *diskfs_root_node;
struct store *store = 0;
struct store_parsed *store_parsed = 0;
char *diskfs_disk_name = 0;

char *diskfs_server_name    = PACKAGE;/* "jfs"; */
char *diskfs_server_version = VERSION;
char *diskfs_extra_version  = "GNU Hurd";

int diskfs_synchronous = 0;

int diskfs_link_max    = JFS_LINK_MAX;
int diskfs_name_max    = JFS_NAME_MAX;

int diskfs_maxsymlinks      = 8;
int diskfs_shortcut_symlink = 1;

int diskfs_readonly      = 1;
int diskfs_hard_readonly = 1;

void *disk_image;

/* char *host_name; */
/* char *mounted_on; */

static const struct argp_option
options[] =
  {
    {0}  /* Later ;-) */
  };

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)  
    {
    case ARGP_KEY_INIT:
      state->child_inputs[0] = state->input; /* ?? */
      break;

    case ARGP_KEY_SUCCESS:
      break;
     
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static const struct argp_child startup_children[] =
  { {&diskfs_store_startup_argp}, {0} };

static struct argp startup_argp = {options, parse_opt, 0, 0, startup_children};


static const struct argp_child runtime_children[] =
  { {&diskfs_std_runtime_argp}, {0} };

static struct argp runtime_argp = 
  { options, parse_opt, 0, 0, runtime_children };

struct argp *diskfs_runtime_argp = (struct argp *) &runtime_argp;

/* this fellow in dir.c */
extern void inode_init ();

int 
main (int argc, char **argv)
{
  mach_port_t bootstrap;
  
  store = diskfs_init_main (&startup_argp, argc, argv,
			    &store_parsed, &bootstrap);
  
  if (store->size < SUPER1_OFF + sizeof jfs_sb - 1)
    jfs_info ("Error: device too small for superblock (%Ld bytes)",
	      store->size);
  if (store->log2_blocks_per_page < 0)
    jfs_info ("Device block size (%zu) greater than page size (%zd)",
	       store->block_size, vm_page_size);

  /* Map the entire disk. */
  create_disk_pager ();

  get_hypermetadata ();

  inode_init ();

  /* As usual, 2 is the root inode in JFS too. */
  diskfs_cached_lookup (2, &diskfs_root_node);

  diskfs_startup_diskfs (bootstrap, 0);
  cthread_exit (0);
  return 0;
}


struct mutex printf_lock = MUTEX_INITIALIZER;

int 
jfs_info (const char *fmt, ...)
{
#if (defined DEBUG && defined LOGFILE)
  va_list arg;
  int done;
  FILE *fp = fopen (LOGFILE, "a+");

  va_start (arg, fmt);
  mutex_lock (&printf_lock);
  done = vfprintf (fp, fmt, arg);
  mutex_unlock (&printf_lock);
  va_end (arg);

  fclose (fp);
  return done;
#endif /* DEBUG && LOGFILE */
  return 0;   /* to kill warning :-| */
}

/*
static char error_buf[1024];

void 
jfs_panic (const char * fmt, ...)
{
  va_list args;

  mutex_lock(&printf_lock);

  va_start (args, fmt);
  vsprintf (error_buf, fmt, args);
  va_end (args);

  fprintf(stderr, "jfs: %s: panic:  %s\n",
	  diskfs_disk_name, error_buf);

  mutex_unlock(&printf_lock);

  exit (1);
}

*/
