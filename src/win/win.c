/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Platform main support module for Windows.
 *
 * Version:	@(#)win.c	1.0.11	2018/05/09
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
#define UNICODE
#define _WIN32_WINNT 0x0501
#include <windows.h>
#ifdef _MSC_VER
# include <io.h>			/* for _open_osfhandle() */
#endif
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <wchar.h>
#include "../emu.h"
#include "../version.h"
#include "../config.h"
#include "../device.h"
#include "../ui/ui.h"
#define GLOBAL
#include "../plat.h"
#ifdef USE_SDL
# include "../sdl.h"
#endif
#ifdef USE_VNC
# include "../vnc.h"
#endif
#ifdef USE_RDP
# include "../rdp.h"
#endif
#include "../devices/input/mouse.h"
#include "../devices/video/video.h"
#ifdef USE_WX
# include "../wx/wx_ui.h"
#endif
#include "win.h"


typedef struct {
    WCHAR str[512];
} rc_str_t;


/* Platform Public data, specific. */
HINSTANCE	hInstance;		/* application instance */
LCID		lang_id;		/* current language ID used */
DWORD		dwSubLangID;


/* Local data. */
static HANDLE	hBlitMutex,		/* video mutex */
		thMain;			/* main thread */
static rc_str_t	*lpRCstr2048,
		*lpRCstr4096,
		*lpRCstr4352,
		*lpRCstr4608,
		*lpRCstr5120,
		*lpRCstr5376,
		*lpRCstr5632,
		*lpRCstr5888,
		*lpRCstr6144,
		*lpRCstr7168;


/* The list with supported VidAPI modules. */
const vidapi_t *plat_vidapis[] = {
#ifdef USE_WX
    &wx_vidapi,
#else
    &ddraw_vidapi,
    &d3d_vidapi,
#endif

#ifdef USE_SDL
    &sdl_vidapi,
#endif
#ifdef USE_VNC
    &vnc_vidapi,
#endif
#ifdef USE_RDP
    &rdp_vidapi,
#endif

    NULL
};


/* Pre-load the strings from our resource file. */
static void
LoadCommonStrings(void)
{
    int i;

    lpRCstr2048 = (rc_str_t *)malloc(STR_NUM_2048*sizeof(rc_str_t));
    lpRCstr4096 = (rc_str_t *)malloc(STR_NUM_4096*sizeof(rc_str_t));
    lpRCstr4352 = (rc_str_t *)malloc(STR_NUM_4352*sizeof(rc_str_t));
    lpRCstr4608 = (rc_str_t *)malloc(STR_NUM_4608*sizeof(rc_str_t));
    lpRCstr5120 = (rc_str_t *)malloc(STR_NUM_5120*sizeof(rc_str_t));
    lpRCstr5376 = (rc_str_t *)malloc(STR_NUM_5376*sizeof(rc_str_t));
    lpRCstr5632 = (rc_str_t *)malloc(STR_NUM_5632*sizeof(rc_str_t));
    lpRCstr5888 = (rc_str_t *)malloc(STR_NUM_5888*sizeof(rc_str_t));
    lpRCstr6144 = (rc_str_t *)malloc(STR_NUM_6144*sizeof(rc_str_t));
    lpRCstr7168 = (rc_str_t *)malloc(STR_NUM_7168*sizeof(rc_str_t));

    for (i=0; i<STR_NUM_2048; i++)
	LoadString(hInstance, 2048+i, lpRCstr2048[i].str, 512);

    for (i=0; i<STR_NUM_4096; i++)
	LoadString(hInstance, 4096+i, lpRCstr4096[i].str, 512);

    for (i=0; i<STR_NUM_4352; i++)
	LoadString(hInstance, 4352+i, lpRCstr4352[i].str, 512);

    for (i=0; i<STR_NUM_4608; i++)
	LoadString(hInstance, 4608+i, lpRCstr4608[i].str, 512);

    for (i=0; i<STR_NUM_5120; i++)
	LoadString(hInstance, 5120+i, lpRCstr5120[i].str, 512);

    for (i=0; i<STR_NUM_5376; i++)
	LoadString(hInstance, 5376+i, lpRCstr5376[i].str, 512);

    for (i=0; i<STR_NUM_5632; i++)
	LoadString(hInstance, 5632+i, lpRCstr5632[i].str, 512);

    for (i=0; i<STR_NUM_5888; i++)
	LoadString(hInstance, 5888+i, lpRCstr5888[i].str, 512);

    for (i=0; i<STR_NUM_6144; i++)
	LoadString(hInstance, 6144+i, lpRCstr6144[i].str, 512);

    for (i=0; i<STR_NUM_7168; i++)
	LoadString(hInstance, 7168+i, lpRCstr7168[i].str, 512);
}


/* Process the commandline, and create standard argc/argv array. */
static int
ProcessCommandLine(wchar_t ***argw)
{
    WCHAR *cmdline;
    wchar_t *argbuf;
    wchar_t **args;
    int argc_max;
    int i, q, argc;

    cmdline = GetCommandLine();
    i = wcslen(cmdline) + 1;
    argbuf = (wchar_t *)malloc(sizeof(wchar_t)*i);
    wcscpy(argbuf, cmdline);

    argc = 0;
    argc_max = 64;
    args = (wchar_t **)malloc(sizeof(wchar_t *) * argc_max);
    if (args == NULL) {
	free(argbuf);
	return(0);
    }

    /* parse commandline into argc/argv format */
    i = 0;
    while (argbuf[i]) {
	while (argbuf[i] == L' ')
		  i++;

	if (argbuf[i]) {
		if ((argbuf[i] == L'\'') || (argbuf[i] == L'"')) {
			q = argbuf[i++];
			if (!argbuf[i])
				break;
		} else
			q = 0;

		args[argc++] = &argbuf[i];

		if (argc >= argc_max) {
			argc_max += 64;
			args = realloc(args, sizeof(wchar_t *)*argc_max);
			if (args == NULL) {
				free(argbuf);
				return(0);
			}
		}

		while ((argbuf[i]) && ((q)
			? (argbuf[i]!=q) : (argbuf[i]!=L' '))) i++;

		if (argbuf[i]) {
			argbuf[i] = 0;
			i++;
		}
	}
    }

    args[argc] = NULL;
    *argw = args;

    return(argc);
}


/* Load an icon from the resources. */
HICON
LoadIconEx(PCTSTR name)
{
    return((HICON)LoadImage(hInstance, name, IMAGE_ICON, 16, 16, LR_SHARED));
}


/* For the Windows platform, this is the start of the application. */
int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpszArg, int nCmdShow)
{
    wchar_t **argw = NULL;
    int	argc, i;

    /* Set this to the default value (windowed mode). */
    vid_fullscreen = 0;

    /* We need this later. */
    hInstance = hInst;

    /* Initialize the version data. CrashDump needs it early. */
    pc_version("Windows");

#ifdef USE_CRASHDUMP
    /* Enable crash dump services. */
    InitCrashDump();
#endif

    /* First, set our (default) language. */
    plat_set_language(0x0409);

    /* Create console window. */
    if (force_debug)
	plat_console(1);

    /* Process the command line for options. */
    argc = ProcessCommandLine(&argw);

    /* Pre-initialize the system, this loads the config file. */
    if (! pc_setup(argc, argw)) {
	/* Detach from console. */
	plat_console(0);
	return(1);
    }

    /* Cleanup: we may no longer need the console. */
    if (! force_debug)
	plat_console(0);

    /* Create a mutex for the video handler. */
    hBlitMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);

    /* Handle our GUI. */
    i = ui_init(nCmdShow);

    return(i);
}


