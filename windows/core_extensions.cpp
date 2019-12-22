/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2015-2019  Jean-Christophe Hessemann, core extensions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "core_main.h"
#include "core_helpers.h"
#include "core_tables.h"


/* Free42 to 42 suffix translator
 *
 */
void core_Free42To42_suffix(arg_struct *arg, unsigned char *buf, int *pt) {
	unsigned char suffix = 0;

	if (arg->type != ARGTYPE_NONE) {	
		switch (arg->type) {
			case ARGTYPE_IND_NUM :
				suffix = 0x80;
			case ARGTYPE_NUM :
				suffix += arg->val.num;
				break;
			case ARGTYPE_IND_STK :
				suffix = 0x80;
			case ARGTYPE_STK :
				switch (arg->val.stk) {
					case 'X' :
						suffix += 0x73;
						break;
					case 'Y' :
						suffix += 0x72;
						break;
					case 'Z' :
						suffix += 0x71;
						break;
					case 'T' :
						suffix += 0x70;
						break;
					case 'L' :
						suffix += 0x74;
						break;
				}
				break;
			case ARGTYPE_LCLBL :
				if (arg->val.lclbl >= 'A' && arg->val.lclbl <= 'J') {
					suffix = arg->val.lclbl - 'A' + 0x66;
				}
				else if (arg->val.lclbl >= 'a' && arg->val.lclbl <= 'e') {
					suffix = arg->val.lclbl - 'a' + 0x7b;
				}
				break;
			//default:
				/* Shouldn't happen */
				/* Values not handled above are ARGTYPE_NEG_NUM,
				 * which is converted to ARGTYPE_NUM by
				 * get_next_command(); ARGTYPE_DOUBLE, which only
                 * occurs with CMD_NUMBER, which is handled in the
                 * special-case section, above; ARGTYPE_COMMAND,
                 * which is handled in the special-case section;
                 * and ARGTYPE_LBLINDEX, which is converted to
                 * ARGTYPE_STR before being stored in a program.
                 */
		}
		buf[(*pt)++] = suffix;
     }
}

/* Free42 to 42 standards instruction translator
 *
 */
void core_Free42To42_standard(int4 hp42s_code, arg_struct *arg, unsigned char *buf, int *pt) {
	int i;

    if (arg->type == ARGTYPE_STR || arg->type == ARGTYPE_IND_STR) {
		// 42s extended  parametrized functions
		buf[(*pt)++] = 0xf1 + arg->length;
		buf[(*pt)++] = (arg->type == ARGTYPE_STR) ? hp42s_code >> 16 : (hp42s_code >> 16) | 0x08;
		for (i = 0; i < arg->length; i++) {
			buf[(*pt)++] = arg->val.text[i];
		}
	}
	else {
		if ((hp42s_code >> 8) & 0xff) {
			buf[(*pt)++] = hp42s_code >> 8;
		}
		buf[(*pt)++] = hp42s_code;
		core_Free42To42_suffix(arg, buf, pt);
	}
}

/* Free42 to 42 lbl translator
 *
 */
void core_Free42To42_lbl(int4 hp42s_code, arg_struct *arg, unsigned char *buf, int *pt) {
	int i;

	if (arg->type == ARGTYPE_NUM && arg->val.num <= 14) {
		buf[(*pt)++] = 0x01 + arg->val.num;
	}
	else if (arg->type == ARGTYPE_STR) {
		buf[(*pt)++] = 0xc0;
		buf[(*pt)++] = 0x00;
		buf[(*pt)++] = 0xf1 + arg->length;
		buf[(*pt)++] = 0x00;
		for (i = 0; i < arg->length; i++) {
			buf[(*pt)++] = arg->val.text[i];
		}
 	}
	else {
		core_Free42To42_standard(hp42s_code, arg, buf, pt);
	}
}

/* Free42 to 42 input translator
 *
 */
