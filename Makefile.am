
bin_PROGRAMS = jfs

jfs_LDADD = -lpthread -ldiskfs -lpager -lstore -liohelp -lfshelp \
-lports -lihash -lshouldbeinlibc

AM_CPPFLAGS = -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64

AM_CFLAGS = -Wall -g -O2

EXTRA_DIST = Makefile.sep

jfs_SOURCES = jfs.c \
	jfs.h\
	callbacks.c \
	dir.c \
	hyper.c \
	jfs_btree.h \
	jfs_dinode.h \
	jfs_dirread.c \
	jfs_dirread.h \
	jfs_dtree.h \
	jfs_fileread.c \
	jfs_imap.h \
	jfs_inode.c \
	jfs_superblock.h \
	jfs_types.h \
	jfs_unicode.c \
	jfs_unicode.h \
	jfs_xtree.h \
	pager.c \
	storeinfo.c \
	typedefs.h
