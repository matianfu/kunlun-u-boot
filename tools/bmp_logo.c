#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__)
#include <stdint.h>
#else
#ifdef __CYGWIN__
#include "elf.h"
#else
#include <inttypes.h>
#endif
#endif

typedef struct bitmap_s {		/* bitmap description */
	uint32_t width;
	uint32_t height;
	uint8_t	palette[256*3];
	uint8_t	*data;
} bitmap_t;

#define DEFAULT_CMAP_SIZE	 0 /* size of default color map	*/

/*
 * Neutralize little endians.
 */
uint32_t le_short(uint32_t x)
{
    uint32_t val;
    uint8_t *p = (uint8_t *)(&x);

    val =  (*p++ & 0xff) << 0;
    val |= (*p++ & 0xff) << 8;
    val |= (*p++ & 0xff) << 16;
    val |= (*p & 0xff) << 24;
    return val;
}

void skip_bytes (FILE *fp, int n)
{
	while (n-- > 0)
		fgetc (fp);
}

uint16_t rgb24_to_rgb16 (uint32_t val)
{
#if 1
	return ((val >> 3) & 0x1F) \
		   + (((val >> 10) & 0x3F) << 5) \
		   + (((val >> 19) & 0x1F) << 11);
#endif
}

uint16_t rgb555_to_rgb565 (uint32_t val)
{
	return (val & 0x1F) \
		   + ((((val >> 5) & 0x1f) << 1) << 5) \
		   + ((((val >> 10) & 0x1f)) << 11);
}