void core_Free42To42_input(int4 hp42s_code, arg_struct *arg, unsigned char *buf, int *pt) {
	if (arg->type == ARGTYPE_IND_NUM || arg->type == ARGTYPE_IND_STK) {
		// change code on the fly
		hp42s_code = 0x61c5f2ee;
		}
	core_Free42To42_standard(hp42s_code, arg, buf, pt);
}

/* Free42 to 42 xeq translator
 *
 */
void core_Free42To42_xeq(int4 hp42s_code, arg_struct *arg, unsigned char *buf, int *pt) {
	int i;

	if (arg->type == ARGTYPE_NUM || arg->type == ARGTYPE_LCLBL) {
		hp42s_code = 0x71a7e000;
		core_Free42To42_standard(hp42s_code, arg, buf, pt);
	}
	else if (arg->type == ARGTYPE_STR) {
        buf[(*pt)++] = 0x1e;
        buf[(*pt)++] = 0xf0 + arg->length;
		for (i = 0; i < arg->length; i++) {
			buf[(*pt)++] = arg->val.text[i];
		}
	}
	else {
		core_Free42To42_standard(hp42s_code, arg, buf, pt);
	}
}

/* Free42 to 42 gto translator
 *
 */
void core_Free42To42_gto(int4 hp42s_code, arg_struct *arg, unsigned char *buf, int *pt) {
	int i;

	if (arg->type == ARGTYPE_NUM && arg->val.num <= 14) {
		buf[(*pt)++] = 0xb1 + arg->val.num;
		buf[(*pt)++] = 0x00;
	}
	else if (arg->type == ARGTYPE_NUM || arg->type == ARGTYPE_LCLBL) {
		hp42s_code = 0x81a6d000;
		core_Free42To42_standard(hp42s_code, arg, buf, pt);
	}
	else if (arg->type == ARGTYPE_IND_NUM || arg->type == ARGTYPE_IND_STK) {
		buf[(*pt)++] = 0xae;
		arg->type = (arg->type == ARGTYPE_IND_NUM) ? ARGTYPE_NUM : ARGTYPE_STK;
		core_Free42To42_suffix(arg, buf, pt);;
	}
	else if (arg->type == ARGTYPE_STR) {
        buf[(*pt)++] = 0x1d;
        buf[(*pt)++] = 0xf0 + arg->length;
		for (i = 0; i < arg->length; i++) {
			buf[(*pt)++] = arg->val.text[i];
		}
	}
	else {
		core_Free42To42_standard(hp42s_code, arg, buf, pt);
	}
}

/* Free42 to 42 number translator
 *
 */
void core_Free42To42_number(arg_struct *arg, unsigned char *buf, int *pt) {
	char *p;
	char dot, c;

	p = phloat2program(arg->val_d);
	dot = flags.f.decimal_point ? '.' : ',';
    while ((c = *p++) != 0) {
		if (c >= '0' && c <= '9') {
	        buf[(*pt)++] = c - 0x20;
			c = c - 0x20;
		}
		else if (c == dot) {
			buf[(*pt)++] = 0x1a;
		}
		else if (c == 24) {
			buf[(*pt)++] = 0x1b;
		}
		else if (c == '-') {
			buf[(*pt)++] = 0x1c;
		}
		else {
			/* Should not happen */
			continue;
		}
	}
	buf[(*pt)++] = 0x00;
}

/* Free42 to 42 string translator
 *
 */
void core_Free42To42_string(arg_struct *arg, unsigned char *buf, int *pt) {
	int i;

	buf[(*pt)++] = 0xf0 + arg->length;
	for (i = 0; i < arg->length; i++) {
		buf[(*pt)++] = arg->val.text[i];
	}
 }

/* Free42 to 42 assign nn translator
 *
 */
