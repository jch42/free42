/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
 * Free42 eXtensions -- adding HP-IL to free42
 * Copyright (C) 2014-2020 Jean-Christophe HESSEMANN
 *
 * This program is free software// you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY// without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program// if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#include <stdio.h>

#include "stdafx.h"

#include "core_display.h"
#include "core_ebml.h"
#include "core_globals.h"
#include "core_helpers.h"
#include "core_main.h"
#include "hpil_common.h"
#include "hpil_controller.h"
#include "hpil_plotter.h"
#include "hpil_printer.h"
#include "shell.h"
#include "shell_extensions.h"
#include "string.h"

HPIL_Settings hpil_settings;

HPIL_Controller hpil_core;
HpilXController hpilXCore;
int controllerCommand;
int frame;
uint4 loopTimeout;
AltBuf hpil_controllerAltBuf;
DataBuf hpil_controllerDataBuf;

void (*hpil_emptyListenBuffer)(void);
void (*hpil_fillTalkBuffer)(void);

// hpil completion nano machine & stack
int (*hpil_completion)(int);
int hpil_step;
#define Hpil_stack_depth 10
int (*hpil_completion_st[Hpil_stack_depth])(int);
int hpil_step_st[Hpil_stack_depth];
int hpil_sp;
// enable some background processing within an running hpil operation
int (*hpil_pending)(int);

int IFCRunning;
int IFCCountdown, IFCNoisy;		// IFC tries and IFC noisy ignored frame 
char hpil_text[23];				// generic test buffer
uint4 lastTimeCheck, timeCheck;

int lateRx;						// async received frame
int waitingRx;					// frame send / frame received indicator
#define FRAMESENDONLOOP		1
#define WAITINGFRAMERETURN	2
#define LATEFRAMERETURN		3
#define NOFRAMERETURNED		4

AlphaSplit alphaSplit;

void clear_ilStack();
int hpil_write_frame(int frame);
int hpil_read_frame(int *frameRx);

/* restart
 *
 * performs power up sequence
 * and controller state initialization
 * but does not clear stack
 */
void hpil_restart(void) {
	hpilXCore.buf = hpil_controllerDataBuf.data;
	hpilXCore.bufSize = ControllerDataBufSize;
	hpilXCore.bufPtr = 0;
	hpilXCore.statusFlags = 0;
	hpil_core.begin(&hpilXCore);
}

/* start
 *
 * same as above
 * but reset stack and completion
 *
 */
void hpil_start(void) {
	hpil_restart();
	hpil_completion = NULL;
	hpil_pending = NULL;
	hpil_settings.printStacked = 0;
	clear_ilStack();
}

/* IL init
 *
 */
void hpil_init(bool modeEnabled, bool modePIL_Box) {
	hpil_settings.modeEnabled = modeEnabled;
	hpil_settings.modePIL_Box = modePIL_Box;
	hpil_start();
	// should init all hpil modules
	hpil_plotter_init();
	if (hpil_settings.modePIL_Box) {
		shell_write_frame(M_CON);
		if (!(hpil_read_frame(&frame) == 1 && frame == M_CON)) {
			hpil_settings.modeEnabled = false;
		}
	}
	if (hpil_settings.modeEnabled) {
		ILCMD_IFC;
		IFCRunning = 1;
		IFCCountdown = IFCNoisy = 5;
		shell_set_shadow();
	}
}

void hpil_close(bool modeEnabled, bool modePil_Box) {
	if (hpil_settings.modePIL_Box) {
		shell_write_frame(M_COFF);
		hpil_read_frame(&frame);
	}
}

/* hpil_checkup
 *
 * check command validity and hpil loop input / output connected
 */
int hpil_check() {
	int err;
	if (!core_settings.enable_ext_hpil) {
        err = ERR_NONEXISTENT;
	}
	else if (!hpil_settings.modeEnabled) {
		err = ERR_BROKEN_LOOP;
	}
	else if (hpilXCore.statusFlags & (CmdNew | CmdRun)) {
		// another already running hpil command...
		if (hpil_completion == hpil_print_completion) {
			err = ERR_OVERLAPED_OPERATION;
		}
		else {
			err = ERR_RESTRICTED_OPERATION;
		}
	}
	else { 
		err = shell_check_connectivity();
	}
	return err;
}

