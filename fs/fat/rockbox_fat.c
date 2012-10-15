/*******************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: fat.c,v 1.119 2005/09/07 01:35:15 rasher Exp $
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
 *     CVS version 1.123 of 'firmware/drivers/fat.c' from rockbox CVS server.
 *
 ****************************************************************************/

#include <common.h>
#include <config.h>

#if defined(CFG_ROCKBOX_FAT)
#include <linux/ctype.h>
#include <fat.h>
#include "rockbox_debug.h"

/*
 * Override this parameter to boost performance ONLY if you are sure your driver
 * can handle requests of more than 256 sectors!
 */
#if !defined(CFG_ROCKBOX_FAT_MAX_SECS_PER_XFER)
#define CFG_ROCKBOX_FAT_MAX_SECS_PER_XFER	256
#endif

#define	SWAB16	FAT2CPU16
#define	SWAB32	FAT2CPU32

#if !defined(NO_ROCKBOX_FAT16_SUPPORT)
#define	HAVE_FAT16SUPPORT
#else
#undef	HAVE_FAT16SUPPORT
#endif

extern int disk_read(__u32 startblock, __u32 getsize, __u8 * bufptr);
extern int disk_write(__u32 startblock, __u32 putsize, __u8 * bufptr);

/*
 * The current working directory.
 */
extern char file_cwd[];

/*
 * The current device and partition information. Updated by
 * fat_register_device().
 */
extern	cur_block_dev_t	cur_block_dev;

#define BYTES2INT16(array,pos) \
          (array[pos] | (array[pos+1] << 8 ))
#define BYTES2INT32(array,pos)					\
    ((long)array[pos] | ((long)array[pos+1] << 8 ) |		\
     ((long)array[pos+2] << 16 ) | ((long)array[pos+3] << 24 ))

#define FATTYPE_FAT12       0
#define FATTYPE_FAT16       1
#define FATTYPE_FAT32       2

/* BPB offsets; generic */
#define BS_JMPBOOT          0
#define BS_OEMNAME          3
#define BPB_BYTSPERSEC      11
#define BPB_SECPERCLUS      13
#define BPB_RSVDSECCNT      14
#define BPB_NUMFATS         16
#define BPB_ROOTENTCNT      17
#define BPB_TOTSEC16        19
#define BPB_MEDIA           21
#define BPB_FATSZ16         22
#define BPB_SECPERTRK       24
#define BPB_NUMHEADS        26
#define BPB_HIDDSEC         28
#define BPB_TOTSEC32        32

/* fat12/16 */
#define BS_DRVNUM           36
#define BS_RESERVED1        37
#define BS_BOOTSIG          38
#define BS_VOLID            39
#define BS_VOLLAB           43
#define BS_FILSYSTYPE       54

/* fat32 */
#define BPB_FATSZ32         36
#define BPB_EXTFLAGS        40
#define BPB_FSVER           42
#define BPB_ROOTCLUS        44
#define BPB_FSINFO          48
#define BPB_BKBOOTSEC       50
#define BS_32_DRVNUM        64
#define BS_32_BOOTSIG       66
#define BS_32_VOLID         67
#define BS_32_VOLLAB        71
#define BS_32_FILSYSTYPE    82

#define BPB_LAST_WORD       510

/* attributes */
#define FAT_ATTR_LONG_NAME   (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | \
                              FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)
#define FAT_ATTR_LONG_NAME_MASK (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | \
                                 FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID | \
                                 FAT_ATTR_DIRECTORY | FAT_ATTR_ARCHIVE )

/* NTRES flags */
#define FAT_NTRES_LC_NAME    0x08
#define FAT_NTRES_LC_EXT     0x10

#define FATDIR_NAME          0
#define FATDIR_ATTR          11
#define FATDIR_NTRES         12
#define FATDIR_CRTTIMETENTH  13
#define FATDIR_CRTTIME       14
#define FATDIR_CRTDATE       16
#define FATDIR_LSTACCDATE    18
#define FATDIR_FSTCLUSHI     20
#define FATDIR_WRTTIME       22
#define FATDIR_WRTDATE       24
#define FATDIR_FSTCLUSLO     26
#define FATDIR_FILESIZE      28

#define FATLONG_ORDER        0
#define FATLONG_TYPE         12
#define FATLONG_CHKSUM       13

#define CLUSTERS_PER_FAT_SECTOR (SECTOR_SIZE / 4)
#define CLUSTERS_PER_FAT16_SECTOR (SECTOR_SIZE / 2)
#define DIR_ENTRIES_PER_SECTOR  (SECTOR_SIZE / DIR_ENTRY_SIZE)
#define DIR_ENTRY_SIZE       32
#define NAME_BYTES_PER_ENTRY 13
#define FAT_BAD_MARK         0x0ffffff7
#define FAT_EOF_MARK         0x0ffffff8

/* filename charset conversion table */
static const unsigned char unicode2iso8859_2[] = {
    0x00, 0x00, 0xc3, 0xe3, 0xa1, 0xb1, 0xc6, 0xe6,  /* 0x0100 */
    0x00, 0x00, 0x00, 0x00, 0xc8, 0xe8, 0xcf, 0xef,  /* 0x0108 */
    0xd0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0110 */
    0xca, 0xea, 0xcc, 0xec, 0x00, 0x00, 0x00, 0x00,  /* 0x0118 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0120 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0128 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0130 */
    0x00, 0xc5, 0xe5, 0x00, 0x00, 0xa5, 0xb5, 0x00,  /* 0x0138 */
    0x00, 0xa3, 0xb3, 0xd1, 0xf1, 0x00, 0x00, 0xd2,  /* 0x0140 */
    0xf2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0148 */
    0xd5, 0xf5, 0x00, 0x00, 0xc0, 0xe0, 0x00, 0x00,  /* 0x0150 */
    0xd8, 0xf8, 0xa6, 0xb6, 0x00, 0x00, 0xaa, 0xba,  /* 0x0158 */
    0xa9, 0xb9, 0xde, 0xfe, 0xab, 0xbb, 0x00, 0x00,  /* 0x0160 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd9, 0xf9,  /* 0x0168 */
    0xdb, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0170 */
    0x00, 0xac, 0xbc, 0xaf, 0xbf, 0xae, 0xbe, 0x00,  /* 0x0178 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0180 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0188 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0190 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0198 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01a0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01a8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01b0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01b8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01c0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01c8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01d0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01d8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01e0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01e8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01f0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x01f8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0200 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0208 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0210 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0218 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0220 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0228 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0230 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0238 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0240 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0248 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0250 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0258 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0260 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0268 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0270 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0278 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0280 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0288 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0290 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x0298 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x02a0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x02a8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x02b0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x02b8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb7,  /* 0x02c0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x02c8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x02d0 */
    0xa2, 0xff, 0x00, 0xb2, 0x00, 0xbd, 0x00, 0x00,  /* 0x02d8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x02e0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x02e8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x02f0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   /* 0x02f8 */
};

struct fsinfo {
    unsigned long freecount;	/* last known free cluster count */
    unsigned long nextfree;		/* first cluster to start looking for free
									clusters, or 0xffffffff for no hint */
};

/* fsinfo offsets */
#define FSINFO_FREECOUNT 488
#define FSINFO_NEXTFREE  492

struct bpb {
    int bpb_bytspersec;  /* Bytes per sector, typically 512 */
    unsigned int bpb_secperclus;	/* Sectors per cluster */
    int bpb_rsvdseccnt;  /* Number of reserved sectors */
    int bpb_numfats;     /* Number of FAT structures, typically 2 */
    int bpb_totsec16;    /* Number of sectors on the volume (old 16-bit) */
    int bpb_media;       /* Media type (typically 0xf0 or 0xf8) */
    int bpb_fatsz16;     /* Number of used sectors per FAT structure */
    unsigned long bpb_totsec32;	/* Num sectors on the volume (new 32-bit) */
    unsigned int last_word;		/* 0xAA55 */

    /**** FAT32 specific *****/
    long bpb_fatsz32;
    long bpb_rootclus;
    long bpb_fsinfo;

    /* variables for internal use */
    unsigned long fatsize;
    unsigned long totalsectors;
    unsigned long rootdirsector;
    unsigned long firstdatasector;
    unsigned long startsector;
    unsigned long dataclusters;
    struct fsinfo fsinfo;
#ifdef HAVE_FAT16SUPPORT
    int bpb_rootentcnt;  /* Number of dir entries in the root */
    /* internals for FAT16 support */
    bool is_fat16; /* true if we mounted a FAT16 partition, false if FAT32 */
    unsigned int rootdiroffset; /* sector offset of root dir relative to start
                                 * of first pseudo cluster */
#endif /* #ifdef HAVE_FAT16SUPPORT */
};

/*
 * mounted partition info
 */
static struct bpb fat_bpb;

static int update_fsinfo(void);
static int flush_fat(void);
static int bpb_is_sane(void);
static void *cache_fat_sector(long secnum, bool dirty);
static void create_dos_name(const unsigned char *name, unsigned char *newname);
static void randomize_dos_name(unsigned char *name);
static unsigned long find_free_cluster(unsigned long start);
static int transfer(unsigned long start, long count, char* buf, bool write);

#define FAT_CACHE_SIZE 0x20
#define FAT_CACHE_MASK (FAT_CACHE_SIZE-1)

struct fat_cache_entry
{
    long secnum;
    bool inuse;
    bool dirty;
};

static char fat_cache_sectors[FAT_CACHE_SIZE][SECTOR_SIZE];
static struct fat_cache_entry fat_cache[FAT_CACHE_SIZE];

static long cluster2sec(long cluster)
{
#ifdef HAVE_FAT16SUPPORT
    /* negative clusters (FAT16 root dir) don't get the 2 offset */
    int zerocluster = cluster < 0 ? 0 : 2;
#else
    const long zerocluster = 2;
#endif

    if (cluster > (long)(fat_bpb.dataclusters + 1)) {
		fat_eprintf("Bad cluster number (cluster %ld)\n", cluster);
        return -1;
    }

    return (cluster - zerocluster) * fat_bpb.bpb_secperclus
           + fat_bpb.firstdatasector;
}

void fat_size(unsigned long* size, unsigned long* free)
{
    if (size)
      *size = fat_bpb.dataclusters * fat_bpb.bpb_secperclus / 2;

    if (free)
      *free = fat_bpb.fsinfo.freecount * fat_bpb.bpb_secperclus / 2;
}

