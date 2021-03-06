/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Implementation of the Generic ESC/P Dot-Matrix printer.
 *
 * Version:	@(#)prt_escp.c	1.0.1	2018/09/02
 *
 * Authors:	Michael Dr�ing, <michael@drueing.de>
 *		Fred N. van Kempen, <decwiz@yahoo.com>
 *
 *		Based on code by Frederic Weymann (originally for DosBox.)
 *
 *		Copyright 2018 Michael Dr�ing.
 *		Copyright 2018 Fred N. van Kempen.
 *
 *		Redistribution and  use  in source  and binary forms, with
 *		or  without modification, are permitted  provided that the
 *		following conditions are met:
 *
 *		1. Redistributions of  source  code must retain the entire
 *		   above notice, this list of conditions and the following
 *		   disclaimer.
 *
 *		2. Redistributions in binary form must reproduce the above
 *		   copyright  notice,  this list  of  conditions  and  the
 *		   following disclaimer in  the documentation and/or other
 *		   materials provided with the distribution.
 *
 *		3. Neither the  name of the copyright holder nor the names
 *		   of  its  contributors may be used to endorse or promote
 *		   products  derived from  this  software without specific
 *		   prior written permission.
 *
 * THIS SOFTWARE  IS  PROVIDED BY THE  COPYRIGHT  HOLDERS AND CONTRIBUTORS
 * "AS IS" AND  ANY EXPRESS  OR  IMPLIED  WARRANTIES,  INCLUDING, BUT  NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE  ARE  DISCLAIMED. IN  NO  EVENT  SHALL THE COPYRIGHT
 * HOLDER OR  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES;  LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON  ANY
 * THEORY OF  LIABILITY, WHETHER IN  CONTRACT, STRICT  LIABILITY, OR  TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  IN ANY  WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <math.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "../../emu.h"
#include "../../cpu/cpu.h"
#include "../../machines/machine.h"
#include "../../timer.h"
#include "../../mem.h"
#include "../../rom.h" 
#include "../../plat.h" 
#include "../../png.h"
#include "../ports/parallel_dev.h"
#include "printer.h"


/* Default page values (for now.) */
#define PAGE_WIDTH	8.5			/* standard U.S. Letter */
#define PAGE_HEIGHT	11
#define PAGE_LMARGIN	0.0
#define PAGE_RMARGIN	PAGE_WIDTH
#define PAGE_TMARGIN	0.0
#define PAGE_BMARGIN	PAGE_HEIGHT
#define PAGE_DPI	360
#define PAGE_CPI	10.0			/* standard 10 cpi */
#define PAGE_LPI	6.0			/* standard 6 lpi */


#define PATH_FREETYPE_DLL	"freetype.dll"


/* FreeType library handles - global so they can be shared. */
FT_Library	ft_lib = NULL;
void		*ft_handle = NULL;

static int	(*ft_Init_FreeType)(FT_Library *alibrary);
static int	(*ft_Done_Face)(FT_Face face);
static int	(*ft_New_Face)(FT_Library library, const char *filepathname, 
			       FT_Long face_index, FT_Face *aface);
static int	(*ft_Set_Char_Size)(FT_Face face, FT_F26Dot6 char_width,
				    FT_F26Dot6 char_height,
				    FT_UInt horz_resolution,
				    FT_UInt vert_resolution);
static int	(*ft_Set_Transform)(FT_Face face, FT_Matrix *matrix,
				    FT_Vector *delta);
static int	(*ft_Get_Char_Index)(FT_Face face, FT_ULong charcode);
static int	(*ft_Load_Glyph)(FT_Face face, FT_UInt glyph_index,
				 FT_Int32 load_flags);
static int	(*ft_Render_Glyph)(FT_GlyphSlot slot,
				   FT_Render_Mode render_mode);


static const dllimp_t ft_imports[] = {
  { "FT_Init_FreeType",		&ft_Init_FreeType	},
  { "FT_New_Face",		&ft_New_Face		},
  { "FT_Done_Face",		&ft_Done_Face		},
  { "FT_Set_Char_Size",		&ft_Set_Char_Size	},
  { "FT_Set_Transform",		&ft_Set_Transform	},
  { "FT_Get_Char_Index",	&ft_Get_Char_Index	},
  { "FT_Load_Glyph",		&ft_Load_Glyph		},
  { "FT_Render_Glyph",		&ft_Render_Glyph	},
  { NULL,			NULL			}
};


/* The fonts. */
#define FONT_DEFAULT		0
#define FONT_ROMAN		1
#define FONT_SANSSERIF		2
#define FONT_COURIER		3
#define FONT_SCRIPT		4
#define FONT_OCRA		5
#define FONT_OCRB		6

/* Font styles. */
#define STYLE_PROP		0x0001
#define STYLE_CONDENSED		0x0002
#define STYLE_BOLD		0x0004
#define STYLE_DOUBLESTRIKE	0x0008
#define STYLE_DOUBLEWIDTH	0x0010
#define STYLE_ITALICS		0x0020
#define STYLE_UNDERLINE		0x0040
#define STYLE_SUPERSCRIPT	0x0080
#define STYLE_SUBSCRIPT		0x0100
#define STYLE_STRIKETHROUGH	0x0200
#define STYLE_OVERSCORE		0x0400
#define STYLE_DOUBLEWIDTHONELINE 0x0800
#define STYLE_DOUBLEHEIGHT	0x1000

/* Underlining styles. */
#define SCORE_NONE		0x00
#define SCORE_SINGLE		0x01
#define SCORE_DOUBLE		0x02
#define SCORE_SINGLEBROKEN	0x05
#define SCORE_DOUBLEBROKEN	0x06

/* Print quality. */
#define QUALITY_DRAFT		0x01
#define QUALITY_LQ		0x02

/* Typefaces. */
#define TYPEFACE_ROMAN		0
#define TYPEFACE_SANSSERIF	1
#define TYPEFACE_COURIER	2
#define TYPEFACE_PRESTIGE	3
#define TYPEFACE_SCRIPT		4
#define TYPEFACE_OCRB		5
#define TYPEFACE_OCRA		6
#define TYPEFACE_ORATOR		7
#define TYPEFACE_ORATORS	8
#define TYPEFACE_SCRIPTC	9
#define TYPEFACE_ROMANT		10
#define TYPEFACE_SANSSERIFH	11
#define TYPEFACE_SVBUSABA	30
#define TYPEFACE_SVJITTRA	31


/* Some helper macros. */
#define PARAM16(x)		(dev->esc_parms[x+1] * 256 + dev->esc_parms[x])
#define PIXX			(floor(dev->curr_x * dev->dpi + 0.5))
#define PIXY			(floor(dev->curr_y * dev->dpi + 0.5))


typedef struct {
    int8_t	dirty;		/* has the page been printed on? */
    char	pad;

    uint16_t	w;		/* size and pitch info */
    uint16_t	h;
    uint16_t	pitch;

    uint8_t	*pixels;	/* grayscale pixel data */
} psurface_t;


typedef struct {
    const char	*name;

    /* page data (TODO: make configurable) */
    double	page_width,	/* all in inches */
		page_height,
		left_margin,
		top_margin, 
		right_margin,
		bottom_margin;
    uint16_t	dpi;
    double	cpi;		/* defined chars per inch */
    double	lpi;		/* defined lines per inch */

    /* font data */
    double	actual_cpi;	/* actual cpi as with current font */
    double	linespacing;	/* in inch */
    double	hmi;		/* hor. motion index (inch); overrides CPI */

    /* tabstops */
    double	horizontal_tabs[32];
    int16_t	num_horizontal_tabs;
    double	vertical_tabs[16];
    int16_t	num_vertical_tabs;

    /* bit graphics data */
    uint16_t	bg_h_density;		/* in dpi */
    uint16_t	bg_v_density;		/* in dpi */
    int8_t	bg_adjacent;		/* print adjacent pixels (ignored) */
    uint8_t	bg_bytes_per_column;
    uint16_t	bg_remaining_bytes;	/* #bytes left before img is complete */
    uint8_t	bg_column[6];		/* #bytes of the current and last col */
    uint8_t	bg_bytes_read;		/* #bytes read so far for current col */

    /* handshake data */
    uint8_t	data;
    int8_t	ack;
    int8_t	select;
    int8_t	busy;
    int8_t	int_pending;
    int8_t	error;
    int8_t	autofeed;

    /* ESC command data */
    int8_t	esc_seen;		/* set to 1 if an ESC char was seen */
    uint16_t	esc_pending;		/* in which ESC command are we */
    uint8_t	esc_parms_req;
    uint8_t	esc_parms_curr;
    uint8_t	esc_parms[10];		/* 10 should be enough for everybody */

    /* internal page data */
    wchar_t	fontpath[1024];
    wchar_t	pagepath[1024];
    psurface_t	*page;
    double	curr_x, curr_y;		/* print head position (inch) */
    uint16_t	current_font;
    FT_Face	fontface;
    int8_t	lq_typeface;
    int8_t	font_style;
    int8_t	print_quality;
    uint8_t	font_score;
    double	extra_intra_space;	/* extra spacing between chars (inch) */

    /* other internal data */
    uint16_t	char_tables[4];		/* the character tables for ESC t */
    uint16_t	curr_char_table;	/* the active char table index */
    uint16_t	curr_cpmap[256];	/* current ASCII->Unicode map table */

    int8_t	multipoint_mode;	/* multipoint mode, ESC X */
    double	multipoint_size;	/* size of font, in points */
    double	multipoint_cpi;		/* chars per inch in multipoint mode */

    uint8_t	density_k;		/* density modes for ESC K/L/Y/Z */
    uint8_t	density_l;
    uint8_t	density_y;
    uint8_t	density_z;

    int8_t	print_upper_control;	/* ESC 6, ESC 7 */
    int8_t	print_everything_count;	/* for ESC ( ^ */

    double	defined_unit;		/* internal unit for some ESC/P
					 * commands. -1 = use default */

    int8_t	msb;			/* MSB mode, -1 = off */
} escp_t;


/* Codepage table, needed for ESC t ( */
static const uint16_t codepages[15] = {
      0, 437, 932, 850, 851, 853, 855, 860,
    863, 865, 852, 857, 862, 864, 866
};


/* "patches" to the codepage for the international charsets 
 * these bytes patch the following 12 positions of the char table, in order:
 * 0x23  0x24  0x40  0x5b  0x5c  0x5d  0x5e  0x60  0x7b  0x7c  0x7d  0x7e 
 * TODO: Implement the missing international charsets
 */
static const uint16_t intCharSets[15][12] = {
    { 0x0023, 0x0024, 0x0040, 0x005b, 0x005c, 0x005d,	/* 0 USA */
      0x005e, 0x0060, 0x007b, 0x007c, 0x007d, 0x007e	},

    { 0x0023, 0x0024, 0x00e0, 0x00ba, 0x00e7, 0x00a7,	/* 1 France */
      0x005e, 0x0060, 0x00e9, 0x00f9, 0x00e8, 0x00a8	},

    { 0x0023, 0x0024, 0x00a7, 0x00c4, 0x00d6, 0x00dc,	/* 2 Germany */
      0x005e, 0x0060, 0x00e4, 0x00f6, 0x00fc, 0x00df	},

    { 0x00a3, 0x0024, 0x0040, 0x005b, 0x005c, 0x005d,	/* 3 UK */
      0x005e, 0x0060, 0x007b, 0x007c, 0x007d, 0x007e	},

    { 0x0023, 0x0024, 0x0040, 0x00c6, 0x00d8, 0x00c5,	/* 4 Denmark (1) */
      0x005e, 0x0060, 0x00e6, 0x00f8, 0x00e5, 0x007e	},

    { 0x0023, 0x00a4, 0x00c9, 0x00c4, 0x00d6, 0x00c5,	/* 5 Sweden */
      0x00dc, 0x00e9, 0x00e4, 0x00f6, 0x00e5, 0x00fc	},

    { 0x0023, 0x0024, 0x0040, 0x00ba, 0x005c, 0x00e9,	/* 6 Italy */
      0x005e, 0x00f9, 0x00e0, 0x00f2, 0x00e8, 0x00ec	},

    { 0x0023, 0x0024, 0x0040, 0x005b, 0x005c, 0x005d,	/* 7 Spain 1 */
      0x005e, 0x0060, 0x007b, 0x007c, 0x007d, 0x007e	}, /* TODO */

    { 0x0023, 0x0024, 0x0040, 0x005b, 0x005c, 0x005d,	/* 8 Japan (English) */
      0x005e, 0x0060, 0x007b, 0x007c, 0x007d, 0x007e	}, /* TODO */

    { 0x0023, 0x0024, 0x0040, 0x005b, 0x005c, 0x005d,	/* 9 Norway */
      0x005e, 0x0060, 0x007b, 0x007c, 0x007d, 0x007e	}, /* TODO */

    { 0x0023, 0x0024, 0x0040, 0x005b, 0x005c, 0x005d,	/* 10 Denmark (2) */
      0x005e, 0x0060, 0x007b, 0x007c, 0x007d, 0x007e	}, /* TODO */

    { 0x0023, 0x0024, 0x0040, 0x005b, 0x005c, 0x005d,	/* 11 Spain (2) */
      0x005e, 0x0060, 0x007b, 0x007c, 0x007d, 0x007e	}, /* TODO */

    { 0x0023, 0x0024, 0x0040, 0x005b, 0x005c, 0x005d,	/* 12 Latin America */
      0x005e, 0x0060, 0x007b, 0x007c, 0x007d, 0x007e	}, /* TODO */

    { 0x0023, 0x0024, 0x0040, 0x005b, 0x005c, 0x005d,	/* 13 Korea */
      0x005e, 0x0060, 0x007b, 0x007c, 0x007d, 0x007e	}, /* TODO */

    { 0x0023, 0x0024, 0x00a7, 0x00c4, 0x0027, 0x0022,	/* 14 Legal */
      0x00b6, 0x0060, 0x00a9, 0x00ae, 0x2020, 0x2122	}
};


/* Select a ASCII->Unicode mapping by CP number */
static void
init_codepage(escp_t *dev, uint16_t num)
{
    const uint16_t *cp;

    /* Get the codepage map for this number. */
    cp = select_codepage(num);

    /* Copy the map over since it might get modified later. */
    memcpy(dev->curr_cpmap, cp, 256 * sizeof(uint16_t));
}


static void
update_font(escp_t *dev)
{
    wchar_t path[1024];
    const wchar_t *fn;
    char temp[1024];
    FT_Matrix matrix;
    double hpoints = 10.5;
    double vpoints = 10.5;

    /* We need the FreeType library. */
    if (ft_lib == NULL) return;

    /* Release current font if we have one. */
    if (dev->fontface)
	ft_Done_Face(dev->fontface);

    if (dev->print_quality == QUALITY_DRAFT) {
#ifdef FONT_FILE_DOTMATRIX
	fn = FONT_FILE_DOTMATRIX;
#else
	fn = FONT_FILE_COURIER;
#endif
    } else switch (dev->lq_typeface) {
	case TYPEFACE_ROMAN:
		fn = FONT_FILE_ROMAN;
		break;

	case TYPEFACE_SANSSERIF:
		fn = FONT_FILE_SANSSERIF;
		break;

	case TYPEFACE_COURIER:
		fn = FONT_FILE_COURIER;
		break;

	case TYPEFACE_SCRIPT:
		fn = FONT_FILE_SCRIPT;
		break;

	case TYPEFACE_OCRA:
		fn = FONT_FILE_OCRA;
		break;

	case TYPEFACE_OCRB:
		fn = FONT_FILE_OCRB;
		break;

	default:
#ifdef FONT_FILE_DOTMATRIX
		fn = FONT_FILE_DOTMATRIX;
#else
		fn = FONT_FILE_COURIER;
#endif
    }

    /* Create a full pathname for the ROM file. */
    wcscpy(path, dev->fontpath);
    wcscat(path, fn);

    /* Convert (back) to ANSI for the FreeType API. */
    wcstombs(temp, path, sizeof(temp));

    /* Load the new font. */
    if (ft_New_Face(ft_lib, temp, 0, &dev->fontface)) {
	pclog("ESC/P: unable to load font '%s'\n", temp);
	pclog("ESC/P: text printing disabled\n");
	dev->fontface = 0;
    }

    if (dev->multipoint_mode == 0) {
	dev->actual_cpi = dev->cpi;

	if ((dev->cpi != 10.0) && !(dev->font_style & STYLE_CONDENSED)) {
		hpoints *= 10.0 / dev->cpi;
		vpoints *= 10.0 / dev->cpi;
	}

	if (! (dev->font_style & STYLE_PROP)) {
		if ((dev->cpi == 10.0) && (dev->font_style & STYLE_CONDENSED)) {
			dev->actual_cpi = 17.14;
			hpoints *= 10.0 / 17.14;
			vpoints *= 10.0 / 17.14;
		}

		if ((dev->cpi == 12) && (dev->font_style & STYLE_CONDENSED)) {
			dev->actual_cpi = 20.0;
			hpoints *= 10.0 / 20.0;
			vpoints *= 10.0 / 20.0;
		}
	}

	if (dev->font_style & (STYLE_PROP | STYLE_CONDENSED)) {
		hpoints /= 2.0;
		vpoints /= 2.0;
	}

	if ((dev->font_style & STYLE_DOUBLEWIDTH) ||
	    (dev->font_style & STYLE_DOUBLEWIDTHONELINE)) {
		dev->actual_cpi /= 2.0;
		hpoints *= 2.0;
	}

	if (dev->font_style & STYLE_DOUBLEHEIGHT)
		vpoints *= 2.0;
    } else {
	/* Multipoint mode. */
	dev->actual_cpi = dev->multipoint_cpi;
	hpoints = vpoints = dev->multipoint_size;
    }

    if ((dev->font_style & STYLE_SUPERSCRIPT) ||
	(dev->font_style & STYLE_SUBSCRIPT)) {
	hpoints *= 2.0 / 3.0;
	vpoints *= 2.0 / 3.0;
	dev->actual_cpi /= 2.0 / 3.0;
    }

    ft_Set_Char_Size(dev->fontface,
		     (uint16_t)(hpoints * 64),
		     (uint16_t)(vpoints * 64),
		     dev->dpi, dev->dpi);

    if ((dev->font_style & STYLE_ITALICS) ||
	(dev->char_tables[dev->curr_char_table] == 0)) {
	/* Italics transformation. */
	matrix.xx = 0x10000L;
	matrix.xy = (FT_Fixed)(0.20 * 0x10000L);
	matrix.yx = 0;
	matrix.yy = 0x10000L;
	ft_Set_Transform(dev->fontface, &matrix, 0);
    }
}


static int
dump_pgm(const wchar_t *fn, int inv, uint8_t *pix, int16_t w, int16_t h)
{
    int16_t x, y;
    uint8_t b;
    FILE *fp;

    /* Create the image file. */
    fp = plat_fopen(fn, L"wb");
    if (fp == NULL) {
	pclog("ESC/P: unable to create print page '%ls'\n", fn);
	return(0);
    }

    fprintf(fp, "P5 %d %d 255\n", w, h);

    for (y = 0; y < h; y++) {
	for (x = 0; x < w; x++) {
		if (inv)
			b = 255 - pix[(y * w) + x];
		  else
			b = pix[(y * w) + x];
		fputc((char)b, fp);
	}
    }

    /* All done, close the file. */
    fclose(fp);

    return(1);
}


/* Dump the current page into a formatted file. */
static void 
dump_page(escp_t *dev)
{
    wchar_t path[1024];
    wchar_t temp[128];

    wcscpy(path, dev->pagepath);
#ifdef USE_LIBPNG
    /* Try PNG first. */
    plat_tempfile(temp, NULL, L".png");
    wcscat(path, temp);
    if (! png_write_gray(path, 1, dev->page->pixels, dev->page->w, dev->page->h)) {
	wcscpy(path, dev->pagepath);
#endif
	plat_tempfile(temp, NULL, L".pgm");
	wcscat(path, temp);
	dump_pgm(path, 1, dev->page->pixels, dev->page->w, dev->page->h);
#ifdef USE_LIBPNG
    }
#endif
}


static void
new_page(escp_t *dev)
{
    /* Dump the current page if needed. */
    if (dev->page->dirty)
	dump_page(dev);

    /* Clear page. */
    dev->curr_y = dev->top_margin;
    dev->page->dirty = 0;
    memset(dev->page->pixels, 0x00, dev->page->h * dev->page->pitch);
}


static void
reset_printer(escp_t *dev)
{
    int16_t i;

    /* TODO: these should be configurable. */
    dev->page_width = PAGE_WIDTH;
    dev->page_height = PAGE_HEIGHT;
    dev->left_margin = PAGE_LMARGIN;
    dev->right_margin = PAGE_RMARGIN;
    dev->top_margin = PAGE_TMARGIN;
    dev->bottom_margin = PAGE_BMARGIN;
    dev->dpi = PAGE_DPI;
    dev->cpi = PAGE_CPI;
    dev->lpi = PAGE_LPI;

    dev->hmi = -1.0;
    dev->curr_x = dev->curr_y = 0.0;
    dev->linespacing = 1.0 / dev->lpi;

    dev->char_tables[0] = 0; /* italics */
    dev->char_tables[1] = dev->char_tables[2] = dev->char_tables[3] = 437; /* all other tables use CP437 */
    dev->curr_char_table = 1;
    init_codepage(dev, dev->char_tables[dev->curr_char_table]);

    dev->num_horizontal_tabs = 32;
    for (i = 0; i < 32; i++)
	dev->horizontal_tabs[i] = i * 8.0 / dev->cpi;
    dev->num_vertical_tabs = -1;

    dev->current_font = FONT_COURIER;
    dev->lq_typeface = TYPEFACE_COURIER;
    dev->fontface = 0;
    dev->multipoint_mode = 0;
    dev->multipoint_size = 0.0;
    dev->multipoint_cpi = 0.0;
    dev->font_style = 0;
    dev->font_score = 0;
    dev->print_quality = QUALITY_DRAFT;

    dev->bg_h_density = dev->bg_v_density = 0;
    dev->bg_adjacent = 0;
    dev->bg_bytes_per_column = dev->bg_bytes_read = 0;
    dev->bg_remaining_bytes = 0;
    memset(dev->bg_column, 0x00, sizeof(dev->bg_column));

    dev->esc_seen = 0;
    dev->esc_pending = 0;
    dev->esc_parms_req = dev->esc_parms_curr = 0;
    memset(dev->esc_parms, 0x00, sizeof(dev->esc_parms));

    dev->msb = -1;
    dev->print_everything_count = 0;
    dev->print_upper_control = 0;
    if (dev->page != NULL)
	dev->page->dirty = 0;
    dev->extra_intra_space = 0.0;
    dev->defined_unit = -1.0;
    dev->density_k = 0;
    dev->density_l = 1;
    dev->density_y = 2;
    dev->density_z = 3;

    update_font(dev);

    pclog("ESC/P: width=%.1fin,height=%.1fin dpi=%i cpi=%i lpi=%i\n",
	dev->page_width, dev->page_height,
	(int)dev->dpi, (int)dev->cpi, (int)dev->lpi);
}


static void
setup_bit_image(escp_t *dev, uint8_t density, uint16_t num_columns)
{
    switch (density) {
	case 0:
		dev->bg_h_density = 60;
		dev->bg_v_density = 60;
		dev->bg_adjacent = 1;
		dev->bg_bytes_per_column = 1;
		break;

	case 1:
		dev->bg_h_density = 120;
		dev->bg_v_density = 60;
		dev->bg_adjacent = 1;
		dev->bg_bytes_per_column = 1;
		break;

	case 2:
		dev->bg_h_density = 120;
		dev->bg_v_density = 60;
		dev->bg_adjacent = 0;
		dev->bg_bytes_per_column = 1;
		break;

	case 3:
		dev->bg_h_density = 60;
		dev->bg_v_density = 240;
		dev->bg_adjacent = 0;
		dev->bg_bytes_per_column = 1;
		break;

	case 4:
		dev->bg_h_density = 80;
		dev->bg_v_density = 60;
		dev->bg_adjacent = 1;
		dev->bg_bytes_per_column = 1;
		break;

	case 6:
		dev->bg_h_density = 90;
		dev->bg_v_density = 60;
		dev->bg_adjacent = 1;
		dev->bg_bytes_per_column = 1;
		break;

	case 32:
		dev->bg_h_density = 60;
		dev->bg_v_density = 180;
		dev->bg_adjacent = 1;
		dev->bg_bytes_per_column = 3;
		break;

	case 33:
		dev->bg_h_density = 120;
		dev->bg_v_density = 180;
		dev->bg_adjacent = 1;
		dev->bg_bytes_per_column = 3;
		break;

	case 38:
		dev->bg_h_density = 90;
		dev->bg_v_density = 180;
		dev->bg_adjacent = 1;
		dev->bg_bytes_per_column = 3;
		break;

	case 39:
		dev->bg_h_density = 180;
		dev->bg_v_density = 180;
		dev->bg_adjacent = 1;
		dev->bg_bytes_per_column = 3;
		break;

	case 40:
		dev->bg_h_density = 360;
		dev->bg_v_density = 180;
		dev->bg_adjacent = 0;
		dev->bg_bytes_per_column = 3;
		break;

	case 71:
		dev->bg_h_density = 180;
		dev->bg_v_density = 360;
		dev->bg_adjacent = 1;
		dev->bg_bytes_per_column = 6;
		break;

	case 72:
		dev->bg_h_density = 360;
		dev->bg_v_density = 360;
		dev->bg_adjacent = 0;
		dev->bg_bytes_per_column = 6;
		break;

	case 73:
		dev->bg_h_density = 360;
		dev->bg_v_density = 360;
		dev->bg_adjacent = 1;
		dev->bg_bytes_per_column = 6;
		break;

	default:
		pclog("ESC/P: Unsupported bit image density %d.\n", density);
    }

    dev->bg_remaining_bytes = num_columns * dev->bg_bytes_per_column;
    dev->bg_bytes_read = 0;
}


/* This is the actual ESC/P interpreter. */
static int
process_char(escp_t *dev, uint8_t ch)
{
    double new_x, new_y;
    double move_to;
    double unit_size;
    uint16_t rel_move;
    int16_t i;

    /* Determine number of additional command params that are expected. */
    if (dev->esc_seen) {
	dev->esc_seen = 0;
	dev->esc_pending = ch;
	dev->esc_parms_curr = 0;

	switch (dev->esc_pending) {
		case 0x02: // Undocumented
		case 0x0e: // Select double-width printing (one line) (ESC SO)		
		case 0x0f: // Select condensed printing (ESC SI)
		case 0x23: // Cancel MSB control (ESC #)
		case 0x30: // Select 1/8-inch line spacing (ESC 0)
		case 0x32: // Select 1/6-inch line spacing (ESC 2)
		case 0x34: // Select italic font (ESC 4)
		case 0x35: // Cancel italic font (ESC 5)
		case 0x36: // Enable printing of upper control codes (ESC 6)
		case 0x37: // Enable upper control codes (ESC 7)
		case 0x3c: // Unidirectional mode (one line) (ESC <)
		case 0x3d: // Set MSB to 0 (ESC =)
		case 0x3e: // Set MSB to 1 (ESC >)
		case 0x40: // Initialize printer (ESC @)
		case 0x45: // Select bold font (ESC E)
		case 0x46: // Cancel bold font (ESC F)
		case 0x47: // Select double-strike printing (ESC G)
		case 0x48: // Cancel double-strike printing (ESC H)
		case 0x4d: // Select 10.5-point, 12-cpi (ESC M)
		case 0x4f: // Cancel bottom margin			
		case 0x50: // Select 10.5-point, 10-cpi (ESC P)
		case 0x54: // Cancel superscript/subscript printing (ESC T)
		case 0x67: // Select 10.5-point, 15-cpi (ESC g)
		case 0x73: // Select low-speed mode (ESC s)
			dev->esc_parms_req = 0;
			break;

		case 0x19: // Control paper loading/ejecting (ESC EM)
		case 0x20: // Set intercharacter space (ESC SP)
		case 0x21: // Master select (ESC !)
		case 0x2b: // Set n/360-inch line spacing (ESC +)
		case 0x2d: // Turn underline on/off (ESC -)
		case 0x2f: // Select vertical tab channel (ESC /)
		case 0x33: // Set n/180-inch line spacing (ESC 3)
		case 0x41: // Set n/60-inch line spacing
		case 0x43: // Set page length in lines (ESC C)
		case 0x4a: // Advance print position vertically (ESC J n)
		case 0x4e: // Set bottom margin (ESC N)
		case 0x51: // Set right margin (ESC Q)
		case 0x52: // Select an international character set (ESC R)
		case 0x53: // Select superscript/subscript printing (ESC S)
		case 0x55: // Turn unidirectional mode on/off (ESC U)
		case 0x57: // Turn double-width printing on/off (ESC W)
		case 0x61: // Select justification (ESC a)
		case 0x6b: // Select typeface (ESC k)
		case 0x6c: // Set left margin (ESC 1)
		case 0x70: // Turn proportional mode on/off (ESC p)
		case 0x72: // Select printing color (ESC r)
		case 0x74: // Select character table (ESC t)
		case 0x77: // Turn double-height printing on/off (ESC w)
		case 0x78: // Select LQ or draft (ESC x)
			dev->esc_parms_req = 1;
			break;

		case 0x24: // Set absolute horizontal print position (ESC $)
		case 0x3f: // Reassign bit-image mode (ESC ?)
		case 0x4b: // Select 60-dpi graphics (ESC K)
		case 0x4c: // Select 120-dpi graphics (ESC L)
		case 0x59: // Select 120-dpi, double-speed graphics (ESC Y)
		case 0x5a: // Select 240-dpi graphics (ESC Z)
		case 0x5c: // Set relative horizontal print position (ESC \)
		case 0x63: // Set horizontal motion index (HMI) (ESC c)
			dev->esc_parms_req = 2;
			break;

		case 0x2a: // Select bit image (ESC *)
		case 0x58: // Select font by pitch and point (ESC X)
			dev->esc_parms_req = 3;
			break;

		case 0x62: // Set vertical tabs in VFU channels (ESC b)
		case 0x42: // Set vertical tabs (ESC B)
			dev->num_vertical_tabs = 0;
			return 1;

		case 0x44: // Set horizontal tabs (ESC D)
			dev->num_horizontal_tabs = 0;
			return 1;

		case 0x25: // Select user-defined set (ESC %)
		case 0x26: // Define user-defined characters (ESC &)
		case 0x3a: // Copy ROM to RAM (ESC :)
			pclog("ESC/P: User-defined characters not supported.\n");
			return 1;

		case 0x28: // Two bytes sequence
			/* return and wait for second ESC byte */
			return 1;

		default:
			pclog("ESC/P: Unknown command ESC %c (0x%02x). Unable to skip parameters.\n", 
				dev->esc_pending >= 0x20 ? dev->esc_pending : '?',
				dev->esc_pending);
			dev->esc_parms_req = 0;
			dev->esc_pending = 0;
			return 1;
	}

	if (dev->esc_parms_req > 0) {
		/* return and wait for parameters to appear */
		return 1;
	}
    }

    /* parameter checking for the 2-byte ESC/P2 commands */
    if (dev->esc_pending == 0x28) {
	dev->esc_pending = 0x0200 + ch;

	switch (dev->esc_pending) {
		case 0x0242: // Bar code setup and print (ESC (B)
		case 0x025e: // Print data as characters (ESC (^)
			dev->esc_parms_req = 2;
			break;

		case 0x0255: // Set unit (ESC (U)
			dev->esc_parms_req = 3;
			break;

		case 0x0243: // Set page length in defined unit (ESC (C)
		case 0x0256: // Set absolute vertical print position (ESC (V)
		case 0x0276: // Set relative vertical print position (ESC (v)
			dev->esc_parms_req = 4;
			break;

		case 0x0228: // Assign character table (ESC (t)
		case 0x022d: // Select line/score (ESC (-)
			dev->esc_parms_req = 5;
			break;

		case 0x0263: // Set page format (ESC (c)
			dev->esc_parms_req = 6;
			break;

		default:
			// ESC ( commands are always followed by a "number of parameters" word parameter
			pclog("ESC/P: Skipping unsupported extended command ESC ( %c (0x%02x).\n", 
				dev->esc_pending >= 0x20 ? dev->esc_pending : '?',
				dev->esc_pending);
			dev->esc_parms_req = 2;
			dev->esc_pending = 0x101; /* dummy value to be checked later */
			return 1;
	}

	/* If we need parameters, return and wait for them to appear. */
	if (dev->esc_parms_req > 0) return 1;
    }

    /* Ignore VFU channel setting. */
    if (dev->esc_pending == 0x62) {
	dev->esc_pending = 0x42;
	return 1;
    }

    /* Collect vertical tabs. */
    if (dev->esc_pending == 0x42) {
	/* check if we're done */
	if ((ch == 0) || 
	    (dev->num_vertical_tabs > 0 && dev->vertical_tabs[dev->num_vertical_tabs - 1] > (double)ch * dev->linespacing)) {
		dev->esc_pending = 0;
	} else {
		if (dev->num_vertical_tabs < 16) {
			dev->vertical_tabs[dev->num_vertical_tabs++] = (double)ch * dev->linespacing;
		}
	}
    }

    /* Collect horizontal tabs. */
    if (dev->esc_pending == 0x44) {
	/* check if we're done... */
	if ((ch == 0) || 
	    (dev->num_horizontal_tabs > 0 && dev->horizontal_tabs[dev->num_horizontal_tabs - 1] > (double)ch * (1.0 / dev->cpi))) {
		dev->esc_pending = 0;
	} else {
		if (dev->num_horizontal_tabs < 32) {
			dev->horizontal_tabs[dev->num_horizontal_tabs++] = (double)ch * (1.0 / dev->cpi);
		}
	}
    }

    /* Check if we're still collecting parameters for the current command. */
    if (dev->esc_parms_curr < dev->esc_parms_req) {
	/* store current parameter */
	dev->esc_parms[dev->esc_parms_curr++] = ch;

	/* do we still need to continue collecting parameters? */
	if (dev->esc_parms_curr < dev->esc_parms_req)
		return 1;
    }

    /* Handle the pending ESC command. */
    if (dev->esc_pending != 0) {
	switch (dev->esc_pending) {
		case 0x02:	/* undocumented; ignore */
			break;

		case 0x0e:	/* select double-width (one line) (ESC SO) */
			if (! dev->multipoint_mode) {
				dev->hmi = -1;
				dev->font_style |= STYLE_DOUBLEWIDTHONELINE;
				update_font(dev);
			}
			break;

		case 0x0f:	/* select condensed printing (ESC SI) */
			if (! dev->multipoint_mode) {
				dev->hmi = -1;
				dev->font_style |= STYLE_CONDENSED;
				update_font(dev);
			}
			break;

		case 0x19:	/* control paper loading/ejecting (ESC EM) */
				/* We are not really loading paper, so most
				 * commands can be ignored */
			if (dev->esc_parms[0] == 'R')
				new_page(dev);

			break;
		case 0x20:	/* set intercharacter space (ESC SP) */
			if (! dev->multipoint_mode) {
				dev->extra_intra_space = (double)dev->esc_parms[0] / (dev->print_quality == QUALITY_DRAFT ? 120.0 : 180.0);
				dev->hmi = -1;
				update_font(dev);
			}
			break;

		case 0x21:	/* master select (ESC !) */
			dev->cpi = dev->esc_parms[0] & 0x01 ? 12.0 : 10.0;

			/* Reset first seven bits. */
			dev->font_style &= ~0x7f;
			if (dev->esc_parms[0] & 0x02)
				dev->font_style |= STYLE_PROP;
			if (dev->esc_parms[0] & 0x04)
				dev->font_style |= STYLE_CONDENSED;
			if (dev->esc_parms[0] & 0x08)
				dev->font_style |= STYLE_BOLD;
			if (dev->esc_parms[0] & 0x10)
				dev->font_style |= STYLE_DOUBLESTRIKE;
			if (dev->esc_parms[0] & 0x20)
				dev->font_style |= STYLE_DOUBLEWIDTH;
			if (dev->esc_parms[0] & 0x40)
				dev->font_style |= STYLE_ITALICS;
			if (dev->esc_parms[0] & 0x80) {
				dev->font_score = SCORE_SINGLE;
				dev->font_style |= STYLE_UNDERLINE;
			}

			dev->hmi = -1;
			dev->multipoint_mode = 0;
			update_font(dev);
			break;

		case 0x23:	/* cancel MSB control (ESC #) */
			dev->msb = 255;
			break;

		case 0x24:	/* set abs horizontal print position (ESC $) */
			unit_size = dev->defined_unit;
			if (unit_size < 0)
				unit_size = 60.0;

			new_x = dev->left_margin + (double)PARAM16(0) / unit_size;
			if (new_x <= dev->right_margin)
				dev->curr_x = new_x;
			break;

		case 0x2a:	/* select bit image (ESC *) */
			setup_bit_image(dev, dev->esc_parms[0], PARAM16(1));
			break;

		case 0x2b:	/* set n/360-inch line spacing (ESC +) */
			dev->linespacing = (double)dev->esc_parms[0] / 360.0;
			break;

		case 0x2d:	/* turn underline on/off (ESC -) */
			if (dev->esc_parms[0] == 0 || dev->esc_parms[0] == '0')
				dev->font_style &= ~STYLE_UNDERLINE;
			if (dev->esc_parms[0] == 1 || dev->esc_parms[0] == '1') {
				dev->font_style |= STYLE_UNDERLINE;
				dev->font_score = SCORE_SINGLE;
			}
			update_font(dev);
			break;

		case 0x2f:	/* select vertical tab channel (ESC /) */
			/* Ignore */
			break;

		case 0x30:	/* select 1/8-inch line spacing (ESC 0) */
			dev->linespacing = 1.0 / 8.0;
			break;

		case 0x32:	/* select 1/6-inch line spacing (ESC 2) */
			dev->linespacing = 1.0 / 6.0;
			break;

		case 0x33:	/* set n/180-inch line spacing (ESC 3) */
			dev->linespacing = (double)dev->esc_parms[0] / 180.0;
			break;

		case 0x34:	/* select italic font (ESC 4) */
			dev->font_style |= STYLE_ITALICS;
			update_font(dev);
			break;

		case 0x35:	/* cancel italic font (ESC 5) */
			dev->font_style &= ~STYLE_ITALICS;
			update_font(dev);
			break;

		case 0x36:	/* enable printing of upper control codes (ESC 6) */
			dev->print_upper_control = 1;
			break;

		case 0x37:	/* enable upper control codes (ESC 7) */
			dev->print_upper_control = 0;
			break;

		case 0x3c:	/* unidirectional mode (one line) (ESC <) */
				/* We don't have a print head, so just
				 * ignore this. */
			break;

		case 0x3d:	/* set MSB to 0 (ESC =) */
			dev->msb = 0;
			break;

		case 0x3e:	/* set MSB to 1 (ESC >) */
			dev->msb = 1;
			break;

		case 0x3f:	/* reassign bit-image mode (ESC ?) */
			if (dev->esc_parms[0] == 'K')
				dev->density_k = dev->esc_parms[1];
			if (dev->esc_parms[0] == 'L')
				dev->density_l = dev->esc_parms[1];
			if (dev->esc_parms[0] == 'Y')
				dev->density_y = dev->esc_parms[1];
			if (dev->esc_parms[0] == 'Z')
				dev->density_z = dev->esc_parms[1];
			break;

		case 0x40:	/* initialize printer (ESC @) */
			reset_printer(dev);
			break;

		case 0x41:	/* set n/60-inch line spacing */
			dev->linespacing = (double)dev->esc_parms[0] / 60.0;
			break;

		case 0x43:	/* set page length in lines (ESC C) */
			if (dev->esc_parms[0]) {
				dev->page_height = dev->bottom_margin = (double)dev->esc_parms[0] * dev->linespacing;
			} else {	/* == 0 => Set page length in inches */
				dev->esc_parms_req = 1;
				dev->esc_parms_curr = 0;
				dev->esc_pending = 0x100; /* dummy value for later */
				return 1;
			}
			break;

		case 0x45:	/* select bold font (ESC E) */
			dev->font_style |= STYLE_BOLD;
			update_font(dev);
			break;

		case 0x46:	/* cancel bold font (ESC F) */
			dev->font_style &= ~STYLE_BOLD;
			update_font(dev);
			break;

		case 0x47:	/* select dobule-strike printing (ESC G) */
			dev->font_style |= STYLE_DOUBLESTRIKE;
			break;

		case 0x48:	/* cancel double-strike printing (ESC H) */
			dev->font_style &= ~STYLE_DOUBLESTRIKE;
			break;

		case 0x4a:	/* advance print pos vertically (ESC J n) */
			dev->curr_y += (double)dev->esc_parms[0] / 180.0;
			if (dev->curr_y > dev->bottom_margin) {
				new_page(dev);
			}
			break;

		case 0x4b:	/* select 60-dpi graphics (ESC K) */
			/* TODO: graphics stuff */
			setup_bit_image(dev, dev->density_k, PARAM16(0));
			break;

		case 0x4c:	/* select 120-dpi graphics (ESC L) */
			/* TODO: graphics stuff */
			setup_bit_image(dev, dev->density_l, PARAM16(0));
			break;

		case 0x4d:	/* select 10.5-point, 12-cpi (ESC M) */
			dev->cpi = 12.0;
			dev->hmi = -1;
			dev->multipoint_mode = 0;
			update_font(dev);
			break;

		case 0x4e:	/* set bottom margin (ESC N) */
			dev->top_margin = 0.0;
			dev->bottom_margin = (double)dev->esc_parms[0] * dev->linespacing;
			break;

		case 0x4f:	/* cancel bottom (and top) margin */
			dev->top_margin = 0.0;
			dev->bottom_margin = dev->page_height;
			break;

		case 0x50:	/* select 10.5-point, 10-cpi (ESC P) */
			dev->cpi = 10.0;
			dev->hmi = -1;
			dev->multipoint_mode = 0;
			update_font(dev);
			break;

		case 0x51:	/* set right margin */
			dev->right_margin = ((double)dev->esc_parms[0] - 1.0) / dev->cpi;
			break;

		case 0x52:	/* select an intl character set (ESC R) */
			if (dev->esc_parms[0] <= 13 || dev->esc_parms[0] == '@') {
				if (dev->esc_parms[0] == '@')
					dev->esc_parms[0] = 14;

				dev->curr_cpmap[0x23] = intCharSets[dev->esc_parms[0]][0];
				dev->curr_cpmap[0x24] = intCharSets[dev->esc_parms[0]][1];
				dev->curr_cpmap[0x40] = intCharSets[dev->esc_parms[0]][2];
				dev->curr_cpmap[0x5b] = intCharSets[dev->esc_parms[0]][3];
				dev->curr_cpmap[0x5c] = intCharSets[dev->esc_parms[0]][4];
				dev->curr_cpmap[0x5d] = intCharSets[dev->esc_parms[0]][5];
				dev->curr_cpmap[0x5e] = intCharSets[dev->esc_parms[0]][6];
				dev->curr_cpmap[0x60] = intCharSets[dev->esc_parms[0]][7];
				dev->curr_cpmap[0x7b] = intCharSets[dev->esc_parms[0]][8];
				dev->curr_cpmap[0x7c] = intCharSets[dev->esc_parms[0]][9];
				dev->curr_cpmap[0x7d] = intCharSets[dev->esc_parms[0]][10];
				dev->curr_cpmap[0x7e] = intCharSets[dev->esc_parms[0]][11];
			}
			break;

		case 0x53:	/* select superscript/subscript printing (ESC S) */
			if (dev->esc_parms[0] == 0 || dev->esc_parms[0] == '0')
				dev->font_style |= STYLE_SUBSCRIPT;
			if (dev->esc_parms[0] == 1 || dev->esc_parms[1] == '1')
				dev->font_style |= STYLE_SUPERSCRIPT;
			update_font(dev);
			break;

		case 0x54:	/* cancel superscript/subscript printing (ESC T) */
			dev->font_style &= ~(STYLE_SUPERSCRIPT | STYLE_SUBSCRIPT);
			update_font(dev);
			break;

		case 0x55:	/* turn unidirectional mode on/off (ESC U) */
			/* We don't have a print head, so just ignore this. */
			break;

		case 0x57:	/* turn double-width printing on/off (ESC W) */
			if (!dev->multipoint_mode) {
				dev->hmi = -1;
				if (dev->esc_parms[0] == 0 || dev->esc_parms[0] == '0')
					dev->font_style &= ~STYLE_DOUBLEWIDTH;
				if (dev->esc_parms[0] == 1 || dev->esc_parms[0] == '1')
					dev->font_style |= STYLE_DOUBLEWIDTH;
				update_font(dev);
			}
			break;

		case 0x58:	/* select font by pitch and point (ESC X) */
			dev->multipoint_mode = 1;
			/* Copy currently non-multipoint CPI if no value was set so far. */
			if (dev->multipoint_cpi == 0.0) {
				dev->multipoint_cpi= dev->cpi;
			}
			if (dev->esc_parms[0] > 0) {	/* set CPI */
				if (dev->esc_parms[0] == 1) {
					/* Proportional spacing. */
					dev->font_style |= STYLE_PROP;
				} else if (dev->esc_parms[0] >= 5) {
					dev->multipoint_cpi = 360.0 / (double)dev->esc_parms[0];
				}
			}
			if (dev->multipoint_size == 0.0) {
				dev->multipoint_size = 10.5;
			}
			if (PARAM16(1) > 0) {
				/* set points */
				dev->multipoint_size = ((double)PARAM16(1)) / 2.0;
			}
			update_font(dev);
			break;

		case 0x59:	/* select 120-dpi, double-speed graphics (ESC Y) */
			/* TODO: graphics stuff */
			setup_bit_image(dev, dev->density_y, PARAM16(0));
			break;

		case 0x5a:	/* select 240-dpi graphics (ESC Z) */
			/* TODO: graphics stuff */
			setup_bit_image(dev, dev->density_z, PARAM16(0));
			break;

		case 0x5c:	/* set relative horizontal print pos (ESC \) */
			rel_move = PARAM16(0);
			unit_size = dev->defined_unit;
			if (unit_size < 0)
				unit_size = (dev->print_quality == QUALITY_DRAFT ? 120.0 : 180.0);
			dev->curr_x += ((double)rel_move / unit_size);
			break;

		case 0x61:	/* select justification (ESC a) */
			/* Ignore. */
			break;

		case 0x63:	/* set horizontal motion index (HMI) (ESC c) */
			dev->hmi = (double)PARAM16(0) / 360.0;
			dev->extra_intra_space = 0.0;
			break;

		case 0x67:	/* select 10.5-point, 15-cpi (ESC g) */
			dev->cpi = 15;
			dev->hmi = -1;
			dev->multipoint_mode = 0;
			update_font(dev);
			break;

		case 0x6b:	/* select typeface (ESC k) */
			if (dev->esc_parms[0] <= 11 || dev->esc_parms[0] == 30 || dev->esc_parms[0] == 31) {
				dev->lq_typeface = dev->esc_parms[0];
			}
			update_font(dev);
			break;

		case 0x6c:	/* set left margin (ESC 1) */
			dev->left_margin = ((double)dev->esc_parms[0] - 1.0) / dev->cpi;
			if (dev->curr_x < dev->left_margin)
				dev->curr_x = dev->left_margin;
			break;

		case 0x70:	/* Turn proportional mode on/off (ESC p) */
			if (dev->esc_parms[0] == 0 || dev->esc_parms[0] == '0')
				dev->font_style &= ~STYLE_PROP;
			if (dev->esc_parms[0] == 1 || dev->esc_parms[0] == '1') {
				dev->font_style |= STYLE_PROP;
				dev->print_quality = QUALITY_LQ;
			}
			dev->multipoint_mode = 0;
			dev->hmi = -1;
			update_font(dev);
			break;

		case 0x72:	/* select printing color (ESC r) */
			if (dev->esc_parms[0])
				pclog("ESC/P: Color printing not yet supported.\n");
			break;

		case 0x73:	/* select low-speed mode (ESC s) */
			/* Ignore. */
			break;

		case 0x74:	/* select character table (ESC t) */
			if (dev->esc_parms[0] < 4) {
				dev->curr_char_table = dev->esc_parms[0];
			} else if ((dev->esc_parms[0] >= '0') && (dev->esc_parms[0] <= '3')) {
				dev->curr_char_table = dev->esc_parms[0] - '0';
			}
			init_codepage(dev, dev->char_tables[dev->curr_char_table]);
			update_font(dev);
			break;

		case 0x77:	/* turn double-height printing on/off (ESC w) */
			if (! dev->multipoint_mode) {
				if (dev->esc_parms[0] == 0 || dev->esc_parms[0] == '0')
					dev->font_style &= ~STYLE_DOUBLEHEIGHT;
				if (dev->esc_parms[0] == 1 || dev->esc_parms[0] == '1')
					dev->font_style |= STYLE_DOUBLEHEIGHT;
				update_font(dev);
			}
			break;

		case 0x78:	/* select LQ or draft (ESC x) */
			if (dev->esc_parms[0] == 0 || dev->esc_parms[0] == '0')
				dev->print_quality = QUALITY_DRAFT;
			if (dev->esc_parms[0] == 1 || dev->esc_parms[0] == '1')
				dev->print_quality = QUALITY_LQ;
			update_font(dev);
			break;

		/* Our special command markers. */
		case 0x0100:	/* set page length in inches (ESC C NUL) */
			dev->page_height = (double)dev->esc_parms[0];
			dev->bottom_margin = dev->page_height;
			dev->top_margin = 0.0;
			break;

		case 0x0101:	/* skip unsupported ESC ( command */
			dev->esc_parms_req = PARAM16(0);
			dev->esc_parms_curr = 0;
			break;

		/* Extended ESC ( <x> commands */
		case 0x0228:	/* assign character table (ESC (t) */
			if (dev->esc_parms[2] < 4 && dev->esc_parms[3] < 16) {
				dev->char_tables[dev->esc_parms[2]] = codepages[dev->esc_parms[3]];
				if (dev->esc_parms[2] == dev->curr_char_table) {
					init_codepage(dev, dev->char_tables[dev->curr_char_table]);
				}
			}
			break;

		case 0x022d:	/* select line/score (ESC (-)  */
			dev->font_style &= ~(STYLE_UNDERLINE | STYLE_STRIKETHROUGH | STYLE_OVERSCORE);
			dev->font_score = dev->esc_parms[4];
			if (dev->font_score) {
				if (dev->esc_parms[3] == 1)
					dev->font_style |= STYLE_UNDERLINE;
				if (dev->esc_parms[3] == 2)
					dev->font_style |= STYLE_STRIKETHROUGH;
				if (dev->esc_parms[3] == 3)
					dev->font_style |= STYLE_OVERSCORE;
			}
			update_font(dev);
			break;

		case 0x0242:	/* bar code setup and print (ESC (B) */
			pclog("ESC/P: Bardcode printing not supported.\n");

			/* Find out how many bytes to skip. */
			dev->esc_parms_req = PARAM16(0);
			dev->esc_parms_curr = 0;
			break;

		case 0x0243:	/* set page length in defined unit (ESC (C) */
			if (dev->esc_parms[0] && (dev->defined_unit> 0)) {
				dev->page_height = dev->bottom_margin = (double)PARAM16(2) * dev->defined_unit;
				dev->top_margin = 0.0;
			}
			break;

		case 0x0255:	/* set unit (ESC (U) */
			dev->defined_unit = 3600.0 / (double)dev->esc_parms[2];
			break;

		case 0x0256:	/* set abse vertical print pos (ESC (V) */
			unit_size = dev->defined_unit;
			if (unit_size < 0)
				unit_size = 360.0;
			new_y = dev->top_margin + (double)PARAM16(2) * unit_size;
			if (new_y > dev->bottom_margin)
				new_page(dev);
			else
				dev->curr_y = new_y;
			break;

		case 0x025e:	/* print data as characters (ESC (^) */
			dev->print_everything_count = PARAM16(0);
			break;

		case 0x0263:	/* set page format (ESC (c) */
			if (dev->defined_unit > 0.0) {
				dev->top_margin = (double)PARAM16(2) * dev->defined_unit;
				dev->bottom_margin = (double)PARAM16(4) * dev->defined_unit;
			}
			break;

		case 0x0276:	/* set relative vertical print pos (ESC (v) */
			{
				unit_size = dev->defined_unit;
				if (unit_size < 0.0)
					unit_size = 360.0;
				new_y = dev->curr_y + (double)((int16_t)PARAM16(2)) * unit_size;
				if (new_y > dev->top_margin) {
					if (new_y > dev->bottom_margin) {
						new_page(dev);
					} else {
						dev->curr_y = new_y;
					}
				}
			}
			break;

		default:
			pclog("ESC/P: Unhandled ESC command.\n");
			break;
	}

	dev->esc_pending = 0;
	return 1;
    }

    /* Now handle the "regular" control characters. */
    switch (ch) {
	case 0x07:  /* Beeper (BEL) */
		/* TODO: beep? */
		return 1;

	case 0x08:	/* Backspace (BS) */
		new_x = dev->curr_x - (1.0 / dev->actual_cpi);
		if (dev->hmi > 0)
			new_x = dev->curr_x - dev->hmi;
		if (new_x >= dev->left_margin)
			dev->curr_x = new_x;
		return 1;

	case 0x09:	/* Tab horizontally (HT) */
		/* Find tab right to current pos. */
		move_to = -1.0;
		for (i = 0; i < dev->num_horizontal_tabs; i++) {
			if (dev->horizontal_tabs[i] > dev->curr_x) {
				move_to = dev->horizontal_tabs[i];
				break;
			}
		}

		/* Nothing found or out of page bounds => Ignore. */
		if (move_to > 0.0 && move_to < dev->right_margin)
			dev->curr_x = move_to;
		return 1;

	case 0x0b:	/* Tab vertically (VT) */
		if (dev->num_vertical_tabs == 0) {
			/* All tabs cleared? => Act like CR */
			dev->curr_x = dev->left_margin;
		} else if (dev->num_vertical_tabs < 0) {
			/* No tabs set since reset => Act like LF */
			dev->curr_x = dev->left_margin;
			dev->curr_y += dev->linespacing;
			if (dev->curr_y > dev->bottom_margin)
				new_page(dev);
		} else {
			/* Find tab below current pos. */
			move_to = -1;
			for (i = 0; i < dev->num_vertical_tabs; i++) {
				if (dev->vertical_tabs[i] > dev->curr_y) {
					move_to = dev->vertical_tabs[i];
					break;
				}
			}

			/* Nothing found => Act like FF. */
			if (move_to > dev->bottom_margin || move_to < 0)
				new_page(dev);
			else
				dev->curr_y = move_to;
		}

		if (dev->font_style & STYLE_DOUBLEWIDTHONELINE) {
			dev->font_style &= ~STYLE_DOUBLEWIDTHONELINE;
			update_font(dev);
		}
		return 1;

	case 0x0c:	/* Form feed (FF) */
		if (dev->font_style & STYLE_DOUBLEWIDTHONELINE) {
			dev->font_style &= ~STYLE_DOUBLEWIDTHONELINE;
			update_font(dev);
		}
		new_page(dev);
		return 1;

	case 0x0d:	/* Carriage Return (CR) */
		dev->curr_x = dev->left_margin;
		if (!dev->autofeed)
		{
			return 1;
		}
		/*FALLTHROUGH*/

	case 0x0a:	/* Line feed */
		if (dev->font_style & STYLE_DOUBLEWIDTHONELINE) {
			dev->font_style &= ~STYLE_DOUBLEWIDTHONELINE;
			update_font(dev);
		}
		dev->curr_x = dev->left_margin;
		dev->curr_y += dev->linespacing;
		if (dev->curr_y > dev->bottom_margin) {
			new_page(dev);
		}
		return 1;

	case 0x0e:	/* select Real64-width printing (one line) (SO) */
		if (! dev->multipoint_mode) {
			dev->hmi = -1;
			dev->font_style |= STYLE_DOUBLEWIDTHONELINE;
			update_font(dev);
		}
		return 1;

	case 0x0f:	/* select condensed printing (SI) */
		if (! dev->multipoint_mode) {
			dev->hmi = -1;
			dev->font_style |= STYLE_CONDENSED;
			update_font(dev);
		}
		return 1;

	case 0x11:	/* select printer (DC1) */
		/* Ignore. */
		return 1;

	case 0x12:	/* cancel condensed printing (DC2) */
		dev->hmi = -1;
		dev->font_style &= ~STYLE_CONDENSED;
		update_font(dev);
		return 1;

	case 0x13:	/* deselect printer (DC3) */
		/* Ignore. */
		return 1;

	case 0x14:	/* cancel double-width printing (one line) (DC4) */
		dev->hmi = -1;
		dev->font_style &= ~STYLE_DOUBLEWIDTHONELINE;
		update_font(dev);
		return 1;

	case 0x18:	/* cancel line (CAN) */
		return 1;

	case 0x1b:	/* ESC */
		dev->esc_seen = 1;
		return 1;

	default:
		break;
    }

    /* This is a printable character -> print it. */
    return 0;
}


/* TODO: This can be optimized quite a bit... I'm just too lazy right now ;-) */
static void
blit_glyph(escp_t *dev, uint16_t destx, uint16_t desty, int8_t add)
{
    FT_Bitmap *bitmap = &dev->fontface->glyph->bitmap;
    uint16_t x, y;
    uint8_t *src, *dst;

    /* check if freetype is available */
    if (ft_lib == NULL) return;

    for (y = 0; y < bitmap->rows; y++) {
	for (x = 0; x < bitmap->width; x++) {
		src = bitmap->buffer + y * bitmap->pitch + x;
		/* ignore background, and respect page size */
		if (*src && (destx + x < dev->page->w) && (desty + y < dev->page->h)) {
			dst = (uint8_t *)dev->page->pixels + x + destx + (y + desty) * dev->page->pitch;
			if (add) {
				if ((uint16_t)(*dst) + (uint16_t)(*src) > 255) {
					*dst = 255;
				} else {
					*dst += *src;
				}
			} else {
				*dst = *src;
			}
		}
	}
    }
}


/* Draw anti-aliased line. */
static void
draw_hline(escp_t *dev, uint16_t from_x, uint16_t to_x, uint16_t y, int8_t broken)
{
    uint16_t breakmod = dev->dpi / 15;
    uint16_t gapstart = (breakmod * 4) / 5;
    uint16_t x;

    for (x = from_x; x <= to_x; x++) {
	/* Skip parts if broken line or going over the border. */
	if ((!broken || (x % breakmod <= gapstart)) && (x < dev->page->w)) {
		if (y > 0 && (y - 1) < dev->page->h)
			*((uint8_t*)dev->page->pixels + x + (y - 1)*dev->page->pitch) = 120;
		if (y < dev->page->h)
			*((uint8_t*)dev->page->pixels + x + y * dev->page->pitch) = !broken ? 255 : 120;
		if (y + 1 < dev->page->h)
			*((uint8_t*)dev->page->pixels + x + (y + 1)*dev->page->pitch) = 120;
	}
    }
}


static void
print_bit_graph(escp_t *dev, uint8_t ch)
{
    uint8_t pixel_w; /* width of the "pixel" */
    uint8_t pixel_h; /* height of the "pixel" */
    uint8_t i, j, xx, yy, pixel;
    double old_y;

    dev->bg_column[dev->bg_bytes_read++] = ch;
    dev->bg_remaining_bytes--;

    /* Only print after reading a full column. */
    if (dev->bg_bytes_read < dev->bg_bytes_per_column)
	return;

    old_y = dev->curr_y;

    /* if page DPI is bigger than bitgraphics DPI, drawn pixels get "bigger" */
    pixel_w = dev->dpi / dev->bg_h_density > 0 ? dev->dpi / dev->bg_h_density : 1;
    pixel_h = dev->dpi / dev->bg_v_density > 0 ? dev->dpi / dev->bg_v_density : 1;

    for (i = 0; i < dev->bg_bytes_per_column; i++) {
	for (j = 7; j >= 0; j--) {
		pixel = (dev->bg_column[i] >> j) & 0x01;

		if (pixel) {
			/* draw a "pixel" */
			for (yy = 0; yy < pixel_h; yy++) {
				for (xx = 0; xx < pixel_w; xx++) {
					if (((PIXX + xx) < dev->page->w) && ((PIXY + yy) < dev->page->h)) {
						((uint8_t *)(dev->page->pixels))[((int)PIXY + yy) * dev->page->pitch + (int)PIXX + xx] = 255;
					}
				}
			}
		}

		dev->curr_y += 1.0 / dev->bg_v_density;
	}
    }

    /* Mark page dirty. */
    dev->page->dirty = 1;

    /* Restore Y-position. */
    dev->curr_y = old_y;

    /* Advance print head. */
    dev->curr_x += 1.0 / dev->bg_h_density;
}


static void
handle_char(escp_t *dev)
{
    FT_UInt char_index;
    uint16_t pen_x, pen_y;
    uint8_t ch = dev->data;
    uint16_t line_start, line_y;

    if (dev->page == NULL) return;

    /* MSB mode */
    if (dev->msb == 0)
	ch &= 0x7f;
      else if (dev->msb == 1)
	ch |= 0x80;

    if (dev->bg_remaining_bytes > 0) {
	print_bit_graph(dev, ch);
	return;
    }

    /* "print everything" mode? aka. ESC ( ^ */
    if (dev->print_everything_count > 0) {
	/* do not process command char, just continue */
	--dev->print_everything_count;
    } else if (process_char(dev, ch)) {
	/* command was processed */
	return;
    }

    /* We cannot print if we have no font loaded. */
    if (dev->fontface == 0) return;

    /* ok, so we need to print the character now */
    if (ft_lib) {
	char_index = ft_Get_Char_Index(dev->fontface, dev->curr_cpmap[ch]);
	ft_Load_Glyph(dev->fontface, char_index, FT_LOAD_DEFAULT);
	ft_Render_Glyph(dev->fontface->glyph, FT_RENDER_MODE_NORMAL);
    }

    pen_x = (uint16_t)PIXX + dev->fontface->glyph->bitmap_left;
    pen_y = (uint16_t)PIXY - dev->fontface->glyph->bitmap_top + (uint16_t)(dev->fontface->size->metrics.ascender / 64);
	
    if (dev->font_style & STYLE_SUBSCRIPT)
	pen_y += dev->fontface->glyph->bitmap.rows / 2;

    /* mark the page as dirty if anything is drawn */
    if ((ch != 0x20) || (dev->font_score != SCORE_NONE))
	dev->page->dirty = 1;

    /* draw the glyph */
    blit_glyph(dev, pen_x, pen_y, 0);

    /* doublestrike -> draw glyph a second time, 1px below */
    if (dev->font_style & STYLE_DOUBLESTRIKE)
	blit_glyph(dev, pen_x, pen_y + 1, 1);

    /* bold -> draw glyph a second time, 1px to the right */
    if (dev->font_style & STYLE_BOLD)
	blit_glyph(dev, pen_x + 1, pen_y, 1);

    line_start = (uint16_t)PIXX;

    if (dev->font_style & STYLE_PROP) {
	dev->curr_x += dev->fontface->glyph->advance.x / (dev->dpi * 64.0);
    } else {
	if (dev->hmi < 0)
		dev->curr_x += 1.0 / dev->actual_cpi;
	  else
		dev->curr_x += dev->hmi;
    }

    dev->curr_x += dev->extra_intra_space;

    /* Line printing (underline etc.) */
    if (dev->font_score != SCORE_NONE && (dev->font_style & (STYLE_UNDERLINE | STYLE_STRIKETHROUGH | STYLE_OVERSCORE))) {
	/* Find out where to put the line. */
	line_y = (uint16_t)PIXY;

	if (dev->font_style & STYLE_UNDERLINE)
		line_y = pen_y + 5 + dev->fontface->glyph->bitmap.rows;
	if (dev->font_style & STYLE_STRIKETHROUGH)
		line_y = (uint16_t)(PIXY + dev->fontface->size->metrics.ascender / 128.0);
	if (dev->font_style & STYLE_OVERSCORE)
		line_y = (uint16_t)PIXY - ((dev->font_score == SCORE_DOUBLE || dev->font_score == SCORE_DOUBLEBROKEN) ? 5 : 0);

	draw_hline(dev, pen_x, (uint16_t)PIXX, line_y, dev->font_score == SCORE_SINGLEBROKEN || dev->font_score == SCORE_DOUBLEBROKEN);

	if (dev->font_score == SCORE_DOUBLE || dev->font_score == SCORE_DOUBLEBROKEN)
		draw_hline(dev, line_start, (uint16_t)PIXX, line_y + 5, dev->font_score == SCORE_SINGLEBROKEN || dev->font_score == SCORE_DOUBLEBROKEN);
    }
}


static void
write_data(uint8_t val, void *priv)
{
    escp_t *dev = (escp_t *)priv;

    dev->data = val;
}


static void
write_ctrl(uint8_t val, void *priv)
{
    escp_t *dev = (escp_t *)priv;

    /* set autofeed value */
    dev->autofeed = val & 0x02 ? 1 : 0;

    if (val & 0x08) {		/* SELECT */
	/* select printer */
	dev->select = 1;
    }

    if ((val & 0x04) == 0) {
	/* reset printer */
	dev->select = 0;

	reset_printer(dev);
    }

    if (val & 0x01) {		/* STROBE */
	/* Process incoming character. */
	handle_char(dev);

	/* ACK it, will be read on next READ STATUS. */
	dev->ack = 1;
    }
}


static uint8_t
read_status(void *priv)
{
    escp_t *dev = (escp_t *)priv;

    uint8_t status = (dev->ack ? 0x00 : 0x40) |
		     (dev->select ? 0x10 : 0x00) |
		     (dev->busy ? 0x00 : 0x80) |
		     (dev->int_pending ? 0x00 : 0x04) |
		     (dev->error ? 0x00 : 0x08);

    /* Clear ACK after reading status. */
    dev->ack = 0;

    return(status);
}


static void *
escp_init(const lpt_device_t *info)
{
    escp_t *dev;

    pclog("ESC/P: LPT printer '%s' initializing\n", info->name);

    /* Dynamically load FreeType. */
    if (ft_handle == NULL) {
	ft_handle = dynld_module(PATH_FREETYPE_DLL, ft_imports);
	if (ft_handle == NULL) {
		pclog("ESC/P: unable to load FreeType DLL !\n");
		return(NULL);
	}
    }

    /* Initialize FreeType. */
    if (ft_lib == NULL) {
	if (ft_Init_FreeType(&ft_lib)) {
		pclog("ESC/P: error initializing FreeType !\n");
		ft_lib = NULL;
		return(NULL);
	}
    }

    /* Initialize a device instance. */
    dev = malloc(sizeof(escp_t));
    memset(dev, 0x00, sizeof(escp_t));
    dev->name = info->name;

    /* Create a full pathname for the font files. */
    wcscpy(dev->fontpath, rom_path(PRINTER_PATH));
    plat_append_slash(dev->fontpath);
    wcscat(dev->fontpath, PFONTS_PATH);
    plat_append_slash(dev->fontpath);

    /* Create the full path for the page images. */
    plat_append_filename(dev->pagepath, usr_path, PRINTER_PATH);
    if (! plat_dir_check(dev->pagepath))
        plat_dir_create(dev->pagepath);
    plat_append_slash(dev->pagepath);

    /* Initialize parameters. */
    reset_printer(dev);

    /* Create 8-bit grayscale buffer for the page. */
    dev->page = (psurface_t *)malloc(sizeof(psurface_t));
    dev->page->w = (int)(dev->dpi * dev->page_width);
    dev->page->h = (int)(dev->dpi * dev->page_height);
    dev->page->pitch = dev->page->w;
    dev->page->pixels = (uint8_t *)malloc(dev->page->pitch * dev->page->h);
    memset(dev->page->pixels, 0x00, dev->page->pitch * dev->page->h);

#if 0
    pclog("ESC/P: created a virtual page of dimensions %d x %d pixels.\n",
						dev->page->w, dev->page->h);
#endif

    return(dev);
}


static void
escp_close(void *priv)
{
    escp_t *dev = (escp_t *)priv;

    if (dev == NULL) return;

    if (dev->page != NULL) {
	/* Print last page if it contains data. */
	if (dev->page->dirty)
		dump_page(dev);

	if (dev->page->pixels != NULL)
		free(dev->page->pixels);
	free(dev->page);
    }

    free(dev);
}


const lpt_device_t lpt_prt_escp_device = {
    "EPSON ESC/P compatible printer",
    0,
    escp_init,
    escp_close,
    write_data,
    write_ctrl,
    read_status
};
