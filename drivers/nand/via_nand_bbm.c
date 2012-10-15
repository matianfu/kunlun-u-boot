/*
 * linux/drivers/nand/via_nand_bbm.c
 * Bad Block Table support for NAND.
 *
 * Copyright (C) 2011 Via Telecom Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <common.h>
#include <malloc.h>
#include <linux/mtd/compat.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/via_nand_bbm.h>
#include <asm/errno.h>

extern int nand_unlock(struct mtd_info *nand, ulong off, ulong size);

#define	NAND_RELOC_HEADER	0x4c51
/*the last 128 block in nand reserved to relocate the bad block*/
#define	MAX_RELOC_ENTRY	127
/*the last 40 pages in block 0 is used to store BBM table*/
#define	MAX_BBM_SLOT	40

#define NAND_UBOOT_END        0x0100000 /* Giving a space of 4 blocks for uboot = (4*128)KB */

//#define	BBM_DBG
#ifdef	BBM_DBG
#define   bbm_dbg(format, arg...) printk("[BBM]: " format "\n" , ## arg)
#else
#define   bbm_dbg(format, arg...) do {} while (0)
#endif

static int dump_reloc_table(void)
{
	struct via_nand_bbm *bbm = g_via_bbm;
	int i;

	if(bbm == NULL){
		return 0;
	}
	printk("current_slot=%d, total=%d.\n", bbm->current_slot, bbm->table->total);
	for (i = 0; i < bbm->table->total; i++) {
		printk("block: %08x is relocated to block: %08x\n",
			(bbm->reloc[i].from) << bbm->erase_shift, (bbm->reloc[i].to) << bbm->erase_shift);
	}

	return i;
}

#define MAX_PAGE_SIZE (2048 + 64)
static uint8_t  pagebuf[MAX_PAGE_SIZE];
struct via_nand_bbm *g_via_bbm;

void dump_buf(unsigned char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		printk(" %02x", buf[i]);
		if (0 == ((i + 1) % 16))
			printk("\n");
	}
	printk("\n");
}

/* add the relocation entry into the relocation table
 * If the relocated block is bad, an new entry will be added into the
 * bottom of the relocation table.
 */
