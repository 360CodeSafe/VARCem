/*
 * VARCem	Virtual Archaelogical Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Implementation of the ADLIB sound device.
 *
 * Version:	@(#)snd_adlib.c	1.0.1	2018/02/14
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
#include "../mca.h"
#include "../device.h"
#include "sound.h"
#include "snd_adlib.h"
#include "snd_opl.h"


typedef struct adlib_t
{
        opl_t   opl;
        
        uint8_t pos_regs[8];
} adlib_t;


static void adlib_get_buffer(int32_t *buffer, int len, void *p)
{
        adlib_t *adlib = (adlib_t *)p;
        int c;

        opl2_update2(&adlib->opl);
        
        for (c = 0; c < len * 2; c++)
                buffer[c] += (int32_t)adlib->opl.buffer[c];

        adlib->opl.pos = 0;
}

uint8_t adlib_mca_read(int port, void *p)
{
        adlib_t *adlib = (adlib_t *)p;
        
        pclog("adlib_mca_read: port=%04x\n", port);
        
        return adlib->pos_regs[port & 7];
}

void adlib_mca_write(int port, uint8_t val, void *p)
{
        adlib_t *adlib = (adlib_t *)p;

        if (port < 0x102)
                return;
        
        pclog("adlib_mca_write: port=%04x val=%02x\n", port, val);
        
        switch (port)
        {
                case 0x102:
                if ((adlib->pos_regs[2] & 1) && !(val & 1))
                        io_removehandler(0x0388, 0x0002, opl2_read, NULL, NULL, opl2_write, NULL, NULL, &adlib->opl);
                if (!(adlib->pos_regs[2] & 1) && (val & 1))
                        io_sethandler(0x0388, 0x0002, opl2_read, NULL, NULL, opl2_write, NULL, NULL, &adlib->opl);
                break;
        }
        adlib->pos_regs[port & 7] = val;
}

void *adlib_init(device_t *info)
{
        adlib_t *adlib = malloc(sizeof(adlib_t));
        memset(adlib, 0, sizeof(adlib_t));
        
        pclog("adlib_init\n");
        opl2_init(&adlib->opl);
        io_sethandler(0x0388, 0x0002, opl2_read, NULL, NULL, opl2_write, NULL, NULL, &adlib->opl);
        sound_add_handler(adlib_get_buffer, adlib);
        return adlib;
}

void *adlib_mca_init(device_t *info)
{
        adlib_t *adlib = adlib_init(info);
        
        io_removehandler(0x0388, 0x0002, opl2_read, NULL, NULL, opl2_write, NULL, NULL, &adlib->opl);
        mca_add(adlib_mca_read, adlib_mca_write, adlib);
        adlib->pos_regs[0] = 0xd7;
        adlib->pos_regs[1] = 0x70;

        return adlib;
}

void adlib_close(void *p)
{
        adlib_t *adlib = (adlib_t *)p;

        free(adlib);
}

device_t adlib_device =
{
        "AdLib",
        DEVICE_ISA,
	0,
        adlib_init, adlib_close, NULL,
        NULL, NULL, NULL, NULL,
        NULL
};

device_t adlib_mca_device =
{
        "AdLib (MCA)",
        DEVICE_MCA,
	0,
        adlib_init, adlib_close, NULL,
        NULL, NULL, NULL, NULL,
        NULL
};
