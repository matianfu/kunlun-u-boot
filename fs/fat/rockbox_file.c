/*******************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: file.c,v 1.62 2005/01/28 21:32:16 hohensoh Exp $
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
 *     CVS version 1.66 of 'firmware/common/file.c' from rockbox CVS server.
 *
 ****************************************************************************/
#include <common.h>
#include <config.h>

#if defined(CFG_ROCKBOX_FAT)
#include <asm/errno.h>
#include <asm/string.h>
#include <fat.h>
#include "rockbox_debug.h"

#ifndef __HAVE_ARCH_STRNICMP
extern int strnicmp(const char *s1, const char *s2, size_t len);
#endif

/*
  These functions provide a roughly POSIX-compatible file IO API.

  Since the fat32 driver only manages sectors, we maintain a one-sector
  cache for each open file. This way we can provide byte access without
  having to re-read the sector each time.
  The penalty is the RAM used for the cache and slightly more complex code.
*/

#if !defined(CFG_ROCKBOX_FAT_MAX_OPEN_FILES)
#define CFG_ROCKBOX_FAT_MAX_OPEN_FILES 1
#endif

struct filedesc {
    unsigned char cache[SECTOR_SIZE];
    int cacheoffset; /* invariant: 0 <= cacheoffset <= SECTOR_SIZE */
    long fileoffset;
    long size;
    int attr;
    struct fat_file fatfile;
    bool busy;
    bool write;
    bool dirty;
    bool trunc;
};

static struct filedesc openfiles[CFG_ROCKBOX_FAT_MAX_OPEN_FILES];

static int flush_sector_cache(int fd);

int creat(const char *pathname, mode_t mode)
{
    (void)mode;
    return open(pathname, O_WRONLY|O_CREAT|O_TRUNC);
}

int open_write_from_nand(const char* pathname, int flags)
{
	DIR* dir;
	int fd;
	char pathnamecopy[MAX_PATH];
	char* name;
	struct filedesc* file = NULL;
	/* find a free file descriptor */
	for ( fd=0; fd<CFG_ROCKBOX_FAT_MAX_OPEN_FILES; fd++ )
		if ( !openfiles[fd].busy )
			break;
	if ( fd == CFG_ROCKBOX_FAT_MAX_OPEN_FILES ) {
		fat_deprintf("Too many files open\n");
		errno = EMFILE;
		return -2;
		}
	file = &openfiles[fd];
	memset(file, 0, sizeof(struct filedesc));
	file ->write = true;

	strncpy(pathnamecopy,pathname,sizeof(pathnamecopy));
	pathnamecopy[sizeof(pathnamecopy)-1] = 0;

	/* locate filename */
	name=strrchr(pathnamecopy+1,'/');
	if ( name ) {
		*name = 0;
		dir = opendir(pathnamecopy);
		*name = '/';
		name++;
		}
	else {
		dir = opendir("/");
		name = pathnamecopy+1;
		}
	if (!dir) {
		fat_deprintf("Failed to open directory\n");
		errno = EIO;
		file->busy = false;
		return -4;
		}

}
int open(const char* pathname, int flags)
{
    DIR* dir;
    struct dirent* entry;
    int fd;
    char pathnamecopy[MAX_PATH];
    char* name;
    struct filedesc* file = NULL;
    int rc;

    fat_dprintf("pathname \"%s\", flags %d\n",pathname,flags);

	/*
	 * Only absolute paths are supported.
	 */
    if ( pathname[0] != '/' ) {
        fat_deprintf("\"%s\" is not an absolute path\n",pathname);
        errno = EINVAL;
        return -1;
    }

    /* find a free file descriptor */
    for ( fd=0; fd<CFG_ROCKBOX_FAT_MAX_OPEN_FILES; fd++ )
        if ( !openfiles[fd].busy )
            break;

    if ( fd == CFG_ROCKBOX_FAT_MAX_OPEN_FILES ) {
        fat_deprintf("Too many files open\n");
        errno = EMFILE;
        return -2;
    }

    file = &openfiles[fd];
    memset(file, 0, sizeof(struct filedesc));

    if (flags & (O_RDWR | O_WRONLY)) {
        file->write = true;

        if (flags & O_TRUNC)
            file->trunc = true;
    }
    file->busy = true;

    strncpy(pathnamecopy,pathname,sizeof(pathnamecopy));
    pathnamecopy[sizeof(pathnamecopy)-1] = 0;

    /* locate filename */
    name=strrchr(pathnamecopy+1,'/');
    if ( name ) {
        *name = 0;
        dir = opendir(pathnamecopy);
        *name = '/';
        name++;
    }
    else {
        dir = opendir("/");
        name = pathnamecopy+1;
    }
    if (!dir) {
        fat_deprintf("Failed to open directory\n");
        errno = EIO;
        file->busy = false;
        return -4;
    }

    if(name[0] == 0) {
        fat_deprintf("Empty file name\n");
        errno = EINVAL;
        file->busy = false;
        closedir(dir);
        return -5;
    }

    /* scan dir for name */
    while ((entry = readdir(dir))) {
        if ( !strnicmp(name, entry->d_name, strlen(name)) ) {
            fat_open( entry->startcluster,
                     &(file->fatfile),
                     &(dir->fatdir));
            file->size = file->trunc ? 0 : entry->size;
            file->attr = entry->attribute;
            break;
        }
    }

    if ( !entry ) {
        fat_dprintf("Didn't find file \"%s\"\n",name);
        if ( file->write && (flags & O_CREAT) ) {
            rc = fat_create_file(name, &(file->fatfile), &(dir->fatdir));
            if (rc < 0) {
                fat_deprintf("Couldn't create \"%s\" in \"%s\"\n",
							name,pathnamecopy);
                errno = EIO;
                file->busy = false;
                closedir(dir);
                return rc * 10 - 6;
            }
            file->size = 0;
            file->attr = 0;
        }
        else {
            fat_deprintf("Couldn't find \"%s\" in \"%s\"\n",
					name,pathnamecopy);
            errno = ENOENT;
            file->busy = false;
            closedir(dir);
            return -7;
        }
    } else {
        if(file->write && (file->attr & FAT_ATTR_DIRECTORY)) {
            errno = EISDIR;
            file->busy = false;
            closedir(dir);
            return -8;
        }
    }
    closedir(dir);

    file->cacheoffset = -1;
    file->fileoffset = 0;

    if (file->write && (flags & O_APPEND)) {
        rc = lseek(fd,0,SEEK_END);
        if (rc < 0 )
            return rc * 10 - 9;
    }

    return fd;
}