void core_Free42To42_asgn(int4 hp42s_code, arg_struct *arg, unsigned char *buf, int *pt) {
	int i;
	const command_spec *cs;

	if (arg->type == ARGTYPE_STR) {
		buf[(*pt)++] = 0xf2 + arg->length;
		buf[(*pt)++] = 0xc0;
		for (i = 0; i < arg->length; i++) {
			buf[(*pt)++] = arg->val.text[i];
		}
	}
	else {
		/* arg.type == ARGTYPE_COMMAND; we don't use that
		 * any more, but just to be safe (in case anyone ever 
		 * actually used this in a program), we handle it
		 * anyway.
		 */
		cs = cmdlist(arg->val.cmd);
		buf[(*pt)++] = 0xf2 + cs->name_length;
		buf[(*pt)++] = 0xf0;
		for (i = 0; i < cs->name_length; i++) {
			buf[(*pt)++] = cs->name[i];
		}
	}
	buf[(*pt)++] = hp42s_code;
}

/* Free42 to 42 key nn xeq / gto translator
 *
 */
void core_Free42To42_key(int4 hp42s_code, arg_struct *arg, unsigned char *buf, int *pt) {
	int i, keyCode;

	keyCode = hp42s_code >> 16;
    if (arg->type == ARGTYPE_STR || arg->type == ARGTYPE_IND_STR){
		buf[(*pt)++] = 0xf2 + arg->length;
		buf[(*pt)++] = (arg->type == ARGTYPE_STR) ? keyCode : keyCode | 0x08;
       	buf[(*pt)++] = hp42s_code;
		for (i = 0; i < arg->length; i++) {
			buf[(*pt)++] = arg->val.text[i];
		}
	}
	else {
		buf[(*pt)++] = 0xf3;
		buf[(*pt)++] = keyCode | 0x20;
       	buf[(*pt)++] = hp42s_code;
		core_Free42To42_suffix(arg, buf, pt);
	}
}

/* Compile instructions
 *
 * translate command to lif format
 * caller has to save / set global current_prgm
 * high byte flags :
 *   0x00 - normal
 *   0x01 - sto
 *   0x02 - rcl
 *   0x03 - fix /sci /eng
 *   0x04 - size
 *   0x05 - lbl
 *   0x06 - input
 *   0x07 - xeq
 *   0x08 - gto
 *   0x09 - end
 *   0x0a - number
 *   0x0b - string
 *   0x0c - assign
 *   0x0d - key n xeq / gto
 *   0x0e - xrom
 *   0x0f - invalid
 */
int core_Free42To42 (int4 *pc, unsigned char *buf, int *pt) {
    int cmd;
    arg_struct arg;
    uint4 hp42s_code;
	int done = 0;

    get_next_command(pc, &cmd, &arg, 0);
	hp42s_code = cmdlist(cmd)->hp42s_code;
	// use high nibble to deal with special cases
	switch (hp42s_code >> 28) {
		case 0x00 :
			core_Free42To42_standard(hp42s_code, &arg, buf, pt);
			break;
		case 0x01 :
		case 0x02 :
			if (arg.type == ARGTYPE_NUM && arg.val.num <= 15) {
				// short STO / RCL
				buf[(*pt)++] = ((cmd == CMD_RCL) ? 0x20 : 0x30) + arg.val.num;
			}
			else {
				core_Free42To42_standard(hp42s_code, &arg, buf, pt);
			}
			break;
		case 0x03 :
			if (arg.type == ARGTYPE_NUM && arg.val.num > 9) {
				// extended fix / sci /eng
				buf[(*pt)++] = 0xf1;
				buf[(*pt)++] = (unsigned char)(((arg.val.num + 3) << 4) + ((hp42s_code & 0x0f) - 7));
			}
			else {
				core_Free42To42_standard(hp42s_code , &arg, buf, pt);
			}
			break;
		case 0x04 :
			// size
			buf[(*pt)++] = 0xf3;
			buf[(*pt)++] = 0xf7;
			buf[(*pt)++] = arg.val.num >> 8;
			buf[(*pt)++] = arg.val.num & 0xff;
			break;
		case 0x05 :
			core_Free42To42_lbl(hp42s_code, &arg, buf, pt);
			break;
		case 0x06 :
			core_Free42To42_input(hp42s_code, &arg, buf, pt);
			break;
		case 0x07 :
			core_Free42To42_xeq(hp42s_code, &arg, buf, pt);
			break;
		case 0x08 :
			core_Free42To42_gto(hp42s_code, &arg, buf, pt);
			break;
		case 0x09 :
			// end
			buf[(*pt)++] = 0xc0;
			buf[(*pt)++] = 0x00;
			buf[(*pt)++] = 0x0d;
			done = 1;
			break;
		case 0x0a :
			core_Free42To42_number(&arg, buf, pt);
			break;
		case 0x0b :
			core_Free42To42_string(&arg, buf, pt);
			break;
		case 0x0c :
			core_Free42To42_asgn(hp42s_code, &arg, buf, pt);
			break;
		case 0x0d :
			core_Free42To42_key(hp42s_code, &arg, buf, pt);
			break;
		case 0x0e :
			// xrom
            buf[(*pt)++] = (0xa0 + ((arg.val.num >> 8) & 7));
            buf[(*pt)++] = arg.val.num;
			break;
	}
	return done;
}

