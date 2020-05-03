/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
 * Free42 eXtensions -- adding HP-IL to free42
 * Copyright (C) 2014-2020 Jean-Christophe HESSEMANN
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

/* extended i/o
 *
 */

//#include <stdio.h>
#include <stdlib.h>
#include "core_commands2.h"
#include "core_display.h"
#include "core_globals.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_variables.h"
#include "hpil_common.h"
#include "hpil_controller.h"
#include "hpil_extended.h"
#include "string.h"

#define MethodNone			0x01
#define MethodChar			0x02
#define MethodCrLf			0x03
#define MethodEnd			0x04
#define MethodLen			0x05
#define MethodCharOrCrlf	0x06

#define XRegister			0x01
#define AlphaRegister		0x10

#define _ParseNone			0x00	// parse not started
#define _ParseSign			0x01	// mantissa sign parsed
#define _ParseInt			0x02	// parsing mantissa int part
#define _ParseFrac			0x03	// parsing mantissa fractionnal part
#define _ParseE				0x04	// exponent parsed
#define _ParseExp			0x05	// parsing exponent
#define _ParsePostImaginary	0x06	// i when at end
#define _ParseNext			0x07	// one more number after ?
#define _ParseDone			0x08	// parse done
#define _ParseIllegal		0xff	// parsing stopped by incorrect char

int ioEndMethod;
int ioTargetRegister;
int ioLength;

extern HPIL_Settings hpil_settings;
extern HpilXController hpilXCore;
extern int controllerCommand;
extern int hpil_step;
extern int hpil_worker(int interrupted);
extern int (*hpil_completion)(int);
extern int (*hpil_pending)(int);
extern void (*hpil_emptyListenBuffer)(void);
extern void (*hpil_fillTalkBuffer)(void);

static int defrd_clrdev(int error);
static int defrd_clrloop(int error);
static int defrd_inac(int error);
static int defrd_inacl(int error);
static int defrd_inae(int error);
static int defrd_inan(int error);
static int defrd_inxb(int error);
static int defrd_inaccl(int error);
static int defrd_outac(int error);
static int defrd_outacl(int error);
static int defrd_outae(int error);
static int defrd_outan(int error);
static int defrd_outxb(int error);

static int hpil_clrdev_completion(int error);
static int hpil_clrloop_completion(int error);
static int hpil_inx_completion(int error);
static int hpil_outx_completion(int error);

/* alpha_trim_left
 *
 * remove upto n chars for alpha
 * return resulting length
 */
int alpha_trim_left (int trim) {
	int i;
	reg_alpha_length = (trim < reg_alpha_length) ? reg_alpha_length - trim : 0;
	for (i = 0; i < reg_alpha_length; i++) {
		reg_alpha[i] = reg_alpha[i + trim];
	}
	return reg_alpha_length;
}

/* Xtract_num
 *
 * extract a substring representing a number
 * return step
 * update end, is_im, # of digits
 */
