/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
 * EBML state file format
 * Copyright (C) 2018-2019  Jean-Christophe Hessemann
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

#ifndef CORE_EBML_H
#define CORE_EBML_H 1

#include "core_globals.h"

typedef struct {
    int docId;              // document Id
    int docLen;             // document length
    int docFirstEl;         // position of first element in the document
    int elId;               // element Id
    int elLen;              // element length
    int elPos;              // element starting position in file
    int elData;             // start of data in element
    int pos;                // current position in file
} ebmlElement_Struct;

/* schema (kindof...)
 *
 * Header
 *
 *  <element name="EBMLFree42" path=1*1(\EBMLFree42) id="0x0x4672ee420" minOccurs="1" maxOccurs="1" type="master element"/>
 *  <documentation This element is the Free42 top master element/>
 *
 *      <element name="EBMLFree42Desc" path=1*1(\EBMLFree42\EBMLFree42Desc) id="0x0013" minOccurs="1" maxOccurs="1" type="string"/>
 *      <documentation This element is the Free42 description/>
 *
 *      <element name="EBMLFree42Version" path=1*1(\EBMLFree42\EBMLFree42Version) id="0x0021" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *      <documentation This element is the Free42 specs used to create the document/>
 *
 *      <element name="EBMLFree42ReadVersion" path=1*1(\EBMLFree42\EBMLFree42ReadVersion) id="0x0031" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *      <documentation This element is the minimum version required to be able to read this document/>
 *
 * Shell
 *
 *      <element name="EBMLFree42Shell" path=1*1(\EBMLFree42\EBMLFree42Shell) id="0x1000" minOccurs="1" maxOccurs="1" type="master element"/>
 *      <documentation This element is the shell state master element and contains shell specific elements/>
 *
 *          <element name="EBMLFree42ShellVersion" path=1*1(\EBMLFree42\EBMLFree42Shell\EBMLFree42ShellVersion) id="0x1011" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *          <documentation This element is the shell state specs used to create the shell state master element/>
 *
 *          <element name="EBMLFree42ShellReadVersion" path=1*1(\EBMLFree42\EBMLFree42Shell\EBMLFree42ShellReadVersion) id="0x1021" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *          <documentation This element is the minimum version to read the shell state master element/>
 *
 *          <element name="EBMLFree42ShellOS" path=1*1(\EBMLFree42\EBMLFree42Shell\EBMLFree42ShellOS) id="0x1033" minOccurs="1" maxOccurs="1" type="string"/>
 *          <documentation This element is the shell operating system generic short name/>
 *
 * Core
 *
 *      <element name="EBMLFree42Core" path=1*1(\EBMLFree42\EBMLFree42Core) id="0x2000" minOccurs="1" maxOccurs="1" type="master element"/>
 *      <documentation This element contains all core state elements excluding variables and programs/>
 *
 *          <element name="EBMLFree42CoreVersion" path=1*1(\EBMLFree42\EBMLFree42Core\EBMLFree42CoreVersion) id="0x2011" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *          <documentation This element is the core state specs used to create the core state master element/>
 *
 *          <element name="EBMLFree42CoreReadVersion" path=1*1(\EBMLFree42\EBMLFree42Core\EBMLFree42CoreReadVersion) id="0x2021" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *          <documentation This element is the minimum version to read the core state master element/>
 *
 * Variables
 *
 *      <element name="EBMLFree42Vars" path=1*1(\EBMLFree42\EBMLFree42Vars) id="0x4000" minOccurs="1" maxOccurs="1" type="master element"/>
 *      <documentation This element contains all explicit variables/>
 *
 *          <element name="EBMLFree42VarsVersion" path=1*1(\EBMLFree42\EBMLFree42Vars\EBMLFree42VarsVersion) id="0x4011" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *          <documentation This element is the vars specs used to create the vars master element/>
 *
 *          <element name="EBMLFree42VarsReadVersion" path=1*1(\EBMLFree42\EBMLFree42Vars\EBMLFree42VarsReadVersion) id="0x4021" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *          <documentation This element is the minimum version to read the vars master element/>
 *
 *          <element name="EBMLFree42VarsCount" path=1*1(\EBMLFree42\EBMLFree42Vars\EBMLFree42VarsCount) id="0x4031" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *          <documentation This element is the number of variables in the vars master element/>
 *
 * Programs
 *
 *      <element name="EBMLFree42Progs" path=1*1(\EBMLFree42\EBMLFree42Progs) id="0x6000" minOccurs="1" maxOccurs="1" type="master element"/>
 *      <documentation This element contains all explicit variables/>
 *
 *          <element name="EBMLFree42ProgsVersion" path=1*1(\EBMLFree42\EBMLFree42Progs\EBMLFree42ProgsVersion) id="0x6011" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *          <documentation This element is the vats specs used to create the Progs master element/>
 *
 *          <element name="EBMLFree42ProgsReadVersion" path=1*1(\EBMLFree42\EBMLFree42Progs\EBMLFree42ProgsReadVersion) id="0x6021" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *          <documentation This element is the minimum version to read the Progs master element/>
 *
 *          <element name="EBMLFree42ProgsCount" path=1*1(\EBMLFree42\EBMLFree42Progs\EBMLFree42ProgsCount) id="0x6031" minOccurs="1" maxOccurs="1" type="vinteger"/>
 *          <documentation This element is the number of variables in the Progs master element/>
 *
 * Elements coding
 *
 *  0 > Master element
 *  1 > vint
 *  2 > integer
 *  3 > string
 *  4 > binary
 *  5 > boolean
 *
 * Variables master elements
 *
 *  <element name=EBMLFree42VarNull path=*(\EBMLFree42\EBMLFree42Core\EBMLFree42VarNull
 *                                      / \EBMLFree42\EBMLFree42Vars\EBMLFree42VarNull) id="0x400" type="master element"/>
 *
 *  <element name=EBMLFree42VarReal path=*(\EBMLFree42\EBMLFree42Core\EBMLFree42VarReal
 *                                      / \EBMLFree42\EBMLFree42Vars\EBMLFree42VarReal) id="0x410" type="master element"/>
 *
 *  <element name=EBMLFree42VarCpx  path=*(\EBMLFree42\EBMLFree42Core\EBMLFree42VarCpx
 *                                      / \EBMLFree42\EBMLFree42Vars\EBMLFree42VarCpx ) id="0x420" type="master element"/>
 *
 *  <element name=EBMLFree42VarRMtx path=*(\EBMLFree42\EBMLFree42Core\EBMLFree42VarRMtx
 *                                      / \EBMLFree42\EBMLFree42Vars\EBMLFree42VarRMtx) id="0x430" type="master element"/>
 *
 *  <element name=EBMLFree42VarCMtx path=*(\EBMLFree42\EBMLFree42Core\EBMLFree42VarCMtx
 *                                      / \EBMLFree42\EBMLFree42Vars\EBMLFree42VarCMtx) id="0x440" type="master element"/>
 *
 *  <element name=EBMLFree42VarStr  path=*(\EBMLFree42\EBMLFree42Core\EBMLFree42VarStr
 *                                      / \EBMLFree42\EBMLFree42Vars\EBMLFree42VarStr ) id="0x450" type="master element"/>
 *
 * variables global elements
 *
 *  <element name=EBMLFree42VarSize path=1*1(EBMLFree42VarNull\EBMLFree42VarSize
 *                                         / EBMLFree42VarReal\EBMLFree42VarSize
 *                                         / EBMLFree42VarCpx\EBMLFree42VarSize
 *                                         / EBMLFree42VarRMtx\EBMLFree42VarSize
 *                                         / EBMLFree42VarCMtx\EBMLFree42VarSize
 *                                         / EBMLFree42VarStr\EBMLFree42VarSize ) id ="0x511" type="vint"/>
 *
 *  <element name=EBMLFree42VarName path=1*1(EBMLFree42VarNull\EBMLFree42VarName
 *                                         / EBMLFree42VarReal\EBMLFree42VarName
 *                                         / EBMLFree42VarCpx\EBMLFree42VarName
 *                                         / EBMLFree42VarRMtx\EBMLFree42VarName
 *                                         / EBMLFree42VarCMtx\EBMLFree42VarName
 *                                         / EBMLFree42VarStr\EBMLFree42VarName ) id ="0x523" type="string"/>
 *
 *  <element name=EBMLFree42VarRows path=1*1(EBMLFree42VarRMtx\EBMLFree42VarRows
 *                                         / EBMLFree42VarCMtx\EBMLFree42VarRows ) id ="0x532" type="integer"/>
 *
 *  <element name=EBMLFree42VarColumns path=1*1(EBMLFree42VarRMtx\EBMLFree42VarColumns
 *                                            / EBMLFree42VarCMtx\EBMLFree42VarColumns ) id ="0x542" type="integer"/>
 *
 *  <element name=EBMLFree42VarPhloat path=1*1(EBMLFree42VarReal\EBMLFree42VarPhloat
 *                                           / EBMLFree42VarCpx\EBMLFree42VarPhloat
 *                                           / EBMLFree42VarRMtx\EBMLFree42VarPhloat
 *                                           / EBMLFree42VarCMtx\EBMLFree42VarPhloat ) id ="0x584" type="binary"/>
 *
 *  <element name=EBMLFree42VarStr path=1*1(EBMLFree42VarRMtx\EBMLFree42VarStr
 *                                        / EBMLFree42VarString\EBMLFree42VarStr ) id ="0x583" type="string"/>
 *
 * programs elements
 *
 *  <element name="EBMLFree42Prog" path=1*1(\EBMLFree42\EBMLFree42Progs\EBMLFree42Prog) id="0x600" minOccurs="1" maxOccurs="1" type="master element"/>
 *
 *      <element name="EBMLFree42ProgSize" path=1*1(EBMLFree42Prog\EBMLFree42ProgSize) id="0x611" minOccurs="1" maxOccurs="1" type="vint"/>
 *
 *      <element name="EBMLFree42ProgName" path=1*1(EBMLFree42Progs\EBMLFree42ProgName) id="0x623" minOccurs="1" maxOccurs="1" type="string"/>
 *
 *      <element name="EBMLFree42ProgData" path=1*1(EBMLFree42Progs\EBMLFree42ProgData) id="0x634" minOccurs="1" maxOccurs="1" type="binary"/>
 *
 */

