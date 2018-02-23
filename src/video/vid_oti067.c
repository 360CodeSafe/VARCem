/*
 * VARCem	Virtual Archaelogical Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Oak OTI067/077 emulation.
 *
 * Version:	@(#)vid_oti067.c	1.0.2	2018/02/22
 *
 * Authors:	Fred N. van Kempen, <decwiz@yahoo.com>
 *		Miran Grca, <mgrca8@gmail.com>
 *		Sarah Walker, <tommowalker@tommowalker.co.uk>
 *
 *		Copyright 2017,2018 Fred N. van Kempen.
 *		Copyright 2016-2018 Miran Grca.
 *		Copyright 2008-2018 Sarah Walker.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free  Software  Foundation; either  version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is  distributed in the hope that it will be useful, but
 * WITHOUT   ANY  WARRANTY;  without  even   the  implied  warranty  of
 * MERCHANTABILITY  or FITNESS  FOR A PARTICULAR  PURPOSE. See  the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *   Free Software Foundation, Inc.
 *   59 Temple Place - Suite 330
 *   Boston, MA 02111-1307
 *   USA.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "../emu.h"
#include "../io.h"
#include "../mem.h"
#include "../rom.h"
#include "../device.h"
#include "video.h"
#include "vid_oti067.h"
#include "vid_svga.h"


#define BIOS_67_PATH	L"roms/video/oti/bios.bin"
#define BIOS_77_PATH	L"roms/video/oti/oti077.vbi"


typedef struct {
    svga_t svga;

    rom_t bios_rom;

    int index;
    uint8_t regs[32];

    uint8_t pos;

    uint32_t vram_size;
    uint32_t vram_mask;

    uint8_t chip_id;
} oti_t;


static void
oti_out(uint16_t addr, uint8_t val, void *p)
{
    oti_t *oti = (oti_t *)p;
    svga_t *svga = &oti->svga;
    uint8_t old;

    if ((((addr&0xFFF0) == 0x3D0 || (addr&0xFFF0) == 0x3B0) && addr < 0x3de) &&
	!(svga->miscout & 1)) addr ^= 0x60;

    switch (addr) {
	case 0x3D4:
		svga->crtcreg = val & 31;
		return;

	case 0x3D5:
		if ((svga->crtcreg < 7) && (svga->crtc[0x11] & 0x80))
			return;
		if ((svga->crtcreg == 7) && (svga->crtc[0x11] & 0x80))
			val = (svga->crtc[7] & ~0x10) | (val & 0x10);
		old = svga->crtc[svga->crtcreg];
		svga->crtc[svga->crtcreg] = val;
		if (old != val) {
			if (svga->crtcreg < 0xE || svga->crtcreg > 0x10) {
				svga->fullchange = changeframecount;
				svga_recalctimings(svga);
			}
		}
		break;

	case 0x3DE: 
		oti->index = val & 0x1f; 
		return;

	case 0x3DF:
		oti->regs[oti->index] = val;
		switch (oti->index) {
			case 0xD:
				svga->vram_display_mask = (val & 0xc) ? oti->vram_mask : 0x3ffff;
				if ((val & 0x80) && oti->vram_size == 256)
					mem_mapping_disable(&svga->mapping);
				else
					mem_mapping_enable(&svga->mapping);
				if (!(val & 0x80))
					svga->vram_display_mask = 0x3ffff;
				break;

			case 0x11:
				svga->read_bank = (val & 0xf) * 65536;
				svga->write_bank = (val >> 4) * 65536;
				break;
		}
		return;
    }

    svga_out(addr, val, svga);
}


static uint8_t
oti_in(uint16_t addr, void *p)
{
    oti_t *oti = (oti_t *)p;
    svga_t *svga = &oti->svga;
    uint8_t temp;
	
    if ((((addr&0xFFF0) == 0x3D0 || (addr&0xFFF0) == 0x3B0) && addr < 0x3de) &&
	!(svga->miscout & 1)) addr ^= 0x60;
	
    switch (addr) {
	case 0x3D4:
		temp = svga->crtcreg;
		break;

	case 0x3D5:
		temp = svga->crtc[svga->crtcreg];
		break;
		
	case 0x3DE: 
		temp = oti->index | (oti->chip_id << 5);
		break;	       

	case 0x3DF: 
		if (oti->index==0x10)
			temp = 0x18;
		  else
			temp = oti->regs[oti->index];
		break;

	default:
		temp = svga_in(addr, svga);
		break;
    }

    return(temp);
}


static void
oti_pos_out(uint16_t addr, uint8_t val, void *p)
{
    oti_t *oti = (oti_t *)p;

    if ((val & 8) != (oti->pos & 8)) {
	if (val & 8)
		io_sethandler(0x03c0, 32, oti_in, NULL, NULL,
			      oti_out, NULL, NULL, oti);
	else
		io_removehandler(0x03c0, 32, oti_in, NULL, NULL,
				 oti_out, NULL, NULL, oti);
    }

    oti->pos = val;
}


static uint8_t
oti_pos_in(uint16_t addr, void *p)
{
    oti_t *oti = (oti_t *)p;

    return(oti->pos);
}	


static void
oti_recalctimings(svga_t *svga)
{
    oti_t *oti = (oti_t *)svga->p;

    if (oti->regs[0x14] & 0x08) svga->ma_latch |= 0x10000;

    if (oti->regs[0x0d] & 0x0c) svga->rowoffset <<= 1;

    if (oti->regs[0x14] & 0x80) {
	svga->vtotal *= 2;
	svga->dispend *= 2;
	svga->vblankstart *= 2;
	svga->vsyncstart *=2;
	svga->split *= 2;			
    }
}


static void *
oti_init(device_t *info)
{
    oti_t *oti = malloc(sizeof(oti_t));
    wchar_t *romfn = NULL;

    memset(oti, 0x00, sizeof(oti_t));
    oti->chip_id = info->local;

    switch(oti->chip_id) {
	case 2:
		romfn = BIOS_67_PATH;
		break;

	case 5:
		romfn = BIOS_77_PATH;
		break;
    }

    rom_init(&oti->bios_rom, romfn,
	     0xc0000, 0x8000, 0x7fff, 0, MEM_MAPPING_EXTERNAL);

    oti->vram_size = device_get_config_int("memory");
    oti->vram_mask = (oti->vram_size << 10) - 1;

    svga_init(&oti->svga, oti, oti->vram_size << 10,
	      oti_recalctimings, oti_in, oti_out, NULL, NULL);

    io_sethandler(0x03c0, 32,
		  oti_in, NULL, NULL, oti_out, NULL, NULL, oti);
    io_sethandler(0x46e8, 1, oti_pos_in,NULL,NULL, oti_pos_out,NULL,NULL, oti);
    oti->svga.miscout = 1;

    return(oti);
}


static void
oti_close(void *p)
{
    oti_t *oti = (oti_t *)p;

    svga_close(&oti->svga);

    free(oti);
}


static void
oti_speed_changed(void *p)
{
    oti_t *oti = (oti_t *)p;

    svga_recalctimings(&oti->svga);
}
	

static void
oti_force_redraw(void *p)
{
    oti_t *oti = (oti_t *)p;

    oti->svga.fullchange = changeframecount;
}


static void
oti_add_status_info(char *s, int max_len, void *p)
{
    oti_t *oti = (oti_t *)p;

    svga_add_status_info(s, max_len, &oti->svga);
}


static int
oti067_available(void)
{
    return(rom_present(BIOS_67_PATH));
}


static device_config_t oti067_config[] =
{
	{
		"memory", "Memory size", CONFIG_SELECTION, "", 512,
		{
			{
				"256 kB", 256
			},
			{
				"512 kB", 512
			},
			{
				""
			}
		}
	},
	{
		"", "", -1
	}
};


static int
oti077_available(void)
{
    return(rom_present(BIOS_77_PATH));
}


static device_config_t oti077_config[] =
{
	{
		"memory", "Memory size", CONFIG_SELECTION, "", 1024,
		{
			{
				"256 kB", 256
			},
			{
				"512 kB", 512
			},
			{
				"1 MB", 1024
			},
			{
				""
			}
		}
	},
	{
		"", "", -1
	}
};


device_t oti067_device =
{
	"Oak OTI-067",
	DEVICE_ISA,
	2,
	oti_init, oti_close, NULL,
	oti067_available,
	oti_speed_changed,
	oti_force_redraw,
	oti_add_status_info,
	oti067_config
};

device_t oti077_device =
{
	"Oak OTI-077",
	DEVICE_ISA,
	5,
	oti_init, oti_close, NULL,
	oti077_available,
	oti_speed_changed,
	oti_force_redraw,
	oti_add_status_info,
	oti077_config
};
