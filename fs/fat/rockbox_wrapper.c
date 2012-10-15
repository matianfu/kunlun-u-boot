/*------------------------------------------------------------------------------
 * (C) Copyright 2006
 * Keith Outwater, (outwater4@comcast.net)
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <config.h>

#if defined(CFG_ROCKBOX_FAT)

/*
 * FIXME: The pwd, cd and file path builder code needs to be fixed/implemented.
 */

#include <asm/errno.h>
#include <asm/string.h>
#include <fat.h>
#include <linux/string.h>
#include "rockbox_debug.h"

const char *month[] = {
	"(null)",
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/*
 * The current working directory.
 * Should we keep track of this on a per-partiton basis?
 */
char	file_cwd[ROCKBOX_CWD_LEN + 1] = "/";

#ifndef __HAVE_ARCH_STRNICMP
/*
 * Convert a character to lower case
 */
__inline__ static char
_tolower(char c)
{
    if ((c >= 'A') && (c <= 'Z')) {
        c = (c - 'A') + 'a';
    }
    return c;
}

/**
 * strnicmp - Case insensitive, length-limited string comparison
 * @s1: One string
 * @s2: The other string
 * @len: the maximum number of characters to compare
 */
int strnicmp(const char *s1, const char *s2, size_t len)
{
	/* Yes, Virginia, it had better be unsigned */
	unsigned char c1, c2;

	c1 = 0;	c2 = 0;
	if (len) {
		do {
			c1 = *s1; c2 = *s2;
			s1++; s2++;
			if (!c1)
				break;
			if (!c2)
				break;
			if (c1 == c2)
				continue;
			c1 = _tolower(c1);
			c2 = _tolower(c2);
			if (c1 != c2)
				break;
		} while (--len);
	}
	return (int)c1 - (int)c2;
}
#endif

/*
 * U-Boot 'fat ls' wrapper. Code lifted from rockbox "filetree.c"
 * Return number of bytes read on success, -1 on error.
 */
int
file_fat_ls(const char *dirname)
{
	int i;
	int name_buffer_used = 0;
	DIR *dir;

	struct tree_context {
		int filesindir;
		int dirsindir;
		int dirlength;
		void* dircache;
		char name_buffer[2048];
		int name_buffer_size;
		bool dirfull;
	} c;

	struct entry {
		short			attr;			/* FAT attributes + file type flags */
		unsigned long	time_write;		/* Last write time */
		char			*name;
	};

	if (strlen(dirname) > ROCKBOX_CWD_LEN) {
		errno = ENAMETOOLONG;
		return -1;
	}

	dir = opendir(dirname);
    if(!dir) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Error: ls() of \"%s\" failed: %s\n",
				dirname, strerror(errno));
#endif
        return -1;
	}

    c.dirsindir = 0;
    c.dirfull = false;
	c.name_buffer_size = sizeof(c.name_buffer) - 1;

	/* FIXME: figure out max i */
    for ( i=0; i < 10; i++ ) {
        int len;
        struct dirent *entry = readdir(dir);
        struct entry* dptr =
            (struct entry*)(c.dircache + i * sizeof(struct entry));

        if (!entry)
            break;

        len = strlen(entry->d_name);
		printf("%9lu %s %02d %4d %02d:%02d:%02d %s\n",
			   entry->size,
			   month[(entry->wrtdate >> 5) & 0xf],
			   entry->wrtdate & 0xf, ((entry->wrtdate >> 9) & 0x7f) + 1980,
			   (entry->wrttime >> 11) & 0x1f,
			   (entry->wrttime >> 5) & 0x3f,
			   entry->wrttime & 0x1f,
			   entry->d_name);
#if 0
		/*
		 * skip directories . and ..
		 */
		if ((entry->attribute & FAT_ATTR_DIRECTORY) &&
				(((len == 1) &&
				(!strncmp(entry->d_name, ".", 1))) ||
				((len == 2) &&
				(!strncmp(entry->d_name, "..", 2))))) {
			i--;
			continue;
		}

		/*
		 * Skip FAT volume ID
		 */
		if (entry->attribute & FAT_ATTR_VOLUME_ID) {
			i--;
			continue;
		}
#endif

        dptr->attr = entry->attribute;
        if (len > c.name_buffer_size - name_buffer_used - 1) {
            /* Tell the world that we ran out of buffer space */
            c.dirfull = true;
			fat_dprintf("Dir buffer is full\n");
            break;
        }

        dptr->name = &c.name_buffer[name_buffer_used];
        dptr->time_write = (long)entry->wrtdate<<16 | (long)entry->wrttime;
        strcpy(dptr->name,entry->d_name);
        name_buffer_used += len + 1;

        if (dptr->attr & FAT_ATTR_DIRECTORY) /* count the remaining dirs */
            c.dirsindir++;
    }

    c.filesindir = i;
    c.dirlength = i;
    closedir(dir);
	printf("\n%d file%s %d dir%s",
		   c.filesindir, c.filesindir == 1 ? ",":"s,",
		   c.dirsindir, c.dirsindir == 1 ? "\n":"s\n");
    return 0;
}

/*
 * U-Boot 'file read' wrapper.
 * Return number of bytes read on success, -1 on error.
 */
long
file_fat_read(const char *filename, void *buffer, unsigned long maxsize)
{
	int		fd;
	long	bytes;
	long	file_size;

	printf("Reading %s\n", filename);
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Error: open() of \"%s\" failed: %s\n",
				filename, strerror(errno));
