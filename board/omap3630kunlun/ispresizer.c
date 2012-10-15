#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <stdarg.h>
#include <linux/types.h>

#include "ispresizer.h"

#define DEBUG

#ifdef DEBUG
#define DPRINTK_ISPRESZ(fmt,...)  printf(fmt,## __VA_ARGS__)
#else
#define DPRINTK_ISPRESZ(fmt,...)
#endif
/**
 * struct isp_res - Structure for the resizer module to store its information.
 * @res_inuse: Indicates if resizer module has been reserved. 1 - Reserved,
 *             0 - Freed.
 * @h_startphase: Horizontal starting phase.
 * @v_startphase: Vertical starting phase.
 * @h_resz: Horizontal resizing value.
 * @v_resz: Vertical resizing value.
 * @outputwidth: Output Image Width in pixels.
 * @outputheight: Output Image Height in pixels.
 * @inputwidth: Input Image Width in pixels.
 * @inputheight: Input Image Height in pixels.
 * @algo: Algorithm select. 0 - Disable, 1 - [-1 2 -1]/2 high-pass filter,
 *        2 - [-1 -2 6 -2 -1]/4 high-pass filter.
 * @ipht_crop: Vertical start line for cropping.
 * @ipwd_crop: Horizontal start pixel for cropping.
 * @cropwidth: Crop Width.
 * @cropheight: Crop Height.
 * @resinput: Resizer input.
 * @coeflist: Register configuration for Resizer.
 */
static struct isp_res {
	u8 res_inuse;
	u8 h_startphase;
	u8 v_startphase;
	u16 h_resz;
	u16 v_resz;
	u32 outputwidth;
	u32 outputheight;
	u32 inputwidth;
	u32 inputheight;
	u8 algo;
	u32 ipht_crop;
	u32 ipwd_crop;
	u32 cropwidth;
	u32 cropheight;
	dma_addr_t tmp_buf;
	enum ispresizer_input resinput;
	struct isprsz_coef coeflist;
} ispres_obj;

static struct isprsz_yenh ispreszdefaultyenh = {0, 0, 0, 0};

static struct isprsz_coef ispreszdefcoef = {
	{
		0x0000, 0x0100, 0x0000, 0x0000,
		0x03FA, 0x00F6, 0x0010, 0x0000,
		0x03F9, 0x00DB, 0x002C, 0x0000,
		0x03FB, 0x00B3, 0x0053, 0x03FF,
		0x03FD, 0x0082, 0x0084, 0x03FD,
		0x03FF, 0x0053, 0x00B3, 0x03FB,
		0x0000, 0x002C, 0x00DB, 0x03F9,
		0x0000, 0x0010, 0x00F6, 0x03FA
	},
	{
		0x0000, 0x0100, 0x0000, 0x0000,
		0x03FA, 0x00F6, 0x0010, 0x0000,
		0x03F9, 0x00DB, 0x002C, 0x0000,
		0x03FB, 0x00B3, 0x0053, 0x03FF,
		0x03FD, 0x0082, 0x0084, 0x03FD,
		0x03FF, 0x0053, 0x00B3, 0x03FB,
		0x0000, 0x002C, 0x00DB, 0x03F9,
		0x0000, 0x0010, 0x00F6, 0x03FA
	},
	{
		0x0004, 0x0023, 0x005A, 0x0058,
		0x0023, 0x0004, 0x0000, 0x0002,
		0x0018, 0x004d, 0x0060, 0x0031,
		0x0008, 0x0000, 0x0001, 0x000f,
		0x003f, 0x0062, 0x003f, 0x000f,
		0x0001, 0x0000, 0x0008, 0x0031,
		0x0060, 0x004d, 0x0018, 0x0002
	},
	{
		0x0004, 0x0023, 0x005A, 0x0058,
		0x0023, 0x0004, 0x0000, 0x0002,
		0x0018, 0x004d, 0x0060, 0x0031,
		0x0008, 0x0000, 0x0001, 0x000f,
		0x003f, 0x0062, 0x003f, 0x000f,
		0x0001, 0x0000, 0x0008, 0x0031,
		0x0060, 0x004d, 0x0018, 0x0002
	}
};