#define _EBMLFree42Desc                     "Free42 ebml state"
#define _EBMLFree42Version                  1
#define _EBMLFree42ReadVersion              1

#define _EBMLFree42ShellVersion             1
#define _EBMLFree42ShellReadVersion         1

#define _EBMLFree42ExtensionsVersion		1
#define _EBMLFree42ExtensionsReadVersion	1

#define _EBMLFree42ExtensionsHpilVersion		1
#define _EBMLFree42ExtensionsHpilReadVersion	1

#define _EBMLFree42CoreVersion              1
#define _EBMLFree42CoreReadVersion          1

#define _EBMLFree42DisplayVersion           1
#define _EBMLFree42DisplayReadVersion       1

#define _EBMLFree42VarsVersion              1
#define _EBMLFree42VarsReadVersion          1

#define _EBMLFree42ProgsVersion             1
#define _EBMLFree42ProgsReadVersion         1

/*
 * types for base elements Ids
 * combine with each element Id's (4 lsb)
 */
#define EBMLFree42MasterElement             0x00        // master, no value
#define EBMLFree42VIntElement               0x01        // vint, value encoded as variable size integer
#define EBMLFree42IntElement                0x02        // signed int, size in bytes as vint, value stored in little endianness 
#define EBMLFree42StringElement             0x03        // string, size in bytes as vint, ascii string
#define EBMLFree42PhloatElement             0x04        // phloat as binary, size in bytes as vint, value in little endianess
#define EBMLFree42BooleanElement            0x05        // boolean, value encoded as variable size integer (0 or 1)
#define EBMLFree42BinaryElement             0x06        // binary, size in bytes as vint, binary undefined


