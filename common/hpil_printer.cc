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

/* printer code
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "core_display.h"
#include "core_globals.h"
#include "core_main.h"
#include "core_variables.h"
#include "hpil_common.h"
#include "hpil_controller.h"
#include "hpil_printer.h"
#include "shell.h"

/* common vars
 *
 */
extern HPIL_Settings hpil_settings;
extern HpilXController hpilXCore;
extern int controllerCommand;
extern int (*hpil_completion)(int);
extern int (*hpil_pending)(int);
extern int hpil_step;
extern int hpil_sp;
extern int hpil_worker(int interrupted);
extern void (*hpil_emptyListenBuffer)(void);
extern void (*hpil_fillTalkBuffer)(void);
extern int frame;

/* buffers structs
 *
 */
extern AltBuf hpil_controllerAltBuf;
extern DataBuf hpil_controllerDataBuf;

/* Status byte definitions for 82162 
 * First byte
 */
#define PrinterStatusER 0x08	// Error

/* Status byte definitions for 82162 
 * Second byte
 */
#define PrinterStatusLC 0x01	// Lower Case
#define PrinterStatusCO 0x02	// Column mode
#define PrinterStatusDW 0x04	// Double Wide
#define PrinterStatusRJ 0x08	// Right Justify
#define PrinterStatusEB 0x10	// Eight Bits mode
#define PrinterStatusBE 0x20	// Buffer Empty
#define PrinterStatusID 0x40	// printer IDle
#define PrinterStatusEL 0x80	// End Line

#define skip0Column		0xb8	// skip column

/* Printer variables
 *
 */
static unsigned char printBuf[540];
static unsigned char *source;
static int textLength;
static int rasterBytesPerLine;
static int rasterX, rasterY, rasterWidth, rasterHeight;
static int rasterMode = 0;

/* Translation table for 82162A printer
 *
 */
const unsigned char TranslateCode[] = {
	0xAF, 0x01, 0x3f, 0x53, 0x1f, 0x7e, 0x3e, 0x7b,
	0x3f, 0x3f, 0x0a, 0x3f, 0x1d, 0x0d, 0x07, 0x7d,
	0x03, 0x0c, 0x1e, 0x3f, 0x13, 0x3f, 0x16, 0x7c,
	0x45, 0x1b, 0x3f, 0x3f, 0x17, 0x1a, 0x1e, 0x3f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0xfb, 0xfc, 0xfd, 0xfe, 0x7f
};

int hpil_print_completion(int error);

/* hpil_printText
 *
 * entry point for print worker
 */
void hpil_printText(const char *text, int length, int left_justified) {
	int i, err;
	if (hpil_settings.prtAid == -1) {
		/* explicit disabled hpil printer */
		return;
	}
	err = hpil_check();
	if (err == ERR_NONE) {
		if (hpil_settings.printStacked++ == 0) {
			// reinit buffer 
			shell_log("starting");
			textLength = 0;
			rasterMode = 0;
		}
		if (hpil_completion != NULL) {
			// printing in an hpil funtion ?
			shell_log("Pending mode");
			hpil_pending = hpil_print_completion;
		}
		else {
			// independant printing
			shell_log("Shadow mode");
			hpil_step = 0;
			hpil_completion = hpil_print_completion;
			hpil_enterShadow();
			ILCMD_AAU;
		}
	}
	else if (err != ERR_OVERLAPED_OPERATION) {
		shell_log("Illegal mode");
		return;
	}
	/* horrible trick
	 * case encountered = prp in normal mode whill issue two lines in the same run...
	 * as hpil printing is hopefully not started yet, just concatenate the lines!
	 */
	if (textLength + length < 535) {
		// store length including length and print formating
		printBuf[textLength++] = length;
		// combine justification / print width & lowercase in one byte
		printBuf[textLength] = 0;
		if (flags.f.lowercase_print) {
			printBuf[textLength] |= PrinterStatusLC;
		}
		if (flags.f.double_wide_print) {
			printBuf[textLength] |= PrinterStatusDW;
		}
		if (!left_justified) {
			printBuf[textLength] |= PrinterStatusRJ;
		}
		textLength++; 
		for (i = 0; i < (length) && textLength < 0x21b; i++) {
			printBuf[textLength++] = text[i];
		}
		printBuf[textLength] = 0xfe;
	}
	else {
		shell_log("Print buffer overflow");
	}
    return;
}

/* hpil_printLCD
 *
 * entry point for print worker
 */