int Xtract_num(int step, char *inputBuf, int inputLen, int *lastParsed, char *outputBuf, int *outputLen, int *digits, bool *is_im) {
	char c, sep, dot;
	int i, j;
    sep = (flags.f.decimal_point) ? ',' : '.';
    dot = (flags.f.decimal_point) ? '.' : ',';
	i = 0;
	j = 0;
	while (i < inputLen) {
		c = inputBuf[i];
		// start parsing alpha reg
		if (c == '+') {
			switch (step) {
				case _ParseNone :
					step = _ParseSign;
					break;
				case _ParseInt :
				case _ParseFrac :
				case _ParseExp :
					step = _ParseNext;
					break;
				case _ParseE :
					step = _ParseExp;
					break;
				case _ParsePostImaginary:
					step = _ParseNext;
					break;
				default :
					step = _ParseIllegal;
			}
		}
		else if (c == '-') {
			switch (step) {
				case _ParseNone :
					outputBuf[j++] = c;
					step = _ParseSign;
					break;
				case _ParseInt :
				case _ParseFrac :
				case _ParseExp :
					step = _ParseNext;
					break;
				case _ParseE :
					outputBuf[j++] = c;
					step = _ParseExp;
					break;
				case _ParsePostImaginary:
					step = _ParseNext;
					break;
				default :
					step = _ParseIllegal;
			}
		}
		else if (c >= '0' && c <= '9') {
			switch (step) {
				case _ParseNone :
				case _ParseSign :
					step = _ParseInt;
				case _ParseInt :
				case _ParseFrac :
				case _ParseExp :
					outputBuf[j++] = c;
					(*digits)++;			
					break;
				case _ParseE :
					outputBuf[j++] = c;
					(*digits)++;			
					step = _ParseExp;
					break;
				default :
					step = _ParseIllegal;
			}
		}
		else if (flags.f.thousands_separators && c == sep) {
			switch (step) {
				case _ParseInt :
				case _ParseFrac :
					break;
				default :
					step = _ParseIllegal;
			}
		}
		else if (c == dot) {
			switch (step) {
				case _ParseNone :
				case _ParseSign :
					outputBuf[j++] = '0';
				case _ParseInt :
					outputBuf[j++] = dot;
					step = _ParseFrac;
					break;
				default :
					step = _ParseIllegal;
			}
		}
		else if (c == 'E' || c == 'e') {
			switch (step) {
				case _ParseSign :
					outputBuf[j++] = '1';
					(*digits)++;
				case _ParseInt :
				case _ParseFrac :
					outputBuf[j++] = 'e';
					step = _ParseE;
					break;
				default :
					step = _ParseIllegal;
			}
		}
		else if (c == 'i' && (flags.f.real_result_only == 0)) {
			switch (step) {
				case _ParseNone :
				case _ParseSign :
					*is_im = true;
					step = _ParseInt;
					break;
				case _ParseInt :
				case _ParseFrac :
				case _ParseExp :
					*is_im = true;
					step = _ParsePostImaginary;
					break;
				default :
					step = _ParseIllegal;
			}
		}
		else if (c == ' ') {
			switch (step) {
				case _ParseNone :
				case _ParseSign :
					break;
				case _ParseInt :
				case _ParseFrac :
				case _ParseExp :
				case _ParsePostImaginary:
					step = _ParseNext;
					break;
				default :
					step = _ParseIllegal;
			}
		}
		else if (step != _ParseNone) {
			// if parsing not engaged, implicit go on, else illegal
			step = _ParseIllegal;
		}
//		if (step == _ParseNext && c == 'i') {
//			// skip for next scan
//			i++;
//		}
		// one more char parsed, should we go on ? 
		if (step == _ParseIllegal || step == _ParseNext) {
			if (*digits == 0 && *is_im) {
				// just 'i'
				outputBuf[j++] = '1';
				(*digits)++;
				break;
			}
			else if (*digits == 0) {
				// skip header 
				step = _ParseNone;
			}
			else {
				break;
			}
		}
		i++;
	}
	if (i == inputLen) {
		step = _ParseIllegal;
	}
	*lastParsed = i;
	*outputLen = j;
	return (step);
}


/* anumdel
 *
 * extract real or complex numbers from Alpha reg
 */