/* hpil_worker
 *
 * main, interruptible loop for processing hpil messages
 */
int hpil_worker(int interrupted) {
	int err = ERR_INTERRUPTIBLE;
	int saveIfc;
	// debug...
	//char s[100];
	if (interrupted) {
        err = ERR_STOP;
	}
	// New command ?
	else  if (hpilXCore.statusFlags & CmdNew) {
		// clear buffers status flags
		hpilXCore.statusFlags &= ~MaskBuf;
		if (controllerCommand == Local) {
			// fake command, next step
			hpilXCore.statusFlags &= ~CmdNew;
			if (hpil_completion) {
				err =  hpil_completion(ERR_NONE);
			}
			else {
				hpil_core.pseudoSet(hshk);
			}
		}
		else if (controllerCommand == (Local | pse)) {
			// fake command, just pause
			if (waitingRx == NOFRAMERETURNED) {
				waitingRx = 0;
				hpilXCore.statusFlags &= ~CmdNew;
				if (hpil_completion) {
					err =  hpil_completion(ERR_NONE);
				}
				else {
					hpil_core.pseudoSet(hshk);
				}
			}
			else if (waitingRx != WAITINGFRAMERETURN) {
				// dummy read !
				waitingRx = FRAMESENDONLOOP;
				hpil_read_frame(NULL);
			}
		}
		else {
			if (controllerCommand & Local) {
				// local controller command
				hpilXCore.statusFlags &= ~CmdNew;
				hpilXCore.statusFlags |= CmdRun;
				hpil_core.pseudoSet(controllerCommand & ~Local);
				if (!(controllerCommand & tlk)) {
					// simulate hshk for local commands
					hpil_core.pseudoSet(hshk);
				}
			}
			else {
				// anything else
				hpilXCore.statusFlags &= ~CmdNew;
				hpilXCore.statusFlags |= CmdRun;
				hpil_core.controllerCmd(controllerCommand);
			}
			while (hpil_core.process()) {
				if (hpil_core.pseudoTcl(outf)) {
					frame = hpil_core.frameTx();
					if (hpil_write_frame(frame) != ERR_NONE) {
						err = ERR_BROKEN_IP;
					}
				}
			}
		}
	}
	else if (waitingRx == FRAMESENDONLOOP || waitingRx == LATEFRAMERETURN) {
		if (hpil_read_frame(&frame)) {
			hpil_core.frameRx((uint16_t)frame);
			if (IFCRunning) {
				saveIfc= IFCRunning;
				switch (IFCRunning) {
					case 1 :
						if (frame == M_IFC) {					// > IFC has been looped back
							while (hpil_core.process()) {
								if (hpil_core.pseudoTcl(outf)) {
									frame = hpil_core.frameTx();
									if (hpil_write_frame(frame) != ERR_NONE) {
										// check on first send only
										err = ERR_BROKEN_IP;
									}
								}
							}
							IFCRunning++;
						}
						break;
					case 2 :	
						if (frame == M_RFC) {					// > RFC looped too	
							while (hpil_core.process());
							IFCRunning = 0;
							hpil_settings.modeEnabled = true;
							flags.f.VIRTUAL_input = hpil_settings.modeEnabled;
						}
						break;
				}
				if (IFCRunning == saveIfc)  {					// > IFC no change
					if (IFCNoisy --) {
						hpil_write_frame(-1);					// fake write to reactivate frame rx
					}
					else if (IFCCountdown--) {					// > IFC timeout
						hpil_core.begin(&hpilXCore);
						hpilXCore.statusFlags = 0;
						ILCMD_IFC;
					}
				}
			}
			else {
				while (hpil_core.process()) {
					if (hpil_core.pseudoTcl(outf)) {
						frame = hpil_core.frameTx();
						if (hpil_write_frame(frame) != ERR_NONE) {
							err = ERR_BROKEN_IP;
						}
					}
				}
			}
		}
		else {
			// waiting returning frame...
			err = ERR_SHADOWRUNNING;
		}
	}
	else if (waitingRx == NOFRAMERETURNED) {
		if (IFCRunning && IFCCountdown--) {			// > IFC timeout
			hpil_core.begin(&hpilXCore);
			hpilXCore.statusFlags = 0;
			ILCMD_IFC;
		}
		else {
			err = ERR_BROKEN_LOOP;
		}
	}
	else {
		while (hpil_core.process()) {
			if (hpil_core.pseudoTcl(outf)) {
				frame = hpil_core.frameTx();
				if (hpil_write_frame(frame) != ERR_NONE) {
					err = ERR_BROKEN_IP;
				}
			}
		}
	}
	// check end of current command
	if (err == ERR_INTERRUPTIBLE && hpil_core.pseudoTcl(hshk)){
		hpilXCore.statusFlags &= ~CmdRun;
		hpilXCore.statusFlags |= CmdHshk;
		if (hpil_completion) {
			err = hpil_completion(ERR_NONE);
		}
		else {
			err = ERR_NONE;
		}
	}
	if (err == ERR_NONE || err == ERR_INTERRUPTIBLE || err == ERR_RUN) {
		if (err == ERR_NONE) {
			// clear hpil_completion
			hpil_completion = NULL;
		}
		// check buffers level and process
		if (hpilXCore.statusFlags & FullListenBuf) {
			if (hpilXCore.statusFlags & RunAgainListenBuf) {
				hpilXCore.statusFlags &= ~FullListenBuf;
			}
			if (hpil_emptyListenBuffer) {
				hpil_emptyListenBuffer();
				hpilXCore.bufPtr = 0;
			}
		}
		else if ((hpilXCore.statusFlags & EmptyTalkBuf) && (hpilXCore.statusFlags & RunAgainTalkBuf)) {
			hpilXCore.statusFlags &= ~EmptyTalkBuf;
			if (hpil_fillTalkBuffer) {
				hpil_fillTalkBuffer();
				hpilXCore.bufPtr = 0;
				if (!(hpilXCore.statusFlags & RunAgainTalkBuf)) {
					hpil_fillTalkBuffer = NULL;
				}
			}
			else {
				hpilXCore.statusFlags &= ~RunAgainTalkBuf;
			}
		}
	}
	else if (err == ERR_SHADOWRUNNING) {
		// nothing to do
	}
	else {
		hpil_start();
        shell_annunciators(-1, -1, 0, -1, -1, -1);
	}
	return err;
}