/* read string to buf
 *
 *
 */
void core_42ToFree42_string (unsigned char *buf, int pt, int len, arg_struct *arg) {
	int i;
	for (i = 0; i < len; i++) {
		arg->val.text[i] = buf[pt + i];
	}
	arg->length = i;
}

/* Decode Alpha Gto / Xeq / W
 *
 * W will result to CMS_NULL, cf remark about len byte not being 0xfn
 */
void core_42ToFree42_alphaGto (unsigned char *buf, int *pt) {
	int cmd;
	arg_struct arg;
	switch 	(buf[*pt]) {
		case 0x1d :
			cmd = CMD_GTO;
			arg.type = ARGTYPE_STR;
			break;
		case 0x1e :
			cmd = CMD_XEQ;
			arg.type = ARGTYPE_STR;
			break;
		case 0x1f :
			// W, treat as null but consume string
			cmd = CMD_NONE;
			arg.type = ARGTYPE_NONE;
			break;
	}
	core_42ToFree42_string (buf, (*pt) + 2, buf[(*pt)+1] & 0x0f, &arg);
	// should not append !!!
    // 'Extend Your HP41' section 15.7 says that if the high nybble of 
    //   the second byte is not f, then the pc is only incremented by 2 
	(*pt) += ((buf[(*pt) + 1] >> 4) == 0xf) ? (buf[(*pt) + 1] & 0xf) + 2 : 2;
	store_command_after(&pc, cmd, &arg);
}

/* builds up digits
 *
 * take care of len to avoid infinite loop
 */
#define MAXDIGITSBUF 50
void core_42ToFree42_number (unsigned char *buf, int *pt, int l) {
	int cmd;
	arg_struct arg;
	int i = 0;
	int s;
	char DigitsBuf[MAXDIGITSBUF];
	cmd = CMD_NUMBER;
	arg.type = ARGTYPE_DOUBLE;
	do  {
		switch (buf[*pt]) {
			case 0x1a :
				DigitsBuf[i++] = '.';
				break;
			case 0x1b :
				DigitsBuf[i++] = 'E';
				break;
			case 0x1c :
				DigitsBuf[i++] = '-';
				break;
			default :
				DigitsBuf[i++] = buf[*pt] + 0x20;
		}
		(*pt)++;
	} while ((buf[*pt] >= 0x10) && (buf[*pt] <= 0x1C) && (i < l) && (i <MAXDIGITSBUF));
	if (buf[*pt] == 0x00) {
		// ignore 0x00 digit separator
		(*pt)++;
	}
	DigitsBuf[i++] = 0;
    arg.val_d = parse_number_line(DigitsBuf);
	store_command_after(&pc, cmd, &arg);
}

/* decode suffix
 *
 *
 */
