/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Define the available hard disk types.
 *
 * Version:	@(#)hdd_table.c	1.0.2	2018/03/29
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
#include <stdarg.h>
#include <wchar.h>
#include "../emu.h"
#include "hdd.h"


const hddtab_t hdd_table[128] = {
    {  306,  4, 17 },		/* 0 - 7 */
    {  615,  2, 17 },
    {  306,  4, 26 },
    { 1024,  2, 17 },
    {  697,  3, 17 },
    {  306,  8, 17 },
    {  614,  4, 17 },
    {  615,  4, 17 },

    {  670,  4, 17 },		/* 8 - 15 */
    {  697,  4, 17 },
    {  987,  3, 17 },
    {  820,  4, 17 },
    {  670,  5, 17 },
    {  697,  5, 17 },
    {  733,  5, 17 },
    {  615,  6, 17 },

    {  462,  8, 17 },		/* 016-023 */
    {  306,  8, 26 },
    {  615,  4, 26 },
    { 1024,  4, 17 },
    {  855,  5, 17 },
    {  925,  5, 17 },
    {  932,  5, 17 },
    { 1024,  2, 40 },

    {  809,  6, 17 },		/* 024-031 */
    {  976,  5, 17 },
    {  977,  5, 17 },
    {  698,  7, 17 },
    {  699,  7, 17 },
    {  981,  5, 17 },
    {  615,  8, 17 },
    {  989,  5, 17 },

    {  820,  4, 26 },		/* 032-039 */
    { 1024,  5, 17 },
    {  733,  7, 17 },
    {  754,  7, 17 },
    {  733,  5, 26 },
    {  940,  6, 17 },
    {  615,  6, 26 },
    {  462,  8, 26 },

    {  830,  7, 17 },		/* 040-047 */
    {  855,  7, 17 },
    {  751,  8, 17 },
    { 1024,  4, 26 },
    {  918,  7, 17 },
    {  925,  7, 17 },
    {  855,  5, 26 },
    {  977,  7, 17 },

    {  987,  7, 17 },		/* 048-055 */
    { 1024,  7, 17 },
    {  823,  4, 38 },
    {  925,  8, 17 },
    {  809,  6, 26 },
    {  976,  5, 26 },
    {  977,  5, 26 },
    {  698,  7, 26 },

    {  699,  7, 26 },		/* 056-063 */
    {  940,  8, 17 },
    {  615,  8, 26 },
    { 1024,  5, 26 },
    {  733,  7, 26 },
    { 1024,  8, 17 },
    {  823, 10, 17 },
    {  754, 11, 17 },

    {  830, 10, 17 },		/* 064-071 */
    {  925,  9, 17 },
    { 1224,  7, 17 },
    {  940,  6, 26 },
    {  855,  7, 26 },
    {  751,  8, 26 },
    { 1024,  9, 17 },
    {  965, 10, 17 },
    
    {  969,  5, 34 },		/* 072-079 */
    {  980, 10, 17 },
    {  960,  5, 35 },
    {  918, 11, 17 },
    { 1024, 10, 17 },
    {  977,  7, 26 },
    { 1024,  7, 26 },
    { 1024, 11, 17 },

    {  940,  8, 26 },		/* 080-087 */
    {  776,  8, 33 },
    {  755, 16, 17 },
    { 1024, 12, 17 },
    { 1024,  8, 26 },
    {  823, 10, 26 },
    {  830, 10, 26 },
    {  925,  9, 26 },

    {  960,  9, 26 },		/* 088-095 */
    { 1024, 13, 17 },
    { 1224, 11, 17 },
    {  900, 15, 17 },
    {  969,  7, 34 },
    {  917, 15, 17 },
    {  918, 15, 17 },
    { 1524,  4, 39 },

    { 1024,  9, 26 },		/* 096-103 */
    { 1024, 14, 17 },
    {  965, 10, 26 },
    {  980, 10, 26 },
    { 1020, 15, 17 },
    { 1023, 15, 17 },
    { 1024, 15, 17 },
    { 1024, 16, 17 },

    { 1224, 15, 17 },		/* 104-111 */
    {  755, 16, 26 },
    {  903,  8, 46 },
    {  984, 10, 34 },
    {  900, 15, 26 },
    {  917, 15, 26 },
    { 1023, 15, 26 },
    {  684, 16, 38 },

    { 1930,  4, 62 },		/* 112-119 */
    {  967, 16, 31 },
    { 1013, 10, 63 },
    { 1218, 15, 36 },
    {  654, 16, 63 },
    {  659, 16, 63 },
    {  702, 16, 63 },
    { 1002, 13, 63 },

    {  854, 16, 63 },		/* 119-127 */
    {  987, 16, 63 },
    {  995, 16, 63 },
    { 1024, 16, 63 },
    { 1036, 16, 63 },
    { 1120, 16, 59 },
    { 1054, 16, 63 },
    {    0,  0,  0 }
};