/* hpil_rxWorker
 *
 * place holder for differed frame rx 
 */
void hpil_rxWorker(int frameOk, int frameRx) {

	if (frameOk) {
		lateRx = frameRx;
		waitingRx = LATEFRAMERETURN;
	}
	else {
		waitingRx = NOFRAMERETURNED;
	}
}

/* hpil_enterShadow()
 *
 * bootstrap to shell
 */
int hpil_enterShadow() {
	return shell_set_shadow();
}

/* hpil_write_frame
 * 
 * send frame, prepare for timeout
 */
int hpil_write_frame(int frameTx) {

	if ((frameTx & 0x0600) == 0x0600) {	// IDY
		loopTimeout = 250;				//  -> 250 ms (augmented from initial 50 ms, too much latency for idy)
	}
	else if (frameTx == 0x0490) {		// IFC - special timing - increase timing at each loop
		loopTimeout = 500 + (100 * (5 - IFCCountdown));
	}
	else if (frameTx & 0x0400) {		// CMD
		loopTimeout = 1000;				//  -> 1000 ms
	}
	else {								// DOE or RDY
		loopTimeout = 3000;				//  -> 3000 ms
	}
	if (frameTx != -1) {
		if (shell_write_frame(frameTx)) {
			return ERR_BROKEN_IP;
		}
	}
	else {								// Idy noisy frames, empty pipe
		loopTimeout = 0;
	}
	waitingRx = FRAMESENDONLOOP;
	return ERR_NONE;
}

