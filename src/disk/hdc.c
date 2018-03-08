/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Common code to handle all sorts of disk controllers.
 *
 * Version:	@(#)hdc.c	1.0.1	2018/02/14
 *
 * Authors:	Fred N. van Kempen, <decwiz@yahoo.com>
 *		Miran Grca, <mgrca8@gmail.com>
 *
 *		Copyright 2017,2018 Fred N. van Kempen.
 *		Copyright 2016-2018 Miran Grca.
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
#include <wchar.h>
#include "../emu.h"
#include "../machine/machine.h"
#include "../device.h"
#include "hdc.h"


char	*hdc_name;		/* configured HDC name */
int	hdc_current;


static void *
null_init(device_t *info)
{
    return(NULL);
}


static void
null_close(void *priv)
{
}


static device_t null_device = {
    "Null HDC", 0, 0,
    null_init, null_close, NULL,
    NULL, NULL, NULL, NULL, NULL
};


static void *
inthdc_init(device_t *info)
{
    return(NULL);
}


static void
inthdc_close(void *priv)
{
}


static device_t inthdc_device = {
    "Internal Controller", 0, 0,
    inthdc_init, inthdc_close, NULL,
    NULL, NULL, NULL, NULL, NULL
};


static struct {
    char	*name;
    char	*internal_name;
    device_t	*device;
    int		is_mfm;
} controllers[] = {
    { "None",						"none",		
      &null_device,					0		},

    { "Internal Controller",				"internal",
      &inthdc_device,					0		},

    { "[ISA] [MFM] IBM PC Fixed Disk Adapter",		"mfm_xt",
      &mfm_xt_xebec_device,				1		},

    { "[ISA] [MFM] DTC-5150X Fixed Disk Adapter",	"mfm_dtc5150x",
      &mfm_xt_dtc5150x_device,				1		},

    { "[ISA] [MFM] IBM PC/AT Fixed Disk Adapter",	"mfm_at",
      &mfm_at_wd1003_device,				1		},

    { "[ISA] [ESDI] PC/AT ESDI Fixed Disk Adapter",	"esdi_at",
      &esdi_at_wd1007vse1_device,			0		},

    { "[ISA] [IDE] PC/AT IDE Adapter",			"ide_isa",
      &ide_isa_device,					0		},

    { "[ISA] [IDE] PC/AT IDE Adapter (Dual-Channel)",	"ide_isa_2ch",
      &ide_isa_2ch_device,				0		},

    { "[ISA] [IDE] PC/AT XTIDE",			"xtide_at",
      &xtide_at_device,					0		},

    { "[ISA] [IDE] PS/2 AT XTIDE (1.1.5)",		"xtide_at_ps2",
      &xtide_at_ps2_device,				0		},

    { "[ISA] [XT IDE] Acculogic XT IDE",		"xtide_acculogic",
      &xtide_acculogic_device,				0		},

    { "[ISA] [XT IDE] PC/XT XTIDE",			"xtide",
      &xtide_device		,			0		},

    { "[MCA] [ESDI] IBM PS/2 ESDI Fixed Disk Adapter","esdi_mca",
      &esdi_ps2_device,					1		},

    { "[PCI] [IDE] PCI IDE Adapter",			"ide_pci",
      &ide_pci_device,					0		},

    { "[PCI] [IDE] PCI IDE Adapter (Dual-Channel)",	"ide_pci_2ch",
      &ide_pci_2ch_device,				0		},

    { "[VLB] [IDE] PC/AT IDE Adapter",			"vlb_isa",
      &ide_vlb_device,					0		},

    { "[VLB] [IDE] PC/AT IDE Adapter (Dual-Channel)",	"vlb_isa_2ch",
      &ide_vlb_2ch_device,				0		},

    { "",						"", NULL, 0	}
};


/* Initialize the 'hdc_current' value based on configured HDC name. */
void
hdc_init(char *name)
{
    int c;

    pclog("HDC: initializing..\n");

    for (c=0; controllers[c].device; c++) {
	if (! strcmp(name, controllers[c].internal_name)) {
		hdc_current = c;
		break;
	}
    }
}


/* Reset the HDC, whichever one that is. */
void
hdc_reset(void)
{
    pclog("HDC: reset(current=%d, internal=%d)\n",
	hdc_current, (machines[machine].flags & MACHINE_HDC)?1:0);

    /* If we have a valid controller, add its device. */
    if (hdc_current > 1)
	device_add(controllers[hdc_current].device);
}


char *
hdc_get_name(int hdc)
{
    return(controllers[hdc].name);
}


char *
hdc_get_internal_name(int hdc)
{
    return(controllers[hdc].internal_name);
}


device_t *
hdc_get_device(int hdc)
{
    return(controllers[hdc].device);
}


int
hdc_get_flags(int hdc)
{
    return(controllers[hdc].device->flags);
}


int
hdc_available(int hdc)
{
    return(device_available(controllers[hdc].device));
}


int
hdc_current_is_mfm(void)
{
    return(controllers[hdc_current].is_mfm);
}
