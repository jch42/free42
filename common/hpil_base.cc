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

#include "core_commands2.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_variables.h"
#include "hpil_base.h"
#include "hpil_controller.h"
#include "hpil_common.h"
#include "shell.h"

extern int frame;
extern int controllerCommand;
extern int (*hpil_completion)(int);
extern int hpil_step;
extern int (*hpil_pending)(int);

extern int IFCRunning;
extern int IFCCountdown, IFCNoisy;	// IFC timing

/* deferred running for hpil functions
 *
 */
static int defrd_nloop(int error);
static int defrd_ifc(int error);
static int defrd_select(int error);
static int defrd_prtsel(int error);
static int defrd_dsksel(int error);
static int defrd_autoio(int error);
static int defrd_manio(int error);
static int defrd_stat(int error);
static int defrd_id(int error);
static int defrd_aid(int error);

static int hpil_init_completion(int error);
static int hpil_nloop_completion(int error);
static int hpil_stat_completion(int error);
static int hpil_id_completion(int error);
static int hpil_aid_completion(int error);

extern AltBuf hpil_controllerAltBuf;
extern DataBuf hpil_controllerDataBuf;

extern HPIL_Settings hpil_settings;

extern HPIL_Controller hpil_core;
extern HpilXController hpilXCore;

int docmd_ifc(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_ifc;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_ifc(ERR_NONE);
}

static int defrd_ifc(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		ILCMD_nop;
		hpil_step = 0;
		hpil_completion = hpil_init_completion;
		mode_interruptible = hpil_worker;
		mode_stoppable = true;
	}
	return error;
}

int docmd_nloop(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_nloop;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_nloop(ERR_NONE);
}

static int defrd_nloop(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		ILCMD_AAU;
		hpil_step = 0;
		hpil_completion = hpil_nloop_completion;
		mode_interruptible = hpil_worker;
		mode_stoppable = true;
	}
	return error;
}

int docmd_select(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_select;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_select(ERR_NONE);
}

static int defrd_select(int error) {
	if (error == ERR_NONE) {
		error = mappable_x_hpil(31,&hpil_settings.selected);
	}
	return error;
}

int docmd_rclsel(arg_struct *arg) {
	vartype *v;
	int err;
	err = hpil_check();
	if (err == ERR_NONE || err == ERR_OVERLAPED_OPERATION) {
		v = new_real(hpil_settings.selected);
		if (v == NULL) {
			err = ERR_INSUFFICIENT_MEMORY;
		}
		else {
			err = ERR_NONE;
			recall_result(v);
		}
	}
	return err;
}

int docmd_prtsel(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_prtsel;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_prtsel(ERR_NONE);
}

static int defrd_prtsel(int error) {
	if (error == ERR_NONE) {
		error = mappable_device_hpil(31,&hpil_settings.prtAid);
	}
	return error;
}

int docmd_dsksel(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_dsksel;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_dsksel(ERR_NONE);
}

static int defrd_dsksel(int error) {
	if (error == ERR_NONE) {
		error = mappable_x_hpil(31,&hpil_settings.dskAid);
	}
	return error;
}

int docmd_autoio(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_autoio;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_autoio(ERR_NONE);
}

static int defrd_autoio(int error) {
	if (error == ERR_NONE) {
		flags.f.manual_IO_mode = 0;
	}
	return error;
}

int docmd_manio(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_manio;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_manio(ERR_NONE);
}

static int defrd_manio(int error) {
	if (error == ERR_NONE) {
		flags.f.manual_IO_mode = 1;
	}
	return error;
}

int docmd_stat(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_stat;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_stat(ERR_NONE);
}

static int defrd_stat(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		ILCMD_AAU;
		hpil_step = 0;
		hpil_completion = hpil_stat_completion;
		mode_interruptible = hpil_worker;
		mode_stoppable = true;
	}
	return error;
}

int docmd_id(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_id;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_id(ERR_NONE);
}

static int defrd_id(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		ILCMD_AAU;
		hpil_step = 0;
		hpil_completion = hpil_id_completion;
		mode_interruptible = hpil_worker;
		mode_stoppable = true;
	}
	return error;
}

int docmd_aid(arg_struct *arg) {
	int err;
	err = hpil_check();
	if (err == ERR_OVERLAPED_OPERATION) {
		mode_interruptible = defrd_aid;
		return ERR_INTERRUPTIBLE;
	}
	else if (err != ERR_NONE) {
		return err;
	}
	return defrd_aid(ERR_NONE);
}

static int defrd_aid(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		ILCMD_AAU;
		hpil_step = 0;
		hpil_completion = hpil_aid_completion;
		mode_interruptible = hpil_worker;
		mode_stoppable = true;
	}
	return error;
}