static int update_reloc_tb(struct mtd_info *mtd, int block)
{
	struct via_nand_bbm *bbm = g_via_bbm;
	struct reloc_table *table = bbm->table;
	struct reloc_item *item = bbm->reloc;
	struct erase_info instr;
	int reloc_block, ret, entry_num = -1;
	int i;

	bbm_dbg("update reloc table: 0x%x\n", block << bbm->erase_shift);
	if (bbm->table_init == 0) {
		memset(table, 0, sizeof(struct reloc_table));
		table->header = NAND_RELOC_HEADER;
		bbm->table_init = 1;
	}

	if (table->total > bbm->max_reloc_entry) {
		printk("Relocation table exceed max num,");
		printk("cannot relocate block 0x%x\n", block);
		return -EINVAL;
	}
	reloc_block = (mtd->size >> bbm->erase_shift) - 1;

	//identify whether the block has been relocated
	for(i = table->total - 1; i >= 0; i --) {
		if(block == item[i].from)
			entry_num = i;
	}

	//find the available block with the largest number in reservered area
	for(i = table->total - 1; i >= 0; i --) {
		if (item[i].to != 65535) {
			if (reloc_block >= item[i].to)
				reloc_block = item[i].to - 1;
		} else {
			if (reloc_block >= item[i].from)
				reloc_block = item[i].from - 1;
		}
	}

	if (reloc_block < ((mtd->size >> bbm->erase_shift) - bbm->max_reloc_entry))
		return -ENOSPC;

	/* Make sure that reloc_block is pointing to a valid block */
	for (; ; reloc_block--) {
              bbm_dbg("update reloc table: check 0x%x\n", reloc_block << bbm->erase_shift);
		for (i = table->total-1; i >= 0; i--) {
			if (reloc_block == item[i].from) {
				if (item[i].to != 65535)
					printk(KERN_ERR "Res block marked invalid \
							in reloc table\n");
				reloc_block--;
				i = table->total-1;
				if (reloc_block <
						((mtd->size >> bbm->erase_shift) -
						 bbm->max_reloc_entry))
					return -ENOSPC;
			}
		}
		/* The relocate table is full */
		if (reloc_block <
				((mtd->size >> bbm->erase_shift) - bbm->max_reloc_entry))
			return -ENOSPC;

		memset(&instr, 0, sizeof(struct erase_info));
		instr.mtd = mtd;
		instr.addr = reloc_block << bbm->erase_shift;
		instr.len = (1 << bbm->erase_shift);

		nand_unlock(mtd, 0, mtd->size);
		ret = mtd->erase(mtd, &instr);
		if (!ret)
			break;
		else {
			bbm_dbg("status:%d found at erasing reloc block 0x%x\n",
				ret, reloc_block << bbm->erase_shift);
			/* skip it if the reloc_block is also a
			 * bad block
			 */
			item[table->total].from = reloc_block;
			item[table->total].to = 65535;
			table->total++;
		}
	}
	/* Create the relocated block information in the table */
	//when the block is relocated before, blob should modify the original entry to new
	//relocated block and the old relocated block point to 65535. If not the situation,
	//create a new entry
	if (entry_num != -1) {
		item[table->total].from = item[entry_num].to;
		item[table->total].to = 65535;
		bbm_dbg("update reloc table: mark 0x%x to be bad\n",
			(item[table->total].from)<< bbm->erase_shift);
		table->total++;
		item[entry_num].to = reloc_block;

	} else {
		item[table->total].from = block;
		item[table->total].to = reloc_block;
		table->total++;
	}
	bbm_dbg("update reloc table: 0x%x located to 0x%x\n",
		block<< bbm->erase_shift, reloc_block << bbm->erase_shift);
	return 0;
}

/* Write the relocation table back to device, if there's room. */
static int sync_reloc_tb(struct mtd_info *mtd, int idx)
{
	struct via_nand_bbm *bbm = g_via_bbm;
	int start_page, len;
	unsigned int retlen;
	uint8_t *tmp;

	if (idx >= bbm->max_bbm_slots) {
		printk(KERN_ERR "Can't write relocation table to device any more.\n");
		return -1;
	}

	if (idx < 0) {
		printk(KERN_ERR "Wrong Slot is specified.\n");
		return -1;
	}

	bbm->table->header = NAND_RELOC_HEADER;
	len = 4;
	len += bbm->table->total << 2;
	/* write to device */
	/* the write page should be after the current slot */
	start_page = (1 << (bbm->erase_shift - bbm->page_shift)) - 1;
	start_page = start_page - idx;
	bbm_dbg("sync reloc table: page=%d, index=%d.\n", start_page, idx);
	tmp = (uint8_t *)bbm->data_buf;
	nand_unlock(mtd, 0, mtd->size);
	mtd->write(mtd, start_page << bbm->page_shift,
			1 << bbm->page_shift, &retlen, tmp);

	return 0;
}

/* check whether a page and spare area is empty
 * size should be sum of page size and spare area length
 */
static int is_buf_blank(u8 * buf, int size)
{
	int i = 0;
	while (i < size) {
		if (*((unsigned long *)(buf + i)) != 0xFFFFFFFF)
			return 0;
		i += 4;
	}
	if (i > size) {
		i -= 4;
		while (i < size) {
			if (*(buf + i) != 0xFF)
				return 0;
			i++;
		}
	}
	return 1;
}