int docmd_anumdel(arg_struct *arg) {
	phloat p[2];
	vartype *v;
	bool e, is_im[2] = {false,false};
	char buf[44];
	// digit count
	int step, k, lastParsed, len , digits;	
	// catch error when converting
	flags.f.numeric_data_input = 0;
	v = NULL;
	k = 0;
	lastParsed = 0;
	len = 0;
	digits = 0;
	step = _ParseNone;
	do {
		step = Xtract_num(step, reg_alpha, reg_alpha_length, &lastParsed, buf, &len, &digits, &(is_im[k]));
		if ((step == _ParseIllegal || step == _ParseNext) && digits != 0) {
			// got a number ?
			e = parse_phloat(buf,len,&p[k]);
			if (e) {
				// one more term...
				if (flags.f.real_result_only == 1) {
					// real only - no need to go ahead / hint, will not take care of 'i'
					v = new_real(p[k]);
					alpha_trim_left(lastParsed);
					step = _ParseDone;
				}
				else if (k == 0) {
					// may be cpx res ? trim alpha from already parsed substring
					if (alpha_trim_left(lastParsed) != 0 && step == _ParseNext) {
						// still something to parse in alpha...
						k++;
						len = 0;
						digits = 0;
						step = _ParseNone;
					}
					else {
						step = _ParseDone;
						if (is_im[0]) {
							// pure imaginary
							p[1] = 0;
							v = new_complex(p[1],p[0]);
						}
						else {
							// real
							v = new_real(p[0]);
						}
					}
				}
				else {
					step = _ParseDone;
					// two numbers parsed
					if ((is_im[0] ^ is_im[1])){
						// if cpx res & k == 1 > check imaginary and real part & exit loop
						alpha_trim_left(lastParsed);
						v = (is_im[0] == 1) ? new_complex(p[1],p[0]) : new_complex(p[0],p[1]);
					}
					else {
						// correct digit input, get only first number
						if (is_im[0]) {
							// pure imaginary
							p[1] = 0;
							v = new_complex(p[1],p[0]);
						}
						else {
							// real
							v = new_real(p[0]);
						}
					}
				}
			}
			else {
				if (alpha_trim_left(lastParsed) != 0) {
					step = _ParseNone;
				}
				else {
					step = _ParseDone;
				}
			}
		}
		else {
			// no digits - remove substring
			alpha_trim_left(lastParsed);
			step = _ParseDone;
		}
	} while (step != _ParseDone);
	if (flags.f.trace_print && flags.f.printer_exists) {
		docmd_pra(NULL);
	}
    if (v == NULL) {
		flags.f.numeric_data_input = 0;
	}
	else {
		if (!flags.f.prgm_mode) {
            mode_number_entry = false;
		}
        recall_result(v);
		flags.f.numeric_data_input = 1;
        flags.f.stack_lift_disable = 0;
        flags.f.message = 0;
        flags.f.two_line_message = 0;
	    redisplay();
   }
 	return 0;
}

int docmd_clrdev(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_clrdev;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_clrdev(ERR_NONE);
}

static int defrd_clrdev(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		ILCMD_AAU;
		hpil_step = 0;
		hpil_completion = hpil_clrdev_completion;
		mode_interruptible = hpil_worker;
		mode_stoppable = true;
	}
	return error;
}

int docmd_clrloop(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_clrloop;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_clrloop(ERR_NONE);
}

static int defrd_clrloop(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		ILCMD_AAU;
		hpil_step = 0;
		hpil_completion = hpil_clrloop_completion;
		mode_interruptible = hpil_worker;
		mode_stoppable = true;
	}
	return error;
}

int docmd_inac(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_inac;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_inac(ERR_NONE);
}

static int defrd_inac(int error) {
	if (error == ERR_NONE) {
		error = mappable_x_char(&hpilXCore.endChar);
		if (error == ERR_NONE) {
			error = ERR_INTERRUPTIBLE;
			reg_alpha_length = 1;
			reg_alpha[0]='D';
			ioEndMethod = MethodChar;
			ioTargetRegister = AlphaRegister;
			ILCMD_AAU;
			hpil_step = 0;
			hpil_completion = hpil_inx_completion;
			mode_interruptible = hpil_worker;
			mode_stoppable = true;
		}
	}
	return error;
}

int docmd_inacl(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_inacl;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_inacl(ERR_NONE);
}

static int defrd_inacl(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
	    reg_alpha_length = 1;
		reg_alpha[0]='D';
		ioEndMethod = MethodCrLf;
		ioTargetRegister = AlphaRegister;
		ILCMD_AAU;
		hpil_step = 0;
		hpil_completion = hpil_inx_completion;
		mode_interruptible = hpil_worker;
		mode_stoppable = true;
	}
	return error;
}