void hpil_printLcd(const char *bits, int bytesperline, int x, int y, int width, int height) {
	int err;
	if (hpil_settings.prtAid == -1) {
		/* explicit disabled hpil printer */
		return;
	}
	err = hpil_check();
	if (err == ERR_NONE) {
		rasterMode = 1;
		hpil_step = 0;
		hpil_completion = hpil_print_completion;
		hpil_enterShadow();
		ILCMD_AAU;
	}
	else if (err != ERR_OVERLAPED_OPERATION) {
		return;
	}
	/* another horrible trick
	 * case encountered = prlcd in normal mode whill issue one text and one graphical in the same run.
	 * so add a special 'next length' tag
	 */
	printBuf[textLength++] = 0xff;
	source = (unsigned char*)bits;
	rasterBytesPerLine = bytesperline;
	rasterX = x;
	rasterY = y;
	rasterWidth = width;
	rasterHeight = height;
}

int hpil_processText(unsigned char *source, unsigned char *output, char status, char aid) {
	int i = 0, j = 0, l, s;
	char eightBitsMode = 0;
	// get length
	l = source[i++] + 2;
	s = source[i++];
	if (aid == 0x20) { 
		// only for 82162A ?
		if (!(status & PrinterStatusEB)) {
			// insert escape sequence to go to 8 bits mode
			output[j++] = 0x1b;
			output[j++] = 0x7c;
			status |= PrinterStatusEB;
			}
		eightBitsMode = true;
		// justification
		if ((status ^ s) & PrinterStatusRJ) { 	
			output[j++] = 0xe0 | (s & PrinterStatusRJ);
		}
		if ((status ^ s) & (PrinterStatusLC | PrinterStatusDW)) {
			output[j++] = 0xd0 | (s & (PrinterStatusLC | PrinterStatusDW));
		}
		while ((i < l) && (j < 0x64)) {
			if (TranslateCode[source[i] & 0x7f] & 0x80) {
				// switch back to 7 bits mode / standard char set, will occur only once...
				if (eightBitsMode) {
					output[j++] = 0xfc;
					eightBitsMode = false;
				}
			}
			output[j++] = TranslateCode[source[i++] & 0x7f] & 0x7f;
		}
		output[j++] = 0x0d;
		output[j++] = 0x0a;
	}
	else {
		// other printers
		while ((i < l) && (j < 0x66)) {
			output[j++] = source[i++] & 0x7f;
		}
		if ((i == textLength) && (j < 0x66)) {
			output[j++] = 0x0d;
			output[j++] = 0x0a;
			i++;
		}
	}
	return j;
}

int hpil_processRaster(char *output, char status, char aid) {
	int x, y, r, i, j, skipped;
	char c;
	r = 0;
	// extract 7 lines
	for (x = rasterX; x < rasterWidth; x++) {
		c = 0;
		for (y = 0; (y + r < 7) && (y + rasterY < rasterHeight); y++) {
			c |= (source[(x >> 3) + (rasterBytesPerLine * (y + rasterY))] & (0x01 << (x & 0x07))) ? 1 << (y % 7) : 0; 
		}
		printBuf[x] = (r == 0) ? c : printBuf[x] | (c << r);
	}
//	if (y + r < 7) {
//		r += y;
//	}
//	else {
//		r = 0;
//	}
		rasterY += y;
	// convert to 82162 format
	j = 0;
	skipped = 0;
	if (!(status & PrinterStatusEB)) {
		output[j++] = 0x1b;
		output[j++] = 0x7c;
		status |= PrinterStatusEB;
	}
	if (status & PrinterStatusRJ) {
		output[j++] = 0xe0;
		status &= ~PrinterStatusRJ;
	}
	if (!(status & PrinterStatusCO)) {
		output[j++] = 0xd2;
		status |= PrinterStatusCO;
	}
	for (i = 0; i < x; i++) {
		if (printBuf[i]) {
			if (skipped) {
				skipped = 0;
				j++;
			}
			output[j++] = printBuf[i];
		}
		else if (skipped < 7) {
			skipped ++;
			output[j] = skip0Column + skipped;
		}
		else {
			skipped = 1;
			j++;
			output[j] = skip0Column + skipped;
		}
	}
	output[j++] = 0xd0;
	status &= ~PrinterStatusCO;
	//hpil_showraster(graphics);
	//hpil_showraster(output);
	output[j++] = 0x0d;
	if (rasterY >= rasterHeight) {
		rasterMode = -1;
	}
	return j;
}

