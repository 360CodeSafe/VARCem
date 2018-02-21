/*
 * VARCem	Virtual Archaelogical Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Define the XKBD to ScanCode translation tables for VNC.
 *
 *		VNC uses the XKBD key code definitions to transport keystroke
 *		information, so we just need some tables to translate those
 * 		into PC-ready scan codes.
 *
 *		We only support XKBD pages 0 (Latin-1) and 255 (special keys)
 *		in these tables, other pages (languages) not [yet] supported.
 *
 *		The tables define up to two keystrokes.. the upper byte is
 *		the first keystroke, and the lower byte the second. If value
 *		is 0x00, the keystroke is not sent.
 *
 * NOTE:	The values are as defined in the Microsoft document named
 *		"Keyboard Scan Code Specification", v1.3a of 2000/03/16.
 *
 * Version:	@(#)vnc_keymap.c	1.0.1	2018/02/14
 *
 * Author:	Fred N. van Kempen, <decwiz@yahoo.com>
 *		Based on raw code by RichardG, <richardg867@gmail.com>
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "emu.h"
#include "keyboard.h"
#include "plat.h"
#include "vnc.h"


static int keysyms_00[] = {
    0x0000,	/* 0x00 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x08 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x10 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x18 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0039,	/* 0x20 (XK_space) */
    0x2a02,	/* 0x21 (XK_exclam) */
    0x2a28,	/* 0x22 (XK_quotedbl) */
    0x2a04,	/* 0x23 (XK_numbersign) */
    0x2a05,	/* 0x24 (XK_dollar) */
    0x2a06,	/* 0x25 (XK_percent) */
    0x2a08,	/* 0x26 (XK_ampersand) */
    0x0028,	/* 0x27 (XK_apostrophe) */

    0x2a0a,	/* 0x28 (XK_parenleft) */
    0x2a0b,	/* 0x29 (XK_parenright) */
    0x2a09,	/* 0x2a (XK_asterisk) */
    0x2a0d,	/* 0x2b (XK_plus) */
    0x0033,	/* 0x2c (XK_comma) */
    0x000c,	/* 0x2d (XK_minus) */
    0x0034,	/* 0x2e (XK_period) */
    0x0035,	/* 0x2f (XK_slash) */

    0x000b,	/* 0x30 (XK_0) */
    0x0002,	/* 0x31 (XK_1) */
    0x0003,	/* 0x32 (XK_2) */
    0x0004,	/* 0x33 (XK_3) */
    0x0005,	/* 0x34 (XK_4) */
    0x0006,	/* 0x35 (XK_5) */
    0x0007,	/* 0x36 (XK_6) */
    0x0008,	/* 0x37 (XK_7) */

    0x0009,	/* 0x38 (XK_8) */
    0x000a,	/* 0x39 (XK_9) */
    0x2a27,	/* 0x3a (XK_colon) */
    0x0027,	/* 0x3b (XK_semicolon) */
    0x2a33,	/* 0x3c (XK_less) */
    0x000d,	/* 0x3d (XK_equal) */
    0x2a34,	/* 0x3e (XK_greater) */
    0x2a35,	/* 0x3f (XK_question) */

    0x2a03,	/* 0x40 (XK_at) */
    0x2a1e,	/* 0x41 (XK_A) */
    0x2a30,	/* 0x42 (XK_B) */
    0x2a2e,	/* 0x43 (XK_C) */
    0x2a20,	/* 0x44 (XK_D) */
    0x2a12,	/* 0x45 (XK_E) */
    0x2a21,	/* 0x46 (XK_F) */
    0x2a22,	/* 0x47 (XK_G) */

    0x2a23,	/* 0x48 (XK_H) */
    0x2a17,	/* 0x49 (XK_I) */
    0x2a24,	/* 0x4a (XK_J) */
    0x2a25,	/* 0x4b (XK_K) */
    0x2a26,	/* 0x4c (XK_L) */
    0x2a32,	/* 0x4d (XK_M) */
    0x2a31,	/* 0x4e (XK_N) */
    0x2a18,	/* 0x4f (XK_O) */

    0x2a19,	/* 0x50 (XK_P) */
    0x2a10,	/* 0x51 (XK_Q) */
    0x2a13,	/* 0x52 (XK_R) */
    0x2a1f,	/* 0x53 (XK_S) */
    0x2a14,	/* 0x54 (XK_T) */
    0x2a16,	/* 0x55 (XK_U) */
    0x2a2f,	/* 0x56 (XK_V) */
    0x2a11,	/* 0x57 (XK_W) */

    0x2a2d,	/* 0x58 (XK_X) */
    0x2a15,	/* 0x59 (XK_Y) */
    0x2a2c,	/* 0x5a (XK_Z) */
    0x001a,	/* 0x5b (XK_bracketleft) */
    0x002b,	/* 0x5c (XK_backslash) */
    0x001b,	/* 0x5d (XK_bracketright) */
    0x2a07,	/* 0x5e (XK_asciicircum) */
    0x2a0c,	/* 0x5f (XK_underscore) */

    0x0029,	/* 0x60 (XK_grave) */
    0x001e,	/* 0x61 (XK_a) */
    0x0030,	/* 0x62 (XK_b) */
    0x002e,	/* 0x63 (XK_c) */
    0x0020,	/* 0x64 (XK_d) */
    0x0012,	/* 0x65 (XK_e) */
    0x0021,	/* 0x66 (XK_f) */
    0x0022,	/* 0x67 (XK_g) */

    0x0023,	/* 0x68 (XK_h) */
    0x0017,	/* 0x69 (XK_i) */
    0x0024,	/* 0x6a (XK_j) */
    0x0025,	/* 0x6b (XK_k) */
    0x0026,	/* 0x6c (XK_l) */
    0x0032,	/* 0x6d (XK_m) */
    0x0031,	/* 0x6e (XK_n) */
    0x0018,	/* 0x6f (XK_o) */

    0x0019,	/* 0x70 (XK_p) */
    0x0010,	/* 0x71 (XK_q) */
    0x0013,	/* 0x72 (XK_r) */
    0x001f,	/* 0x73 (XK_s) */
    0x0014,	/* 0x74 (XK_t) */
    0x0016,	/* 0x75 (XK_u) */
    0x002f,	/* 0x76 (XK_v) */
    0x0011,	/* 0x77 (XK_w) */

    0x002d,	/* 0x78 (XK_x) */
    0x0015,	/* 0x79 (XK_y) */
    0x002c,	/* 0x7a (XK_z) */
    0x2a1a,	/* 0x7b (XK_braceleft) */
    0x2a2b,	/* 0x7c (XK_bar) */
    0x2a1b,	/* 0x7d (XK_braceright) */
    0x2a29,	/* 0x7e (XK_asciitilde) */
    0x0053,	/* 0x7f (XK_delete) */

    0x0000,	/* 0x80 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x88 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x90 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x98 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0xa0 (XK_nobreakspace) */
    0x0000,	/* 0xa1 (XK_exclamdown) */
    0x0000,	/* 0xa2 (XK_cent) */
    0x0000,	/* 0xa3 (XK_sterling) */
    0x0000,	/* 0xa4 (XK_currency) */
    0x0000,	/* 0xa5 (XK_yen) */
    0x0000,	/* 0xa6 (XK_brokenbar) */
    0x0000,	/* 0xa7 (XK_section) */

    0x0000,	/* 0xa8 (XK_diaeresis) */
    0x0000,	/* 0xa9 (XK_copyright) */
    0x0000,	/* 0xaa (XK_ordfeminine) */
    0x0000,	/* 0xab (XK_guillemotleft) */
    0x0000,	/* 0xac (XK_notsign) */
    0x0000,	/* 0xad (XK_hyphen) */
    0x0000,	/* 0xae (XK_registered) */
    0x0000,	/* 0xaf (XK_macron) */

    0x0000,	/* 0xb0 (XK_degree) */
    0x0000,	/* 0xb1 (XK_plusminus) */
    0x0000,	/* 0xb2 (XK_twosuperior) */
    0x0000,	/* 0xb3 (XK_threesuperior) */
    0x0000,	/* 0xb4 (XK_acute) */
    0x0000,	/* 0xb5 (XK_mu) */
    0x0000,	/* 0xb6 (XK_paragraph) */
    0x0000,	/* 0xb7 (XK_periodcentered) */

    0x0000,	/* 0xb8 (XK_cedilla) */
    0x0000,	/* 0xb9 (XK_onesuperior) */
    0x0000,	/* 0xba (XK_masculine) */
    0x0000,	/* 0xbb (XK_guillemotright) */
    0x0000,	/* 0xbc (XK_onequarter) */
    0x0000,	/* 0xbd (XK_onehalf) */
    0x0000,	/* 0xbe (XK_threequarters) */
    0x0000,	/* 0xbf (XK_questiondown) */

    0x0000,	/* 0xc0 (XK_Agrave) */
    0x0000,	/* 0xc1 (XK_Aacute) */
    0x0000,	/* 0xc2 (XK_Acircumflex) */
    0x0000,	/* 0xc3 (XK_Atilde) */
    0x0000,	/* 0xc4 (XK_Adiaeresis) */
    0x0000,	/* 0xc5 (XK_Aring) */
    0x0000,	/* 0xc6 (XK_AE) */
    0x0000,	/* 0xc7 (XK_Ccedilla) */

    0x0000,	/* 0xc8 (XK_Egrave) */
    0x0000,	/* 0xc9 (XK_Eacute) */
    0x0000,	/* 0xca (XK_Ecircumflex) */
    0x0000,	/* 0xcb (XK_Ediaeresis) */
    0x0000,	/* 0xcc (XK_Igrave) */
    0x0000,	/* 0xcd (XK_Iacute) */
    0x0000,	/* 0xce (XK_Icircumflex) */
    0x0000,	/* 0xcf (XK_Idiaeresis) */

    0x0000,	/* 0xd0 (XK_ETH, also XK_Eth) */
    0x0000,	/* 0xd1 (XK_Ntilde) */
    0x0000,	/* 0xd2 (XK_Ograve) */
    0x0000,	/* 0xd3 (XK_Oacute) */
    0x0000,	/* 0xd4 (XK_Ocircumflex) */
    0x0000,	/* 0xd5 (XK_Otilde) */
    0x0000,	/* 0xd6 (XK_Odiaeresis) */
    0x0000,	/* 0xd7 (XK_multiply) */

    0x0000,	/* 0xd8 (XK_Ooblique) */
    0x0000,	/* 0xd9 (XK_Ugrave) */
    0x0000,	/* 0xda (XK_Uacute) */
    0x0000,	/* 0xdb (XK_Ucircumflex) */
    0x0000,	/* 0xdc (XK_Udiaeresis) */
    0x0000,	/* 0xdd (XK_Yacute) */
    0x0000,	/* 0xde (XK_THORN) */
    0x0000,	/* 0xdf (XK_ssharp) */

    0x0000,	/* 0xe0 (XK_agrave) */
    0x0000,	/* 0xe1 (XK_aacute) */
    0x0000,	/* 0xe2 (XK_acircumflex) */
    0x0000,	/* 0xe3 (XK_atilde) */
    0x0000,	/* 0xe4 (XK_adiaeresis) */
    0x0000,	/* 0xe5 (XK_aring) */
    0x0000,	/* 0xe6 (XK_ae) */
    0x0000,	/* 0xe7 (XK_ccedilla) */

    0x0000,	/* 0xe8 (XK_egrave) */
    0x0000,	/* 0xe9 (XK_eacute) */
    0x0000,	/* 0xea (XK_ecircumflex) */
    0x0000,	/* 0xeb (XK_ediaeresis) */
    0x0000,	/* 0xec (XK_igrave) */
    0x0000,	/* 0xed (XK_iacute) */
    0x0000,	/* 0xee (XK_icircumflex) */
    0x0000,	/* 0xef (XK_idiaeresis) */

    0x0000,	/* 0xf0 (XK_eth) */
    0x0000,	/* 0xf1 (XK_ntilde) */
    0x0000,	/* 0xf2 (XK_ograve) */
    0x0000,	/* 0xf3 (XK_oacute) */
    0x0000,	/* 0xf4 (XK_ocircumflex) */
    0x0000,	/* 0xf5 (XK_otilde) */
    0x0000,	/* 0xf6 (XK_odiaeresis) */
    0x0000,	/* 0xf7 (XK_division) */

    0x0000,	/* 0xf8 (XK_oslash) */
    0x0000,	/* 0xf9 (XK_ugrave) */
    0x0000,	/* 0xfa (XK_uacute) */
    0x0000,	/* 0xfb (XK_ucircumflex) */
    0x0000,	/* 0xfc (XK_udiaeresis) */
    0x0000,	/* 0xfd (XK_yacute) */
    0x0000,	/* 0xfe (XK_thorn) */
    0x0000	/* 0xff (XK_ydiaeresis) */
};