int docmd_inae(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_inae;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_inae(ERR_NONE);
}

static int defrd_inae(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		reg_alpha_length = 1;
		reg_alpha[0]='D';
		ioEndMethod = MethodEnd;
		ioTargetRegister = AlphaRegister;
		ILCMD_AAU;
		hpil_step = 0;
		hpil_completion = hpil_inx_completion;
		mode_interruptible = hpil_worker;
		mode_stoppable = true;
	}
	return error;
}

int docmd_inan(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_inan;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_inan(ERR_NONE);
}

static int defrd_inan(int error) {
	if (error == ERR_NONE) {
		error = mappable_x_hpil(43,&ioLength);
		if (error == ERR_NONE) {
			error = ERR_INTERRUPTIBLE;
			reg_alpha_length = 1;
			reg_alpha[0]='D';
			ioEndMethod = MethodLen;
			ioTargetRegister = AlphaRegister;
			ILCMD_AAU;
			hpil_step = 0;
			hpil_completion = hpil_inx_completion;
			mode_interruptible = hpil_worker;
		}
	}
	return error;
}

int docmd_inxb(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_inxb;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_inxb(ERR_NONE);
}

static int defrd_inxb(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;	
		ioEndMethod = MethodNone;
		ioTargetRegister = XRegister;
		ILCMD_AAU;
		hpil_step = 0;
		hpil_completion = hpil_inx_completion;
		mode_interruptible = hpil_worker;
	}
	return error;
}

/* inaccl
 *
 * Special function to keep with csv type line
 * read up to c or crlf character
 * if condition Ok, flag 17 is cleared
 * if c, flag 18 is reset
 * if crlf, flag 18 is set
 */
int docmd_inaccl(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_inaccl;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_inaccl(ERR_NONE);
}

static int defrd_inaccl(int error) {
	if (error == ERR_NONE) {
		error = mappable_x_char(&hpilXCore.endChar);
		if (error == ERR_NONE) {
			error = ERR_INTERRUPTIBLE;	
		    reg_alpha_length = 1;
			reg_alpha[0]='D';
			ioEndMethod = MethodCharOrCrlf;
			ioTargetRegister = AlphaRegister;
			ILCMD_AAU;
			hpil_step = 0;
			hpil_completion = hpil_inx_completion;
			mode_interruptible = hpil_worker;
		}
	}
	return error;
}

int docmd_outac(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_outac;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_outac(ERR_NONE);
}

static int defrd_outac(int error) {
	int i;
	if (error == ERR_NONE) {
		error = mappable_x_char(&hpilXCore.endChar);
		if (error == ERR_NONE) {
			error = ERR_INTERRUPTIBLE;	
			for (i = 0; i < (reg_alpha_length - 1); i++) {
				hpilXCore.buf[i] = reg_alpha[i+1];
			}
			hpilXCore.buf[i++] = hpilXCore.endChar;
			hpilXCore.bufSize = i;
			ILCMD_AAU;
			hpil_step = 0;
			hpil_completion = hpil_outx_completion;
			mode_interruptible = hpil_worker;
		}
	}
	return error;
}

int docmd_outacl(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_outacl;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_outacl(ERR_NONE);
}

static int defrd_outacl(int error) {
	int i;
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;	
		for (i = 0; i < (reg_alpha_length - 1); i++) {
			hpilXCore.buf[i] = reg_alpha[i+1];
		}
		hpilXCore.buf[i++] = 0x0d;
		hpilXCore.buf[i++] = 0x0a;
		hpilXCore.bufSize = i;
		ILCMD_AAU;
		hpil_step = 0;
		hpil_completion = hpil_outx_completion;
		mode_interruptible = hpil_worker;
	}
	return error;
}

int docmd_outae(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_outae;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_outae(ERR_NONE);
}