unsigned long mmio_base[OMAP3_ISP_IOMEM_RESZ + 1];

void isp_reg_writel(u32 reg_value, enum isp_mem_resources isp_mmio_range,
		    u32 reg_offset)
{
	__raw_writel(reg_value,mmio_base[isp_mmio_range] + reg_offset);
}

u32 isp_reg_readl(enum isp_mem_resources isp_mmio_range, u32 reg_offset)
{
	return __raw_readl(mmio_base[isp_mmio_range] + reg_offset);
}

void isp_reg_or(enum isp_mem_resources mmio_range, u32 reg,
			      u32 or_bits)
{
	u32 v = isp_reg_readl(mmio_range, reg);

	isp_reg_writel(v | or_bits, mmio_range, reg);
}


void isp_reg_and_or(enum isp_mem_resources mmio_range, u32 reg,
				  u32 and_bits, u32 or_bits)
{
	u32 v = isp_reg_readl(mmio_range, reg);

	isp_reg_writel((v & and_bits) | or_bits, mmio_range, reg);
}

/**
 * ispresizer_try_size - Validates input and output images size.
 * @input_w: input width for the resizer in number of pixels per line
 * @input_h: input height for the resizer in number of lines
 * @output_w: output width from the resizer in number of pixels per line
 *            resizer when writing to memory needs this to be multiple of 16.
 * @output_h: output height for the resizer in number of lines, must be even.
 *
 * Calculates the horizontal and vertical resize ratio, number of pixels to
 * be cropped in the resizer module and checks the validity of various
 * parameters. Formula used for calculation is:-
 *
 * 8-phase 4-tap mode :-
 * inputwidth = (32 * sph + (ow - 1) * hrsz + 16) >> 8 + 7
 * inputheight = (32 * spv + (oh - 1) * vrsz + 16) >> 8 + 4
 * endpahse for width = ((32 * sph + (ow - 1) * hrsz + 16) >> 5) % 8
 * endphase for height = ((32 * sph + (oh - 1) * hrsz + 16) >> 5) % 8
 *
 * 4-phase 7-tap mode :-
 * inputwidth = (64 * sph + (ow - 1) * hrsz + 32) >> 8 + 7
 * inputheight = (64 * spv + (oh - 1) * vrsz + 32) >> 8 + 7
 * endpahse for width = ((64 * sph + (ow - 1) * hrsz + 32) >> 6) % 4
 * endphase for height = ((64 * sph + (oh - 1) * hrsz + 32) >> 6) % 4
 *
 * Where:
 * sph = Start phase horizontal
 * spv = Start phase vertical
 * ow = Output width
 * oh = Output height
 * hrsz = Horizontal resize value
 * vrsz = Vertical resize value
 *
 * Fills up the output/input widht/height, horizontal/vertical resize ratio,
 * horizontal/vertical crop variables in the isp_res structure.
 **/