int docmd_bldspec(arg_struct *arg) {
	int err;
	phloat x;
	if (reg_x->type == TYPE_REAL) {
		x = ((vartype_real *) reg_x)->x;
		if ((x > 0) && (x < 128)) {
			err = ERR_NONE;
		}
		else {
			err = ERR_OUT_OF_RANGE;
		}
	}
	else if (reg_x->type == TYPE_STRING) {
		err = ERR_ALPHA_DATA_IS_INVALID;
	}
	else {
		err = ERR_INVALID_TYPE;
	}
	return err;
}

/* hpil completion routines
 *
 * All high level hpil processing is done there
 */
static int hpil_init_completion(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		switch (hpil_step) {
			case 0 :		// IFC
				ILCMD_IFC;
				IFCRunning = 1;
				IFCCountdown = IFCNoisy = 5; 
				break;
			default :
				error = ERR_NONE;
		}
	}
	hpil_step++;
	return error;
}

static int hpil_nloop_completion(int error) {
	vartype *v;
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		switch (hpil_step) {
			case 0 :		// AAU > AAD
				ILCMD_AAD(0x01);
				hpil_step++;
				break;
			case 1 :		// AAD get frame
				if ((frame & M_AAD) == M_AAD) {
					v = new_real((frame & 0x001f) - 1);
					if (v == NULL) {
						error = ERR_INSUFFICIENT_MEMORY;
					}
					else {
						error = ERR_NONE;
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
				else {
					error = ERR_TRANSMIT_ERROR;
				}
				break;
			default :
				error = ERR_NONE;
		}
	}
	return error;
}

static int hpil_stat_completion(int error) {
	int i = 0;
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
				hpilXCore.bufSize = ControllerDataBufSize;
				hpilXCore.bufPtr = 0;
				break;
			case 3 :		// Local listen > SST
				ILCMD_SST;
				hpil_step++;
				break;
			case 4 :		// SST > lun
				ILCMD_lun;
				hpil_step++;
				break;
			case 5 :		// lun > UNT;
				ILCMD_UNT;
				hpil_step++;
				break;
			case 6 :		// Display result
				if (hpilXCore.bufPtr) {
					reg_alpha[0]='s';
					for (i = 0; i < (hpilXCore.bufPtr > 43 ? 43 : hpilXCore.bufPtr); i++) {
						reg_alpha[i+1] = hpilXCore.buf[i];
					}
					reg_alpha_length = i+1;
					error = ERR_NONE;
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

				}
				else {
					reg_alpha_length = 0;
					error = ERR_NO_RESPONSE;
				}
				break;
			default :
				error = ERR_NONE;
		}
	}
	return error;
}

static int hpil_id_completion(int error) {
	int i = 0;
	int j = 0;
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
				hpilXCore.bufSize = ControllerDataBufSize;
				hpilXCore.bufPtr = 0;
				break;
			case 3 :		// Local listen > SDI
				ILCMD_SDI;
				hpil_step++;
				break;
			case 4 :		// SST > lun
				ILCMD_lun;
				hpil_step++;
				break;
			case 5 :		// lun > UNT;
				ILCMD_UNT;
				hpil_step++;
				break;
			case 6 :		// Display result
				if (hpilXCore.bufPtr) {
					for (i = 0; i < (hpilXCore.bufPtr > 43 ? 43 : hpilXCore.bufPtr); i++) {
						if ((hpilXCore.buf[i] == 0x0a) || (hpilXCore.buf[i] == 0x0d)) {
							break;
						}
						else {
							reg_alpha[i] = hpilXCore.buf[i];
						}
					}
					reg_alpha_length = i;
					error = ERR_NONE;
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
				}
				else {
					reg_alpha_length = 0;
					error = ERR_NO_RESPONSE;
				}
				break;
			default :
				error = ERR_NONE;
		}
	}
	return error;
}

static int hpil_aid_completion(int error) {
	if (error == ERR_NONE) {
		error = ERR_INTERRUPTIBLE;
		switch (hpil_step) {
			case 0 :		// AAU > AAD
				ILCMD_AAD(0x01);
				hpil_step++;
				break;
			case 1 :		// > TAD prepare buffer and call aid sub
				ILCMD_TAD(hpil_settings.selected);
				hpilXCore.buf = hpil_controllerAltBuf.data;
				hpilXCore.bufSize = ControllerAltBufSize;
				hpil_step++;
				error = call_ilCompletion(hpil_aid_sub);
				break;
			case 2 :		// Display result
				if (hpilXCore.bufPtr) {
					vartype *v = new_real(hpilXCore.buf[0] & 0xff);
					if (v == NULL) {
						error = ERR_INSUFFICIENT_MEMORY;
					}
					else {
						error = ERR_NONE;
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
				else {
					error = ERR_NO_RESPONSE;
				}
				break;
			default :
				error = ERR_NONE;
		}
	}
	return error;
}
