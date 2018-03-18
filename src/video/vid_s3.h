/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Emulation of the S3 Trio32, S3 Trio64, and S3 Vision864
 *		graphics cards.
 *
 * Version:	@(#)vid_s3.h	1.0.2	2018/03/15
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
#ifndef VIDEO_S3_H
# define VIDEO_S3_H


extern const device_t s3_bahamas64_vlb_device;
extern const device_t s3_bahamas64_pci_device;
extern const device_t s3_9fx_vlb_device;
extern const device_t s3_9fx_pci_device;
extern const device_t s3_phoenix_trio32_vlb_device;
extern const device_t s3_phoenix_trio32_pci_device;
extern const device_t s3_phoenix_trio64_vlb_device;
extern const device_t s3_phoenix_trio64_onboard_pci_device;
extern const device_t s3_phoenix_trio64_pci_device;
extern const device_t s3_phoenix_vision864_pci_device;
extern const device_t s3_phoenix_vision864_vlb_device;
extern const device_t s3_diamond_stealth64_pci_device;
extern const device_t s3_diamond_stealth64_vlb_device;
#if 0
extern const device_t s3_miro_vision964_device;
#endif


#endif	/*VIDEO_S3_H*/