/*create a new relocation table*/
static int via_new_reloc_tb(struct mtd_info *mtd)
{
	struct via_nand_bbm *bbm = g_via_bbm;
	int i, ret = -EBADSLT;
	int off, start_page;
	unsigned int retlen;

	memset(bbm->data_buf, 0, mtd->oobblock + mtd->oobsize);
	for (off = NAND_UBOOT_END; off < mtd->size; off += mtd->erasesize){
		if (mtd->block_isbad(mtd, off)){
			update_reloc_tb(mtd, off >> bbm->erase_shift);
		}
	}

	start_page = (1 << (bbm->erase_shift - bbm->page_shift)) - 1;
	for(i = 0; i < bbm->max_bbm_slots; i++, start_page--){
		memset(pagebuf, 0, MAX_PAGE_SIZE);
		mtd->read(mtd, (start_page << bbm->page_shift),
			1 << bbm->page_shift, &retlen, pagebuf);
		if(is_buf_blank(pagebuf, 1 << bbm->page_shift)){
			/*maybe there is no exist rlocation table, so caculate the index outside of sync funcition*/
			bbm->current_slot = i;
			sync_reloc_tb(mtd, bbm->current_slot);
			ret = 0;
			bbm_dbg("Create a new relocation table at page:%d, current slot is %d.\n",
				start_page, bbm->current_slot);
			break;
		}
	}

	return ret;
}

static int via_scan_reloc_tb(struct mtd_info *mtd)
{
	struct via_nand_bbm *bbm = g_via_bbm;
	struct reloc_table *table = bbm->table;
	int page, maxslot, ret, valid = 0;
	unsigned int retlen;

	/* there're several relocation tables in the first block.
	 * When new bad blocks are found, a new relocation table will
	 * be generated and written back to the first block. But the
	 * original relocation table won't be erased. Even if the new
	 * relocation table is written wrong, system can still find an
	 * old one.
	 * One page contains one slot.
	 */
	maxslot = 1 << (bbm->erase_shift - bbm->page_shift);
	page = maxslot - bbm->max_bbm_slots;
	for (; page < maxslot; page++) {
		memset(bbm->data_buf, 0,
				mtd->oobblock + mtd->oobsize);
		ret = mtd->read(mtd, (page << bbm->page_shift),
				mtd->oobblock, &retlen, bbm->data_buf);

		if (ret == 0) {
			if (table->header != NAND_RELOC_HEADER) {
				continue;
			} else {
				bbm->current_slot = maxslot - page - 1;
				valid = 1;
				break;
			}
		}
	}

	if (valid) {
		printk(KERN_DEBUG "relocation table at page:%d, current slot is %d.\n", page, bbm->current_slot);
		bbm->table_init = 1;
	} else {
		/* There should be a valid relocation table slot at least. */
		printk("NO VALID relocation table can be recognized\n");
		ret = via_new_reloc_tb(mtd);
		if(ret < 0){
			printk("Fail to create new relocation table.\n");
			memset(bbm->data_buf, 0, mtd->oobblock + mtd->oobsize);
			bbm->table_init = 0;
			return -EINVAL;
              }
	}

	dump_reloc_table();
	return 0;
}

static int via_init_reloc_tb(struct mtd_info *mtd)
{
	struct via_nand_bbm *bbm = g_via_bbm;
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int size = mtd->oobblock + mtd->oobsize;

	bbm->flash_type = FLASH_NAND;
	bbm->page_shift = this->page_shift;
	bbm->erase_shift = this->phys_erase_shift;
	bbm->max_reloc_entry = MAX_RELOC_ENTRY;
	bbm->max_bbm_slots = MAX_BBM_SLOT;
	bbm->table_init = 0;

	bbm_dbg("page_shift=%d, erase_shift=%d, table_size=%d, max_entry=%d, max_slots=%d\n",
                        bbm->page_shift, bbm->erase_shift, size, bbm->max_reloc_entry, bbm->max_bbm_slots);
	bbm->data_buf = kmalloc(size, GFP_KERNEL);
	if (!bbm->data_buf) {
		return -ENOMEM;
	}
	memset(bbm->data_buf, 0x0, size);

	bbm->table = (struct reloc_table *)bbm->data_buf;
	memset(bbm->table, 0x0, sizeof(struct reloc_table));

	bbm->reloc = (struct reloc_item *)((uint8_t *)bbm->data_buf +
			sizeof (struct reloc_table));
	memset(bbm->reloc, 0x0,
			sizeof(struct reloc_item) * bbm->max_reloc_entry);

	/* reduce the mtd size which expose to BBM */
	//mtd->size -=( (g_via_bbm->max_reloc_entry) << (g_via_bbm->erase_shift));
	return via_scan_reloc_tb(mtd);
}