int close_write_from_nand(int fd)
{
    struct filedesc* file = &openfiles[fd];
    int rc = 0;

    fat_dprintf("fd %d\n", fd);

    if (fd < 0 || fd > CFG_ROCKBOX_FAT_MAX_OPEN_FILES-1) {
		errno = EINVAL;
		return -1;
		}
	if (!file->busy) {
		errno = EBADF;
		return -2;
		}
	rc = fat_write_from_nand(&(file->fatfile), file->cache);
	if (rc < 0)
		return rc * 10 - 3;
	rc = fsync_from_nand(fd);
	if (rc < 0)
		return rc * 10 - 3;
	file->busy = false;
	return 0;
}

int close(int fd)
{
    struct filedesc* file = &openfiles[fd];
    int rc = 0;

    fat_dprintf("fd %d\n", fd);

    if (fd < 0 || fd > CFG_ROCKBOX_FAT_MAX_OPEN_FILES-1) {
        errno = EINVAL;
        return -1;
    }
    if (!file->busy) {
        errno = EBADF;
        return -2;
    }
    if (file->write) {
        rc = fsync(fd);
        if (rc < 0)
            return rc * 10 - 3;
    }

    file->busy = false;
    return 0;
}
//
int fsync_from_nand(int fd)
{
	struct filedesc* file = &openfiles[fd];
	int rc = 0;
	fat_dprintf("fd %d\n", fd);
	/* tie up all loose ends */
	rc = fat_closewrite_form_nand(&(file->fatfile), file->size, file->attr);
	if (rc < 0)
		return rc * 10 - 5;
	return 0;
}
int fsync(int fd)
{
    struct filedesc* file = &openfiles[fd];
    int rc = 0;

    fat_dprintf("fd %d\n", fd);

    if (fd < 0 || fd > CFG_ROCKBOX_FAT_MAX_OPEN_FILES-1) {
        errno = EINVAL;
        return -1;
    }
    if (!file->busy) {
        errno = EBADF;
        return -2;
    }
    if (file->write) {
        /* flush sector cache */
        if ( file->dirty ) {
            rc = flush_sector_cache(fd);
            if (rc < 0)
                return rc * 10 - 3;
        }

        /* truncate? */
        if (file->trunc) {
            rc = ftruncate(fd, file->size);
            if (rc < 0)
                return rc * 10 - 4;
        }

        /* tie up all loose ends */
        rc = fat_closewrite(&(file->fatfile), file->size, file->attr);
        if (rc < 0)
            return rc * 10 - 5;
    }
    return 0;
}