static int keysyms_ff[] = {
    0x0000,	/* 0x00 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x000e,	/* 0x08 (XK_BackSpace) */
    0x000f,	/* 0x09 (XK_Tab) */
    0x0000,	/* 0x0a (XK_Linefeed) */
    0x004c,	/* 0x0b (XK_Clear) */
    0x0000,
    0x001c,	/* 0x0d (XK_Return) */
    0x0000,
    0x0000,

    0x0000,	/* 0x10 */
    0x0000,
    0x0000,
    0xff45,	/* 0x13 (XK_Pause) */
    0x0000,	/* 0x14 (XK_Scroll_Lock) */
    0x0000,	/* 0x15 (XK_Sys_Req) */
    0x0000,
    0x0000,

    0x0000,	/* 0x18 */
    0x0000,
    0x0000,
    0x0001,	/* 0x1b (XK_Escape) */
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x20 (XK_Multi_key) */
    0x0000,	/* 0x21 (XK_Kanji; Kanji, Kanji convert) */
    0x0000,	/* 0x22 (XK_Muhenkan; Cancel Conversion) */
    0x0000,	/* 0x23 (XK_Henkan_Mode; Start/Stop Conversion) */
    0x0000,	/* 0x24 (XK_Romaji; to Romaji) */
    0x0000,	/* 0x25 (XK_Hiragana; to Hiragana) */
    0x0000,	/* 0x26 (XK_Katakana; to Katakana) */
    0x0000,	/* 0x27 (XK_Hiragana_Katakana; Hiragana/Katakana toggle) */

    0x0000,	/* 0x28 (XK_Zenkaku; to Zenkaku) */
    0x0000,	/* 0x29 (XK_Hankaku; to Hankaku */
    0x0000,	/* 0x2a (XK_Zenkaku_Hankaku; Zenkaku/Hankaku toggle) */
    0x0000,	/* 0x2b (XK_Touroku; Add to Dictionary) */
    0x0000,	/* 0x2c (XK_Massyo; Delete from Dictionary) */
    0x0000,	/* 0x2d (XK_Kana_Lock; Kana Lock) */
    0x0000,	/* 0x2e (XK_Kana_Shift; Kana Shift) */
    0x0000,	/* 0x2f (XK_Eisu_Shift; Alphanumeric Shift) */

    0x0000,	/* 0x30 (XK_Eisu_toggle; Alphanumeric toggle) */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x38 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,	/* 0x3c (XK_SingleCandidate) */
    0x0000,	/* 0x3d (XK_MultipleCandidate/XK_Zen_Koho) */
    0x0000,	/* 0x3e (XK_PreviousCandidate/XK_Mae_Koho) */
    0x0000,

    0x0000,	/* 0x40 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x48 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0xe047,	/* 0x50 (XK_Home) */
    0xe04b,	/* 0x51 (XK_Left) */
    0xe048,	/* 0x52 (XK_Up) */
    0xe04d,	/* 0x53 (XK_Right) */
    0xe050,	/* 0x54 (XK_Down) */
    0xe049,	/* 0x55 (XK_Prior, XK_Page_Up) */
    0xe051,	/* 0x56 (XK_Next, XK_Page_Down) */
    0xe04f,	/* 0x57 (XK_End) */

    0x0000,	/* 0x58 (XK_Begin) */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x60 (XK_Select) */
    0x0000,	/* 0x61 (XK_Print) */
    0x0000,	/* 0x62 (XK_Execute) */
    0xe052,	/* 0x63 (XK_Insert) */
    0x0000,
    0x0000,	/* 0x65 (XK_Undo) */
    0x0000,	/* 0x66 (XK_Redo) */
    0xe05d,	/* 0x67 (XK_Menu) */

    0x0000,	/* 0x68 (XK_Find) */
    0x0000,	/* 0x69 (XK_Cancel) */
    0x0000,	/* 0x6a (XK_Help) */
    0x0000,	/* 0x6b (XK_Break) */
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x70 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x78 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,	/* 0x7e (XK_Mode_switch,XK_script_switch) */
    0x0045,	/* 0x7f (XK_Num_Lock) */

    0x0039,	/* 0x80 (XK_KP_Space) */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0x88 */
    0x000f,	/* 0x89 (XK_KP_Tab) */
    0x0000,
    0x0000,
    0x0000,
    0xe01c,	/* 0x8d (XK_KP_Enter) */
    0x0000,
    0x0000,

    0x0000,	/* 0x90 */
    0x0000,	/* 0x91 (XK_KP_F1) */
    0x0000,	/* 0x92 (XK_KP_F2) */
    0x0000,	/* 0x93 (XK_KP_F3) */
    0x0000,	/* 0x94 (XK_KP_F4) */
    0x0047,	/* 0x95 (XK_KP_Home) */
    0x004b,	/* 0x96 (XK_KP_Left) */
    0x0048,	/* 0x97 (XK_KP_Up) */

    0x004d,	/* 0x98 (XK_KP_Right) */
    0x0050,	/* 0x99 (XK_KP_Down) */
    0x0049,	/* 0x9a (XK_KP_Prior,XK_KP_Page_Up) */
    0x0051,	/* 0x9b (XK_KP_Next,XK_KP_Page_Down) */
    0x004f,	/* 0x9c (XK_KP_End) */
    0x0000,	/* 0x9d (XK_KP_Begin) */
    0x0052,	/* 0x9e (XK_KP_Insert) */
    0x0053,	/* 0x9f (XK_KP_Delete) */

    0x0000,	/* 0xa0 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0xa8 */
    0x0000,
    0x0037,	/* 0xaa (XK_KP_Multiply) */
    0x004e,	/* 0xab (XK_KP_Add) */
    0x0000,	/* 0xac (XK_KP_Separator) */
    0x004a,	/* 0xad (XK_KP_Subtract) */
    0x0000,	/* 0xae (XK_KP_Decimal) */
    0x0035,	/* 0xaf (XK_KP_Divide) */

    0x0052,	/* 0xb0 (XK_KP_0) */
    0x004f,	/* 0xb1 (XK_KP_1) */
    0x0050,	/* 0xb2 (XK_KP_2) */
    0x0051,	/* 0xb3 (XK_KP_3) */
    0x004b,	/* 0xb4 (XK_KP_4) */
    0x004c,	/* 0xb5 (XK_KP_5) */
    0x004d,	/* 0xb6 (XK_KP_6) */
    0x0047,	/* 0xb7 (XK_KP_7) */

    0x0048,	/* 0xb8 (XK_KP_8) */
    0x0049,	/* 0xb9 (XK_KP_9) */
    0x0000,
    0x0000,
    0x0000,
    0x000d,	/* 0xbd (XK_KP_Equal) */
    0x003b,	/* 0xbe (XK_F1) */
    0x003c,	/* 0xbf (XK_F2) */

    0x003d,	/* 0xc0 (XK_F3) */
    0x003e,	/* 0xc1 (XK_F4) */
    0x003f,	/* 0xc2 (XK_F5) */
    0x0040,	/* 0xc3 (XK_F6) */
    0x0041,	/* 0xc4 (XK_F7) */
    0x0042,	/* 0xc5 (XK_F8) */
    0x0043,	/* 0xc6 (XK_F9) */
    0x0044,	/* 0xc7 (XK_F10) */

    0x0057,	/* 0xc8 (XK_F11,XK_L1) */
    0x0058,	/* 0xc9 (XK_F12,XK_L2) */
    0x0000,	/* 0xca (XK_F13,XK_L3) */
    0x0000,	/* 0xcb (XK_F14,XK_L4) */
    0x0000,	/* 0xcc (XK_F15,XK_L5) */
    0x0000,	/* 0xcd (XK_F16,XK_L6) */
    0x0000,	/* 0xce (XK_F17,XK_L7) */
    0x0000,	/* 0xcf (XK_F18,XK_L8) */

    0x0000,	/* 0xd0 (XK_F19,XK_L9) */
    0x0000,	/* 0xd1 (XK_F20,XK_L10) */
    0x0000,	/* 0xd2 (XK_F21,XK_R1) */
    0x0000,	/* 0xd3 (XK_F22,XK_R2) */
    0x0000,	/* 0xd4 (XK_F23,XK_R3) */
    0x0000,	/* 0xd5 (XK_F24,XK_R4) */
    0x0000,	/* 0xd6 (XK_F25,XK_R5) */
    0x0000,	/* 0xd7 (XK_F26,XK_R6) */

    0x0000,	/* 0xd8 (XK_F27,XK_R7) */
    0x0000,	/* 0xd9 (XK_F28,XK_R8) */
    0x0000,	/* 0xda (XK_F29,XK_R9) */
    0x0000,	/* 0xdb (XK_F30,XK_R10) */
    0x0000,	/* 0xdc (XK_F31,XK_R11) */
    0x0000,	/* 0xdd (XK_F32,XK_R12) */
    0x0000,	/* 0xde (XK_F33,XK_R13) */
    0x0000,	/* 0xdf (XK_F34,XK_R14) */

    0x0000,	/* 0xe0 (XK_F35,XK_R15) */
    0x002a,	/* 0xe1 (XK_Shift_L) */
    0x0036,	/* 0xe2 (XK_Shift_R) */
    0x001d,	/* 0xe3 (XK_Control_L) */
    0xe01d,	/* 0xe4 (XK_Control_R) */
    0x003a,	/* 0xe5 (XK_Caps_Lock) */
    0x003a,	/* 0xe6 (XK_Shift_Lock) */
    0xe05b,	/* 0xe7 (XK_Meta_L) */

    0xe05c,	/* 0xe8 (XK_Meta_R) */
    0x0038,	/* 0xe9 (XK_Alt_L) */
    0xe038,	/* 0xea (XK_Alt_R) */
    0x0000,	/* 0xeb (XK_Super_L) */
    0x0000,	/* 0xec (XK_Super_R) */
    0x0000,	/* 0xed (XK_Hyper_L) */
    0x0000,	/* 0xee (XK_Hyper_R) */
    0x0000,

    0x0000,	/* 0xf0 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,	/* 0xf8 */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0xe071	/* 0xff (XK_Delete) */
};


void
vnc_kbinput(int down, int k)
{
    uint16_t scan;

#if 0
    pclog("VNC: kbinput %d %04x\n", down, k);
#endif

    switch(k >> 8) {
	case 0x00:	/* page 00, Latin-1 */
		scan = keysyms_00[k & 0xff];
		break;

	case 0xff:	/* page FF, Special */
		scan = keysyms_ff[k & 0xff];
		break;

	default:
		pclog("VNC: unhandled Xkbd page: %02x\n", k>>8);
		return;
    }

    if (scan == 0x0000) {
	pclog("VNC: unhandled Xkbd key: %d (%04x)\n", k, k);
	return;
    }
#if 0
    else pclog("VNC: translated to %02x %02x\n", (scan>>8)&0xff, scan&0xff);
#endif

    /* Send this scancode sequence to the PC keyboard. */
    keyboard_input(down, scan);
}