static int via_uninit_reloc_tb(struct mtd_info *mtd)
{
	struct via_nand_bbm *bbm = g_via_bbm;
	kfree(bbm->data_buf);
	return 0;
}

/* Find the relocated block of the bad one.
 * If it's a good block, return 0. Otherwise, return a relocated one.
 * idx points to the next relocation entry
 * If the relocated block is bad, an new entry will be added into the
 * bottom of the relocation table.
 */
static int via_search_reloc_tb(struct mtd_info *mtd, unsigned int block)
{
	struct via_nand_bbm *bbm = g_via_bbm;
	struct reloc_table *table = bbm->table;
	struct reloc_item *item = bbm->reloc;
	int i, max, reloc_block;

	if( (bbm == NULL) ||
                (bbm->table_init == 0) ||
                (block <= 0) ||
	        (block > mtd->size >> bbm->erase_shift) ){
		return block;
	}

	table = bbm->table;
	item = bbm->reloc;

	if(table->total == 0){
		return block;
	}

	if (table->total > bbm->max_reloc_entry)
		table->total = bbm->max_reloc_entry;

	/* If can't find reloc tb entry for block, return block */
	reloc_block = block;
	max = table->total;
	for (i = max-1; i >= 0; i--) {
		if (block == item[i].from) {
			reloc_block = item[i].to;
			bbm_dbg("search: 0x%x -> 0x%x\n",
				block<< bbm->erase_shift, reloc_block << bbm->erase_shift);
			break;
		}
	}

	return reloc_block;
}

static int via_mark_reloc_tb(struct mtd_info *mtd, unsigned int block)
{
	struct via_nand_bbm *bbm = g_via_bbm;
	int ret = 0;

	if ( (NULL == bbm) ||
                (bbm->table_init == 0) ||
                (block <= 0) ||
                (block > mtd->size >> bbm->erase_shift) ){
		return ret;
	}

	bbm_dbg("markbad: 0x%x\n", block << bbm->erase_shift);

	ret = update_reloc_tb(mtd, block);
	if (ret)
		return ret;

	ret = sync_reloc_tb(mtd, bbm->current_slot + 1);
	if(!ret){
		bbm->current_slot++;
	}
	return ret;
}

static int via_get_max_entry(void)
{
	struct via_nand_bbm *bbm = g_via_bbm;

	if ( (NULL == bbm) ||
                (bbm->table_init == 0) ){
		return 0;
	}

	return bbm->max_reloc_entry;
}

struct via_nand_bbm* alloc_via_nand_bbm(void)
{
	/* FIXME: We don't want to add module_init entry
	 * here to avoid dependency issue.
	 */
	struct via_nand_bbm *bbm;

	bbm = kmalloc(sizeof(struct via_nand_bbm), GFP_KERNEL);
	if (!bbm)
		return NULL;
	memset(bbm, 0x0, sizeof(struct via_nand_bbm));
	bbm->init = via_init_reloc_tb;
	bbm->uninit = via_uninit_reloc_tb;
	bbm->search = via_search_reloc_tb;
	bbm->markbad = via_mark_reloc_tb;
	bbm->scan = via_scan_reloc_tb;
	bbm->new = via_new_reloc_tb;
	bbm->dump = dump_reloc_table;
	bbm->entries = via_get_max_entry;
	g_via_bbm = bbm;

	return bbm;
}
//EXPORT_SYMBOL(alloc_via_nand_bbm);

void free_via_nand_bbm(void)
{
	if (g_via_bbm) {
		kfree(g_via_bbm);
		g_via_bbm = NULL;
	}
}
//EXPORT_SYMBOL(free_via_nand_bbm);
