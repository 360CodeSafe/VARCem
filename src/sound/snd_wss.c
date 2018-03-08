/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Implementation of the Windows Sound System sound device.
 *
 * Version:	@(#)snd_wss.c	1.0.1	2018/02/14
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
#include <math.h>  
#include "../emu.h"
#include "../io.h"
#include "../pic.h"
#include "../dma.h"
#include "../device.h"
#include "sound.h"
#include "snd_ad1848.h"
#include "snd_opl.h"
#include "snd_wss.h"


/*530, 11, 3 - 530=23*/
/*530, 11, 1 - 530=22*/
/*530, 11, 0 - 530=21*/
/*530, 10, 1 - 530=1a*/
/*530, 9,  1 - 530=12*/
/*530, 7,  1 - 530=0a*/
/*604, 11, 1 - 530=22*/
/*e80, 11, 1 - 530=22*/
/*f40, 11, 1 - 530=22*/


static int wss_dma[4] = {0, 0, 1, 3};
static int wss_irq[8] = {5, 7, 9, 10, 11, 12, 14, 15}; /*W95 only uses 7-9, others may be wrong*/


typedef struct wss_t
{
        uint8_t config;

        ad1848_t ad1848;        
        opl_t    opl;
} wss_t;

uint8_t wss_read(uint16_t addr, void *p)
{
        wss_t *wss = (wss_t *)p;
        uint8_t temp;
        temp = 4 | (wss->config & 0x40);
        return temp;
}

void wss_write(uint16_t addr, uint8_t val, void *p)
{
        wss_t *wss = (wss_t *)p;

        wss->config = val;
        ad1848_setdma(&wss->ad1848, wss_dma[val & 3]);
        ad1848_setirq(&wss->ad1848, wss_irq[(val >> 3) & 7]);
}

static void wss_get_buffer(int32_t *buffer, int len, void *p)
{
        wss_t *wss = (wss_t *)p;
        
        int c;

        opl3_update2(&wss->opl);
        ad1848_update(&wss->ad1848);
        for (c = 0; c < len * 2; c++)
        {
                buffer[c] += wss->opl.buffer[c];
                buffer[c] += (wss->ad1848.buffer[c] / 2);
        }

        wss->opl.pos = 0;
        wss->ad1848.pos = 0;
}

void *wss_init(device_t *info)
{
        wss_t *wss = malloc(sizeof(wss_t));

        memset(wss, 0, sizeof(wss_t));

        opl3_init(&wss->opl);
        ad1848_init(&wss->ad1848);
        
        ad1848_setirq(&wss->ad1848, 7);
        ad1848_setdma(&wss->ad1848, 3);

        io_sethandler(0x0388, 0x0004, opl3_read,   NULL, NULL, opl3_write,   NULL, NULL,  &wss->opl);
        io_sethandler(0x0530, 0x0004, wss_read,    NULL, NULL, wss_write,    NULL, NULL,  wss);
        io_sethandler(0x0534, 0x0004, ad1848_read, NULL, NULL, ad1848_write, NULL, NULL,  &wss->ad1848);
                
        sound_add_handler(wss_get_buffer, wss);
        
        return wss;
}

void wss_close(void *p)
{
        wss_t *wss = (wss_t *)p;
        
        free(wss);
}

void wss_speed_changed(void *p)
{
        wss_t *wss = (wss_t *)p;
        
        ad1848_speed_changed(&wss->ad1848);
}

device_t wss_device =
{
        "Windows Sound System",
        DEVICE_ISA, 0,
        wss_init, wss_close, NULL,
        NULL,
        wss_speed_changed,
        NULL, NULL,
        NULL
};
