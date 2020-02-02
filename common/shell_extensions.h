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

#ifndef SHELL_EXTENSIONS
#define SHELL_EXTENSIONS 1

#include "free42.h"

LRESULT CALLBACK HpIlPrefs(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

/* open and close extension 
 *
 * read extension parameters and init communications
 * save extension parameters and close communications
 */
void open_extension(char * state_ext);
void close_extension(char * state_ext);

/* shell_check_connectivity()
 *
 * check loop i/o conectivity
 */
int shell_check_connectivity();
 
/* shell_read_frame()
 *
 * Callback from hpil loop, check if frame received.
 * Frame is rebuild from serial rx.
 * Returns 1 when complete frame is received and clears tiemout4
 */
int shell_read_frame(int *rx);

/* shell_write_frame()
 *
 * Callback from hpil loop, send new frame.
 * is using com port, Frame is split according to ilser, full 8 bits improved format.
 * return 1 - IP server unreachable...
 */
int shell_write_frame(int tx);

#endif