/* hpil_read_frame
 * 
 * receive frame
 */
int hpil_read_frame(int *frameRx) {
	int frameOk;
	switch (waitingRx) {
		case FRAMESENDONLOOP:
			// first attempt after frame emission
			frameOk = shell_read_frame(frameRx, loopTimeout);
			if (frameOk) {
				waitingRx = 0;
			}
			else {
				waitingRx = WAITINGFRAMERETURN;
			}
			return frameOk;
			break;
		case LATEFRAMERETURN:
			// frame return, finally
			*frameRx = lateRx;
			waitingRx = 0;
			return 1;
			break;
		default:
			// WAITINGFRAMERETURN should not occur
			// NOFRAMERETURNED broken loop ???
			// anything else should not occur
			waitingRx = 0;
			return -1;
			break;
	}
}

/* IL stack
 *
 * to enable il subroutines calls
 */
int call_ilCompletion(int (*hpil_completion_call)(int)) {
    int err = ERR_INTERRUPTIBLE;
    if (hpil_sp < Hpil_stack_depth - 1) {
		hpil_completion_st[hpil_sp] = hpil_completion;
		hpil_step_st[hpil_sp] = hpil_step;
		hpil_sp++;
		hpil_completion = hpil_completion_call;
		hpil_step = 0;
    }
	else {
        err = ERR_IL_ERROR;
	}
    return err;
}

/* IL stack
 *
 * insert 'background' processing 
 */
int insert_ilCompletion() {
    int err = ERR_INTERRUPTIBLE;
    if (hpil_sp < Hpil_stack_depth - 1) {
		hpil_completion_st[hpil_sp] = hpil_completion;
		hpil_step_st[hpil_sp] = hpil_step;
		hpil_sp++;
		hpil_completion = hpil_pending;
		hpil_step = 0;
    }
	else {
        err = ERR_IL_ERROR;
	}
	hpil_pending = NULL;
    return err;
}

/* IL stack return
 *
 * counterpart for call
 */
int rtn_ilCompletion() {
    int err = ERR_INTERRUPTIBLE;
    if (hpil_sp == 0) {
        err = ERR_IL_ERROR;
	}
	else {
		hpil_sp--;
		hpil_completion = hpil_completion_st[hpil_sp];
		hpil_step = hpil_step_st[hpil_sp];
    }
	return err;
}

/* IL stack init
 *
 * simply set sp to 0
 */
void clear_ilStack() {
    hpil_sp = 0;
}

/* AID internal Command
 *
 * read device ID
 * LAD to be done by caller
 */
int hpil_aid_sub(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		switch (hpil_step) {
			case 0 :		// > buffer clear & ltn
				hpilXCore.bufPtr = 0;
				ILCMD_ltn;
				hpil_step++;
				break;
			case 1 :		// > SAI
				ILCMD_SAI;
				hpil_step++;
				break;
			case 2 :		// lun
				ILCMD_lun;
				hpil_step++;
				break;
			case 3 :		// UNT
				ILCMD_UNT;
				error = rtn_ilCompletion();
				break;
			default :
				error = ERR_NONE;
		}
	}
	return error;
}

/* hpil display and pause
 *
 */
int hpil_displayAndPause_sub(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		switch (hpil_step) {
			case 0 :
				hpil_step++;;
				scroll_display(1,hpil_text,22);
		        if ((flags.f.trace_print || flags.f.normal_print)
				  && flags.f.printer_exists) {
					print_text(hpil_text,22,1);
					if (hpil_pending) {
						// inserted in hpil loop
						if (insert_ilCompletion() == ERR_INTERRUPTIBLE) {
							ILCMD_AAU;
						}
					}
					else {
						// just to let the loop run on
						ILCMD_nop;
					}
				}
				else {
					// just to let the loop run on
					ILCMD_nop;
				}
				break;
			case 1 :
				hpil_step++;
				ILCMD_pse;
				break;
			case 2 :
				ILCMD_nop;
				error = rtn_ilCompletion();
				break;
			default :
				error = ERR_IL_ERROR;
		}
	}
	return error;
}