static void fat_init(void)
{
    unsigned int i;

    /* mark the FAT cache as unused */
    for(i = 0;i < FAT_CACHE_SIZE;i++) {
        fat_cache[i].secnum = 8; /* We use a "safe" sector just in case */
        fat_cache[i].inuse = false;
        fat_cache[i].dirty = false;
    }
}

/*
 * Mount the FAT filesystem located at 'startsector'.
 * Return 0 on success, an error-specific negative number on error.
 */
int rockbox_fat_mount(long startsector)
{
    static unsigned char buf[SECTOR_SIZE];
    int rc;
    long datasec;
#ifdef HAVE_FAT16SUPPORT
    int rootdirsectors;
#endif

	fat_dprintf("startsector %ld\n", startsector);
	fat_init();
    rc = disk_read(startsector, 1, buf);

    if (rc < 0) {
        fat_eprintf("Could not read BPB (rc = %d)\n", rc);
        return rc * 10 - 1;
    }

    memset(&fat_bpb, 0, sizeof(struct bpb));
    fat_bpb.startsector    = startsector;

    fat_bpb.bpb_bytspersec = BYTES2INT16(buf,BPB_BYTSPERSEC);
    fat_bpb.bpb_secperclus = buf[BPB_SECPERCLUS];
    fat_bpb.bpb_rsvdseccnt = BYTES2INT16(buf,BPB_RSVDSECCNT);
    fat_bpb.bpb_numfats    = buf[BPB_NUMFATS];
    fat_bpb.bpb_totsec16   = BYTES2INT16(buf,BPB_TOTSEC16);
    fat_bpb.bpb_media      = buf[BPB_MEDIA];
    fat_bpb.bpb_fatsz16    = BYTES2INT16(buf,BPB_FATSZ16);
    fat_bpb.bpb_fatsz32    = BYTES2INT32(buf,BPB_FATSZ32);
    fat_bpb.bpb_totsec32   = BYTES2INT32(buf,BPB_TOTSEC32);
    fat_bpb.last_word      = BYTES2INT16(buf,BPB_LAST_WORD);

    /* calculate a few commonly used values */
    if (fat_bpb.bpb_fatsz16 != 0)
        fat_bpb.fatsize = fat_bpb.bpb_fatsz16;
    else
        fat_bpb.fatsize = fat_bpb.bpb_fatsz32;

    if (fat_bpb.bpb_totsec16 != 0)
        fat_bpb.totalsectors = fat_bpb.bpb_totsec16;
    else
        fat_bpb.totalsectors = fat_bpb.bpb_totsec32;

#ifdef HAVE_FAT16SUPPORT
    fat_bpb.bpb_rootentcnt = BYTES2INT16(buf,BPB_ROOTENTCNT);
    rootdirsectors = ((fat_bpb.bpb_rootentcnt * 32)
        + (fat_bpb.bpb_bytspersec - 1)) / fat_bpb.bpb_bytspersec;
#endif /* #ifdef HAVE_FAT16SUPPORT */

    fat_bpb.firstdatasector = fat_bpb.bpb_rsvdseccnt
#ifdef HAVE_FAT16SUPPORT
        + rootdirsectors
#endif
        + fat_bpb.bpb_numfats * fat_bpb.fatsize;

    /* Determine FAT type */
    datasec = fat_bpb.totalsectors - fat_bpb.firstdatasector;
    fat_bpb.dataclusters = datasec / fat_bpb.bpb_secperclus;

    if ( fat_bpb.dataclusters < 65525 ) { /* FAT16 */
#ifdef HAVE_FAT16SUPPORT
        fat_bpb.is_fat16 = true;
        if (fat_bpb.dataclusters < 4085) { /* FAT12 */
            fat_eprintf("FAT12 filesystem not supported\n");
            return -2;
        }
#else /* #ifdef HAVE_FAT16SUPPORT */
        fat_eprintf("Not a FAT32 filesystem\n");
        return -2;
#endif /* #ifndef HAVE_FAT16SUPPORT */
    }

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb.is_fat16) { /* FAT16 specific part of BPB */
        int dirclusters;
        fat_bpb.rootdirsector = fat_bpb.bpb_rsvdseccnt
            + fat_bpb.bpb_numfats * fat_bpb.bpb_fatsz16;
        dirclusters = ((rootdirsectors + fat_bpb.bpb_secperclus - 1)
            / fat_bpb.bpb_secperclus); /* rounded up, to full clusters */
        /* I assign negative pseudo cluster numbers for the root directory,
           their range is counted upward until -1. */
        fat_bpb.bpb_rootclus = 0 - dirclusters; /* backwards, before the data */
        fat_bpb.rootdiroffset = dirclusters * fat_bpb.bpb_secperclus
            - rootdirsectors;
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    { /* FAT32 specific part of BPB */
        fat_bpb.bpb_rootclus  = BYTES2INT32(buf,BPB_ROOTCLUS);
        fat_bpb.bpb_fsinfo    = BYTES2INT16(buf,BPB_FSINFO);
        fat_bpb.rootdirsector = cluster2sec(fat_bpb.bpb_rootclus);
    }

    rc = bpb_is_sane();
    if (rc < 0) {
        fat_eprintf("BPB is not sane\n");
        return rc * 10 - 3;
    }

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb.is_fat16) {
        fat_bpb.fsinfo.freecount = 0xffffffff; /* force recalc below */
        fat_bpb.fsinfo.nextfree = 0xffffffff;
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        /* Read the fsinfo sector */
        rc = disk_read(startsector + fat_bpb.bpb_fsinfo, 1, buf);
        if (rc < 0) {
            fat_eprintf("Could not read FSInfo (rc = %d)\n", rc);
            return rc * 10 - 4;
        }
        fat_bpb.fsinfo.freecount = BYTES2INT32(buf, FSINFO_FREECOUNT);
        fat_bpb.fsinfo.nextfree = BYTES2INT32(buf, FSINFO_NEXTFREE);
    }

    /* calculate freecount if unset */
    if ( fat_bpb.fsinfo.freecount == 0xffffffff ) {
        fat_recalc_free();
    }

    fat_dprintf("\n  freecount %ld, nextfree 0x%lx, cluster count 0x%lx\n"
				"  sectors/cluster %d, FAT sectors 0x%lx\n",
				fat_bpb.fsinfo.freecount, fat_bpb.fsinfo.nextfree,
				fat_bpb.dataclusters, fat_bpb.bpb_secperclus,
				fat_bpb.fatsize);
    return 0;
}

void fat_recalc_free(void)
{
    long free = 0;
    unsigned long i;
#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb.is_fat16) {
        for (i = 0; i<fat_bpb.fatsize; i++) {
            unsigned int j;
            unsigned short* fat = cache_fat_sector(i, false);
            for (j = 0; j < CLUSTERS_PER_FAT16_SECTOR; j++) {
                unsigned int c = i * CLUSTERS_PER_FAT16_SECTOR + j;
                if ( c > fat_bpb.dataclusters+1 ) /* nr 0 is unused */
                    break;

                if (SWAB16(fat[j]) == 0x0000) {
                    free++;
                    if ( fat_bpb.fsinfo.nextfree == 0xffffffff )
                        fat_bpb.fsinfo.nextfree = c;
                }
            }
        }
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        for (i = 0; i<fat_bpb.fatsize; i++) {
            unsigned int j;
            unsigned long* fat = cache_fat_sector(i, false);
            for (j = 0; j < CLUSTERS_PER_FAT_SECTOR; j++) {
                unsigned long c = i * CLUSTERS_PER_FAT_SECTOR + j;
                if ( c > fat_bpb.dataclusters+1 ) /* nr 0 is unused */
                    break;

                if (!(SWAB32(fat[j]) & 0x0fffffff)) {
                    free++;
                    if ( fat_bpb.fsinfo.nextfree == 0xffffffff )
                        fat_bpb.fsinfo.nextfree = c;
                }
            }
        }
    }
    fat_bpb.fsinfo.freecount = free;
    update_fsinfo();
}

static int bpb_is_sane(void)
{

    if(fat_bpb.bpb_bytspersec != SECTOR_SIZE) {
        fat_eprintf("Sector size is not %d (detected %d)\n",
					SECTOR_SIZE, fat_bpb.bpb_bytspersec);
        return -1;
    }

    if((long)fat_bpb.bpb_secperclus * (long)fat_bpb.bpb_bytspersec >
	   128L*1024L) {
        fat_eprintf("Cluster size larger than 128K (computed %d)\n",
					fat_bpb.bpb_bytspersec * fat_bpb.bpb_secperclus);
        return -2;
    }

    if(fat_bpb.bpb_numfats != 2) {
        printf("Warning: Number of FATs is not 2 (detected %d)\n",
			   fat_bpb.bpb_numfats);
    }

    if(fat_bpb.bpb_media != 0xf0 && fat_bpb.bpb_media < 0xf8) {
        printf("Warning: Non-standard media type (detected 0x%02x)\n",
			   fat_bpb.bpb_media);
    }

    if(fat_bpb.last_word != 0xaa55) {
		fat_eprintf("Last word is not 0xaa55 (detected 0x%04x)\n",
					fat_bpb.last_word);
        return -3;
    }

    if (fat_bpb.fsinfo.freecount >
			(fat_bpb.totalsectors - fat_bpb.firstdatasector)/
	        fat_bpb.bpb_secperclus) {
        fat_eprintf("FSInfo.Freecount > disk size "
				"(freecount = 0x%04lx)\n", fat_bpb.fsinfo.freecount);
        return -4;
    }

    return 0;
}

static void flush_fat_sector(struct fat_cache_entry *fce,
                             unsigned char *sectorbuf)
{
    int rc;
    long secnum;

    secnum = fce->secnum + fat_bpb.startsector;

    /* Write to the first FAT */
    rc = disk_write(secnum, 1, sectorbuf);
    if(rc < 0) {
		fat_pprintf("Could not write sector %ld (rc = %d)\n", secnum, rc);
    }

    if(fat_bpb.bpb_numfats > 1) {
        /* Write to the second FAT */
        secnum += fat_bpb.fatsize;
        rc = disk_write(secnum, 1, sectorbuf);
        if(rc < 0) {
			fat_pprintf("Could not write sector %ld (rc = %d)\n", secnum, rc);
        }
    }

    fce->dirty = false;
}

