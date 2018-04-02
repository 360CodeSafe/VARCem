/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Interface to the MuNT32 MIDI synthesizer.
 *
 * Version:	@(#)midi_mt32.c	1.0.3	2018/03/31
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
#include "munt/c_interface/c_interface.h"
#include "../emu.h"
#include "../device.h"
#include "../mem.h"
#include "../rom.h"
#include "../plat.h"
#include "sound.h"
#include "midi.h"
#include "midi_mt32.h"


#define MT32_CTRL_ROM_PATH	L"sound/mt32/mt32_control.rom"
#define MT32_PCM_ROM_PATH	L"sound/mt32/mt32_pcm.rom"
#define CM32_CTRL_ROM_PATH	L"sound/cm32l/cm32l_control.rom"
#define CM32_PCM_ROM_PATH	L"sound/cm32l/cm32l_pcm.rom"


extern void givealbuffer_midi(void *buf, uint32_t size);
extern void pclog(const char *format, ...);
extern void al_set_midi(int freq, int buf_size);

extern int soundon;


static const mt32emu_report_handler_i_v0 handler_v0 = {
    /** Returns the actual interface version ID */
    NULL, //mt32emu_report_handler_version (*getVersionID)(mt32emu_report_handler_i i);

    /** Callback for debug messages, in vprintf() format */
    NULL, //void (*printDebug)(void *instance_data, const char *fmt, va_list list);
    /** Callbacks for reporting errors */
    NULL, //void (*onErrorControlROM)(void *instance_data);
    NULL, //void (*onErrorPCMROM)(void *instance_data);
    /** Callback for reporting about displaying a new custom message on LCD */
    NULL, //void (*showLCDMessage)(void *instance_data, const char *message);
    /** Callback for reporting actual processing of a MIDI message */
    NULL, //void (*onMIDIMessagePlayed)(void *instance_data);
    /**
     * Callback for reporting an overflow of the input MIDI queue.
     * Returns MT32EMU_BOOL_TRUE if a recovery action was taken
     * and yet another attempt to enqueue the MIDI event is desired.
     */
    NULL, //mt32emu_boolean (*onMIDIQueueOverflow)(void *instance_data);
    /**
     * Callback invoked when a System Realtime MIDI message is detected in functions
     * mt32emu_parse_stream and mt32emu_play_short_message and the likes.
     */
    NULL, //void (*onMIDISystemRealtime)(void *instance_data, mt32emu_bit8u system_realtime);
    /** Callbacks for reporting system events */
    NULL, //void (*onDeviceReset)(void *instance_data);
    NULL, //void (*onDeviceReconfig)(void *instance_data);
    /** Callbacks for reporting changes of reverb settings */
    NULL, //void (*onNewReverbMode)(void *instance_data, mt32emu_bit8u mode);
    NULL, //void (*onNewReverbTime)(void *instance_data, mt32emu_bit8u time);
    NULL, //void (*onNewReverbLevel)(void *instance_data, mt32emu_bit8u level);
    /** Callbacks for reporting various information */
    NULL, //void (*onPolyStateChanged)(void *instance_data, mt32emu_bit8u part_num);
    NULL, //void (*onProgramChanged)(void *instance_data, mt32emu_bit8u part_num, const char *sound_group_name, const char *patch_name);
};


#define RENDER_RATE 100
#define BUFFER_SEGMENTS 10


static thread_t *thread_h = NULL;
static event_t *event = NULL;
static uint32_t samplerate = 44100;
static int buf_size = 0;
static float* buffer = NULL;
static int16_t* buffer_int16 = NULL;
static int midi_pos = 0;
static const mt32emu_report_handler_i handler = { &handler_v0 };
static mt32emu_context context = NULL;
static int mtroms_present[2] = {-1, -1};


mt32emu_return_code
mt32_check(const char *func, mt32emu_return_code ret, mt32emu_return_code expected)
{
    if (ret != expected) {
#if 0
	pclog("%s() failed, expected %d but returned %d\n", func, expected, ret);
#endif
                return 0;
    }

    return 1;
}


int
mt32_available(void)
{
    if (mtroms_present[0] < 0)
	mtroms_present[0] = (rom_present(MT32_CTRL_ROM_PATH) &&
			     rom_present(MT32_PCM_ROM_PATH));
    return mtroms_present[0];
}


int
cm32l_available(void)
{
    if (mtroms_present[1] < 0)
	mtroms_present[1] = (rom_present(CM32_CTRL_ROM_PATH) &&
			     rom_present(CM32_PCM_ROM_PATH));
    return mtroms_present[1];
}


void
mt32_stream(float* stream, int len)
{
    if (context) mt32emu_render_float(context, stream, len);
}


void
mt32_stream_int16(int16_t* stream, int len)
{
    if (context) mt32emu_render_bit16s(context, stream, len);
}


void
mt32_poll(void)
{
    midi_pos++;
    if (midi_pos == 48000/RENDER_RATE) {
	midi_pos = 0;
	thread_set_event(event);
    }
}


static void
mt32_thread(void *param)
{
    int buf_pos = 0;
    int bsize = buf_size / BUFFER_SEGMENTS;

    while (1) {
	thread_wait_event(event, -1);

	if (sound_is_float) {
		float *buf = (float *) ((uint8_t*)buffer + buf_pos);

		memset(buf, 0, bsize);
		mt32_stream(buf, bsize / (2 * sizeof(float)));
		buf_pos += bsize;
		if (buf_pos >= buf_size) {
			if (soundon)
				givealbuffer_midi(buffer, buf_size / sizeof(float));
			buf_pos = 0;
		}
	} else {
		int16_t *buf = (int16_t *) ((uint8_t*)buffer_int16 + buf_pos);

		memset(buf, 0, bsize);
		mt32_stream_int16(buf, bsize / (2 * sizeof(int16_t)));
		buf_pos += bsize;
		if (buf_pos >= buf_size) {
			if (soundon)
				givealbuffer_midi(buffer_int16, buf_size / sizeof(int16_t));
			buf_pos = 0;
		}
	}
    }
}


void
mt32_msg(uint8_t* val)
{
    if (context) mt32_check("mt32emu_play_msg", mt32emu_play_msg(context, *(uint32_t*)val), MT32EMU_RC_OK);
}


void
mt32_sysex(uint8_t* data, unsigned int len)
{
    if (context) mt32_check("mt32emu_play_sysex", mt32emu_play_sysex(context, data, len), MT32EMU_RC_OK);
}


static void *
mt32emu_init(wchar_t *control_rom, wchar_t *pcm_rom)
{
    wchar_t path[1024];
    char fn[1024];

    context = mt32emu_create_context(handler, NULL);

    pc_path(path, sizeof_w(path), rom_path(control_rom));
    wcstombs(fn, path, sizeof(fn));
    if (!mt32_check("mt32emu_add_rom_file", mt32emu_add_rom_file(context, fn), MT32EMU_RC_ADDED_CONTROL_ROM)) return 0;

    pc_path(path, sizeof_w(path), rom_path(pcm_rom));
    wcstombs(fn, path, sizeof(fn));
    if (!mt32_check("mt32emu_add_rom_file", mt32emu_add_rom_file(context, fn), MT32EMU_RC_ADDED_PCM_ROM)) return 0;

    if (!mt32_check("mt32emu_open_synth", mt32emu_open_synth(context), MT32EMU_RC_OK)) return 0;

    event = thread_create_event();
    thread_h = thread_create(mt32_thread, 0);
    samplerate = mt32emu_get_actual_stereo_output_samplerate(context);

    /* buf_size = samplerate/RENDER_RATE*2; */
    if (sound_is_float) {
	buf_size = (samplerate/RENDER_RATE)*2*BUFFER_SEGMENTS*sizeof(float);
	buffer = malloc(buf_size);
	buffer_int16 = NULL;
    } else {
	buf_size = (samplerate/RENDER_RATE)*2*BUFFER_SEGMENTS*sizeof(int16_t);
	buffer = NULL;
	buffer_int16 = malloc(buf_size);
    }

    mt32emu_set_output_gain(context, device_get_config_int("output_gain")/100.0f);
    mt32emu_set_reverb_enabled(context, device_get_config_int("reverb"));
    mt32emu_set_reverb_output_gain(context, device_get_config_int("reverb_output_gain")/100.0f);
    mt32emu_set_reversed_stereo_enabled(context, device_get_config_int("reversed_stereo"));
    mt32emu_set_nice_amp_ramp_enabled(context, device_get_config_int("nice_ramp"));

    /* pclog("mt32 output gain: %f\n", mt32emu_get_output_gain(context));
    pclog("mt32 reverb output gain: %f\n", mt32emu_get_reverb_output_gain(context));
    pclog("mt32 reverb: %d\n", mt32emu_is_reverb_enabled(context));
    pclog("mt32 reversed stereo: %d\n", mt32emu_is_reversed_stereo_enabled(context)); */

    al_set_midi(samplerate, buf_size);

    /* pclog("mt32 (Munt %s) initialized, samplerate %d, buf_size %d\n", mt32emu_get_library_version_string(), samplerate, buf_size); */

    midi_device_t* dev = malloc(sizeof(midi_device_t));
    memset(dev, 0, sizeof(midi_device_t));

    dev->play_msg = mt32_msg;
    dev->play_sysex = mt32_sysex;
    dev->poll = mt32_poll;

    midi_init(dev);

    return dev;
}


static void *
mt32_init(const device_t *info)
{
    return mt32emu_init(MT32_CTRL_ROM_PATH, MT32_PCM_ROM_PATH);
}


static void *
cm32l_init(const device_t *info)
{
    return mt32emu_init(CM32_CTRL_ROM_PATH, CM32_PCM_ROM_PATH);
}


static void
mt32_close(void *priv)
{
    if (priv == NULL) return;

    if (thread_h)
	thread_kill(thread_h);
    if (event)
	thread_destroy_event(event);
    event = NULL;
    thread_h = NULL;

    if (context) {
	mt32emu_close_synth(context);
	mt32emu_free_context(context);
    }
    context = NULL;

    if (buffer)
	free(buffer);
    buffer = NULL;

    if (buffer_int16)
	free(buffer_int16);
    buffer_int16 = NULL;

    midi_close();

    free((midi_device_t*)priv);
}


static const device_config_t mt32_config[] =
{
	{
		.name = "output_gain",
		.description = "Output Gain",
		.type = CONFIG_SPINNER,
		.spinner =
		{
			.min = 0,
			.max = 100
		},
		.default_int = 100
	},
	{
		.name = "reverb",
		.description = "Reverb",
		.type = CONFIG_BINARY,
		.default_int = 1
	},
	{
		.name = "reverb_output_gain",
		.description = "Reverb Output Gain",
		.type = CONFIG_SPINNER,
		.spinner =
		{
			.min = 0,
			.max = 100
		},
		.default_int = 100
	},
	{
		.name = "reversed_stereo",
		.description = "Reversed stereo",
		.type = CONFIG_BINARY,
		.default_int = 0
	},
	{
		.name = "nice_ramp",
		.description = "Nice ramp",
		.type = CONFIG_BINARY,
		.default_int = 1
	},
	{
		.type = -1
	}
};


const device_t mt32_device = {
    "Roland MT-32 Emulation",
    0, 0,
    mt32_init, mt32_close, NULL,
    mt32_available,
    NULL, NULL, NULL,
    mt32_config
};

const device_t cm32l_device = {
    "Roland CM-32L Emulation",
    0, 0,
    cm32l_init, mt32_close, NULL,
    cm32l_available,
    NULL, NULL, NULL,
    mt32_config
};