int ispresizer_try_size(u32 *input_width, u32 *input_height, u32 *output_w,u32 *output_h)
{
	u32 rsz, rsz_7, rsz_4;
	u32 sph;
	u32 input_w, input_h;
	int max_in_otf, max_out_7tap;

	input_w = *input_width;
	input_h = *input_height;

	if (input_w < 32 || input_h < 32) {
		return -1;
	}
#if 1
	input_w -= 6;
	input_h -= 6;
#endif

	if (input_h > MAX_IN_HEIGHT)
		return -1;

	if (*output_w < 16)
		*output_w = 16;

	if (*output_h < 2)
		*output_h = 2;


	max_in_otf = MAX_IN_WIDTH_ONTHEFLY_MODE_ES2;
	max_out_7tap = MAX_7TAP_VRSZ_OUTWIDTH_ES2;

	if (ispres_obj.resinput == RSZ_OTFLY_YUV) {
		if (input_w > max_in_otf)
			return -1;
	} else {
		if (input_w > MAX_IN_WIDTH_MEMORY_MODE)
			return -1;
	}

	*output_h &= 0xfffffffe;
	sph = DEFAULTSTPHASE;

	rsz_7 = ((input_h - 7) * 256) / (*output_h - 1);
	rsz_4 = ((input_h - 4) * 256) / (*output_h - 1);

	rsz = (input_h * 256) / *output_h;

	if (rsz <= MID_RESIZE_VALUE) {
		rsz = rsz_4;
		if (rsz < MINIMUM_RESIZE_VALUE) {
			rsz = MINIMUM_RESIZE_VALUE;
			*output_h = (((input_h - 4) * 256) / rsz) + 1;
			DPRINTK_ISPRESZ("%s: using output_h %d instead\n",
			       __func__, *output_h);
		}
	} else {
		rsz = rsz_7;
		if (*output_w > max_out_7tap)
			*output_w = max_out_7tap;
		if (rsz > MAXIMUM_RESIZE_VALUE) {
			rsz = MAXIMUM_RESIZE_VALUE;
			*output_h = (((input_h - 7) * 256) / rsz) + 1;
			DPRINTK_ISPRESZ( "%s: using output_h %d instead\n",
			       __func__, *output_h);
		}
	}


	if (rsz > MID_RESIZE_VALUE) {
		input_h =
			(((64 * sph) + ((*output_h - 1) * rsz) + 32) / 256) + 7;
	} else {
		input_h =
			(((32 * sph) + ((*output_h - 1) * rsz) + 16) / 256) + 4;
	}


	ispres_obj.outputheight = *output_h;
	ispres_obj.v_resz = rsz;
	ispres_obj.inputheight = input_h;
	ispres_obj.ipht_crop = DEFAULTSTPIXEL;
	ispres_obj.v_startphase = sph;

	*output_w &= 0xfffffff0;
	sph = DEFAULTSTPHASE;

	rsz_7 = ((input_w - 7) * 256) / (*output_w - 1);
	rsz_4 = ((input_w - 4) * 256) / (*output_w - 1);

	rsz = (input_w * 256) / *output_w;
	if (rsz > MID_RESIZE_VALUE) {
		rsz = rsz_7;
		if (rsz > MAXIMUM_RESIZE_VALUE) {
			rsz = MAXIMUM_RESIZE_VALUE;
			*output_w = (((input_w - 7) * 256) / rsz) + 1;
			*output_w = (*output_w + 0xf) & 0xfffffff0;
			DPRINTK_ISPRESZ( "%s: using output_w %d instead\n",
			       __func__, *output_w);
		}
	} else {
		rsz = rsz_4;
		if (rsz < MINIMUM_RESIZE_VALUE) {
			rsz = MINIMUM_RESIZE_VALUE;
			*output_w = (((input_w - 4) * 256) / rsz) + 1;
			*output_w = (*output_w + 0xf) & 0xfffffff0;
			DPRINTK_ISPRESZ( "%s: using output_w %d instead\n",
			       __func__, *output_w);
		}
	}

	/* Recalculate input based on TRM equations */
	if (rsz > MID_RESIZE_VALUE) {
		input_w =
			(((64 * sph) + ((*output_w - 1) * rsz) + 32) / 256) + 7;
	} else {
		input_w =
			(((32 * sph) + ((*output_w - 1) * rsz) + 16) / 256) + 7;
	}

	ispres_obj.outputwidth = *output_w;
	ispres_obj.h_resz = rsz;
	ispres_obj.inputwidth = input_w;
	ispres_obj.ipwd_crop = DEFAULTSTPIXEL;
	ispres_obj.h_startphase = sph;

	*input_height = input_h;
	*input_width = input_w;
	return 0;
}

/**
 * ispresizer_config_luma_enhance - Configures luminance enhancer parameters.
 * @yenh: Pointer to structure containing desired values for core, slope, gain
 *        and algo parameters.
 **/