/* mappable_x_hpil()
 *
 * Validate x value for message assembly
 */
int mappable_x_hpil(int max, int *cmd) {
	int arg;
    if (reg_x->type == TYPE_REAL) {
        phloat x = ((vartype_real *) reg_x)->x;
		if ((x < 0) || (x > 65535 )) {
            return ERR_INVALID_DATA;
		}
        arg = to_int(x);
		if (x > max) {
            return ERR_INVALID_DATA;
		}
	}
	else {
        return ERR_INVALID_TYPE;
	}
	*cmd = (uint16_t) arg;
    return ERR_NONE;
}

/* mappable_device_hpil()
 *
 * Validate x value for message assembly
 * accepting -1, disable device, especially printer
 */
int mappable_device_hpil(int max, int *cmd) {
	int arg;
    if (reg_x->type == TYPE_REAL) {
        phloat x = ((vartype_real *) reg_x)->x;
		if ((x < -1) || (x > 32767 )) {
            return ERR_INVALID_DATA;
		}
        arg = to_int(x);
		if (x > max) {
            return ERR_INVALID_DATA;
		}
	}
	else {
        return ERR_INVALID_TYPE;
	}
	*cmd = (uint32_t) arg;
    return ERR_NONE;
}

/* mappable_x_char()
 *
 * Validate x value for a char
 */
int mappable_x_char(uint8_t *c) {
	phloat x;
    vartype_string *s;
	if (reg_x->type == TYPE_REAL) {
        phloat x = ((vartype_real *) reg_x)->x;
		if (x < 0) {
            x = -x;
		}
		if (x > 255) {
			return ERR_INVALID_DATA;
		}
		*c = (uint8_t)to_int(x);
    }
	else if (reg_x->type == TYPE_STRING) {
		s = (vartype_string *) reg_x;
		if (s->length == 0) {
			return ERR_INVALID_DATA;
		}
		*c = (uint8_t) s->text[0];
	}
	else {
	    return ERR_INVALID_TYPE;
	}
	return ERR_NONE;
}

/* splitAlphaReg
 *
 * extract strings from alpha reg
 */
int hpil_splitAlphaReg(int mode) {
	int i;
	int dotSeen;
	// check alpha format
	alphaSplit.len1 = 0;
	alphaSplit.len2 = 0;
	dotSeen = false;
	if (reg_alpha_length == 0) {
		return ERR_ALPHA_DATA_IS_INVALID;
	}
	for (i = 0; i < reg_alpha_length; i++) {
		if ((reg_alpha[i] == '.') && !dotSeen) {
			dotSeen = true;
		}
		//else if ((reg_alpha[i] == '*') && (i == (reg_alpha_length - 1)) && dotSeen) {
		//	starSeen = true;
		//}
		else if (!dotSeen && (alphaSplit.len1 < 10)) {
			alphaSplit.str1[alphaSplit.len1++] = reg_alpha[i];
		}
		else if (dotSeen && (alphaSplit.len2 < 7)) {
			alphaSplit.str2[alphaSplit.len2++] = reg_alpha[i];
		}
		else {
			return ERR_ALPHA_DATA_IS_INVALID;
		}
	}
	// pad file name
	if (mode & SPLIT_MODE_VAR) {
		for (alphaSplit.len1; alphaSplit.len1 < 10; alphaSplit.len1++) {
			alphaSplit.str1[alphaSplit.len1] = ' ';
		}
	}
	if ((mode & SPLIT_MODE_PRGM) && (alphaSplit.len2 != 0)) {
		for (alphaSplit.len2; alphaSplit.len2 < 10; alphaSplit.len2++) {
			alphaSplit.str2[alphaSplit.len2] = ' ';
		}
	}
	return ERR_NONE;
}