/****************************************************************************
 *                                                                          *
 * Master document header                                                   *
 *                                                                          *
 ****************************************************************************/

#define EBMLFree42                          0x4672ee42  // Free42 top master element, vint
#define EBMLFree42Desc                      0x0013      // Free42 document description, string
#define EBMLFree42Version                   0x0021      // specs used to create the document
#define EBMLFree42ReadVersion               0x0031      // minimum version to read the document

/*
 * verbosity
 */
#define EBMLFree42Verbose                   0x7333      // Element, verbosity, string

/*
 * end of document for unsized documents
 */
#define EBMLFree42EOD                       0x7def      // Tag for end of unsized document


/****************************************************************************
 *                                                                          *
 * shell document                                                           *
 *                                                                          *
 ****************************************************************************/

#define EBMLFree42Shell                     0x1000      // Master element, document containing shell state
#define EBMLFree42ShellVersion              0x1011      // Element, version used to create this document, vint
#define EBMLFree42ShellReadVersion          0x1021      // Element, minimum version to read this document, vint
#define EBMLFree42ShellOS                   0x1033      // Element, OS name, string


/****************************************************************************
 *                                                                          *
 * extension document                                                           *
 *                                                                          *
 ****************************************************************************/

#define EBMLFree42Extensions                0x8000      // Master element, document containing extensions
#define EBMLFree42ExtensionsVersion         0x8011      // Element, version used to create this document, vint
#define EBMLFree42ExtensionsReadVersion     0x8021      // Element, minimum version to read this document, vint