void ispresizer_config_luma_enhance(struct isprsz_yenh *yenh)
{
	DPRINTK_ISPRESZ("ispresizer_config_luma_enhance()+\n");
	ispres_obj.algo = yenh->algo;
	isp_reg_writel((yenh->algo << ISPRSZ_YENH_ALGO_SHIFT) |
		       (yenh->gain << ISPRSZ_YENH_GAIN_SHIFT) |
		       (yenh->slope << ISPRSZ_YENH_SLOP_SHIFT) |
		       (yenh->coreoffset << ISPRSZ_YENH_CORE_SHIFT),
		       OMAP3_ISP_IOMEM_RESZ,
		       ISPRSZ_YENH);
	DPRINTK_ISPRESZ("ispresizer_config_luma_enhance()-\n");
}



/**
 * ispresizer_config_outlineoffset - Configures the write address line offset.
 * @offset: Line offset for the preview output.
 *
 * Returns 0 if successful, or -1 if address is not 32 bits aligned.
 **/
static int ispresizer_config_outlineoffset(u32 offset)
{
	DPRINTK_ISPRESZ("ispresizer_config_outlineoffset()+\n");
	if (offset % 32)
		return -1;
	isp_reg_writel(offset << ISPRSZ_SDR_OUTOFF_OFFSET_SHIFT,
		       OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_OUTOFF);
	DPRINTK_ISPRESZ("ispresizer_config_outlineoffset()-\n");
	return 0;
}

/**
 * Configures the memory address to which the output frame is written.
 * @addr: 32bit memory address aligned on 32byte boundary.
 **/
int ispresizer_set_outaddr(u32 addr)
{
	//DPRINTK_ISPRESZ("ispresizer_set_outaddr(%#0x)+\n",addr);
	if (addr % 32)
		return -1;
	isp_reg_writel(addr << ISPRSZ_SDR_OUTADD_ADDR_SHIFT,
		       OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_OUTADD);
	//DPRINTK_ISPRESZ("ispresizer_set_outaddr()-\n");
	return 0;
}


/**
 * ispresizer_config_filter_coef - Sets filter coefficients for 4 & 7-tap mode.
 * This API just updates the isp_res struct.Actual register write happens in
 * ispresizer_config_size.
 * @coef: Structure containing horizontal and vertical filter coefficients for
 *        both 4-tap and 7-tap mode.
 **/
static void ispresizer_config_filter_coef(struct isprsz_coef *coef)
{
	int i;
	DPRINTK_ISPRESZ("ispresizer_config_filter_coef()+\n");
	for (i = 0; i < 32; i++) {
		ispres_obj.coeflist.h_filter_coef_4tap[i] =
			coef->h_filter_coef_4tap[i];
		ispres_obj.coeflist.v_filter_coef_4tap[i] =
			coef->v_filter_coef_4tap[i];
	}
	for (i = 0; i < 28; i++) {
		ispres_obj.coeflist.h_filter_coef_7tap[i] =
			coef->h_filter_coef_7tap[i];
		ispres_obj.coeflist.v_filter_coef_7tap[i] =
			coef->v_filter_coef_7tap[i];
	}
	DPRINTK_ISPRESZ("ispresizer_config_filter_coef()-\n");
}

/**
 * ispresizer_config_size - Configures input and output image size.
 * @input_w: input width for the resizer in number of pixels per line.
 * @input_h: input height for the resizer in number of lines.
 * @output_w: output width from the resizer in number of pixels per line.
 * @output_h: output height for the resizer in number of lines.
 *
 * Configures the appropriate values stored in the isp_res structure in the
 * resizer registers.
 *
 * Returns 0 if successful, or -1 if passed values haven't been verified
 * with ispresizer_try_size() previously.
 **/
