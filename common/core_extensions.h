/*****************************************************************************
 * FreeIL -- an HP-IL loop emulation
 * Copyright (C) 2014-2019 Jean-Christophe HESSEMANN
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

#ifndef HPIL_Utils_h
#define HPIL_Utils_h 1

/* Functions needed for HP IL, could be added to the different original free42 files.
 * But I put them here to make the least possible change in oriignal free42
 *
 */

/* core_42ToFree42
 *
 * reworked Free42 encoding to allow opcode by opcode decoding
 * mostly base on T.Okken work, main change is on using flags for early decoding
 */
int core_Free42To42 (int4 *pc, unsigned char *buf, int *pt);

/* core_42ToFree42
 *
 * reworked Free42 decoding to allow opcode by opcode decoding
 * mix between T.Duell & T.Okken methods
 */
int core_42ToFree42 (unsigned char *, int *, int );

#endif