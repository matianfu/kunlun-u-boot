/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: fat.h,v 1.13 2005/02/26 21:18:05 jyp Exp $
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * 01/17/2006 Keith Outwater (outwater4@comcast.net) - port to U-Boot using
 *     CVS version 1.13 of 'firmware/export/fat.h' from rockbox CVS server.
 *
 ****************************************************************************/

#ifndef ROCKBOX_FAT_H
#define ROCKBOX_FAT_H

#include <stdbool.h>

/*
 * The max length of the current working dorectory.
 */
#define	ROCKBOX_CWD_LEN	512

struct fat_direntry
{
    unsigned char name[256];        /* Name plus \0 */
    unsigned short attr;            /* Attributes */
    unsigned char crttimetenth;     /* Millisecond creation
                                       time stamp (0-199) */
    unsigned short crttime;         /* Creation time */
    unsigned short crtdate;         /* Creation date */
    unsigned short lstaccdate;      /* Last access date */
    unsigned short wrttime;         /* Last write time */
    unsigned short wrtdate;         /* Last write date */
    unsigned long filesize;         /* File size in bytes */
    long firstcluster;              /* fstclusterhi<<16 + fstcluslo */
};

#define FAT_ATTR_READ_ONLY   0x01
#define FAT_ATTR_HIDDEN      0x02
#define FAT_ATTR_SYSTEM      0x04
#define FAT_ATTR_VOLUME_ID   0x08
#define FAT_ATTR_DIRECTORY   0x10
#define FAT_ATTR_ARCHIVE     0x20
#define FAT_ATTR_VOLUME      0x40 /* this is a volume, not a real directory */

struct fat_file
{
    long firstcluster;    /* first cluster in file */
    long lastcluster;     /* cluster of last access */
    long lastsector;      /* sector of last access */
    long clusternum;      /* current clusternum */
    long sectornum;       /* sector number in this cluster */
    unsigned int direntry;   /* short dir entry index from start of dir */
    unsigned int direntries; /* number of dir entries used by this file */
    long dircluster;      /* first cluster of dir */
    bool eof;
};

struct fat_dir
{
    unsigned int entry;
    unsigned int entrycount;
    long sector;
    struct fat_file file;
    unsigned char sectorcache[3][SECTOR_SIZE];
};


extern int rockbox_fat_mount(long startsector);
extern void fat_size(unsigned long* size, unsigned long* free); /* public for info */
extern void fat_recalc_free(void); /* public for debug info
screen */
extern int fat_create_dir(const char* name,
                          struct fat_dir* newdir,
                          struct fat_dir* dir);
extern long fat_startsector(void); /* public for config
sector */
extern int fat_open( long cluster,
                    struct fat_file* ent,
                    const struct fat_dir* dir);
extern int fat_create_file(const char* name,
                           struct fat_file* ent,
                           struct fat_dir* dir);
extern long fat_readwrite(struct fat_file *ent, long sectorcount,
                         void* buf, bool write );
extern int fat_closewrite(struct fat_file *ent, long size, int attr);
extern int fat_seek(struct fat_file *ent, unsigned long sector );
extern int fat_remove(struct fat_file *ent);
extern int fat_truncate(const struct fat_file *ent);
extern int fat_rename(struct fat_file* file,
                      struct fat_dir* dir,
                      const unsigned char* newname,
                      long size, int attr);

extern int fat_opendir( struct fat_dir *ent, unsigned long currdir,
                       const struct fat_dir *parent_dir);
extern int fat_getnext(struct fat_dir *ent, struct fat_direntry *entry);

#ifndef _ROCKBOX_FILE_H_
#include <rockbox_file.h>
#endif

#ifndef _ROCKBOX_DIR_H
#include <rockbox_dir.h>
#endif

#endif /* #ifndef ROCKBOX_FAT_H */
/* vim: set ts=4 tw=80 sw=4 fo=tcroq: */
