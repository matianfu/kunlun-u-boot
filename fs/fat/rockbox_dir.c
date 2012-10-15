/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: dir.c,v 1.28 2005/01/28 21:32:16 hohensoh Exp $
 *
 * Copyright (C) 2002 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * 01/17/2006 Keith Outwater (outwater4@comcast.net) - port to U-Boot using
 *     CVS version 1.29 of 'firmware/common/dir.c' from rockbox CVS server.
 *
 ****************************************************************************/

#include <common.h>
#include <config.h>

#if defined(CFG_ROCKBOX_FAT)

#include <asm/errno.h>
#include <linux/string.h>
#include <fat.h>
#include "rockbox_debug.h"

#ifndef __HAVE_ARCH_STRNICMP
extern int strnicmp(const char *s1, const char *s2, size_t len);
#endif

#if !defined(CFG_ROCKBOX_FAT_MAX_OPEN_DIRS)
#define CFG_ROCKBOX_FAT_MAX_OPEN_DIRS 8
#endif

int errno;

static DIR opendirs[CFG_ROCKBOX_FAT_MAX_OPEN_DIRS];

DIR* opendir(const char* name)
{
    char namecopy[MAX_PATH];
    char* part;
    struct fat_direntry entry;
    int dd;
    DIR* pdir = opendirs;

    if ( name[0] != '/' ) {
        fprintf(stderr, "Error: Only absolute paths supported\n");
        return NULL;
    }

    /* find a free dir descriptor */
    for ( dd=0; dd<CFG_ROCKBOX_FAT_MAX_OPEN_DIRS; dd++, pdir++)
        if ( !pdir->busy )
            break;

    if ( dd == CFG_ROCKBOX_FAT_MAX_OPEN_DIRS ) {
        fat_eprintf("Too many dirs open\n");
        errno = EMFILE;
        return NULL;
    }

    pdir->busy = true;

    strncpy(namecopy,name,sizeof(namecopy)); /* just copy */
    namecopy[sizeof(namecopy)-1] = '\0';

    if ( fat_opendir(&pdir->fatdir, 0, NULL) < 0 ) {
        fat_deprintf("Failed to open root dir\n");
        pdir->busy = false;
        return NULL;
    }

    for ( part = strtok(namecopy, "/"); part;
          part = strtok(NULL, "/")) {
        /* scan dir for name */
        while (1) {
            if ((fat_getnext(&pdir->fatdir,&entry) < 0) ||
                (!entry.name[0])) {
                pdir->busy = false;
                return NULL;
            }
            if ( (entry.attr & FAT_ATTR_DIRECTORY) &&
                 (!strnicmp(part, entry.name, strlen(part))) ) {
                pdir->parent_dir = pdir->fatdir;
                if ( fat_opendir(&pdir->fatdir,
                                 entry.firstcluster,
                                 &pdir->parent_dir) < 0 ) {
                    fat_deprintf("Failed to open dir \"%s\" "
								 "(firstcluster = %ld)\n",
								 part, entry.firstcluster);
                    pdir->busy = false;
                    return NULL;
                }
                break;
            }
        }
    }

    return pdir;
}

int closedir(DIR* dir)
{
    dir->busy=false;
    return 0;
}

struct dirent* readdir(DIR* dir)
{
    struct fat_direntry entry;
    struct dirent* theent = &(dir->theent);

    if (!dir->busy)
        return NULL;

    /* normal directory entry fetching follows here */
    if (fat_getnext(&(dir->fatdir),&entry) < 0)
        return NULL;

    if ( !entry.name[0] )
        return NULL;

    strncpy(theent->d_name, entry.name, sizeof( theent->d_name ) );
    theent->attribute = entry.attr;
    theent->size = entry.filesize;
    theent->startcluster = entry.firstcluster;
    theent->wrtdate = entry.wrtdate;
    theent->wrttime = entry.wrttime;

    return theent;
}

int mkdir(const char *name, int mode)
{
    DIR *dir;
    char namecopy[MAX_PATH];
    char* end;
    char *basename;
    char *parent;
    struct dirent *entry;
    struct fat_dir newdir;
    int rc;

    (void)mode;

    if ( name[0] != '/' ) {
        fprintf(stderr, "Error: Only absolute paths supported\n");
        return -1;
    }

    strncpy(namecopy,name,sizeof(namecopy));
    namecopy[sizeof(namecopy)-1] = 0;

    /* Split the base name and the path */
    end = strrchr(namecopy, '/');
    *end = 0;
    basename = end+1;

    if(namecopy == end) /* Root dir? */
        parent = "/";
    else
        parent = namecopy;

    fat_dprintf("parent: \"%s\", name: \"%s\"\n", parent, basename);

    dir = opendir(parent);

    if(!dir) {
        fprintf(stderr, "Can't open parent dir\n");
        return -2;
    }

    if(basename[0] == 0) {
        fat_eprintf("Empty directory name\n");
        errno = EINVAL;
        return -3;
    }

    /* Now check if the name already exists */
    while ((entry = readdir(dir))) {
        if ( !strnicmp(basename, entry->d_name, strlen(basename)) ) {
            fat_eprintf("File exists\n");
            errno = EEXIST;
            closedir(dir);
            return - 4;
        }
    }

    memset(&newdir, sizeof(struct fat_dir), 0);
    rc = fat_create_dir(basename, &newdir, &(dir->fatdir));
    closedir(dir);
    return rc;
}

int rmdir(const char* name)
{
    int rc;
    DIR* dir;
    struct dirent* entry;

    dir = opendir(name);
    if (!dir)
    {
        errno = ENOENT; /* open error */
        return -1;
    }

    /* check if the directory is empty */
    while ((entry = readdir(dir)))
    {
        if (strcmp(entry->d_name, ".") &&
            strcmp(entry->d_name, ".."))
        {
            fat_eprintf("Directory is not empty\n");
            errno = ENOTEMPTY;
            closedir(dir);
            return -2;
        }
    }

    rc = fat_remove(&(dir->fatdir.file));
    if ( rc < 0 ) {
        fat_eprintf("Failed to remove directory (rc = %d)\n", rc);
        errno = EIO;
        rc = rc * 10 - 3;
    }

    closedir(dir);
    return rc;
}

/* vim: set ts=4 tw=80 sw=4 fo=tcroq: */
#endif /* #if defined(CFG_ROCKBOX_FAT) */
