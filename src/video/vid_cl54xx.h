/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Definitions for the CLGD5428 driver.
 *
 * Version:	@(#)vid_cl54xx.h	1.0.6	2018/03/20
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
#ifndef VIDEO_CL54XX_H
# define VIDEO_CL54XX_H


extern const device_t gd5426_vlb_device;
extern const device_t gd5428_isa_device;
extern const device_t gd5428_vlb_device;
extern const device_t gd5429_isa_device;
extern const device_t gd5429_vlb_device;
extern const device_t gd5430_vlb_device;
extern const device_t gd5430_pci_device;
extern const device_t gd5434_isa_device;
extern const device_t gd5434_vlb_device;
extern const device_t gd5434_pci_device;
extern const device_t gd5436_pci_device;
extern const device_t gd5446_pci_device;
extern const device_t gd5446_stb_pci_device;
extern const device_t gd5480_pci_device;


#endif	/*VIDEO_CL54XX_H*/