/*
 * We do this here since there is platform-specific stuff
 * going on here, and we do it in a function separate from
 * main() so we can call it from the UI module as well.
 */
void
plat_start(void)
{
    LARGE_INTEGER qpc;

    /* We have not stopped yet. */
    quited = 0;

    /* Initialize the high-precision timer. */
    timeBeginPeriod(1);
    QueryPerformanceFrequency(&qpc);
    timer_freq = qpc.QuadPart;
    pclog("Main timer precision: %llu\n", timer_freq);

    /* Start the emulator, really. */
    thMain = thread_create(pc_thread, &quited);
    SetThreadPriority(thMain, THREAD_PRIORITY_HIGHEST);
}


/* Cleanly stop the emulator. */
void
plat_stop(void)
{
    quited = 1;

    plat_delay_ms(100);

    pc_close(thMain);

    thMain = NULL;
}


/* Set (or re-set) the language for the application. */
void
plat_set_language(int id)
{
    LCID lcidNew = MAKELCID(id, dwSubLangID);

    if (lang_id != lcidNew) {
	/* Set our new language ID. */
	lang_id = lcidNew;

	SetThreadLocale(lang_id);

	/* Load the strings table for this ID. */
	LoadCommonStrings();

#if 0
	/* Update the menus for this ID. */
	MenuUpdate();
#endif
    }
}


#ifndef USE_WX
/* Create a console if we don't already have one. */
void
plat_console(int init)
{
    HANDLE h;
    FILE *fp;
    fpos_t p;
    int i;

    if (! init) {
	FreeConsole();
	return;
    }

    /* Are we logging to a file? */
    p = 0;
    (void)fgetpos(stdout, &p);
    if (p != -1) return;

    /* Not logging to file, attach to console. */
    if (! AttachConsole(ATTACH_PARENT_PROCESS)) {
	/* Parent has no console, create one. */
	if (! AllocConsole()) {
		/* Cannot create console, just give up. */
		return;
	}
    }

    fp = NULL;
    if ((h = GetStdHandle(STD_OUTPUT_HANDLE)) != NULL) {
	/* We got the handle, now open a file descriptor. */
	if ((i = _open_osfhandle((intptr_t)h, _O_TEXT)) != -1) {
		/* We got a file descriptor, now allocate a new stream. */
		if ((fp = _fdopen(i, "w")) != NULL) {
			/* Got the stream, re-initialize stdout with it. */
			(void)freopen("CONOUT$", "w", stdout);
			setvbuf(stdout, NULL, _IONBF, 0);
			fflush(stdout);
		}
	}
    }
}
#endif	/*USE_WX*/