static void *cache_fat_sector(long fatsector, bool dirty)
{
    long secnum = fatsector + fat_bpb.bpb_rsvdseccnt;
    int cache_index = secnum & FAT_CACHE_MASK;
    struct fat_cache_entry *fce = &fat_cache[cache_index];
    unsigned char *sectorbuf = &fat_cache_sectors[cache_index][0];
    int rc;

    /* Delete the cache entry if it isn't the sector we want */
    if(fce->inuse && (fce->secnum != secnum)) {
        /* Write back if it is dirty */
        if(fce->dirty) {
            flush_fat_sector(fce, sectorbuf);
        }
        fce->inuse = false;
    }

    /* Load the sector if it is not cached */
    if(!fce->inuse) {
        rc = disk_read(secnum + fat_bpb.startsector,1, sectorbuf);
        if(rc < 0) {
            fprintf(stderr, "Read of sector %ld failed (rc = %d)\n",secnum, rc);
            return NULL;
        }

        fce->inuse = true;
        fce->secnum = secnum;
    }

    if (dirty)
        fce->dirty = true; /* dirt remains, sticky until flushed */

    return sectorbuf;
}
//
static int curr_pt=0;
static unsigned long find_free_cluster_write_from_nand(unsigned long startcluster)
{
	unsigned long sector;
	unsigned long offset;
	unsigned long i;

	sector = startcluster / CLUSTERS_PER_FAT_SECTOR;
	offset = startcluster % CLUSTERS_PER_FAT_SECTOR;
	for (i = 0; i<fat_bpb.fatsize; i++) {
		unsigned int j;
		unsigned long nr = (i + sector) % fat_bpb.fatsize;
		unsigned long* fat;
		long secnum = nr + fat_bpb.bpb_rsvdseccnt;
		struct fat_cache_entry *fce = &fat_cache[curr_pt%2];
		struct fat_cache_entry *fce_n = &fat_cache[(curr_pt+1)%2];
		unsigned char *sectorbuf = &fat_cache_sectors[curr_pt][0];
		unsigned char *sectorbuf_n = &fat_cache_sectors[(curr_pt+1)%2][0];
		int rc;
		/* Delete the cache entry if it isn't the sector we want */
		if((fce->secnum != secnum)&&(fce_n->secnum != secnum)) {
			//printf("find_free_cluster_write_from_nand:secnum 0x%x fce->secnum:0x%x\n",secnum,fce->secnum);
			/* Write back if it is dirty */

			rc = disk_write(fce_n->secnum + fat_bpb.startsector, 1, sectorbuf_n);
			if(rc < 0) {
				fat_pprintf("Could not write sector %ld (rc = %d)\n", secnum, rc);
				}
			/* Load the sector if it is not cached */
			rc = disk_read(secnum + fat_bpb.startsector,1, sectorbuf_n);
			if(rc < 0) {
				fprintf(stderr, "Read of sector %ld failed (rc = %d)\n",secnum, rc);
				return NULL;
				}
			fce_n->secnum = secnum;
			curr_pt++;
			curr_pt=curr_pt%2;
			}
		if(fce->secnum == secnum)
			fat = sectorbuf;
		if(fce_n->secnum == secnum)
			fat = sectorbuf_n;
		if ( !fat )
			break;
		for (j = 0; j < CLUSTERS_PER_FAT_SECTOR; j++) {
			int k = (j + offset) % CLUSTERS_PER_FAT_SECTOR;
			if (!(SWAB32(fat[k]) & 0x0fffffff)) {
				unsigned long c = nr * CLUSTERS_PER_FAT_SECTOR + k;
				/* Ignore the reserved clusters 0 & 1, and also
                        cluster numbers out of bounds */
                    if ( c < 2 || c > fat_bpb.dataclusters+1 )
                        continue;
                    fat_dprintf("startcluster %lu, c %lu\n",startcluster,c);
                    fat_bpb.fsinfo.nextfree = c;
                    return c;
                }
            }
            offset = 0;
        }
	fat_dprintf("startcluster 0\n");
	return 0; /* 0 is an illegal cluster number */
}
static unsigned long find_free_cluster(unsigned long startcluster)
{
    unsigned long sector;
    unsigned long offset;
    unsigned long i;

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb.is_fat16) {
        sector = startcluster / CLUSTERS_PER_FAT16_SECTOR;
        offset = startcluster % CLUSTERS_PER_FAT16_SECTOR;

        for (i = 0; i<fat_bpb.fatsize; i++) {
            unsigned int j;
            unsigned int nr = (i + sector) % fat_bpb.fatsize;
            unsigned short* fat = cache_fat_sector(nr, false);
            if ( !fat )
                break;

            for (j = 0; j < CLUSTERS_PER_FAT16_SECTOR; j++) {
                int k = (j + offset) % CLUSTERS_PER_FAT16_SECTOR;
                if (SWAB16(fat[k]) == 0x0000) {
                    unsigned int c = nr * CLUSTERS_PER_FAT16_SECTOR + k;
                     /* Ignore the reserved clusters 0 & 1, and also
                        cluster numbers out of bounds */
                    if ( c < 2 || c > fat_bpb.dataclusters+1 ) {
                        continue;
					}

                    fat_dprintf("startcluster %lu, c %u\n",startcluster,c);
                    fat_bpb.fsinfo.nextfree = c;
                    return c;
                }
            }
            offset = 0;
        }
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        sector = startcluster / CLUSTERS_PER_FAT_SECTOR;
        offset = startcluster % CLUSTERS_PER_FAT_SECTOR;

        for (i = 0; i<fat_bpb.fatsize; i++) {
            unsigned int j;
            unsigned long nr = (i + sector) % fat_bpb.fatsize;
            unsigned long* fat = cache_fat_sector(nr, false);
            if ( !fat )
                break;

            for (j = 0; j < CLUSTERS_PER_FAT_SECTOR; j++) {
                int k = (j + offset) % CLUSTERS_PER_FAT_SECTOR;
                if (!(SWAB32(fat[k]) & 0x0fffffff)) {
                    unsigned long c = nr * CLUSTERS_PER_FAT_SECTOR + k;
                     /* Ignore the reserved clusters 0 & 1, and also
                        cluster numbers out of bounds */
                    if ( c < 2 || c > fat_bpb.dataclusters+1 )
                        continue;
                    fat_dprintf("startcluster %lu, c %lu\n",startcluster,c);
                    fat_bpb.fsinfo.nextfree = c;
                    return c;
                }
            }
            offset = 0;
        }
    }

    fat_dprintf("startcluster 0\n");
    return 0; /* 0 is an illegal cluster number */
}
//
static int update_fat_entry_write_from_nand(unsigned long entry, unsigned long val)
{
	long sector = entry / CLUSTERS_PER_FAT_SECTOR;
	int offset = entry % CLUSTERS_PER_FAT_SECTOR;
	unsigned long* sec;
	fat_dprintf("entry %lx, val %lx\n", entry,val);
	if (entry == val)
		fat_pprintf("Creating FAT loop: entry = %lx, val = %lx\n",entry,val);
	if (entry < 2)
		fat_pprintf("Updating reserved FAT entry %ld\n", entry);

	long secnum = sector + fat_bpb.bpb_rsvdseccnt;
	struct fat_cache_entry *fce = &fat_cache[curr_pt%2];
	struct fat_cache_entry *fce_n = &fat_cache[(curr_pt+1)%2];
	unsigned char *sectorbuf = &fat_cache_sectors[curr_pt][0];
	unsigned char *sectorbuf_n = &fat_cache_sectors[(curr_pt+1)%2][0];
	int rc;
	/* Delete the cache entry if it isn't the sector we want */
	if((fce->secnum != secnum)&&(fce_n->secnum != secnum))  {
		/* Write back if it is dirty */
		rc = disk_write(fce_n->secnum + fat_bpb.startsector, 1, sectorbuf_n);
		if(rc < 0) {
			fat_pprintf("Could not write sector %ld (rc = %d)\n", secnum, rc);
			}
		/* Load the sector if it is not cached */
		rc = disk_read(secnum + fat_bpb.startsector,1, sectorbuf_n);
		if(rc < 0) {
			fprintf(stderr, "Read of sector %ld failed (rc = %d)\n",secnum, rc);
			return NULL;
			}
		fce_n->secnum = secnum;
		curr_pt++;
		curr_pt=curr_pt%2;
		}
	if(fce->secnum == secnum)
		sec = sectorbuf;
	if(fce_n->secnum == secnum)
		sec = sectorbuf_n;
	if ( val ) {
		if (!(SWAB32(sec[offset]) & 0x0fffffff) && fat_bpb.fsinfo.freecount > 0)
			fat_bpb.fsinfo.freecount--;
        }
        else {
            if (SWAB32(sec[offset]) & 0x0fffffff)
                fat_bpb.fsinfo.freecount++;
        }

        fat_dprintf("%ld free clusters\n", fat_bpb.fsinfo.freecount);

        /* don't change top 4 bits */
        sec[offset] &= SWAB32(0xf0000000);
        sec[offset] |= SWAB32(val & 0x0fffffff);
}
static int update_fat_entry(unsigned long entry, unsigned long val)
{
#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb.is_fat16) {
        int sector = entry / CLUSTERS_PER_FAT16_SECTOR;
        int offset = entry % CLUSTERS_PER_FAT16_SECTOR;
        unsigned short* sec;

        val &= 0xFFFF;

        fat_dprintf("entry %x, val %x\n", entry,val);

        if (entry == val)
			fat_pprintf("Creating FAT loop: entry = %lx, val = %lx\n",
						entry,val);

        if (entry < 2)
			fat_pprintf("Updating reserved FAT entry %ld\n", entry);

        sec = cache_fat_sector(sector, true);
        if (!sec) {
            fat_eprintf("Could not cache sector %d\n", sector);
            return -1;
        }

        if ( val ) {
            if (SWAB16(sec[offset]) == 0x0000 && fat_bpb.fsinfo.freecount > 0)
                fat_bpb.fsinfo.freecount--;
        }
        else {
            if (SWAB16(sec[offset]))
                fat_bpb.fsinfo.freecount++;
        }

        fat_dprintf("%d free clusters\n", fat_bpb.fsinfo.freecount);
        sec[offset] = SWAB16(val);
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        long sector = entry / CLUSTERS_PER_FAT_SECTOR;
        int offset = entry % CLUSTERS_PER_FAT_SECTOR;
        unsigned long* sec;

        fat_dprintf("entry %lx, val %lx\n", entry,val);

        if (entry == val)
			fat_pprintf("Creating FAT loop: entry = %lx, val = %lx\n",
						entry,val);

        if (entry < 2)
			fat_pprintf("Updating reserved FAT entry %ld\n", entry);

        sec = cache_fat_sector(sector, true);
        if (!sec) {
            fat_eprintf("Could not cache sector %ld\n", sector);
            return -1;
        }

        if ( val ) {
            if (!(SWAB32(sec[offset]) & 0x0fffffff) &&
                fat_bpb.fsinfo.freecount > 0)
                fat_bpb.fsinfo.freecount--;
        }
        else {
            if (SWAB32(sec[offset]) & 0x0fffffff)
                fat_bpb.fsinfo.freecount++;
        }

        fat_dprintf("%ld free clusters\n", fat_bpb.fsinfo.freecount);

        /* don't change top 4 bits */
        sec[offset] &= SWAB32(0xf0000000);
        sec[offset] |= SWAB32(val & 0x0fffffff);
    }

    return 0;
}
//
static long read_fat_entry_write_from_nand(unsigned long entry)
{
	long sector = entry / CLUSTERS_PER_FAT_SECTOR;
	int offset = entry % CLUSTERS_PER_FAT_SECTOR;
	unsigned long* sec;
	long secnum = sector + fat_bpb.bpb_rsvdseccnt;
	struct fat_cache_entry *fce = &fat_cache[curr_pt%2];
	struct fat_cache_entry *fce_n = &fat_cache[(curr_pt+1)%2];
	unsigned char *sectorbuf = &fat_cache_sectors[curr_pt][0];
	unsigned char *sectorbuf_n = &fat_cache_sectors[(curr_pt+1)%2][0];
	int rc;
	/* Delete the cache entry if it isn't the sector we want */
	if((fce->secnum != secnum)&&(fce_n->secnum != secnum))  {
		/* Write back if it is dirty */
		rc = disk_write(fce_n->secnum + fat_bpb.startsector, 1, sectorbuf_n);
		if(rc < 0) {
			fat_pprintf("Could not write sector %ld (rc = %d)\n", secnum, rc);
			}
		/* Load the sector if it is not cached */
		rc = disk_read(secnum + fat_bpb.startsector,1, sectorbuf_n);
		if(rc < 0) {
			fprintf(stderr, "Read of sector %ld failed (rc = %d)\n",secnum, rc);
			return NULL;
			}
		fce_n->secnum = secnum;
		curr_pt++;
		curr_pt=curr_pt%2;
		}
	if(fce->secnum == secnum)
		sec = sectorbuf;
	if(fce_n->secnum == secnum)
		sec = sectorbuf_n;
	if (!sec) {
		fat_eprintf("Could not cache sector %ld\n", sector);
		return -1;
		}
	return SWAB32(sec[offset]) & 0x0fffffff;
}
static long read_fat_entry(unsigned long entry)
{
#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb.is_fat16) {
        int sector = entry / CLUSTERS_PER_FAT16_SECTOR;
        int offset = entry % CLUSTERS_PER_FAT16_SECTOR;
        unsigned short* sec;

        sec = cache_fat_sector(sector, false);
        if (!sec) {
            fat_eprintf("Could not cache sector %d\n", sector);
            return -1;
        }

        return SWAB16(sec[offset]);
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        long sector = entry / CLUSTERS_PER_FAT_SECTOR;
        int offset = entry % CLUSTERS_PER_FAT_SECTOR;
        unsigned long* sec;

        sec = cache_fat_sector(sector, false);
        if (!sec) {
            fat_eprintf("Could not cache sector %ld\n", sector);
            return -1;
        }

        return SWAB32(sec[offset]) & 0x0fffffff;
    }
}
//
static long get_next_cluster_write_from_nand(long cluster)
{
	long next_cluster;
	long eof_mark = FAT_EOF_MARK;
	next_cluster = read_fat_entry_write_from_nand(cluster);
	/* is this last cluster in chain? */
	if ( next_cluster >= eof_mark )
		return 0;
	else
		return next_cluster;
}
static long get_next_cluster(long cluster)
{
    long next_cluster;
    long eof_mark = FAT_EOF_MARK;

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb.is_fat16) {
        eof_mark &= 0xFFFF; /* only 16 bit */
        if (cluster < 0) /* FAT16 root dir */
            return cluster + 1; /* don't use the FAT */
    }