int hpil_print_completion(int error) {
	static int i, n;
	static unsigned char aid = 0;
	static unsigned char *k;
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		switch (hpil_step) {
			case 0 :
				ILCMD_AAD(0x01);
				hpil_step ++;
				break;
			case 1 :		// select device
				n = (frame & 0x001f) - 1;
				if (hpil_settings.prtAid) {
					i = hpil_settings.prtAid;
					hpil_step++;	// explicit device adressing / no loop
				}
				else if (flags.f.manual_IO_mode) {
					i = hpil_settings.selected;
					hpil_step++;	// manual mode / no loop
				}
				else if (hpil_settings.selected) {
					i = hpil_settings.selected;
				}
				else {
					i = 1;
				}
				hpilXCore.buf = hpil_controllerAltBuf.data;
				hpilXCore.bufPtr = 0;
				hpilXCore.bufSize = 1;
				ILCMD_TAD(i);
				hpil_step++;
				error = call_ilCompletion(hpil_aid_sub);
				break;
			case 2 :		// loop for AutoIO mode
				if (hpilXCore.bufPtr && ((hpil_controllerAltBuf.data[0] & 0x20) == 0x20)) {
					ILCMD_nop;
					hpil_step++;
				}
				else {
					if (i < n) {
						ILCMD_TAD(++i);
						error = call_ilCompletion(hpil_aid_sub);
					}
					else {
						ILCMD_nop;
						hpil_step++;
					}
				}
				break;
			case 3 :		// final check for device class, prepare for reading status
				if (hpilXCore.bufPtr) {
					if ((hpil_controllerAltBuf.data[0] & 0x20) == 0x20) {
						k = printBuf;
						aid = hpil_controllerAltBuf.data[0];
						hpil_settings.print = i;
						hpilXCore.bufPtr = 0;
						hpilXCore.bufSize = 2;
						ILCMD_TAD(hpil_settings.print);
						hpil_step++;
					}
					else {
						error = ERR_NO_PRINTER;
					}
				}
				else {
					error = ERR_NO_RESPONSE;
				}
				break;
			case 4 :
				ILCMD_ltn;
				hpil_step++;
				break;
			case 5 :
				ILCMD_SST;
				hpil_step++;
				break;
			case 6 :
				ILCMD_lun;
				if (hpilXCore.bufPtr == 0) {
					error = ERR_NO_RESPONSE;
				}
				else {
					hpil_step++;
				}
				break;
			case 7 :		// > UNT
				ILCMD_UNT;
				hpil_step++;
				break;
			case 8 :		// combine flags & status, prepare and build buffer
				if (hpilXCore.buf[0] & PrinterStatusER) {
					error = ERR_PRINTER_ERR;
				}
				else {
					if (rasterMode == 0) {
						hpilXCore.bufSize = hpil_processText(k, hpil_controllerDataBuf.data, hpilXCore.buf[1], aid);
					}
					else {
						hpilXCore.bufSize = hpil_processRaster((char*)hpil_controllerDataBuf.data, hpilXCore.buf[1], aid);
					}
					hpilXCore.buf = hpil_controllerDataBuf.data;
					hpilXCore.bufPtr = 0;
					ILCMD_LAD(hpil_settings.print);
					hpil_step++;
				}
				break;
			case 9 :
				ILCMD_tlk;
				hpil_step++;
				break;
			case 10 :
				ILCMD_UNL;
				if ((rasterMode == 0) && (k[k[0] + 2] != 0xfe)) {
					if (k[k[0] + 2] == 0xff) {
						// pending graphics
						rasterMode = 1;
					}
					else {
						k = &k[k[0] + 2];
					}
					hpilXCore.bufPtr = 0;
					hpilXCore.bufSize = 2;
				}
				else if (rasterMode == 1) {
					hpilXCore.bufPtr = 0;
				}
				else {
					// jump over
					hpil_step++;
				}
				hpil_step++;
				break;
			case 11 :
				ILCMD_TAD(hpil_settings.print);
				hpil_step = 4;
				break;
			case 12 :
				if (rtn_ilCompletion() != ERR_INTERRUPTIBLE) {
					// presume printing outside of HPIL operation
					error = ERR_NONE;
				}
				else {
					ILCMD_nop;
				}
				shell_log("ending");
				hpil_settings.printStacked = 0;
				break;
			default :
				error = ERR_IL_ERROR;
		}
	}
	return error;
}