#define EBMLFree42ExtensionsHpil				0x81000      // Master element, document containing HP-IL extensions
#define EBMLFree42ExtensionsHpilVersion         0x81011      // Element, version used to create this document, vint
#define EBMLFree42ExtensionsHpilReadVersion     0x81021      // Element, minimum version to read this document, vint

#define EL_hpil_comPort							0x81113		// string
#define EL_hpil_outIP							0x81121		// vint
#define EL_hpil_inTcpPort						0x81131		// vint
#define EL_hpil_outTcpPort						0x81141		// vint
#define EL_hpil_highSpeed						0x81155		// bool
#define EL_hpil_medSpeed						0x81165		// bool
#define EL_hpil_pilBox							0x81175		// bool

#define EL_hpil_selected						0x81212		// int
#define EL_hpil_print							0x81222		// int
#define EL_hpil_disk							0x81232		// int
#define EL_hpil_plotter							0x81242		// int
#define EL_hpil_prtAid							0x81252		// int
#define EL_hpil_dskAid							0x81262		// int


/****************************************************************************
 *                                                                          *
 * core document                                                            *
 *                                                                          *
 ****************************************************************************/

#define EBMLFree42Core                      0x2000      // Master element, document containing core state
#define EBMLFree42CoreVersion               0x2011      // Element, version used to create this document, vint
#define EBMLFree42CoreReadVersion           0x2021      // Element, minimum version to read this document, vint

/*
 * args common elements
 */
#define EBMLFree42ArgSize                   0x2111      // size of argument, vint
#define EBMLFree42ArgType                   0x2121      // type of argument, vint
#define EBMLFree42ArgLength                 0x2131      // length in arg struct, vint
#define EBMLFree42ArgTarget                 0x2142      // target in arg struct, int
#define EBMLFree42ArgVal                    0x2150      // val as union in arg struct, variable type


/****************************************************************************
 *                                                                          *
 * display document                                                         *
 *                                                                          *
 ****************************************************************************/

#define EBMLFree42Display                   0x3000      // Master element, document containing display data
#define EBMLFree42DisplayVersion            0x3011      // Element, version used to create this document, vint
#define EBMLFree42DisplayReadVersion        0x3021      // Element, minimum version to read this document, vint


/****************************************************************************
 *                                                                          *
 * variables document                                                       *
 *                                                                          *
 ****************************************************************************/

#define EBMLFree42Vars                      0x4000      // master element, document containing explicit variables
#define EBMLFree42VarsVersion               0x4011      // element, version used to create this document, vint
#define EBMLFree42VarsReadVersion           0x4021      // element, minimum version to read this document, vint
#define EBMLFree42VarsCount                 0x4031      // element, number of objects in variables document, vint

/*
 * variables sub documents
 * All types derived from EBMLFree42VarNull + vartype << 4
 */