int remove(const char* name)
{
    int rc;
    struct filedesc* file;
    int fd = open(name, O_WRONLY);
    if ( fd < 0 )
        return fd * 10 - 1;

    file = &openfiles[fd];
    rc = fat_remove(&(file->fatfile));
    if ( rc < 0 ) {
        fat_deprintf("Failed to remove file (rc = %d)\n", rc);
        errno = EIO;
        return rc * 10 - 3;
    }

    file->size = 0;

    rc = close(fd);
    if (rc<0)
        return rc * 10 - 4;

    return 0;
}

int rename(const char* path, const char* newpath)
{
    int rc, fd;
    DIR* dir;
    char* nameptr;
    char* dirptr;
    struct filedesc* file;
    char newpath2[MAX_PATH];

    /* verify new path does not already exist */
    /* If it is a directory, errno == EISDIR if the name exists */
    fd = open(newpath, O_RDONLY);
    if ( fd >= 0 || errno == EISDIR) {
        close(fd);
        errno = EBUSY;
        return -1;
    }
    close(fd);

    fd = open(path, O_RDONLY);
    if ( fd < 0 ) {
        errno = EIO;
        return fd * 10 - 2;
    }

    /* extract new file name */
    nameptr = strrchr(newpath,'/');
    if (nameptr)
        nameptr++;
    else
        return - 3;

    /* Extract new path */
    strcpy(newpath2, newpath);

    dirptr = strrchr(newpath2,'/');
    if(dirptr)
        *dirptr = 0;
    else
        return - 4;

    dirptr = newpath2;

    if(strlen(dirptr) == 0) {
        dirptr = "/";
    }

    dir = opendir(dirptr);
    if(!dir)
        return - 5;

    file = &openfiles[fd];
    rc = fat_rename(&file->fatfile, &dir->fatdir, nameptr,
                    file->size, file->attr);
    if ( rc < 0 ) {
        fat_deprintf("Failed to remane file\n", rc);
        errno = EIO;
        return rc * 10 - 6;
    }

    rc = close(fd);
    if (rc<0) {
        errno = EIO;
        return rc * 10 - 7;
    }

    rc = closedir(dir);
    if (rc<0) {
        errno = EIO;
        return rc * 10 - 8;
    }

    return 0;
}

int ftruncate(int fd, off_t size)
{
    int rc, sector;
    struct filedesc* file = &openfiles[fd];

    sector = size / SECTOR_SIZE;
    if (size % SECTOR_SIZE)
        sector++;

    rc = fat_seek(&(file->fatfile), sector);
    if (rc < 0) {
        errno = EIO;
        return rc * 10 - 1;
    }

    rc = fat_truncate(&(file->fatfile));
    if (rc < 0) {
        errno = EIO;
        return rc * 10 - 2;
    }

    file->size = size;

    return 0;
}

static int flush_sector_cache(int fd)
{
    int rc;
    struct filedesc* file = &openfiles[fd];
    long sector = file->fileoffset / SECTOR_SIZE;

    fat_dprintf("Flush dirty sectors\n");

    /* make sure we are on correct sector */
    rc = fat_seek(&(file->fatfile), sector);
    if ( rc < 0 )
        return rc * 10 - 3;

    rc = fat_readwrite(&(file->fatfile), 1,
                       file->cache, true );

    if ( rc < 0 ) {
        if(file->fatfile.eof)
            errno = ENOSPC;

        return rc * 10 - 2;
    }

    file->dirty = false;

    return 0;
}