static int defrd_outae(int error) {
	int i;
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;	
		for (i = 0; i < (reg_alpha_length - 1); i++) {
			hpilXCore.buf[i] = reg_alpha[i+1];
		}
		hpilXCore.bufSize = i;
		hpilXCore.statusFlags |= LastIsEndTalkBuf;
		ILCMD_AAU;
		hpil_step = 0;
		hpil_completion = hpil_outx_completion;
		mode_interruptible = hpil_worker;
	}
	return error;
}

int docmd_outan(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_outan;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_outan(ERR_NONE);
}

static int defrd_outan(int error) {
	int i;
	if (error == ERR_NONE) {
		error =  mappable_x_hpil(43,&ioLength);
		if (error == ERR_NONE) {
			error = ERR_INTERRUPTIBLE;	
			if (ioLength > (reg_alpha_length - 1) || ioLength == 0) {
				ioLength = reg_alpha_length; 
			}
			for (i = 0; i < ioLength; i++) {
				hpilXCore.buf[i] = reg_alpha[i+1];
			}
			hpilXCore.bufSize = i;
			ILCMD_AAU;
			hpil_step = 0;
			hpil_completion = hpil_outx_completion;
			mode_interruptible = hpil_worker;
		}
	}
	return error;
}

int docmd_outxb(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_outxb;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_outxb(ERR_NONE);
}

static int defrd_outxb(int error) {
	if (error == ERR_NONE) {
		error = mappable_x_hpil(255,(int *)hpilXCore.buf);
		if (error == ERR_NONE) {
			error = ERR_INTERRUPTIBLE;	
			hpilXCore.bufSize = 1;
			ILCMD_AAU;
			hpil_step = 0;
			hpil_completion = hpil_outx_completion;
			mode_interruptible = hpil_worker;
		}
	}
	return error;

}

static int hpil_clrdev_completion(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		switch (hpil_step) {
			case 0 :		// AAU > AAD
				ILCMD_AAD(0x01);
				break;
			case 1 :		// AAD > TAD Selected
				ILCMD_LAD(hpil_settings.selected);
				break;
			case 2 :		// TAD > SDC
				ILCMD_SDC;
				break;
			case 3 :		// SDC > UNL
				ILCMD_UNL;
				break;
			default :
				error = ERR_NONE;
		}
		hpil_step++;
	}
	return error;
}

static int hpil_clrloop_completion(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		switch (hpil_step) {
			case 0 :		// AAU > AAD
				ILCMD_AAD(0x01);
				break;
			case 1 :		// AAD > TAD Selected
				ILCMD_DCL;
				break;
			default :
				error = ERR_NONE;
		}
		hpil_step++;
	}
	return error;
}

