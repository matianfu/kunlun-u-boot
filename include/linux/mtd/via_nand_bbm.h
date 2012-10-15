#ifndef	__VIA_NAND_BBM_H__
#define	__VIA_NAND_BBM_H__

enum flash_type {
	FLASH_NAND,
	FLASH_ONENAND,
};

struct reloc_item {
	unsigned short from;
	unsigned short to;
};

struct reloc_table {
	unsigned short header;
	unsigned short total;
};

struct via_nand_bbm {
	int	flash_type;

	int	current_slot;

	/* NOTES: this field impact the partition table. Please make sure
	 * that this value align with partitions definition.
	 */
	u32	max_reloc_entry;

	u32	max_bbm_slots;

	void	*data_buf;

	/* These two fields should be in (one)nand_chip.
	 * Add here to handle onenand_chip and nand_chip
	 * at the same time.
	 */
	int	page_shift;
	int	erase_shift;

	unsigned int		table_init;
	struct reloc_table	*table;
	struct reloc_item	*reloc;

	int	(*init)(struct mtd_info *mtd);
	int	(*uninit)(struct mtd_info *mtd);
	int	(*search)(struct mtd_info *mtd, unsigned int block);
	int	(*markbad)(struct mtd_info *mtd, unsigned int block);
	int	(*scan)(struct mtd_info *mtd);
	int	(*new)(struct mtd_info *mtd);
	int	(*dump)(void);
	int	(*entries)(void);
};

extern struct via_nand_bbm *g_via_bbm;
struct via_nand_bbm* alloc_via_nand_bbm(void);
void free_via_bbm(struct via_nand_bbm *);
#endif