#define EBMLFree42VarNull                   0x400       // all master elements
#define EBMLFree42VarReal                   0x410
#define EBMLFree42VarCpx                    0x420
#define EBMLFree42VarRMtx                   0x430
#define EBMLFree42VarCMtx                   0x440
#define EBMLFree42VarStr                    0x450

/*
 * variables common elements
 */
#define EBMLFree42VarName                   0x463       // variable's name, string

/*
 * matrix variables common elements
 */
#define EBMLFree42VarRows                   0x472       // matrix's rows, has to be signed int to cope with shadows matrix, int
#define EBMLFree42VarColumns                0x482       // matrix's columns, keep same type as rows, int 

/*
 * variables elements Id's
 */
#define EBMLFree42VarNoType                 0x490       // untyped
#define EBMLFree42VarString                 0x4a3       // type EBMLFree42StringElement
#define EBMLFree42VarPhloat                 0x4b4       // type EBMLFree42PhloatElement

/****************************************************************************
 *                                                                          *
 * programs document                                                        *
 *                                                                          *
 ****************************************************************************/
 
#define EBMLFree42Progs                     0x6000      // Master element, document containing programs
#define EBMLFree42ProgsVersion              0x6011      // Element, version used to create this document, vint
#define EBMLFree42ProgsReadVersion          0x6021      // Element, minimum version to read this document, vint
#define EBMLFree42ProgsCount                0x6031      // Element, number of programs in document, vint

/*
 * program element
 */
#define EBMLFree42Prog                      0x600       // master program subdocument
#define EBMLFree42ProgSize                  0x611       // size of subdocument, vint
#define EBMLFree42ProgName                  0x623       // program name, string
#define EBMLFree42Prog_size                 0x631       // program size in memory, vint
#define EBMLFree42Prog_lclbl_invalid        0x641       // no need to save? vint
#define EBMLFree42Prog_text                 0x653       // program, as string


/*
 * static size of variables elements, don't forget to adjust against header
 */
#define EbmlPhloatSZ        19
#define EbmlStringSZ        3

bool ebmlWriteReg(vartype *v, char reg);
bool ebmlWriteAlphaReg();
bool ebmlWriteVar(var_struct *v);
bool ebmlWriteProgram(int prgm_index, prgm_struct *p);

bool ebmlWriteElBool(unsigned int elId, bool val);
bool ebmlWriteElVInt(unsigned int elId, unsigned int val);
bool ebmlWriteElInt(unsigned int elId, int val);
bool ebmlWriteElString(unsigned int elId, int len, char *val);
bool ebmlWriteElPhloat(unsigned int elId, phloat *p);
bool ebmlWriteElBinary(unsigned int elId, unsigned int l, void *b);
bool ebmlWriteElArg(unsigned int elId, arg_struct *arg);

bool ebmlWriteMasterHeader();
bool ebmlWriteShellDocument(unsigned int version, unsigned int readVersion, unsigned int len, char * OsVersion);
bool ebmlWriteExtensionsDocument();
bool ebmlWriteExtensionsHpilDocument();
bool ebmlWriteCoreDocument();
bool ebmlWriteDisplayDocument();
bool ebmlWriteVarsDocument(unsigned int count);
bool ebmlWriteProgsDocument(unsigned int count);
bool ebmlWriteEndOfDocument();

bool ebmlReadVar(ebmlElement_Struct *el, var_struct *var);
bool ebmlReadAlphaReg(ebmlElement_Struct *el, char *valuse, int *length);
bool ebmlReadProgram(ebmlElement_Struct *el, prgm_struct *p);

bool ebmlReadElBool(ebmlElement_Struct *el, bool *value);
bool ebmlReadElInt(ebmlElement_Struct *el, int *value);
bool ebmlReadElString(ebmlElement_Struct *el, char  *value, int *sz);
bool ebmlReadElPhloat(ebmlElement_Struct *el, phloat *p);
bool ebmlReadElArg(ebmlElement_Struct *el, arg_struct *arg);

int ebmlGetNext(ebmlElement_Struct *el);
int ebmlGetEl(ebmlElement_Struct *el);

bool ebmlGetString(ebmlElement_Struct *el, char *value, int len);
bool ebmlGetBinary(ebmlElement_Struct *el, void *value, int len);

#endif