void core_42ToFree42_suffix (unsigned char suffix, arg_struct *arg) {
	bool ind;
	ind = (suffix & 0x80) ? true : false;
	suffix &= 0x7f;
	if (!ind && (suffix >= 0x66) && (suffix <= 0x6f)) {
		arg->type = ARGTYPE_LCLBL;
        arg->val.lclbl = 'A' + (suffix - 0x66);
	}
	else if (!ind && suffix >= 0x7b) {
		arg->type = ARGTYPE_LCLBL;
		arg->val.lclbl = 'a' + (suffix - 0x7b);
	}
	else if (suffix >= 0x70 && suffix <= 0x74) {
		arg->type = ind ? ARGTYPE_IND_STK : ARGTYPE_STK;
		switch (suffix) {
			case 0x70 :
				arg->val.stk = 'T';
				break;
            case 0x71 :
				arg->val.stk = 'Z';
				break;
            case 0x72 :
				arg->val.stk = 'Y';
				break;
            case 0x73 :
				arg->val.stk = 'X';
				break;
            case 0x74 :
				arg->val.stk = 'L';
				break;
		}
	}
	else {
		arg->type = ind ? ARGTYPE_IND_NUM : ARGTYPE_NUM;
		arg->val.num = suffix;
	}
}

/* Decode xrom (two bytes) instructions
 *
 * linear search in cmd array
 */
void core_42toFree42_xrom (unsigned char *buf, int *pt, int *cmd) {
	int i;
	uint4 inst;
	i = 0;
	inst = (uint4)((buf[*pt] << 8) + buf[(*pt)+1]);
	do {
		if ((inst == ((cmdlist(i)->hp42s_code) & 0xffff)) && ((cmdlist(i)->flags & FLAG_HIDDEN) == 0)) {
			*cmd = i;
			break;
		}
		i++;
	} while (i < CMD_SENTINEL);
}

/* Global labels / end
 *
 */
void core_42ToFree42_globalEnd (unsigned char *buf, int *pt) {
	int cmd;
	arg_struct arg;
	if (buf[(*pt) + 2] & 0x80) {
		cmd = CMD_LBL;
		arg.type = ARGTYPE_STR;
		core_42ToFree42_string (buf, (*pt)+4, (buf[(*pt)+2] & 0x0f) - 1, &arg);
		store_command_after(&pc, cmd, &arg);
	}
	else {
		goto_dot_dot(false);
	}
	(*pt) += (buf[(*pt) + 2] & 0x80) ? (buf[(*pt) + 2] & 0x0f) + 3 : 3;
}

/* Text0, synthetic only
 *
 * special case
 */
void core_42ToFree42_string0 (unsigned char *buf, int *pt) {
	int cmd;
	arg_struct arg;
	cmd = CMD_STRING;
    arg.type = ARGTYPE_STR;
    arg.length = 1;
    arg.val.text[0] = 127;
	(*pt)++;
	store_command_after(&pc, cmd, &arg);
}

/* Text1, mixed text and hp42-s fix/sci/eng
 *
 */
void core_42ToFree42_string1 (unsigned char *buf, int *pt) {
	int cmd;
	arg_struct arg;
    arg.type = ARGTYPE_NUM;
	switch (buf[(*pt)+1]) {
		case 0xd5 :
			cmd = CMD_FIX;
			arg.val.num = 10;
			break;
		case 0xd6 :
			cmd = CMD_SCI;
			arg.val.num = 10;
			break;
		case 0xd7 :
			cmd = CMD_ENG; 
			arg.val.num = 10; 
			break;
		case 0xe5 :
			cmd = CMD_FIX;
			arg.val.num = 11;
			break;
		case 0xe6:
			cmd = CMD_SCI;
			arg.val.num = 11;
			break;
		case 0xe7:
			cmd = CMD_ENG;
			arg.val.num = 11;
			break;
		default :
			cmd = CMD_STRING;
			arg.type = ARGTYPE_STR;
			core_42ToFree42_string (buf, (*pt) + 1, buf[*pt] & 0x0f, &arg);
	}
	(*pt) += (buf[*pt] & 0x0f) + 1;
	store_command_after(&pc, cmd, &arg);
}