#endif
    next_cluster = read_fat_entry(cluster);

    /* is this last cluster in chain? */
    if ( next_cluster >= eof_mark )
        return 0;
    else
        return next_cluster;
}

static int update_fsinfo(void)
{
    static unsigned char fsinfo[SECTOR_SIZE];
    unsigned long* intptr;
    int rc;

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb.is_fat16)
        return 0; /* FAT16 has no FsInfo */
#endif /* #ifdef HAVE_FAT16SUPPORT */

    /* update fsinfo */
    rc = disk_read(fat_bpb.startsector + fat_bpb.bpb_fsinfo, 1,fsinfo);
    if (rc < 0) {
        fat_eprintf("FSInfo read failed (rc = %d)\n", rc);
        return rc * 10 - 1;
    }
    intptr = (long*)&(fsinfo[FSINFO_FREECOUNT]);
    *intptr = SWAB32(fat_bpb.fsinfo.freecount);

    intptr = (long*)&(fsinfo[FSINFO_NEXTFREE]);
    *intptr = SWAB32(fat_bpb.fsinfo.nextfree);

    rc = disk_write(fat_bpb.startsector + fat_bpb.bpb_fsinfo,1,fsinfo);
    if (rc < 0) {
        fat_eprintf("FSInfo write failed (rc = %d)\n", rc);
        return rc * 10 - 2;
    }

    return 0;
}

static int flush_fat(void)
{
    int i;
    int rc;
    unsigned char *sec;
    //fat_dprintf("\n");

    for(i = 0;i < FAT_CACHE_SIZE;i++) {
        struct fat_cache_entry *fce = &fat_cache[i];
        if(fce->inuse && fce->dirty) {
            sec = fat_cache_sectors[i];
            flush_fat_sector(fce, sec);
        }
    }

    rc = update_fsinfo();
    if (rc < 0)
        return rc * 10 - 3;

    return 0;
}

static void fat_time(unsigned short* date,
                     unsigned short* time,
                     unsigned short* tenth )
{
#ifdef HAVE_RTC
    struct tm* tm = get_time();

    if (date)
        *date = ((tm->tm_year - 80) << 9) |
            ((tm->tm_mon + 1) << 5) | tm->tm_mday;

    if (time)
        *time = (tm->tm_hour << 11) | (tm->tm_min << 5) | (tm->tm_sec >> 1);

    if (tenth)
        *tenth = (tm->tm_sec & 1) * 100;
#else
    /* non-RTC version returns an increment from the supplied time, or a
     * fixed standard time/date if no time given as input */
    bool next_day = false;

    if (time) {
        if (0 == *time) {
            /* set to 00:15:00 */
            *time = (15 << 5);
        }
        else {
            unsigned short mins = (*time >> 5) & 0x003F;
            unsigned short hours = (*time >> 11) & 0x001F;
            if ((mins += 10) >= 60) {
                mins = 0;
                hours++;
            }
            if ((++hours) >= 24) {
                hours = hours - 24;
                next_day = true;
            }
            *time = (hours << 11) | (mins << 5);
        }
    }

    if (date) {
        if (0 == *date) {
/* Macros to convert a 2-digit string to a decimal constant.
   (YEAR), MONTH and DAY are set by the date command, which outputs
   DAY as 00..31 and MONTH as 01..12. The leading zero would lead to
   misinterpretation as an octal constant. */

/* FIXME: Tie into U-Boot's date stuff */
#define	YEAR	2006
#define	MONTH	01
#define	DAY		19

#define S100(x) 1 ## x
#define C2DIG2DEC(x) (S100(x)-100)
            /* set to build date */
            *date = ((YEAR - 1980) << 9) | (C2DIG2DEC(MONTH) << 5)
                  | C2DIG2DEC(DAY);
        }
        else {
            unsigned short day = *date & 0x001F;
            unsigned short month = (*date >> 5) & 0x000F;
            unsigned short year = (*date >> 9) & 0x007F;
            if (next_day) {
                /* do a very simple day increment - never go above 28 days */
                if (++day > 28) {
                    day = 1;
                    if (++month > 12) {
                        month = 1;
                        year++;
                    }
                }
                *date = (year << 9) | (month << 5) | day;
            }
        }
    }
    if (tenth)
        *tenth = 0;
#endif /* HAVE_RTC */
}

