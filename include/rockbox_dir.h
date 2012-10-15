/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: dir.h,v 1.12 2005/02/26 21:18:05 jyp Exp $
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
 *     CVS version 1.12 of 'firmware/common/dir.h' from rockbox CVS server.
 *
 ****************************************************************************/

#ifndef _ROCKBOX_DIR_H_
#define _ROCKBOX_DIR_H_

#include <stdbool.h>
#include <rockbox_file.h>

#ifndef DIRENT_DEFINED

struct dirent {
    unsigned char d_name[MAX_PATH];
    int attribute;
    long size;
    long startcluster;
    unsigned short wrtdate; /*  Last write date */
    unsigned short wrttime; /*  Last write time */
};
#endif

#include <rockbox_fat.h>

typedef struct {
    bool busy;
    long startcluster;
    struct fat_dir fatdir;
    struct fat_dir parent_dir;
    struct dirent theent;
} DIR;

#ifndef DIRFUNCTIONS_DEFINED

extern DIR* opendir(const char* name);
extern int closedir(DIR* dir);
extern int mkdir(const char* name, int mode);
extern int rmdir(const char* name);

extern struct dirent* readdir(DIR* dir);

#endif /* DIRFUNCTIONS_DEFINED */

#endif /* #ifndef _ROCKBOX_DIR_H_ */