/* Textn, n > 2 & second byte < 0x80
 *
 */
void core_42ToFree42_stringn (unsigned char *buf, int *pt) {
	int cmd;
	arg_struct arg;
	cmd = CMD_STRING;
	arg.type = ARGTYPE_STR;
	core_42ToFree42_string (buf, (*pt) + 1, buf[*pt] & 0x0f, &arg);
	(*pt) += (buf[*pt] & 0x0f) + 1;
	store_command_after(&pc, cmd, &arg);
}

/* hp-42s extensions decoding
 *
 * from original Free42 parametrized extensions decoding
 */
void core_42ToFree42_42Ext (unsigned char *buf, int *pt) {
	int cmd, flg;
	arg_struct arg;
    cmd = hp42ext[buf[(*pt)+1] & 0x7f] & 0x0fff;
	flg = hp42ext[buf[(*pt)+1] & 0x7f] >> 12;
	if (flg <= 1) {
		arg.type = (flg == 0) ? ARGTYPE_STR : ARGTYPE_IND_STR;
		core_42ToFree42_string(buf, (*pt) + 2, (buf[(*pt)] & 0x0f) -1, &arg);
	}
	else if ((flg == 2) && (buf[*pt] == 0xf2)) {
		core_42ToFree42_suffix(buf[(*pt)+2], &arg);
	}
	else if (flg == 3) {
		switch (buf[(*pt)+1]) {
			case 0xc0 :
				//ASSIGN
				arg.type = ARGTYPE_STR;
				if ((buf[*pt] <= 0xf2) || (buf[(*pt) + (buf[*pt] & 0x0f)] > 17)) {
					cmd = CMD_STRING;
					core_42ToFree42_string (buf, (*pt) + 1, buf[*pt] & 0x0f, &arg);
				}
				else {
					cmd = CMD_ASGN01 + buf[(*pt) + (buf[*pt] & 0x0f)];
					core_42ToFree42_string (buf, (*pt) + 2, (buf[*pt] & 0x0f) - 2, &arg);
				}
				break;
			case 0xc2 :
				// KEY # XEQ name
			case 0xc3 :
				// KEY # GTO name
			case 0xca :
				// KEY # XEQ IND name
			case 0xcb :
				// KEY # GTO IND name
			case 0xe2 :
				// KEY # XEQ lbl
			case 0xe3 :
				// KEY # GTO lbl
				if ((buf[*pt] <= 0xf2) || (buf[(*pt) + 2] < 1) || (buf[(*pt) + 2] > 9)) {
					cmd = CMD_STRING;
					core_42ToFree42_string (buf, (*pt) + 1, (buf[*pt] & 0x0f), &arg);
				}
				else {
					if ((buf[(*pt)+1] & 0x07) == 0x02) {
						cmd = CMD_KEY1X + buf[(*pt) + 2] - 1;
					}
					else {
						cmd = CMD_KEY1G + buf[(*pt) + 2] - 1;
					}
					if ((buf[(*pt)+1] & 0xf8) == 0xc0) {
						arg.type = ARGTYPE_STR;
						core_42ToFree42_string (buf, (*pt) + 3, buf[*pt] & 0x0f - 2, &arg);
					}
					else if ((buf[(*pt)+1] & 0xf8) == 0xc8) {
						arg.type = ARGTYPE_IND_STR;
						core_42ToFree42_string (buf, (*pt) + 3, buf[*pt] & 0x0f - 2, &arg);
					}
					else if (((buf[(*pt)+1] & 0xf8) == 0xe0) && (buf[*pt] == 0xf3)) {
						core_42ToFree42_suffix((buf[(*pt) + 3]), &arg);
					}
					else {
						cmd = CMD_STRING;
						core_42ToFree42_string (buf, (*pt) + 1, buf[*pt] & 0x0f, &arg);
					}
				}
				break;
			case 0xf7 :
				// SIZE nnnn
				if (buf[*pt] == 0xf3) {
					cmd = CMD_SIZE;
                    arg.type = ARGTYPE_NUM;
                    arg.val.num = (uint4)((buf[(*pt)+2] << 8) + buf[(*pt)+3]);
				}
				else {
					cmd = CMD_STRING;
					core_42ToFree42_string (buf, (*pt) + 1, buf[*pt] & 0x0f, &arg);
				}
				break;
		}
	}
	else {
		cmd = CMD_STRING;
		arg.type = ARGTYPE_STR;
		core_42ToFree42_string (buf, (*pt) + 1, buf[*pt] & 0x0f, &arg);
	}
	(*pt) += (buf[(*pt)] & 0x0f) + 1;
	store_command_after(&pc, cmd, &arg);
}