static int write_long_name(struct fat_file* file,
                           unsigned int firstentry,
                           unsigned int numentries,
                           const unsigned char* name,
                           const unsigned char* shortname,
                           bool is_directory)
{
    static unsigned char buf[SECTOR_SIZE];
    unsigned char* entry;
    unsigned int idx = firstentry % DIR_ENTRIES_PER_SECTOR;
    unsigned int sector = firstentry / DIR_ENTRIES_PER_SECTOR;
    unsigned int i, j=0;
    unsigned char chksum = 0;
    int nameidx=0, namelen = strlen(name);
    int rc;

    fat_dprintf("file %ld, first %d, num %d, name \"%s\"\n",
				file->firstcluster, firstentry, numentries, name);

    rc = fat_seek(file, sector);
    if (rc<0)
        return rc * 10 - 1;

    rc = fat_readwrite(file, 1, buf, false);
    if (rc<1)
        return rc * 10 - 2;

    /* calculate shortname checksum */
    for (i=11; i>0; i--)
        chksum = ((chksum & 1) ? 0x80 : 0) + (chksum >> 1) + shortname[j++];

    /* calc position of last name segment */
    if ( namelen > NAME_BYTES_PER_ENTRY )
        for (nameidx=0;
             nameidx < (namelen - NAME_BYTES_PER_ENTRY);
             nameidx += NAME_BYTES_PER_ENTRY);

    for (i=0; i < numentries; i++) {
        /* new sector? */
        if ( idx >= DIR_ENTRIES_PER_SECTOR ) {
            /* update current sector */
            rc = fat_seek(file, sector);
            if (rc<0)
                return rc * 10 - 3;

            rc = fat_readwrite(file, 1, buf, true);
            if (rc<1)
                return rc * 10 - 4;

            /* read next sector */
            rc = fat_readwrite(file, 1, buf, false);
            if (rc<0) {
				fat_eprintf("Failed to write new sector (rc = %d)\n", rc);
                return rc * 10 - 5;
            }
            if (rc==0)
                /* end of dir */
                memset(buf, 0, sizeof buf);

            sector++;
            idx = 0;
        }

        entry = buf + idx * DIR_ENTRY_SIZE;

        /* verify this entry is free */
        if (entry[0] && entry[0] != 0xe5 )
			/* FIXME: Should we halt instead of plowing on? */
            fat_pprintf("Dir entry %d in sector %x is not free! "
                   "%02x %02x %02x %02x",
                   idx, sector, entry[0], entry[1], entry[2], entry[3]);

        memset(entry, 0, DIR_ENTRY_SIZE);
        if ( i+1 < numentries ) {
            /* longname entry */
            int k, l = nameidx;

            entry[FATLONG_ORDER] = numentries-i-1;
            if (i==0) {
                /* mark this as last long entry */
                entry[FATLONG_ORDER] |= 0x40;

                /* pad name with 0xffff  */
                for (k=1; k<12; k++) entry[k] = 0xff;
                for (k=14; k<26; k++) entry[k] = 0xff;
                for (k=28; k<32; k++) entry[k] = 0xff;
            };

            /* set name */
            for (k=0; k<5 && l <= namelen; k++) {
                entry[k*2 + 1] = name[l++];
                entry[k*2 + 2] = 0;
            }

            for (k=0; k<6 && l <= namelen; k++) {
                entry[k*2 + 14] = name[l++];
                entry[k*2 + 15] = 0;
            }

            for (k=0; k<2 && l <= namelen; k++) {
                entry[k*2 + 28] = name[l++];
                entry[k*2 + 29] = 0;
            }

            entry[FATDIR_ATTR] = FAT_ATTR_LONG_NAME;
            entry[FATDIR_FSTCLUSLO] = 0;
            entry[FATLONG_TYPE] = 0;
            entry[FATLONG_CHKSUM] = chksum;
            fat_dprintf("longname entry %d = \"%s\"\n", idx, name+nameidx);
        }
        else {
            /* shortname entry */
            unsigned short date=0, time=0, tenth=0;
            fat_dprintf("shortname entry = \"%s\"\n", shortname);
            strncpy(entry + FATDIR_NAME, shortname, 11);
            entry[FATDIR_ATTR] = is_directory?FAT_ATTR_DIRECTORY:0;
            entry[FATDIR_NTRES] = 0;

            fat_time(&date, &time, &tenth);
            entry[FATDIR_CRTTIMETENTH] = tenth;
            *(unsigned short*)(entry + FATDIR_CRTTIME) = SWAB16(time);
            *(unsigned short*)(entry + FATDIR_WRTTIME) = SWAB16(time);
            *(unsigned short*)(entry + FATDIR_CRTDATE) = SWAB16(date);
            *(unsigned short*)(entry + FATDIR_WRTDATE) = SWAB16(date);
            *(unsigned short*)(entry + FATDIR_LSTACCDATE) = SWAB16(date);
        }
        idx++;
        nameidx -= NAME_BYTES_PER_ENTRY;
    }

    /* update last sector */
    rc = fat_seek(file, sector);
    if (rc<0)
        return rc * 10 - 6;

    rc = fat_readwrite(file, 1, buf, true);
    if (rc<1)
        return rc * 10 - 7;

    return 0;
}

static int fat_checkname(const unsigned char* newname)
{
    /* More sanity checks are probably needed */
    if ( newname[strlen(newname) - 1] == '.' ) {
        return -1;
    }

    return 0;
}

static int add_dir_entry(struct fat_dir* dir,
                         struct fat_file* file,
                         const char* name,
                         bool is_directory,
                         bool dotdir)
{
    static unsigned char buf[SECTOR_SIZE];
    unsigned char shortname[12];
    int rc;
    unsigned int sector;
    bool done = false;
    int entries_needed, entries_found = 0;
    int firstentry;

    fat_dprintf("name \"%s\", firstcluster %ld\n",
				name, file->firstcluster);
memset(buf,0,SECTOR_SIZE);
    /* Don't check dotdirs name for validity */
    if (dotdir == false) {
        rc = fat_checkname(name);
        if (rc < 0) {
            /* filename is invalid */
            return rc * 10 - 1;
        }
    }

    /* The "." and ".." directory entries must not be long names */
    if(dotdir) {
        int i;
        strncpy(shortname, name, 12);
        for(i = strlen(shortname); i < 12; i++)
            shortname[i] = ' ';

        entries_needed = 1;
    } else {
        create_dos_name(name, shortname);

        /* one dir entry needed for every 13 bytes of filename,
           plus one entry for the short name */
        entries_needed = (strlen(name) + (NAME_BYTES_PER_ENTRY-1))
                         / NAME_BYTES_PER_ENTRY + 1;
    }

  restart:
    firstentry = -1;

    rc = fat_seek(&dir->file, 0);
    if (rc < 0)
        return rc * 10 - 2;

    /* step 1: search for free entries and check for duplicate shortname */
    for (sector = 0; !done; sector++) {
        unsigned int i;
fat_dprintf("for (sector = 0; !done; sector++) \n");
        rc = fat_readwrite(&dir->file, 1, buf, false);
        if (rc < 0) {
            fat_eprintf("Could not read dir (rc = %d)\n", rc);
            return rc * 10 - 3;
        }

        if (rc == 0) { /* current end of dir reached */
            fat_dprintf("End of dir on cluster boundary\n");
            break;
        }
	fat_dprintf("for (sector = 0; !done; sector++) \n");
        /* look for free slots */
        for (i = 0; i < DIR_ENTRIES_PER_SECTOR; i++) {
			fat_dprintf("DIR name:0x%x \n",buf[i * DIR_ENTRY_SIZE]);
            switch (buf[i * DIR_ENTRY_SIZE]) {
              case 0:
                entries_found += DIR_ENTRIES_PER_SECTOR - i;
                fat_dprintf("Found end of dir %d\n",
							sector * DIR_ENTRIES_PER_SECTOR + i);
                i = DIR_ENTRIES_PER_SECTOR - 1;
                done = true;
                break;

              case 0xe5:
                entries_found++;
                fat_dprintf("Found free entry %d (%d/%d)\n",
							sector * DIR_ENTRIES_PER_SECTOR + i,
							entries_found, entries_needed);
                break;

              default:
                entries_found = 0;

                /* check that our intended shortname doesn't already exist */
                if (!strncmp(shortname, buf + i * DIR_ENTRY_SIZE, 12)) {
                    /* shortname exists already, make a new one */
                    randomize_dos_name(shortname);
                    fat_dprintf("Duplicate shortname, changing to \"%s\"\n",
								shortname);

                    /* name has changed, we need to restart search */
                    goto restart;
                }
                break;
            }
            if (firstentry < 0 && (entries_found >= entries_needed))
                firstentry = sector * DIR_ENTRIES_PER_SECTOR + i + 1
                             - entries_found;
        }
    }

    /* step 2: extend the dir if necessary */
    if (firstentry < 0) {
        fat_dprintf("Adding new sector(s) to dir\n");
        rc = fat_seek(&dir->file, sector);
        if (rc < 0)
            return rc * 10 - 4;
        memset(buf, 0, sizeof buf);

        /* we must clear whole clusters */
        for (; (entries_found < entries_needed) ||
               (dir->file.sectornum < (int)fat_bpb.bpb_secperclus); sector++) {
            if (sector >= (65536/DIR_ENTRIES_PER_SECTOR))
                return -5; /* dir too large -- FAT specification */

            rc = fat_readwrite(&dir->file, 1, buf, true);
            if (rc < 1)  /* No more room or something went wrong */
		{
		fat_dprintf("fat_readwrite rc:%d\n",rc);
                return rc * 10 - 6;
		}

            entries_found += DIR_ENTRIES_PER_SECTOR;
        }

        firstentry = sector * DIR_ENTRIES_PER_SECTOR - entries_found;
    }

    /* step 3: add entry */
    sector = firstentry / DIR_ENTRIES_PER_SECTOR;
    fat_dprintf("Adding longname to entry %d in sector %d\n",
				firstentry, sector);

    rc = write_long_name(&dir->file, firstentry,
                         entries_needed, name, shortname, is_directory);
    if (rc < 0)
        return rc * 10 - 7;

    /* remember where the shortname dir entry is located */
    file->direntry = firstentry + entries_needed - 1;
    file->direntries = entries_needed;
    file->dircluster = dir->file.firstcluster;
    fat_dprintf("Added new dir entry %d, using %d slots\n",
				file->direntry, file->direntries);
    return 0;
}

unsigned char char2dos(unsigned char c)
{
    switch(c) {
        case 0x22:
        case 0x2a:
        case 0x2b:
        case 0x2c:
        case 0x2e:
        case 0x3a:
        case 0x3b:
        case 0x3c:
        case 0x3d:
        case 0x3e:
        case 0x3f:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x7c:
            /* Illegal char, replace */
            c = '_';
            break;

        default:
            if(c <= 0x20)
                c = 0;   /* Illegal char, remove */
            else
                c = __toupper(c);
            break;
    }

    return c;
}

static void create_dos_name(const unsigned char *name, unsigned char *newname)
{
    int i;
    unsigned char *ext;

    /* Find extension part */
    ext = strrchr(name, '.');
    if (ext == name)         /* handle .dotnames */
        ext = NULL;

    /* Name part */
    for (i = 0; *name && (!ext || name < ext) && (i < 8); name++) {
        unsigned char c = char2dos(*name);
        if (c)
            newname[i++] = c;
    }

    /* Pad both name and extension */
    while (i < 11)
        newname[i++] = ' ';

    if (newname[0] == 0xe5) /* Special kanji character */
        newname[0] = 0x05;

    if (ext) {   /* Extension part */
        ext++;
        for (i = 8; *ext && (i < 11); ext++) {
            unsigned char c = char2dos(*ext);
            if (c)
                newname[i++] = c;
        }
    }
}

