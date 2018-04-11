/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Configuration file handler header.
 *
 * Version:	@(#)config.h	1.0.2	2018/04/10
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
#ifndef EMU_CONFIG_H
# define EMU_CONFIG_H


#ifdef __cplusplus
extern "C" {
#endif

extern void	config_load(void);
extern void	config_save(void);
extern void	config_write(const wchar_t *fn);
extern void	config_dump(void);

extern void	config_delete_var(const char *cat, const char *name);
extern int	config_get_int(const char *cat, const char *name, int def);
extern int	config_get_hex16(const char *cat, const char *name, int def);
extern int	config_get_hex20(const char *cat, const char *name, int def);
extern int	config_get_mac(const char *cat, const char *name, int def);
extern char	*config_get_string(const char *cat, const char *name, const char *def);
extern wchar_t	*config_get_wstring(const char *cat, const char *name, const wchar_t *def);
extern void	config_set_int(const char *cat, const char *name, int val);
extern void	config_set_hex16(const char *cat, const char *name, int val);
extern void	config_set_hex20(const char *cat, const char *name, int val);
extern void	config_set_mac(const char *cat, const char *name, int val);
extern void	config_set_string(const char *cat, const char *name, const char *val);
extern void	config_set_wstring(const char *cat, const char *name, const wchar_t *val);

#ifdef __cplusplus
}
#endif


#endif	/*EMU_CONFIG_H*/
