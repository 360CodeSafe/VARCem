/*
 * VARCem	Virtual Archaelogical Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Define the various platform support functions.
 *
 * Version:	@(#)plat.h	1.0.2	2018/02/21
 *
 * Author:	Fred N. van Kempen, <decwiz@yahoo.com>
 *
 *		Copyright 2017,2018 Fred N. van Kempen.
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
#ifndef EMU_PLAT_H
# define EMU_PLAT_H

#ifndef GLOBAL
# define GLOBAL extern
#endif

/* String ID numbers. */
#include "lang/language.h"

/* The Win32 API uses _wcsicmp and _stricmp. */
#ifdef _WIN32
# define wcscasecmp	_wcsicmp
# define strcasecmp	_stricmp
#endif

#if defined(UNIX) && defined(FREEBSD)
/* FreeBSD has largefile by default. */
# define fopen64        fopen
# define fseeko64       fseeko
# define ftello64       ftello
# define off64_t        off_t
#elif defined(_MSC_VER)
# define fopen64        fopen
# define fseeko64       _fseeki64
# define ftello64       _ftelli64
# define off64_t        off_t
#endif


#ifndef _MSC_VER
  /* A hack (GCC-specific?) to allow us to ignore unused parameters. */
# define UNUSED(arg)	__attribute__((unused))arg
#else
  /* MSVC does not have __attribute__((unused)). */
# define UNUSED(arg) arg
#endif

/* Return the size (in wchar's) of a wchar_t array. */
#define sizeof_w(x)	(sizeof((x)) / sizeof(wchar_t))


#ifdef __cplusplus
extern "C" {
#endif

/* Global variables residing in the platform module. */
GLOBAL int	dopause,			/* system is paused */
		doresize,			/* screen resize requested */
		quited,				/* system exit requested */
		mouse_capture;			/* mouse is captured in app */
GLOBAL uint64_t	timer_freq;
GLOBAL int	infocus;
GLOBAL char	emu_version[128];		/* version ID string */


/* System-related functions. */
extern wchar_t	*fix_exe_path(wchar_t *str);
extern FILE	*plat_fopen(wchar_t *path, wchar_t *mode);
extern void	plat_remove(wchar_t *path);
extern int	plat_getcwd(wchar_t *bufp, int max);
extern int	plat_chdir(wchar_t *path);
extern void	plat_get_exe_name(wchar_t *s, int size);
extern wchar_t	*plat_get_filename(wchar_t *s);
extern wchar_t	*plat_get_extension(wchar_t *s);
extern void	plat_append_filename(wchar_t *dest, wchar_t *s1, wchar_t *s2);
extern void	plat_put_backslash(wchar_t *s);
extern void	plat_path_slash(wchar_t *path);
extern int	plat_path_abs(wchar_t *path);
extern int	plat_dir_check(wchar_t *path);
extern int	plat_dir_create(wchar_t *path);
extern uint64_t	plat_timer_read(void);
extern uint32_t	plat_get_ticks(void);
extern void	plat_delay_ms(uint32_t count);
extern void	plat_pause(int p);
extern void	plat_mouse_capture(int on);
extern int	plat_vidapi(char *name);
extern char	*plat_vidapi_name(int api);
extern int	plat_setvid(int api);
extern void	plat_vidsize(int x, int y);
extern void	plat_setfullscreen(int on);
extern void	plat_resize(int x, int y);


/* Resource management. */
extern void	set_language(int id);
extern wchar_t	*plat_get_string(int id);


/* Emulator start/stop support functions. */
extern void	do_start(void);
extern void	do_stop(void);


/* Platform-specific device support. */
extern uint8_t	host_cdrom_drive_available[26];
extern uint8_t	host_cdrom_drive_available_num;

extern void	cdrom_init_host_drives(void);
extern void	cdrom_eject(uint8_t id);
extern void	cdrom_reload(uint8_t id);
extern void	zip_eject(uint8_t id);
extern void	zip_reload(uint8_t id);
extern void	removable_disk_unload(uint8_t id);
extern void	removable_disk_eject(uint8_t id);
extern void	removable_disk_reload(uint8_t id);
extern int      ioctl_open(uint8_t id, char d);
extern void     ioctl_reset(uint8_t id);
extern void     ioctl_close(uint8_t id);


/* Thread support. */
typedef void thread_t;
typedef void event_t;
typedef void mutex_t;

extern thread_t	*thread_create(void (*thread_func)(void *param), void *param);
extern void	thread_kill(thread_t *arg);
extern int	thread_wait(thread_t *arg, int timeout);
extern event_t	*thread_create_event(void);
extern void	thread_set_event(event_t *arg);
extern void	thread_reset_event(event_t *arg);
extern int	thread_wait_event(event_t *arg, int timeout);
extern void	thread_destroy_event(event_t *arg);

extern mutex_t	*thread_create_mutex(wchar_t *name);
extern void	thread_close_mutex(mutex_t *arg);
extern int	thread_wait_mutex(mutex_t *arg);
extern int	thread_release_mutex(mutex_t *mutex);


/* Other stuff. */
extern void	startblit(void);
extern void	endblit(void);
extern void	take_screenshot(void);

#ifdef __cplusplus
}
#endif


#endif	/*EMU_PLAT_H*/