/*
 * This function is hacked to always start with a fixed number and keep
 * incrementing it until the caller is satisfied with the name.
 * FIXME: add a real randomizer?
 */
static void randomize_dos_name(unsigned char *name)
{
	static unsigned int random = 0;
    int i;
    unsigned char buf[5];

    /* snprintf(buf, sizeof buf, "%04X", (unsigned)rand() & 0xffff); */
    sprintf(buf, "%04X", (random++ & 0xffff));

    for (i = 0; (i < 4) && (name[i] != ' '); i++);
    /* account for possible shortname length < 4 */
    memcpy(&name[i], buf, 4);
}

static int update_short_entry( struct fat_file* file, long size, int attr )
{
    unsigned char buf[SECTOR_SIZE];
    int sector = file->direntry / DIR_ENTRIES_PER_SECTOR;
    unsigned char* entry =
        buf + DIR_ENTRY_SIZE * (file->direntry % DIR_ENTRIES_PER_SECTOR);
    unsigned long* sizeptr;
    unsigned short* clusptr;
    struct fat_file dir;
    int rc;

    fat_dprintf("cluster %lu, entry %d, size %ld\n",
				file->firstcluster, file->direntry, size);

    /* create a temporary file handle for the dir holding this file */
    rc = fat_open(file->dircluster, &dir, NULL);
    if (rc < 0)
        return rc * 10 - 1;

    rc = fat_seek( &dir, sector );
    if (rc<0)
        return rc * 10 - 2;

    rc = fat_readwrite(&dir, 1, buf, false);
    if (rc < 1)
        return rc * 10 - 3;

    if (!entry[0] || entry[0] == 0xe5)
        fat_pprintf("Updating size on empty dir entry %d\n", file->direntry);

    entry[FATDIR_ATTR] = attr & 0xFF;

    clusptr = (short*)(entry + FATDIR_FSTCLUSHI);
    *clusptr = SWAB16(file->firstcluster >> 16);

    clusptr = (short*)(entry + FATDIR_FSTCLUSLO);
    *clusptr = SWAB16(file->firstcluster & 0xffff);

    sizeptr = (long*)(entry + FATDIR_FILESIZE);
    *sizeptr = SWAB32(size);

    {
#ifdef HAVE_RTC
		unsigned short time = 0;
        unsigned short date = 0;
#else
        /* get old time to increment from */
        unsigned short time = SWAB16(*(unsigned short*)(entry +FATDIR_WRTTIME));
        unsigned short date = SWAB16(*(unsigned short*)(entry +FATDIR_WRTDATE));
#endif
        fat_time(&date, &time, NULL);
        *(unsigned short*)(entry + FATDIR_WRTTIME) = SWAB16(time);
        *(unsigned short*)(entry + FATDIR_WRTDATE) = SWAB16(date);
        *(unsigned short*)(entry + FATDIR_LSTACCDATE) = SWAB16(date);
    }

    rc = fat_seek( &dir, sector );
    if (rc < 0)
        return rc * 10 - 4;

    rc = fat_readwrite(&dir, 1, buf, true);
    if (rc < 1)
        return rc * 10 - 5;

    return 0;
}

static int parse_direntry(struct fat_direntry *de, const unsigned char *buf)
{
    int i=0,j=0;
    unsigned char c;
    bool lowercase;

    memset(de, 0, sizeof(struct fat_direntry));
    de->attr = buf[FATDIR_ATTR];
    de->crttimetenth = buf[FATDIR_CRTTIMETENTH];
    de->crtdate = BYTES2INT16(buf,FATDIR_CRTDATE);
    de->crttime = BYTES2INT16(buf,FATDIR_CRTTIME);
    de->wrtdate = BYTES2INT16(buf,FATDIR_WRTDATE);
    de->wrttime = BYTES2INT16(buf,FATDIR_WRTTIME);
    de->filesize = BYTES2INT32(buf,FATDIR_FILESIZE);
    de->firstcluster = ((long)(unsigned)BYTES2INT16(buf,FATDIR_FSTCLUSLO)) |
        ((long)(unsigned)BYTES2INT16(buf,FATDIR_FSTCLUSHI) << 16);
    /* The double cast is to prevent a sign-extension to be done on CalmRISC16.
       (the result of the shift is always considered signed) */

    /* fix the name */
    lowercase = (buf[FATDIR_NTRES] & FAT_NTRES_LC_NAME);
    c = buf[FATDIR_NAME];
    if (c == 0x05)  /* special kanji char */
        c = 0xe5;
    i = 0;
    while (c != ' ') {
        de->name[j++] = lowercase ? __tolower(c) : c;
        if (++i >= 8)
            break;
        c = buf[FATDIR_NAME+i];
    }

    if (buf[FATDIR_NAME+8] != ' ') {
        lowercase = (buf[FATDIR_NTRES] & FAT_NTRES_LC_EXT);
        de->name[j++] = '.';
        for (i = 8; (i < 11) && ((c = buf[FATDIR_NAME+i]) != ' '); i++)
            de->name[j++] = lowercase ? __tolower(c) : c;
    }
    return 1;
}

int fat_open(long startcluster,
             struct fat_file *file,
             const struct fat_dir* dir)
{
    file->firstcluster = startcluster;
    file->lastcluster = startcluster;
    file->lastsector = 0;
    file->clusternum = 0;
    file->sectornum = 0;
    file->eof = false;

    /* remember where the file's dir entry is located */
    if ( dir ) {
        file->direntry = dir->entry - 1;
        file->direntries = dir->entrycount;
        file->dircluster = dir->file.firstcluster;
    }

    fat_dprintf("startcluster %ld, direntry %d\n",startcluster,file->direntry);
    return 0;
}

int fat_create_file(const char* name,
                    struct fat_file* file,
                    struct fat_dir* dir)
{
    int rc;

    fat_dprintf("name \"%s\", file ptr %lx, dir ptr %lx\n",
				name,(long)file,(long)dir);
    rc = add_dir_entry(dir, file, name, false, false);
    if (!rc) {
        file->firstcluster = 0;
        file->lastcluster = 0;
        file->lastsector = 0;
        file->clusternum = 0;
        file->sectornum = 0;
        file->eof = false;
    }

    return rc;
}

int fat_create_dir(const char* name,
                   struct fat_dir* newdir,
                   struct fat_dir* dir)
{
    static unsigned char buf[SECTOR_SIZE];
    int i;
    long sector;
    int rc;
    struct fat_file dummyfile;

    fat_dprintf("name \"%s\", newdir ptr %lx, dir ptr %lx\n",
				name,(long)newdir,(long)dir);

    memset(newdir, 0, sizeof(struct fat_dir));
    memset(&dummyfile, 0, sizeof(struct fat_file));

    /* First, add the entry in the parent directory */
    rc = add_dir_entry(dir, &newdir->file, name, true, false);
    if (rc < 0)
        return rc * 10 - 1;

    /* Allocate a new cluster for the directory */
    newdir->file.firstcluster = find_free_cluster(fat_bpb.fsinfo.nextfree);
    if(newdir->file.firstcluster == 0)
        return -1;

    update_fat_entry(newdir->file.firstcluster, FAT_EOF_MARK);

    /* Clear the entire cluster */
    memset(buf, 0, sizeof buf);
    sector = cluster2sec(newdir->file.firstcluster);
    for(i = 0;i < (int)fat_bpb.bpb_secperclus;i++) {
        rc = transfer(sector + i, 1, buf, true );
        if (rc < 0)
            return rc * 10 - 2;
    }

    /* Then add the "." entry */
    rc = add_dir_entry(newdir, &dummyfile, ".", true, true);
    if (rc < 0)
        return rc * 10 - 3;
    dummyfile.firstcluster = newdir->file.firstcluster;
    update_short_entry(&dummyfile, 0, FAT_ATTR_DIRECTORY);

    /* and the ".." entry */
    rc = add_dir_entry(newdir, &dummyfile, "..", true, true);
    if (rc < 0)
        return rc * 10 - 4;

    /* The root cluster is cluster 0 in the ".." entry */
    if(dir->file.firstcluster == fat_bpb.bpb_rootclus)
        dummyfile.firstcluster = 0;
    else
        dummyfile.firstcluster = dir->file.firstcluster;
    update_short_entry(&dummyfile, 0, FAT_ATTR_DIRECTORY);

    /* Set the firstcluster field in the direntry */
    update_short_entry(&newdir->file, 0, FAT_ATTR_DIRECTORY);

    rc = flush_fat();
    if (rc < 0)
        return rc * 10 - 5;

    return rc;
}