/* Return icon number based on drive type. */
int
plat_fdd_icon(int type)
{
    int ret = 512;

    switch(type) {
	case 0:
		break;

	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		ret = 128;
		break;

	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
		ret = 144;
		break;

	default:
		break;
    }

    return(ret);
}


wchar_t *
plat_get_string(int i)
{
    LPTSTR str;

    if ((i >= 2048) && (i <= 3071))
	str = lpRCstr2048[i-2048].str;
      else if ((i >= 4096) && (i <= 4351))
	str = lpRCstr4096[i-4096].str;
      else if ((i >= 4352) && (i <= 4607))
	str = lpRCstr4352[i-4352].str;
      else if ((i >= 4608) && (i <= 5119))
	str = lpRCstr4608[i-4608].str;
      else if ((i >= 5120) && (i <= 5375))
	str = lpRCstr5120[i-5120].str;
      else if ((i >= 5376) && (i <= 5631))
	str = lpRCstr5376[i-5376].str;
      else if ((i >= 5632) && (i <= 5887))
	str = lpRCstr5632[i-5632].str;
      else if ((i >= 5888) && (i <= 6143))
	str = lpRCstr5888[i-5888].str;
      else if ((i >= 6144) && (i <= 7167))
	str = lpRCstr6144[i-6144].str;
      else
	str = lpRCstr7168[i-7168].str;

    return((wchar_t *)str);
}


void
plat_get_exe_name(wchar_t *bufp, int size)
{
    GetModuleFileName(hInstance, bufp, size);
}


int
plat_getcwd(wchar_t *bufp, int max)
{
    (void)_wgetcwd(bufp, max);

    return(0);
}


int
plat_chdir(const wchar_t *path)
{
    return(_wchdir(path));
}


FILE *
plat_fopen(const wchar_t *path, wchar_t *mode)
{
    return(_wfopen(path, mode));
}


void
plat_remove(const wchar_t *path)
{
    _wremove(path);
}


/* Make sure a path ends with a trailing (back)slash. */
void
plat_append_slash(wchar_t *path)
{
    if ((path[wcslen(path)-1] != L'\\') &&
	(path[wcslen(path)-1] != L'/')) {
	wcscat(path, L"\\");
    }
}


/* Check if the given path is absolute or not. */
int
plat_path_abs(const wchar_t *path)
{
    if ((path[1] == L':') || (path[0] == L'\\') || (path[0] == L'/'))
	return(1);

    return(0);
}


/* Return the last element of a pathname. */
wchar_t *
plat_get_basename(const wchar_t *path)
{
    int c = wcslen(path);

    while (c > 0) {
	if (path[c] == L'/' || path[c] == L'\\')
	   return((wchar_t *)&path[c]);
       c--;
    }

    return((wchar_t *)path);
}


wchar_t *
plat_get_filename(const wchar_t *path)
{
    int c = wcslen(path) - 1;

    while (c > 0) {
	if (path[c] == L'/' || path[c] == L'\\')
	   return((wchar_t *)&path[c+1]);
       c--;
    }

    return((wchar_t *)path);
}


wchar_t *
plat_get_extension(const wchar_t *path)
{
    int c = wcslen(path) - 1;

    if (c <= 0)
	return((wchar_t *)path);

    while (c && path[c] != L'.')
		c--;

    if (!c)
	return((wchar_t *)&path[wcslen(path)]);

    return((wchar_t *)&path[c+1]);
}


void
plat_append_filename(wchar_t *dest, const wchar_t *s1, const wchar_t *s2)
{
    wcscat(dest, s1);
    wcscat(dest, s2);
}


int
plat_dir_check(const wchar_t *path)
{
    DWORD dwAttrib = GetFileAttributes(path);

    return(((dwAttrib != INVALID_FILE_ATTRIBUTES &&
	   (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))) ? 1 : 0);
}


int
plat_dir_create(const wchar_t *path)
{
    return((int)CreateDirectory(path, NULL));
}


uint64_t
plat_timer_read(void)
{
    LARGE_INTEGER li;

    QueryPerformanceCounter(&li);

    return(li.QuadPart);
}


uint32_t
plat_get_ticks(void)
{
    return(GetTickCount());
}


void
plat_delay_ms(uint32_t count)
{
    Sleep(count);
}


/*
 * Get number of VidApi entries.
 *
 * This has to be in this module because only we know
 * the actual size of the plat_vidapis[] array. Not a
 * nice way to do it, but so it is...
 */
int
vidapi_count(void)
{
    return((sizeof(plat_vidapis)/sizeof(vidapi_t *)) - 1);
}


void
plat_startblit(void)
{
    WaitForSingleObject(hBlitMutex, INFINITE);
}


void
plat_endblit(void)
{
    ReleaseMutex(hBlitMutex);
}