int write_from_nand(int fd, void* buf, long count)
{
	long sectors,remain;
	long nread=0;
	struct filedesc* file = &openfiles[fd];
	int rc;
	if ( !file->busy ) {
		errno = EBADF;
		return -1;
		}
	fat_dprintf("fd %d, buf %lx, count %ld,write\n",
				fd,(long)buf,count);
	    /* any head bytes? */

	if ( file->cacheoffset != -1 ) {
		int headbytes;
		int offs = file->cacheoffset;
		if ( count <= SECTOR_SIZE - file->cacheoffset ) {
			headbytes = count;
			file->cacheoffset += count;
			nread+=count;
			if ( file->cacheoffset >= SECTOR_SIZE )
				file->cacheoffset = -1;
			}
		else {
			headbytes = SECTOR_SIZE - file->cacheoffset;
			file->cacheoffset = -1;
			}
		memcpy( file->cache + offs, buf, headbytes );
		nread += headbytes;
		if (offs+headbytes == SECTOR_SIZE) {
			int rc = fat_write_from_nand(&(file->fatfile), file->cache);
			if ( rc < 0 ) {
				errno = EIO;
				return rc * 10 - 2;
				}
			file->cacheoffset = -1;
			}
		else
			file->dirty = true;
		count -= headbytes;
		buf = (void *)((unsigned long)buf+headbytes);
		}
	remain=0;
	sectors = count / SECTOR_SIZE;
	remain = count % SECTOR_SIZE;
	if(remain && (file->cacheoffset == -1)){
		count -= remain;
		nread += remain;
		memcpy( file->cache, (unsigned char *)((unsigned long)buf + count ), remain);
		file->cacheoffset  = remain;
		file->dirty = true;
		}

	while(sectors) {
		int rc;
		rc = fat_write_from_nand(&(file->fatfile), buf);
		if ( rc < 0 ) {
			fat_deprintf("fat_write_from_nand failed\n", rc);
			errno = EIO;
			return rc;
			}
		buf = (void *)((unsigned long)buf+SECTOR_SIZE);
		file->dirty = true;
		sectors--;
		nread += SECTOR_SIZE;
		}
	file->size += nread;
	return nread;

}

static int readwrite(int fd, void* buf, long count, bool write)
{
    long sectors;
    long nread=0;
    struct filedesc* file = &openfiles[fd];
    int rc;

    if ( !file->busy ) {
        errno = EBADF;
        return -1;
    }

    fat_dprintf("fd %d, buf %lx, count %ld, %s\n",
				fd,(long)buf,count,write?"write":"read");

    /* attempt to read past EOF? */
    if (!write && count > file->size - file->fileoffset)
        count = file->size - file->fileoffset;

    /* any head bytes? */
    if ( file->cacheoffset != -1 ) {
        int headbytes;
        int offs = file->cacheoffset;
        if ( count <= SECTOR_SIZE - file->cacheoffset ) {
            headbytes = count;
            file->cacheoffset += count;
            if ( file->cacheoffset >= SECTOR_SIZE )
                file->cacheoffset = -1;
        }
        else {
            headbytes = SECTOR_SIZE - file->cacheoffset;
            file->cacheoffset = -1;
        }

        if (write) {
            memcpy( file->cache + offs, buf, headbytes );
            if (offs+headbytes == SECTOR_SIZE) {
                int rc = flush_sector_cache(fd);
                if ( rc < 0 ) {
                    errno = EIO;
                    return rc * 10 - 2;
                }
                file->cacheoffset = -1;
            }
            else
                file->dirty = true;
        }
        else {
            memcpy( buf, file->cache + offs, headbytes );
        }

        nread = headbytes;
        count -= headbytes;
    }

    /* If the buffer has been modified, either it has been flushed already
     * (if (offs+headbytes == SECTOR_SIZE)...) or does not need to be (no
     * more data to follow in this call). Do NOT flush here. */

    /* read/write whole sectors right into/from the supplied buffer */
    sectors = count / SECTOR_SIZE;
    if ( sectors ) {
        int rc = fat_readwrite(&(file->fatfile), sectors,
            (unsigned char*)buf+nread, write );
        if ( rc < 0 ) {
            fat_deprintf("Failed to %s %ld sectors\n",
						write?"write":"read",sectors);
            errno = EIO;
            if(write && file->fatfile.eof) {
                fat_deprintf("No space left on device\n");
                errno = ENOSPC;
            } else {
                file->fileoffset += nread;
            }
            file->cacheoffset = -1;
            return nread ? nread : rc * 10 - 4;
        }
        else {
            if ( rc > 0 ) {
                nread += rc * SECTOR_SIZE;
                count -= sectors * SECTOR_SIZE;

                /* if eof, skip tail bytes */
                if ( rc < sectors )
                    count = 0;
            }
            else {
                /* eof */
                count=0;
            }

            file->cacheoffset = -1;
        }
    }

    /* any tail bytes? */
    if ( count ) {
        if (write) {
            if ( file->fileoffset + nread < file->size ) {
                /* sector is only partially filled. copy-back from disk */
                int rc;
                fat_dprintf("Copy-back tail cache\n");
                rc = fat_readwrite(&(file->fatfile), 1, file->cache, false );
                if ( rc < 0 ) {
                    fat_deprintf("Failed to write cache to disk\n");
                    errno = EIO;
                    file->fileoffset += nread;
                    file->cacheoffset = -1;
                    return nread ? nread : rc * 10 - 5;
                }
                /* seek back one sector to put file position right */
                rc = fat_seek(&(file->fatfile),
                              (file->fileoffset + nread) /
                              SECTOR_SIZE);
                if ( rc < 0 ) {
                    fat_deprintf("fat_seek() failed\n", rc);
                    errno = EIO;
                    file->fileoffset += nread;
                    file->cacheoffset = -1;
                    return nread ? nread : rc * 10 - 6;
                }
            }
            memcpy( file->cache, (unsigned char*)buf + nread, count );
            file->dirty = true;
        }
        else {
            rc = fat_readwrite(&(file->fatfile), 1, &(file->cache),false);
            if (rc < 1 ) {
                fat_deprintf("Failed to cache sector\n", rc);
                errno = EIO;
                file->fileoffset += nread;
                file->cacheoffset = -1;
                return nread ? nread : rc * 10 - 7;
            }
            memcpy( (unsigned char*)buf + nread, file->cache, count );
        }

        nread += count;
        file->cacheoffset = count;
    }

    file->fileoffset += nread;
    fat_dprintf("fileoffset %ld\n", file->fileoffset);

    /* adjust file size to length written */
    if ( write && file->fileoffset > file->size )
        file->size = file->fileoffset;

    return nread;
}