int fat_truncate(const struct fat_file *file)
{
    /* truncate trailing clusters */
    long next;
    long last = file->lastcluster;

    fat_dprintf("firstcluster %ld, lastcluster %ld\n",
				file->firstcluster, last);

    for ( last = get_next_cluster(last); last; last = next ) {
        next = get_next_cluster(last);
        update_fat_entry(last,0);
    }

    if (file->lastcluster)
        update_fat_entry(file->lastcluster,FAT_EOF_MARK);

    return 0;
}
//
int fat_closewrite_form_nand(struct fat_file *file, long size, int attr)
{
	int rc;
	long secnum;
	unsigned char *sec;
	struct fat_cache_entry *fce = &fat_cache[1];
	struct fat_cache_entry *fce_n = &fat_cache[0];
	fat_dprintf("size %ld\n", size);
	//printf("fat_closewrite:size %ld  file->firstcluster:0x%x lastcluster0x%x\n", size,file->firstcluster,file->lastcluster);
	if (!size) {
		/* empty file */
		if ( file->firstcluster ) {
			update_fat_entry_write_from_nand(file->firstcluster, 0);
			file->firstcluster = 0;
			}
		}
	if (file->dircluster) {
		rc = update_short_entry(file, size, attr);
		if (rc < 0)
			return rc * 10 - 1;
		}
	sec = fat_cache_sectors[0];
	secnum=fce_n->secnum + fat_bpb.startsector;;
	//printf("fat_closewrite_form_nand:secnum 0x%x\n",secnum);
	rc = disk_write(secnum, 1, sec);

	sec = fat_cache_sectors[1];
	secnum=fce->secnum + fat_bpb.startsector;;
	//printf("fat_closewrite_form_nand:secnum 0x%x\n",secnum);
	rc = disk_write(secnum, 1, sec);
	if (rc < 0)
		return rc * 10 - 1;
	return 0;
}
int fat_closewrite(struct fat_file *file, long size, int attr)
{
    int rc;

    fat_dprintf("size %ld\n", size);
	printf("fat_closewrite:size %ld  file->firstcluster:0x%x lastcluster0x%x\n", size,file->firstcluster,file->lastcluster);
    if (!size) {
        /* empty file */
        if ( file->firstcluster ) {
            update_fat_entry(file->firstcluster, 0);
            file->firstcluster = 0;
        }
    }

    if (file->dircluster) {
        rc = update_short_entry(file, size, attr);
        if (rc < 0)
            return rc * 10 - 1;
    }

    flush_fat();

#ifdef TEST_FAT
    if ( file->firstcluster ) {
        /* debug */
        long count = 0;
        long len;
        long next;
        for ( next = file->firstcluster; next;
              next = get_next_cluster(next) ) {
            fat_dprintf("cluster %ld, next %ld\n", count, next);
            count++;
        }

        len = count * fat_bpb.bpb_secperclus * SECTOR_SIZE;
        fat_dprintf("File is %ld clusters (chainlen %ld, size %ld)\n",
					count, len, size );
        if ( len > size + fat_bpb.bpb_secperclus * SECTOR_SIZE)
            fat_pprintf("Cluster chain is too long\n");

        if ( len < size )
            fat_pprintf("Cluster chain is too short\n");
    }
#endif

    return 0;
}

static int free_direntries(struct fat_file* file)
{
    static unsigned char buf[SECTOR_SIZE];
    struct fat_file dir;
    int numentries = file->direntries;
    unsigned int entry = file->direntry - numentries + 1;
    unsigned int sector = entry / DIR_ENTRIES_PER_SECTOR;
    int i;
    int rc;

    /* create a temporary file handle for the dir holding this file */
    rc = fat_open(file->dircluster, &dir, NULL);
    if (rc < 0)
        return rc * 10 - 1;

    rc = fat_seek( &dir, sector );
    if (rc < 0)
        return rc * 10 - 2;

    rc = fat_readwrite(&dir, 1, buf, false);
    if (rc < 1)
        return rc * 10 - 3;

    for (i=0; i < numentries; i++) {
        fat_dprintf("Clearing dir entry %d (%d/%d)\n",
					entry, i+1, numentries);
        buf[(entry % DIR_ENTRIES_PER_SECTOR) * DIR_ENTRY_SIZE] = 0xe5;
        entry++;

        if ( (entry % DIR_ENTRIES_PER_SECTOR) == 0 ) {
            /* flush this sector */
            rc = fat_seek(&dir, sector);
            if (rc < 0)
                return rc * 10 - 4;

            rc = fat_readwrite(&dir, 1, buf, true);
            if (rc < 1)
                return rc * 10 - 5;

            if ( i+1 < numentries ) {
                /* read next sector */
                rc = fat_readwrite(&dir, 1, buf, false);
                if (rc < 1)
                    return rc * 10 - 6;
            }
            sector++;
        }
    }

    if ( entry % DIR_ENTRIES_PER_SECTOR ) {
        /* flush this sector */
        rc = fat_seek(&dir, sector);
        if (rc < 0)
            return rc * 10 - 7;

        rc = fat_readwrite(&dir, 1, buf, true);
        if (rc < 1)
            return rc * 10 - 8;
    }

    return 0;
}

int fat_remove(struct fat_file* file)
{
    long next, last = file->firstcluster;
    int rc;

    fat_dprintf("last %ld\n", last);

    while ( last ) {
        next = get_next_cluster(last);
        update_fat_entry(last,0);
        last = next;
    }

    if ( file->dircluster ) {
        rc = free_direntries(file);
        if (rc < 0)
            return rc * 10 - 1;
    }

    file->firstcluster = 0;
    file->dircluster = 0;

    rc = flush_fat();
    if (rc < 0)
        return rc * 10 - 2;

    return 0;
}

int fat_rename(struct fat_file* file,
                struct fat_dir* dir,
                const unsigned char* newname,
                long size,
                int attr)
{
    int rc;
    struct fat_dir olddir;
    struct fat_file newfile = *file;

    if ( !file->dircluster ) {
        fat_eprintf("File has no dir cluster!\n");
        return -2;
    }

    /* create a temporary file handle */
    rc = fat_opendir(&olddir, file->dircluster, NULL);
    if (rc < 0)
        return rc * 10 - 3;

    /* create new name */
    rc = add_dir_entry(dir, &newfile, newname, false, false);
    if (rc < 0)
        return rc * 10 - 4;

    /* write size and cluster link */
    rc = update_short_entry(&newfile, size, attr);
    if (rc < 0)
        return rc * 10 - 5;

    /* remove old name */
    rc = free_direntries(file);
    if (rc < 0)
        return rc * 10 - 6;

    rc = flush_fat();
    if (rc < 0)
        return rc * 10 - 7;

    return 0;
}
//
static long next_write_cluster_from_nand(struct fat_file* file,long oldcluster,long* newsector)
{
	long cluster = 0;
	long sector;
	/*
	* Enable this for lots of verbosity.
	*/
	if (oldcluster)
		cluster = get_next_cluster_write_from_nand(oldcluster);

	if (!cluster) {
		if (oldcluster > 0)
			cluster = find_free_cluster_write_from_nand(oldcluster+1);
		else if (oldcluster == 0)
			cluster = find_free_cluster_write_from_nand(fat_bpb.fsinfo.nextfree);

		if (cluster) {
			if (oldcluster)
				update_fat_entry_write_from_nand(oldcluster, cluster);
			else
				file->firstcluster = cluster;
			update_fat_entry_write_from_nand(cluster, FAT_EOF_MARK);
			}
		else {
			fat_eprintf("Disk is full!\n");
			return 0;
			}
		}
	sector = cluster2sec(cluster);
	if (sector<0)
		return 0;
	*newsector = sector;
	//printf("next_write_cluster_from_nand:cluster:0x%x sector:0x%x\n",cluster,sector);

	return cluster;
}

static long next_write_cluster(struct fat_file* file,
                              long oldcluster,
                              long* newsector)
{
    long cluster = 0;
    long sector;

/*
 * Enable this for lots of verbosity.
 */
#if 0
    fat_dprintf("firstcluster %ld, lastcluster %ld\n",
				file->firstcluster, oldcluster);
#endif

    if (oldcluster)
        cluster = get_next_cluster(oldcluster);

    if (!cluster) {
        if (oldcluster > 0)
            cluster = find_free_cluster(oldcluster+1);
        else if (oldcluster == 0)
            cluster = find_free_cluster(fat_bpb.fsinfo.nextfree);
#ifdef HAVE_FAT16SUPPORT
        else /* negative, pseudo-cluster of the root dir */
            return 0; /* impossible to append something to the root */
#endif

        if (cluster) {
            if (oldcluster)
                update_fat_entry(oldcluster, cluster);
            else
                file->firstcluster = cluster;
            update_fat_entry(cluster, FAT_EOF_MARK);
        }
        else {
#ifdef TEST_FAT
            if (fat_bpb.fsinfo.freecount>0)
				fat_pprintf("There is free space, but "
							"find_free_cluster() didn't find it!\n");
#endif
            fat_eprintf("Disk is full!\n");
            return 0;
        }
    }
    sector = cluster2sec(cluster);
    if (sector<0)
        return 0;

    *newsector = sector;
    return cluster;
}