static int hpil_inx_completion(int error) {
	int i;
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		switch (hpil_step) {
			case 0 :		// AAU > AAD
				ILCMD_AAD(0x01);
				hpil_step++;
				break;
			case 1 :		// AAD > TAD Selected
				ILCMD_TAD(hpil_settings.selected);
				hpil_step++;
				break;
			case 2 :		// TAD > ltn & prepare buffer
				ILCMD_ltn;
				hpil_step++;
				switch (ioEndMethod) {
					case MethodNone :
						hpilXCore.bufSize = (ioTargetRegister == AlphaRegister) ? 43 : 1;
						break;
					case MethodChar :
						hpilXCore.bufSize = 43;
						hpilXCore.statusFlags |= ListenTilChar;
						break;
					case MethodCrLf :
						hpilXCore.bufSize = 43;
						hpilXCore.statusFlags |= ListenTilCrLf;
						break;
					case MethodEnd :
						hpilXCore.bufSize = 43;
						hpilXCore.statusFlags |= ListenTilEnd;
						break;
					case MethodLen :
						hpilXCore.bufSize = ioLength;
						break;
					case MethodCharOrCrlf :
						hpilXCore.bufSize = 43;
						hpilXCore.statusFlags |= (ListenTilChar | ListenTilCrLf);
						break;
					default :
						error = ERR_INTERNAL_ERROR;
				}
				hpilXCore.bufPtr = 0;
				break;
			case 3 :		// Local listen > SDA
				ILCMD_SDA;
				hpil_step++;
				break;
			case 4 :		// SDA > lun
				ILCMD_lun;
				hpil_step++;
				break;
			case 5 :		// lun > UNT;
				ILCMD_UNT;
				hpil_step++;
				break;
			case 6 :		// get result
				if (hpilXCore.bufPtr == 0) {
					error = ERR_NO_RESPONSE;
				}
				else {
					error = ERR_NONE;
					if (ioTargetRegister == AlphaRegister) {
						for (i = 0; i < (hpilXCore.bufPtr > 43 ? 43 : hpilXCore.bufPtr); i++) {
							reg_alpha[i+1] = hpilXCore.buf[i];
						}
						reg_alpha_length = i+1;
						if (flags.f.trace_print && flags.f.printer_exists) {
							docmd_pra(NULL);
							if (hpil_pending) {
								// inserted in hpil loop
								hpil_step++;
								if (insert_ilCompletion() == ERR_INTERRUPTIBLE) {
									error = ERR_INTERRUPTIBLE;
									ILCMD_AAU;
								}
							}
						}
						switch (ioEndMethod) {
							case MethodChar :
								flags.f.hpil_ina_err = (hpilXCore.statusFlags & ListenTilChar) ? 1 : 0;
								break;
							case MethodCrLf :
								flags.f.hpil_ina_err = (hpilXCore.statusFlags & ListenTilCrLf) ? 1 : 0;
								break;
							case MethodEnd :
								flags.f.hpil_ina_err = (hpilXCore.statusFlags & ListenTilEnd) ? 1 : 0;
								break;
							case MethodCharOrCrlf :
								flags.f.hpil_ina_err = ((hpilXCore.statusFlags & (ListenTilChar | ListenTilCrLf)) == (ListenTilChar | ListenTilCrLf)) ? 1 : 0;
								flags.f.hpil_ina_eol = (hpilXCore.statusFlags & ListenTilCrLf) ? 0 : 1;
								break;
							default :
								// should not occur
								flags.f.hpil_ina_err = 1;
								flags.f.hpil_ina_eol = 0;
						}
					}
					else {
						vartype *v = new_real(hpilXCore.buf[0] & 0xff);
						if (v == NULL) {
							error = ERR_INSUFFICIENT_MEMORY;
						}
						else {
							recall_result(v);
							if (hpil_pending) {
								// inserted in hpil loop
								hpil_step++;
								if (insert_ilCompletion() == ERR_INTERRUPTIBLE) {
									error = ERR_INTERRUPTIBLE;
									ILCMD_AAU;
								}
							}
						}
					}
				}
				hpilXCore.statusFlags &= ~(ListenTilEnd | ListenTilCrLf	| ListenTilChar);
				break;
			case 7 :		// just to let background printing occur
				error = ERR_NONE;
				break;
			default :
				error = ERR_INTERNAL_ERROR;
		}
	}
	if (error != ERR_INTERRUPTIBLE) {
		// ultimate cleanup
		hpilXCore.statusFlags &= ~(ListenTilEnd | ListenTilCrLf	| ListenTilChar);
	}
	return error;
}

static int hpil_outx_completion(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		switch (hpil_step) {
			case 0 :		// AAU > AAD
				ILCMD_AAD(0x01);
				break;
			case 1 :		// AAD > LAD Selected
				ILCMD_LAD(hpil_settings.selected);
				break;
			case 2 :		// LAD > tlk
				ILCMD_tlk;
				hpilXCore.bufPtr = 0;
				break;
			case 3 :		// LAD > UNL
				ILCMD_UNL;
				break;
			default :
				error = ERR_NONE;
		}
	}
	hpil_step++;
	return error;
}