int main (int argc, char *argv[])
{
	int	i, x;
	FILE	*fp;
	bitmap_t bmp;
	bitmap_t *b = &bmp;
	uint32_t data_offset, n_colors;
	uint16_t bits_per_pixel;
	uint32_t compress_type;
	uint32_t  temp_var;


	if (argc < 2) {
		fprintf (stderr, "Usage: %s file\n", argv[0]);
		exit (EXIT_FAILURE);
	}

	if ((fp = fopen (argv[1], "rb")) == NULL) {
		perror (argv[1]);
		exit (EXIT_FAILURE);
	}

	if (fgetc (fp) != 'B' || fgetc (fp) != 'M') {
		fprintf (stderr, "%s is not a bitmap file.\n", argv[1]);
		exit (EXIT_FAILURE);
	}

	/*
	 * read width and height of the image, and the number of colors used;
	 * ignore the rest
	 */
	skip_bytes (fp, 8);
	fread (&data_offset, sizeof (data_offset), 1, fp);
	skip_bytes (fp, 4);
	fread (&b->width,   sizeof (b->width), 1, fp);
	fread (&b->height,  sizeof (b->height), 1, fp);
	skip_bytes(fp,2);
	fread (&bits_per_pixel,  sizeof (bits_per_pixel), 1, fp);
	fread (&compress_type,  sizeof (compress_type), 1, fp);
	skip_bytes (fp, 12);
	fread (&n_colors, sizeof (n_colors), 1, fp);
	skip_bytes (fp, 4);

	/*
	 * Repair endianess.
	 */
	data_offset = le_short(data_offset);
	b->width = le_short(b->width);
	b->height = le_short(b->height);
	n_colors = le_short(n_colors);

#if 0
	/* assume we are working with an 8-bit file */
	if ((n_colors == 0) || (n_colors > 256 - DEFAULT_CMAP_SIZE)) {
		/* reserve DEFAULT_CMAP_SIZE color map entries for default map */
		n_colors = 256 - DEFAULT_CMAP_SIZE;
	}
#endif

	 /* assume we are working with an uncompressed bitmap file */
	if (compress_type != 0 ) {
		fprintf (stderr, "%s is not a uncompressed bitmap file.\n", argv[1]);
		exit (EXIT_FAILURE);
	}

      /* assume we are working with an 8,16,24-bit file */
	if (bits_per_pixel != 8 && bits_per_pixel != 16 && bits_per_pixel != 24 ) {
		fprintf (stderr, "%s is not a bitmap(8-bit) file(%d).\n", argv[1],bits_per_pixel);
		exit (EXIT_FAILURE);
	}

	if(n_colors == 0 && bits_per_pixel == 8)
		n_colors = 256;

	printf ("/*\n"
		" * Automatically generated by \"tools/bmp_logo\"\n"
		" *\n"
		" * DO NOT EDIT\n"
		" *\n"
		" */\n\n\n"
		"#ifndef __BMP_LOGO_H__\n"
		"#define __BMP_LOGO_H__\n\n"
		"#define BMP_LOGO_WIDTH\t\t%d\n"
		"#define BMP_LOGO_HEIGHT\t\t%d\n"
		"#define BMP_LOGO_BPP\t\t%d\n"
		"#define BMP_LOGO_COLORS\t\t%d\n"
		"#define BMP_LOGO_OFFSET\t\t%d\n"
		"\n",
		b->width, b->height, bits_per_pixel,n_colors,
		DEFAULT_CMAP_SIZE);

	printf("/*data offset %d*/\n",data_offset);

	/* allocate memory */
	if ((b->data = (uint8_t *)malloc(b->width * b->height * bits_per_pixel / 8 )) == NULL) {
		fclose (fp);
		printf ("Error allocating memory for file %s.\n", argv[1]);
		exit (EXIT_FAILURE);
	}

	/* read and print the palette information */
	printf ("unsigned short bmp_logo_palette[] = {\n");

	for (i=0; i<n_colors; ++i) {
		b->palette[(int)(i*3+2)] = fgetc(fp);
		b->palette[(int)(i*3+1)] = fgetc(fp);
		b->palette[(int)(i*3+0)] = fgetc(fp);
		x=fgetc(fp);
		temp_var = b->palette[(int)(i*3+2)] >> 3 & 0x1F;
		temp_var += (((b->palette[(int)(i*3+1)] >> 2) & 0x3F) << 5);
		temp_var +=( ((b->palette[(int)(i*3+0)] >> 3) & 0x1F) << 11);

		printf ("%s0x%04X,%s",
			((i%8) == 0) ? "\t" : "  ",
			temp_var,
			((i%8) == 7) ? "\n" : ""
		);
	}

	/* seek to offset indicated by file header */
	fseek(fp, (long)data_offset, SEEK_SET);

	/* read the bitmap; leave room for default color map */
	printf ("\n");
	printf ("};\n");
	printf ("\n");
	if(bits_per_pixel == 8)
		printf ("unsigned char bmp_logo_bitmap[] = {\n");
	else
		printf ("unsigned short bmp_logo_bitmap[] = {\n");

	for (i=(b->height-1)*b->width; i>=0; i-=b->width) {
		for (x = 0; x < b->width; x++) {
			if(bits_per_pixel == 8){
				b->data[(uint32_t) i + x] = (uint8_t) fgetc (fp) + DEFAULT_CMAP_SIZE;
			}else if(bits_per_pixel == 16){
			      fread (&(b->data[(i + x)*2]), 2, 1, fp);
			}else if(bits_per_pixel == 24){
			      fread (&(b->data[(i + x)*3]), 3, 1, fp);
			}
		}
	}
	fclose (fp);

	for (i=0; i<(b->height*b->width); ++i) {
		if ((i%8) == 0)
			putchar ('\t');
		if(bits_per_pixel == 8){
			printf ("0x%02X,%c",b->data[i],((i%8) == 7) ? '\n' : ' ');
		}else if(bits_per_pixel == 16){
			temp_var = *((uint16_t*)&(b->data[i*2]));
		      printf ("0x%04X,%c",rgb555_to_rgb565(temp_var),((i%8) == 7) ? '\n' : ' ');
		}else if(bits_per_pixel == 24){
			temp_var = b->data[i*3];
			temp_var += b->data[i*3 + 1] << 8;
			temp_var += b->data[i*3 + 2] << 16;
		    printf ("0x%04X,%c",rgb24_to_rgb16(temp_var),((i%8) == 7) ? '\n' : ' ');
		}
	}
	printf ("\n"
		"};\n\n"
		"#endif /* __BMP_LOGO_H__ */\n"
	);

	return (0);
}