ssize_t write(int fd, const void* buf, size_t count)
{
    if (!openfiles[fd].write) {
        errno = EACCES;
        return -1;
    }
    return readwrite(fd, (void *)buf, count, true);
}

ssize_t read(int fd, void* buf, size_t count)
{
    return readwrite(fd, buf, count, false);
}


off_t lseek(int fd, off_t offset, int whence)
{
    off_t pos;
    long newsector;
    long oldsector;
    int sectoroffset;
    int rc;
    struct filedesc* file = &openfiles[fd];

    fat_dprintf("fd %d, offset %ld, whence %d\n",fd,offset,whence);

    if ( !file->busy ) {
        errno = EBADF;
        return -1;
    }

    switch ( whence ) {
        case SEEK_SET:
            pos = offset;
            break;

        case SEEK_CUR:
            pos = file->fileoffset + offset;
            break;

        case SEEK_END:
            pos = file->size + offset;
            break;

        default:
            errno = EINVAL;
            return -2;
    }
    if ((pos < 0) || (pos > file->size)) {
        errno = EINVAL;
        return -3;
    }

    /* new sector? */
    newsector = pos / SECTOR_SIZE;
    oldsector = file->fileoffset / SECTOR_SIZE;
    sectoroffset = pos % SECTOR_SIZE;

    if ( (newsector != oldsector) ||
         ((file->cacheoffset==-1) && sectoroffset) ) {

        if ( newsector != oldsector ) {
            if (file->dirty) {
                rc = flush_sector_cache(fd);
                if (rc < 0)
                    return rc * 10 - 5;
            }

            rc = fat_seek(&(file->fatfile), newsector);
            if ( rc < 0 ) {
                errno = EIO;
                return rc * 10 - 4;
            }
        }
        if ( sectoroffset ) {
            rc = fat_readwrite(&(file->fatfile), 1,
                               &(file->cache),false);
            if ( rc < 0 ) {
                errno = EIO;
                return rc * 10 - 6;
            }
            file->cacheoffset = sectoroffset;
        }
        else
            file->cacheoffset = -1;
    }
    else
        if ( file->cacheoffset != -1 )
            file->cacheoffset = sectoroffset;

    file->fileoffset = pos;

    return pos;
}

off_t filesize(int fd)
{
    struct filedesc* file = &openfiles[fd];

    if ( !file->busy ) {
        errno = EBADF;
        return -1;
    }

    return file->size;
}

/* vim: set ts=4 tw=80 sw=4 fo=tcroq: */
#endif /* #if defined(CFG_ROCKBOX_FAT) */