#endif
		return -1;
	}

	/*
	 * If the number of bytes to read is zero, read the entire file.
	 */
	if (maxsize == 0) {
		file_size = filesize(fd);
		if (file_size < 0) {
			fat_eprintf("Call to filesize() failed\n");
			return -1;
		}

		maxsize = (unsigned long)file_size;
		fat_dprintf("Reading entire file (%lu bytes)\n", maxsize);
	}

	bytes = (long)read(fd, buffer, (size_t)maxsize);
	if ((unsigned long)bytes != maxsize) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Read error: %s\n", strerror(errno));
#endif
		close(fd);
		return -1;
	}

	close(fd);
	return bytes;
}

/*
 * U-Boot 'file write' wrapper.
 * Return number of bytes written on success, -1 on error.
 */
long
file_fat_write(const char *filename, void *buffer, unsigned long maxsize)
{
	int		fd;
	long	bytes;

	printf("Writing %s\n", filename);
	fd = open(filename, O_WRONLY | O_CREAT);
	if (fd < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Open of \"%s\" failed: %s\n",
				filename, strerror(errno));
#endif
		return -1;
	}

	bytes = (long)write(fd, buffer, (size_t)maxsize);
	if ((unsigned long)bytes!= maxsize) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Write failed: %s\n", strerror(errno));
#endif
		close(fd);
		return -1;
	}

	close(fd);
	return bytes;
}

int
file_fat_pwd(void)
{
	printf("%s\n", file_cwd);
	return 1;
}

/*
 * U-Boot 'file rm' wrapper.
 * Return 1 on success, -1 on error and set 'errno'.
 */
int
file_fat_rm(const char *filename)
{
	int rc;

	fat_dprintf("Remove \"%s\"\n", filename);
	rc = remove(filename);
	if (rc < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Removal of \"%s\" failed: %s\n",
				filename, strerror(errno));
#endif
		return -1;
	}

	return 1;
}

/*
 * U-Boot 'file mkdir' wrapper.
 * Return 1 on success, -1 on error and set 'errno'.
 */
int
file_fat_mkdir(const char *dirname)
{
	int rc;

	fat_dprintf("Make dir \"%s\"\n", dirname);
	rc = mkdir(dirname, 0);
	if (rc < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Creation of \"%s\" failed: %s\n",
				dirname, strerror(errno));
#endif
		return -1;
	}

	return 1;
}

/*
 * U-Boot 'file rmdir' wrapper.
 * Return 1 on success, -1 on error and set 'errno'.
 */
int
file_fat_rmdir(const char *dirname)
{
	int rc;

	fat_dprintf("Remove dir \"%s\"\n", dirname);
	rc = rmdir(dirname);
	if (rc < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Removal of \"%s\" failed: %s\n",
				dirname, strerror(errno));
#endif
		return -1;
	}

	return 1;
}

static void
pathcpy(char *dest, const char *src)
{
	char *origdest = dest;

	do {
		if (dest-file_cwd >= CWD_LEN) {
			*dest = '\0';
			return;
		}

		*(dest) = *(src);
		if (*src == '\0') {
			if (dest-- != origdest && ISDIRDELIM(*dest)) {
				*dest = '\0';
			}
			return;
		}

		++dest;
		if (ISDIRDELIM(*src)) {
			while (ISDIRDELIM(*src)) src++;
		}
		else {
			src++;
		}
	} while (1);
}

/*
 * U-Boot 'file cd' wrapper.
 */
int
file_fat_cd(const char *path)
{
	if (ISDIRDELIM(*path)) {
		while (ISDIRDELIM(*path)) path++;
		strncpy(file_cwd+1, path, CWD_LEN-1);
	} else {
		const char *origpath = path;
		char *tmpstr = file_cwd;
		int back = 0;

		while (*tmpstr != '\0') tmpstr++;

		do {
			tmpstr--;
		} while (ISDIRDELIM(*tmpstr));

		while (*path == '.') {
			path++;
			while (*path == '.') {
				path++;
				back++;
			}

			if (*path != '\0' && !ISDIRDELIM(*path)) {
				path = origpath;
				back = 0;
				break;
			}

			while (ISDIRDELIM(*path)) path++;

			origpath = path;
		}

		while (back--) {
			/* Strip off path component */
			while (!ISDIRDELIM(*tmpstr)) {
				tmpstr--;
			}

			if (tmpstr == file_cwd) {
				/* Incremented again right after the loop. */
				tmpstr--;
				break;
			}

			/* Skip delimiters */
			while (ISDIRDELIM(*tmpstr)) tmpstr--;
		}

		tmpstr++;
		if (*path == '\0') {
			if (tmpstr == file_cwd) {
				*tmpstr = '/';
				tmpstr++;
			}

			*tmpstr = '\0';
			return 0;
		}

		*tmpstr = '/';
		pathcpy(tmpstr + 1, path);
	}

	printf("New CWD is '%s'\n", file_cwd);
	return 0;
}

/*
 * U-Boot 'file mv' wrapper.
 * Return 1 on success, -1 on error and set 'errno'.
 */
int
file_fat_mv(const char *oldname, const char *newpath)
{
	int rc;

	fat_dprintf("Move \'%s\" to \"%s\"\n", oldname, newpath);
	rc = rename(oldname, newpath);
	if (rc < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Failed to move \"%s\" to \"%s\" : %s\n",
				oldname, newpath, strerror(errno));
#endif
		return -1;
	}

	return 1;
}

/* vim: set ts=4 tw=80 sw=4 ai fo=tcroq: */
#endif /* #if defined(CFG_ROCKBOX_FAT) */
