/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Standard PC/AT implementation.
 *
 * Version:	@(#)m_at.c	1.0.1	2018/02/14
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
#include <wchar.h>
#include "../emu.h"
#include "../pic.h"
#include "../pit.h"
#include "../dma.h"
#include "../mem.h"
#include "../device.h"
#include "../floppy/fdd.h"
#include "../floppy/fdc.h"
#include "../nvr.h"
#include "../game/gameport.h"
#include "../keyboard.h"
#include "../lpt.h"
#include "../disk/hdc.h"
#include "machine.h"


void
machine_at_common_init(machine_t *model)
{
    machine_common_init(model);

    pit_set_out_func(&pit, 1, pit_refresh_timer_at);
    pic2_init();
    dma16_init();

    if (lpt_enabled)
	lpt2_remove();

    nvr_at_init(8);

    if (joystick_type != 7)
	device_add(&gameport_device);
}


void
machine_at_init(machine_t *model)
{
    machine_at_common_init(model);

    device_add(&keyboard_at_device);
}


void
machine_at_ps2_init(machine_t *model)
{
    machine_at_common_init(model);

    device_add(&keyboard_ps2_device);
}


void
machine_at_common_ide_init(machine_t *model)
{
    machine_at_common_init(model);

    device_add(&ide_isa_2ch_opt_device);
}


void
machine_at_ide_init(machine_t *model)
{
    machine_at_init(model);

    device_add(&ide_isa_2ch_opt_device);
}


void
machine_at_ps2_ide_init(machine_t *model)
{
    machine_at_ps2_init(model);

    device_add(&ide_isa_2ch_opt_device);
}


void
machine_at_top_remap_init(machine_t *model)
{
    machine_at_init(model);

    mem_remap_top_384k();
}


void
machine_at_ide_top_remap_init(machine_t *model)
{
    machine_at_ide_init(model);

    mem_remap_top_384k();
}


void
machine_at_ibm_init(machine_t *model)
{
    machine_at_top_remap_init(model);

    device_add(&fdc_at_device);
}