int ispresizer_config_size(u32 input_w, u32 input_h, u32 output_w,
			   u32 output_h)
{
	int i, j;
	u32 res;
	DPRINTK_ISPRESZ("ispresizer_config_size()+, input_w = %d,input_h ="
			" %d, output_w = %d, output_h"
			" = %d,hresz = %d,vresz = %d,"
			" hcrop = %d, vcrop = %d,"
			" hstph = %d, vstph = %d,"
			"algo = %d\n",
			ispres_obj.inputwidth,
			ispres_obj.inputheight,
			ispres_obj.outputwidth,
			ispres_obj.outputheight,
			ispres_obj.h_resz,
			ispres_obj.v_resz,
			ispres_obj.ipwd_crop,
			ispres_obj.ipht_crop,
			ispres_obj.h_startphase,
			ispres_obj.v_startphase,
			ispres_obj.algo);
	if ((output_w != ispres_obj.outputwidth)
	    || (output_h != ispres_obj.outputheight)) {
		DPRINTK_ISPRESZ( "Output parameters passed do not match the"
		       " values calculated by the"
		       " trysize passed w %d, h %d"
		       " \n", output_w , output_h);
		return -1;
	}

	res = isp_reg_readl(OMAP3_ISP_IOMEM_RESZ, ISPRSZ_CNT) &
		~(ISPRSZ_CNT_HSTPH_MASK | ISPRSZ_CNT_VSTPH_MASK);

	isp_reg_writel(res |
		       (ispres_obj.h_startphase << ISPRSZ_CNT_HSTPH_SHIFT) |
		       (ispres_obj.v_startphase << ISPRSZ_CNT_VSTPH_SHIFT),
		       OMAP3_ISP_IOMEM_RESZ,
		       ISPRSZ_CNT);

	isp_reg_writel(0,OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_INADD);
	isp_reg_writel(0,OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_INOFF);

	isp_reg_writel(
		((ispres_obj.ipwd_crop * 2 + 1) << ISPRSZ_IN_START_HORZ_ST_SHIFT) |
		(ispres_obj.ipht_crop << ISPRSZ_IN_START_VERT_ST_SHIFT),
		OMAP3_ISP_IOMEM_RESZ, ISPRSZ_IN_START);
#if 0
	isp_reg_writel((0x00 << ISPRSZ_IN_START_HORZ_ST_SHIFT) |
		       (0x00 << ISPRSZ_IN_START_VERT_ST_SHIFT),
		       OMAP3_ISP_IOMEM_RESZ,
		       ISPRSZ_IN_START);
#endif

	isp_reg_writel((ispres_obj.inputwidth << ISPRSZ_IN_SIZE_HORZ_SHIFT) |
		       (ispres_obj.inputheight <<
			ISPRSZ_IN_SIZE_VERT_SHIFT),
		       OMAP3_ISP_IOMEM_RESZ,
		       ISPRSZ_IN_SIZE);
	if (!ispres_obj.algo) {
		isp_reg_writel((output_w << ISPRSZ_OUT_SIZE_HORZ_SHIFT) |
			       (output_h << ISPRSZ_OUT_SIZE_VERT_SHIFT),
			       OMAP3_ISP_IOMEM_RESZ,
			       ISPRSZ_OUT_SIZE);
	} else {
		isp_reg_writel(((output_w - 4) << ISPRSZ_OUT_SIZE_HORZ_SHIFT) |
			       (output_h << ISPRSZ_OUT_SIZE_VERT_SHIFT),
			       OMAP3_ISP_IOMEM_RESZ,
			       ISPRSZ_OUT_SIZE);
	}

	res = isp_reg_readl(OMAP3_ISP_IOMEM_RESZ, ISPRSZ_CNT) &
		~(ISPRSZ_CNT_HRSZ_MASK | ISPRSZ_CNT_VRSZ_MASK);
	isp_reg_writel(res |
		       ((ispres_obj.h_resz - 1) << ISPRSZ_CNT_HRSZ_SHIFT) |
		       ((ispres_obj.v_resz - 1) << ISPRSZ_CNT_VRSZ_SHIFT),
		       OMAP3_ISP_IOMEM_RESZ,
		       ISPRSZ_CNT);
	if (ispres_obj.h_resz <= MID_RESIZE_VALUE) {
		j = 0;
		for (i = 0; i < 16; i++) {
			isp_reg_writel(
				(ispres_obj.coeflist.h_filter_coef_4tap[j]
				 << ISPRSZ_HFILT10_COEF0_SHIFT) |
				(ispres_obj.coeflist.h_filter_coef_4tap[j + 1]
				 << ISPRSZ_HFILT10_COEF1_SHIFT),
				OMAP3_ISP_IOMEM_RESZ,
				ISPRSZ_HFILT10 + (i * 0x04));
			j += 2;
		}
	} else {
		j = 0;
		for (i = 0; i < 16; i++) {
			if ((i + 1) % 4 == 0) {
				isp_reg_writel((ispres_obj.coeflist.
						h_filter_coef_7tap[j] <<
						ISPRSZ_HFILT10_COEF0_SHIFT),
					       OMAP3_ISP_IOMEM_RESZ,
					       ISPRSZ_HFILT10 + (i * 0x04));
				j += 1;
			} else {
				isp_reg_writel((ispres_obj.coeflist.
						h_filter_coef_7tap[j] <<
						ISPRSZ_HFILT10_COEF0_SHIFT) |
					       (ispres_obj.coeflist.
						h_filter_coef_7tap[j+1] <<
						ISPRSZ_HFILT10_COEF1_SHIFT),
					       OMAP3_ISP_IOMEM_RESZ,
					       ISPRSZ_HFILT10 + (i * 0x04));
				j += 2;
			}
		}
	}
	if (ispres_obj.v_resz <= MID_RESIZE_VALUE) {
		j = 0;
		for (i = 0; i < 16; i++) {
			isp_reg_writel((ispres_obj.coeflist.
					v_filter_coef_4tap[j] <<
					ISPRSZ_VFILT10_COEF0_SHIFT) |
				       (ispres_obj.coeflist.
					v_filter_coef_4tap[j + 1] <<
					ISPRSZ_VFILT10_COEF1_SHIFT),
				       OMAP3_ISP_IOMEM_RESZ,
				       ISPRSZ_VFILT10 + (i * 0x04));
			j += 2;
		}
	} else {
		j = 0;
		for (i = 0; i < 16; i++) {
			if ((i + 1) % 4 == 0) {
				isp_reg_writel((ispres_obj.coeflist.
						v_filter_coef_7tap[j] <<
						ISPRSZ_VFILT10_COEF0_SHIFT),
					       OMAP3_ISP_IOMEM_RESZ,
					       ISPRSZ_VFILT10 + (i * 0x04));
				j += 1;
			} else {
				isp_reg_writel((ispres_obj.coeflist.
						v_filter_coef_7tap[j] <<
						ISPRSZ_VFILT10_COEF0_SHIFT) |
					       (ispres_obj.coeflist.
						v_filter_coef_7tap[j+1] <<
						ISPRSZ_VFILT10_COEF1_SHIFT),
					       OMAP3_ISP_IOMEM_RESZ,
					       ISPRSZ_VFILT10 + (i * 0x04));
				j += 2;
			}
		}
	}

	ispresizer_config_outlineoffset(output_w*2);
	DPRINTK_ISPRESZ("ispresizer_config_size()-\n");
	return 0;
}