/* Decode first row
 *
 * Null & Short Labels
 */
void core_42ToFree42_shortLbl (unsigned char *buf, int *pt) {
	int cmd;
	arg_struct arg;
	if (buf[*pt] == 0) {
		cmd = CMD_NONE;
		arg.type = ARGTYPE_NONE;
	}
	else {
		cmd = CMD_LBL;
		arg.type = ARGTYPE_NUM;
		arg.val.num = buf[*pt] - 1;
	}
	(*pt)++;
	if (cmd != CMD_NONE) {
		store_command_after(&pc, cmd, &arg);
	}
}

/* Decode second row
 *
 * Digits, Gto, Xeq & W
 */
void core_42ToFree42_row1 (unsigned char *buf, int *pt, int l) {
	if (buf[*pt] >= 0x1d) {
		// alpha gto / xeq / w
		core_42ToFree42_alphaGto(buf, pt);
	}
	else {
		// 0..9, +/-, ., E 
		core_42ToFree42_number(buf, pt, l);
	}
}

/* Decode third & fourth rows
 *
 * short sto & rcl
 */
void core_42ToFree42_shortStoRcl (unsigned char *buf, int *pt) {
	int cmd;
	arg_struct arg;
	cmd = (buf[*pt] & 0x10) ? CMD_STO : CMD_RCL;
	arg.type = ARGTYPE_NUM;
	arg.val.num = buf[*pt] & 0x0f;
	(*pt)++;
	store_command_after(&pc, cmd, &arg);
}

/* Decode one byte instructions
 *
 * row 4 to 8
 * search in cmd array
 */
void core_42ToFree42_1Byte (unsigned char *buf, int *pt) {
	int cmd, i;
	arg_struct arg;
	uint4 inst;
	cmd = CMD_NONE;
	arg.type = ARGTYPE_NONE;
	i = 0;
	inst = (uint4)buf[*pt];
	do {
		if ((inst == (cmdlist(i)->hp42s_code & 0x0000ffff)) && ((cmdlist(i)->flags & FLAG_HIDDEN) == 0)) {
			cmd = i;
			store_command_after(&pc, cmd, &arg);
			break;
		}
		i++;
	} while (i < CMD_SENTINEL);
	(*pt)++;
}

/* Decode two byte instructions
 *
 * row 9 to a
 * search in cmd array
 */
void core_42ToFree42_2Byte (unsigned char *buf, int *pt) {
	int cmd, i;
	arg_struct arg;
	uint4 inst;
	cmd = CMD_NONE;
	arg.type = ARGTYPE_NONE;
	if (buf[*pt] == 0xaf) {
		// nothing to do
	}
	else if (buf [*pt] == 0xae) {
		cmd = (buf[(*pt)+1] & 0x80) ? CMD_XEQ : CMD_GTO;
		// once xeq vs gto decoded, force ind mode
		core_42ToFree42_suffix(buf[(*pt)+1] | 0x80, &arg);
	}
	else if ((buf[*pt] >= 0xa0) && (buf[*pt] <= 0xa7)) {
		cmd = CMD_XROM;
		core_42toFree42_xrom(buf, pt, &cmd);
		// take care of unknown XROM numbers
		if (cmd == CMD_XROM) {
			arg.type = ARGTYPE_NUM;
			arg.val.num = (uint4)((buf[*pt] << 8) + buf[(*pt)+1]);
		}
	}
	else {
		i = 0;
		inst = (uint4)buf[*pt];
		do {
			if ((inst == (cmdlist(i)->hp42s_code & 0x0000ffff)) && ((cmdlist(i)->flags & FLAG_HIDDEN) == 0)) {
				cmd = i;
				break;
			}
			i++;
		} while (i < CMD_SENTINEL);
		core_42ToFree42_suffix(buf[(*pt)+1], &arg);
	}
	(*pt) += 2;
	if (cmd != CMD_NONE) {
		store_command_after(&pc, cmd, &arg);
	}
}