static int transfer(unsigned long start, long count, char* buf, bool write)
{
    int rc;

    fat_dprintf("%s, start sector %lu, count %ld buf 0x%x:\n",
				write ? "Write":"Read", start + fat_bpb.startsector, count,buf[0]);
    if (write) {
        unsigned long firstallowed;
#ifdef HAVE_FAT16SUPPORT
        if (fat_bpb.is_fat16)
            firstallowed = fat_bpb.rootdirsector;
        else
#endif
            firstallowed = fat_bpb.firstdatasector;

        if (start < firstallowed) {
			fat_pprintf("Write %ld before data\n", firstallowed - start);
			return -1;
		}

        if (start + count > fat_bpb.totalsectors) {
			fat_pprintf("Write %ld after data\n",
						 start + count - fat_bpb.totalsectors);
			return -1;
		}

        rc = disk_write(start + fat_bpb.startsector, count, buf);
    }
    else {
        rc = disk_read(start + fat_bpb.startsector, count, buf);
	}

    if (rc < 0) {
        fat_eprintf("%s failed: start sector %lu, count %ld (rc = %d)\n",
					write ? "Write":"Read", start, count, rc);
        return rc;
    }



    return 0;
}
//
long fat_write_from_nand(struct fat_file *file,void* buf)
{
	long cluster = file->lastcluster;
	long sector = file->lastsector;
	long clusternum = file->clusternum;
	long numsec = file->sectornum;
	bool eof = file->eof;
	int rc;

	numsec++;
	if ( numsec > (long)fat_bpb.bpb_secperclus || !cluster ) {
		long oldcluster = cluster;
		cluster = next_write_cluster_from_nand(file, cluster, &sector);
		clusternum++;
		numsec=1;
		if (!cluster) {
			/* remember last cluster, in case
                       we want to append to the file */
			cluster = oldcluster;
			clusternum--;
			}
		else
			eof = false;
		}
	else {
		if (sector) {
			sector++;
			}
		else {
			/* look up first sector of file */
			sector = cluster2sec(file->firstcluster);
			numsec=1;
			}
		}
		/*
		 * The original rockbox FAT code had a hard-coded limit of 256 sectors
		 * max per transfer.  We allow more than that now, but BE SURE that your
		 * disk driver is able to deal with a transfer request of more than 256
		 * sectors if you do not use the default!
		 */
	rc = transfer(sector, 1, buf, true );
	if (rc < 0)
		return rc * 10 - 1;
	//printf("fat_write_from_nand:cluster:0x%x numsec:0x%x\n",cluster,numsec);
	file->lastcluster = cluster;
	file->lastsector = sector;
	file->clusternum = clusternum;
	file->sectornum = numsec;
	file->eof = eof;
	return 1;

}
//
long fat_readwrite(struct fat_file *file, long sectorcount,
                   void* buf, bool write )
{
    long cluster = file->lastcluster;
    long sector = file->lastsector;
    long clusternum = file->clusternum;
    long numsec = file->sectornum;
    bool eof = file->eof;
    long first=0, last=0;
    long i;
    int rc;

    fat_dprintf("%s, firstcluster %ld, count %ld, buf 0x%lx, sector %ld, "
				"numsec %ld, %s\n",
				write ? "Write":"Read", file->firstcluster,
				sectorcount, (long)buf, sector, numsec, eof ? "eof":"noeof");

    if ( eof && !write)
        return 0;

    /* find sequential sectors and write them all at once */
    for (i=0; (i < sectorcount) && (sector > -1); i++ ) {
        numsec++;
        if ( numsec > (long)fat_bpb.bpb_secperclus || !cluster ) {
            long oldcluster = cluster;
            if (write)
                cluster = next_write_cluster(file, cluster, &sector);
            else {
                cluster = get_next_cluster(cluster);
                sector = cluster2sec(cluster);
            }

            clusternum++;
            numsec=1;

            if (!cluster) {
                eof = true;
                if ( write ) {
                    /* remember last cluster, in case
                       we want to append to the file */
                    cluster = oldcluster;
                    clusternum--;
                    i = -1; /* Error code */
                    break;
                }
            }
            else
                eof = false;
        }
        else {
            if (sector) {
                sector++;
			}
            else {
                /* look up first sector of file */
                sector = cluster2sec(file->firstcluster);
                numsec=1;
#ifdef HAVE_FAT16SUPPORT
                if (file->firstcluster < 0) {   /* FAT16 root dir */
                    sector += fat_bpb.rootdiroffset;
                    numsec += fat_bpb.rootdiroffset;
                }
#endif
            }
        }

        if (!first)
            first = sector;

		/*
		 * The original rockbox FAT code had a hard-coded limit of 256 sectors
		 * max per transfer.  We allow more than that now, but BE SURE that your
		 * disk driver is able to deal with a transfer request of more than 256
		 * sectors if you do not use the default!
		 */
        if ( ((sector != first) && (sector != last+1)) || /* not sequential */
             (last-first+1 == CFG_ROCKBOX_FAT_MAX_SECS_PER_XFER) ) {
            long count = last - first + 1;
            rc = transfer(first, count, buf, write );
            if (rc < 0)
                return rc * 10 - 1;

            buf = (char *)buf + count * SECTOR_SIZE;
            first = sector;
        }

        if ((i == sectorcount-1) && /* last sector requested */
            (!eof)) {
            long count = sector - first + 1;
            rc = transfer(first, count, buf, write );
            if (rc < 0)
                return rc * 10 - 2;
        }

        last = sector;
    }

    file->lastcluster = cluster;
    file->lastsector = sector;
    file->clusternum = clusternum;
    file->sectornum = numsec;
    file->eof = eof;

    /* if eof, don't report last block as read/written */
    if (eof)
        i--;

    fat_dprintf("Sectors %s = %ld\n", write ? "written":"read", i);
    return i;
}

int fat_seek(struct fat_file *file, unsigned long seeksector )
{
    long clusternum=0, numclusters=0, sectornum=0, sector=0;
    long cluster = file->firstcluster;
    long i;

#ifdef HAVE_FAT16SUPPORT
    if (cluster < 0) /* FAT16 root dir */
        seeksector += fat_bpb.rootdiroffset;
#endif

    file->eof = false;
    if (seeksector) {
        /* we need to find the sector BEFORE the requested, since
           the file struct stores the last accessed sector */
        seeksector--;
        numclusters = clusternum = seeksector / fat_bpb.bpb_secperclus;
        sectornum = seeksector % fat_bpb.bpb_secperclus;

        if (file->clusternum && clusternum >= file->clusternum) {
            cluster = file->lastcluster;
            numclusters -= file->clusternum;
        }

        for (i=0; i<numclusters; i++) {
            cluster = get_next_cluster(cluster);
            if (!cluster) {
                fat_eprintf("Attempt to seek beyond end of file "
							"(sector %ld, cluster %ld)\n", seeksector, i);
                return -1;
            }
        }

        sector = cluster2sec(cluster) + sectornum;
    }
    else {
        sectornum = -1;
    }

    fat_dprintf("firstcluser %ld, seeksector %lu, cluster %ld, "
				"sector %ld, sectornum %ld\n",
				file->firstcluster, seeksector, cluster, sector, sectornum);

    file->lastcluster = cluster;
    file->lastsector = sector;
    file->clusternum = clusternum;
    file->sectornum = sectornum + 1;
    return 0;
}

int fat_opendir(struct fat_dir *dir, unsigned long startcluster,
                const struct fat_dir *parent_dir)
{
    int rc;

    dir->entry = 0;
    dir->sector = 0;

    if (startcluster == 0)
        startcluster = fat_bpb.bpb_rootclus;

    rc = fat_open(startcluster, &dir->file, parent_dir);
    if(rc) {
        fat_eprintf("Could not open dir (rc = %d)\n", rc);
        return rc * 10 - 1;
    }

    return 0;
}

/* convert from unicode to a single-byte charset */
static void unicode2iso(const unsigned char* unicode, unsigned char* iso,
                        int count)
{
    int i;

    for (i=0; i<count; i++) {
        int x = i*2;
        switch (unicode[x+1]) {
            case 0x01: /* latin extended. convert to ISO 8859-2 */
            case 0x02:
                iso[i] = unicode2iso8859_2[unicode[x]];
                break;

            case 0x03: /* greek, convert to ISO 8859-7 */
                iso[i] = unicode[x] + 0x30;
                break;

                /* Sergei says most russians use Win1251, so we will too.
                   Win1251 differs from ISO 8859-5 by an offset of 0x10. */
            case 0x04: /* cyrillic, convert to Win1251 */
                switch (unicode[x]) {
                    case 1:
                        iso[i] = 168;
                        break;

                    case 81:
                        iso[i] = 184;
                        break;

                    default:
                        iso[i] = unicode[x] + 0xb0; /* 0xa0 for ISO 8859-5 */
                        break;
                }
                break;

            case 0x05: /* hebrew, convert to ISO 8859-8 */
                iso[i] = unicode[x] + 0x10;
                break;

            case 0x06: /* arabic, convert to ISO 8859-6 */
            case 0x0e: /* thai, convert to ISO 8859-11 */
                iso[i] = unicode[x] + 0xa0;
                break;

            default:
                iso[i] = unicode[x];
                break;
        }
    }
}

int fat_getnext(struct fat_dir *dir, struct fat_direntry *entry)
{
    bool done = false;
    int i;
    int rc;
    unsigned char firstbyte;
    int longarray[20];
    int longs=0;
    int sectoridx=0;
    unsigned char* cached_buf = dir->sectorcache[0];

    dir->entrycount = 0;

    while(!done) {
        if ( !(dir->entry % DIR_ENTRIES_PER_SECTOR) || !dir->sector ) {
            rc = fat_readwrite(&dir->file, 1, cached_buf, false);
            if (rc == 0) {
                /* eof */
                entry->name[0] = 0;
                break;
            }

            if (rc < 0) {
				fat_eprintf("Could not read dir (rc = %d)\n", rc);
                return rc * 10 - 1;
            }

            dir->sector = dir->file.lastsector;
        }

        for (i = dir->entry % DIR_ENTRIES_PER_SECTOR;
             i < DIR_ENTRIES_PER_SECTOR; i++) {
            unsigned int entrypos = i * DIR_ENTRY_SIZE;

            firstbyte = cached_buf[entrypos];
            dir->entry++;

            if (firstbyte == 0xe5) {
                /* free entry */
                sectoridx = 0;
                dir->entrycount = 0;
                continue;
            }

            if (firstbyte == 0) {
                /* last entry */
                entry->name[0] = 0;
                dir->entrycount = 0;
                return 0;
            }

            dir->entrycount++;

            /* longname entry? */
            if ( ( cached_buf[entrypos + FATDIR_ATTR] &
                   FAT_ATTR_LONG_NAME_MASK ) == FAT_ATTR_LONG_NAME ) {
                longarray[longs++] = entrypos + sectoridx;
            }
            else {
                if ( parse_direntry(entry, &cached_buf[entrypos]) ) {

                    /* don't return volume id entry */
                    if ( entry->attr == FAT_ATTR_VOLUME_ID )
                        continue;

                    /* replace shortname with longname? */
                    if ( longs ) {
                        int j,l=0;
                        /* iterate backwards through the dir entries */
                        for (j=longs-1; j>=0; j--) {
                            unsigned char* ptr = cached_buf;
                            int index = longarray[j];
                            /* current or cached sector? */
                            if ( sectoridx >= SECTOR_SIZE ) {
                                if ( sectoridx >= SECTOR_SIZE*2 ) {
                                    if ( ( index >= SECTOR_SIZE ) &&
                                         ( index < SECTOR_SIZE*2 ))
                                        ptr = dir->sectorcache[1];
                                    else
                                        ptr = dir->sectorcache[2];
                                }
                                else {
                                    if ( index < SECTOR_SIZE )
                                        ptr = dir->sectorcache[1];
                                }

                                index &= SECTOR_SIZE-1;
                            }

                            /* names are stored in unicode, but we
                               only grab the low byte (iso8859-1). */
                            unicode2iso(ptr + index + 1, entry->name + l, 5);
                            l+= 5;
                            unicode2iso(ptr + index + 14, entry->name + l, 6);
                            l+= 6;
                            unicode2iso(ptr + index + 28, entry->name + l, 2);
                            l+= 2;
                        }
                        entry->name[l]=0;
                    }
                    done = true;
                    sectoridx = 0;
                    i++;
                    break;
                }
            }
        }

        /* save this sector, for longname use */
        if ( sectoridx )
            memcpy( dir->sectorcache[2], dir->sectorcache[0], SECTOR_SIZE );
        else
            memcpy( dir->sectorcache[1], dir->sectorcache[0], SECTOR_SIZE );
        sectoridx += SECTOR_SIZE;

    }
    return 0;
}

/* vim: set ts=4 tw=80 sw=4 fo=tcroq: */
#endif /* #if defined(CFG_ROCKBOX_FAT) */