/**
 * ispresizer_config_ycpos - Specifies if output should be in YC or CY format.
 * @yc: 0 - YC format, 1 - CY format
 **/
void ispresizer_config_ycpos(u8 yc)
{
	DPRINTK_ISPRESZ("ispresizer_config_ycpos()+\n");
	isp_reg_and_or(OMAP3_ISP_IOMEM_RESZ, ISPRSZ_CNT, ~ISPRSZ_CNT_YCPOS,
		       (yc ? ISPRSZ_CNT_YCPOS : 0));
	DPRINTK_ISPRESZ("ispresizer_config_ycpos()-\n");
}


/**
 * Sets the chrominance algorithm
 * @cbilin: 0 - chrominance uses same processing as luminance,
 *          1 - bilinear interpolation processing
 **/
void ispresizer_enable_cbilin(u8 enable)
{
	DPRINTK_ISPRESZ("ispresizer_enable_cbilin()+\n");
	isp_reg_and_or(OMAP3_ISP_IOMEM_RESZ, ISPRSZ_CNT, ~ISPRSZ_CNT_CBILIN,
		       (enable ? ISPRSZ_CNT_CBILIN : 0));
	DPRINTK_ISPRESZ("ispresizer_enable_cbilin()-\n");
}


/**
 * ispresizer_config_inlineoffset - Configures the read address line offset.
 * @offset: Line Offset for the input image.
 *
 * Returns 0 if successful, or -1 if offset is not 32 bits aligned.
 **/