/* Decode short GTO
 *
 * row b
 */
void core_42ToFree42_shortGto (unsigned char *buf, int *pt) {
	int cmd;
	arg_struct arg;
	cmd = CMD_NONE;
	if (buf[*pt] != 0xb0) {
		cmd = CMD_GTO;
		arg.type = ARGTYPE_NUM;
		arg.val.num = (buf[*pt] & 0x0f) - 1;
	}
	(*pt) += 2;
	if (cmd != CMD_NONE) {
		store_command_after(&pc, cmd, &arg);
	}
}

/* Decode row c
 *
 * mainly global labels...
 */
void core_42ToFree42_rowc (unsigned char *buf, int *pt) {
	if (buf[*pt] >= 0xce) {
		// x <> or lbl, basicaly a 2 byte instruction
		core_42ToFree42_2Byte(buf, pt);
	}
	else {
		// global label or end...
		core_42ToFree42_globalEnd(buf, pt);
	}
}

/* gto / xeq
 *
 * row d & e
 */
void core_42ToFree42_gtoXeq (unsigned char *buf, int *pt) {
	int cmd;
	arg_struct arg;
	cmd = ((buf[*pt] >> 4) == 0x0e) ? CMD_XEQ : CMD_GTO;
	core_42ToFree42_suffix(buf[(*pt)+2] & 0x7f, &arg);
	(*pt) += 3;
	store_command_after(&pc, cmd, &arg);
}

/* test and hp-42s extensions
 *
 * row f
 */
void core_42ToFree42_string (unsigned char *buf, int *pt) {
	if (buf[*pt] == 0xf0) {
		core_42ToFree42_string0(buf, pt);
	}
	else if (buf[*pt] == 0xf1) {
		core_42ToFree42_string1(buf, pt);
	}
	else if (buf[(*pt)+1] & 0x80) {
		core_42ToFree42_42Ext(buf, pt);
	}
	else {
		core_42ToFree42_stringn(buf, pt);
	}
}

/* Decode instructions
 *
 * adapted from decomp41.c A.R. Duell
 */
int core_42ToFree42 (unsigned char *buf, int *pt, int l){
	int endFlag = 0;
	switch(buf[*pt]>>4) {
		case 0x00 :
			core_42ToFree42_shortLbl(buf, pt);
			break;
		case 0x01 :
			core_42ToFree42_row1(buf, pt, l);
			break;
		case 0x02 :
		case 0x03 :
			core_42ToFree42_shortStoRcl(buf, pt);
			break;
		case 0x04 :
		case 0x05 :
		case 0x06 :
		case 0x07 :
		case 0x08 :
			core_42ToFree42_1Byte(buf, pt);
			break;
		case 0x09 :
		case 0x0a :
			core_42ToFree42_2Byte(buf, pt);
			break;
		case 0x0b :
			core_42ToFree42_shortGto(buf, pt);
			break;
		case 0x0c :
			core_42ToFree42_rowc(buf, pt);
			break;
		case 0x0d :
		case 0x0e :
			core_42ToFree42_gtoXeq(buf, pt);
			break;
		case 0x0f:
			core_42ToFree42_string(buf, pt);
			break;
	}
	return 0;
}