int ispresizer_config_inlineoffset(u32 offset)
{
	DPRINTK_ISPRESZ("ispresizer_config_inlineoffset()+\n");
	if (offset % 32)
		return -1;
	isp_reg_writel(offset << ISPRSZ_SDR_INOFF_OFFSET_SHIFT,
		       OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_INOFF);
	DPRINTK_ISPRESZ("ispresizer_config_inlineoffset()-\n");
	return 0;
}

/**
 * ispresizer_set_inaddr - Sets the memory address of the input frame.
 * @addr: 32bit memory address aligned on 32byte boundary.
 *
 * Returns 0 if successful, or -1 if address is not 32 bits aligned.
 **/
int ispresizer_set_inaddr(u32 addr)
{
	DPRINTK_ISPRESZ("ispresizer_set_inaddr()+\n");
	if (addr % 32)
		return -1;
	isp_reg_writel(addr << ISPRSZ_SDR_INADD_ADDR_SHIFT,
		       OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_INADD);
	ispres_obj.tmp_buf = addr;
	DPRINTK_ISPRESZ("ispresizer_set_inaddr()-\n");
	return 0;
}

/**
 * ispresizer_config_datapath - Specifies which input to use in resizer module
 * @input: Indicates the module that gives the image to resizer.
 *
 * Sets up the default resizer configuration according to the arguments.
 *
 * Returns 0 if successful, or -1 if an unsupported input was requested.
 **/
int ispresizer_config_datapath(enum ispresizer_input input)
{
	u32 cnt = 0;
	DPRINTK_ISPRESZ("ispresizer_config_datapath()+\n");
	ispres_obj.resinput = input;
	switch (input) {
	case RSZ_OTFLY_YUV:
		cnt &= ~ISPRSZ_CNT_INPTYP;
		cnt &= ~ISPRSZ_CNT_INPSRC;
		ispresizer_set_inaddr(0);
		ispresizer_config_inlineoffset(0);
		break;
	case RSZ_MEM_YUV:
		cnt |= ISPRSZ_CNT_INPSRC;
		cnt &= ~ISPRSZ_CNT_INPTYP;
		break;
	case RSZ_MEM_COL8:
		cnt |= ISPRSZ_CNT_INPSRC;
		cnt |= ISPRSZ_CNT_INPTYP;
		break;
	default:
		DPRINTK_ISPRESZ( "ISP_ERR : Wrong Input\n");
		return -1;
	}
	isp_reg_or(OMAP3_ISP_IOMEM_RESZ, ISPRSZ_CNT, cnt);
	//ispresizer_config_ycpos(0);
	ispresizer_config_ycpos(1);
	ispresizer_config_filter_coef(&ispreszdefcoef);
	ispresizer_enable_cbilin(0);
	ispresizer_config_luma_enhance(&ispreszdefaultyenh);
	DPRINTK_ISPRESZ("ispresizer_config_datapath()-\n");
	return 0;
}

void ispresizer_enable(int enable)
{
	int val;
	//DPRINTK_ISPRESZ("+ispresizer_enable()+\n");
	if (enable) {
		val = (isp_reg_readl(OMAP3_ISP_IOMEM_RESZ, ISPRSZ_PCR) & 0x2) |
			ISPRSZ_PCR_ENABLE;
	} else {
		val = isp_reg_readl(OMAP3_ISP_IOMEM_RESZ, ISPRSZ_PCR) &
			~ISPRSZ_PCR_ENABLE;
	}
	isp_reg_writel(val, OMAP3_ISP_IOMEM_RESZ, ISPRSZ_PCR);
	//DPRINTK_ISPRESZ("+ispresizer_enable()-\n");
}

void ispresizer_init(void)
{
	mmio_base[OMAP3_ISP_IOMEM_MAIN] = 0x480BC000;
	mmio_base[OMAP3_ISP_IOMEM_CCDC] = 0x480BC600;
	mmio_base[OMAP3_ISP_IOMEM_RESZ] = 0x480BD000;

	ispresizer_config_filter_coef(&ispreszdefcoef);
}
