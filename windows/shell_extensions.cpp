/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
 * Free42 eXtensions -- adding HP-IL to free42
 * Copyright (C) 2014-2019 Jean-Christophe HESSEMANN
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

#include <winsock2.h>
#include <ws2tcpip.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <io.h>
#include <shlobj.h>
#include <stdio.h>

#include "stdafx.h"
#include "resource.h"

#include "free42.h"
#include "core_globals.h"
#include "core_ebml.h"
#include "core_main.h"
#include "hpil_common.h"
#include "shell.h"
#include "shell_extensions.h"

#pragma comment(lib, "Ws2_32.lib")

typedef struct state_extensions {
	char comPort[15];
	DWORD outIP;
	unsigned int inTcpPort;
	unsigned int outTcpPort;
	bool highSpeed;
	bool medSpeed;
	bool pilBox;
} state_extensions_type;

static state_extensions_type state_extensions;

extern FILE * EbmlStateFile;
extern HPIL_Settings hpil_settings;

// i/o for hpil emulation
bool modeEnabled, modeIP, modePIL_Box;
HANDLE hSerial;
int _lastFrame = 0xffff;
#define highbytesigmsk	0x00e1
#define highbytesig		0x0020
#define highbytevalmsk	0x001e
#define highframemsk	0x0780
#define lowbytesigmsk	0x0080
#define lowbytesig		0x0080
#define lowbytevalmsk	0x007f
#define lowframemsk		0x007f

// IP Winsock
#define DEFAULT_BUFLEN 512
WSADATA wsaData;
SOCKET clientSocket, serverSocket[2];
WSAEVENT serverEvent[2];

static void shell_init_port();
static void shell_close_serial();
static void shell_close_port();

// Message handler for HP-IL preferences dialog.
LRESULT CALLBACK HpIlPrefs(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
HWND ctl;
HKEY hkey;
int dwindex = 0;
char hkeyName[64];
int hkeyNameLen;
char hkeyValue[64];
int hkeyValueLen;
int err;
    switch (message) {
        case WM_INITDIALOG: {
            // Initialize the dialog from the hpilPrefs structure
			EnableWindow(GetDlgItem(hDlg, IDC_HPIL_INTERFACE), 0);
			EnableWindow(GetDlgItem(hDlg, IDC_HPIL_LOW_SPEED), 0);
			EnableWindow(GetDlgItem(hDlg, IDC_HPIL_MED_SPEED), 0);
			EnableWindow(GetDlgItem(hDlg, IDC_HPIL_HIGH_SPEED), 0);
			EnableWindow(GetDlgItem(hDlg, IDC_HPIL_PIL_Box), 0);
			EnableWindow(GetDlgItem(hDlg, IDC_HPIL_IP_ADD), 0);
			EnableWindow(GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_OUT), 0);
			EnableWindow(GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_IN), 0);
            if (core_settings.enable_ext_hpil) {
				// init combo list of ressources
				EnableWindow(GetDlgItem(hDlg, IDC_HPIL_INTERFACE), 1);
                ctl = GetDlgItem(hDlg, IDC_HPIL_INTERFACE);
				SendMessage(ctl,CB_RESETCONTENT, 0, 0);
				SendMessage(ctl, CB_ADDSTRING, -1, (LPARAM)" Disable");
				SendMessage(ctl, CB_SETCURSEL, SendMessage(ctl, CB_FINDSTRING, -1, (LPARAM)" Disable"), 0);
				if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Hardware\\DeviceMap\\SerialComm", 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkey) == ERROR_SUCCESS) {
					do {
						hkeyNameLen = sizeof(hkeyName);
						hkeyValueLen = sizeof(hkeyValue);
						err = RegEnumValue(hkey, dwindex++, (LPSTR)&hkeyName, (LPDWORD)&hkeyNameLen, 0, 0, (LPBYTE)&hkeyValue, (LPDWORD)&hkeyValueLen); 
						if ((err ==ERROR_SUCCESS) && (hkeyValueLen > 4)) {
							SendMessage(ctl, CB_ADDSTRING, -1, (LPARAM)hkeyValue);
							if (!strcmp(hkeyValue,state_extensions.comPort)) {
								SendMessage(ctl, CB_SETCURSEL, SendMessage(ctl, CB_FINDSTRING, -1, (LPARAM)hkeyValue), 0);
								EnableWindow(GetDlgItem(hDlg, IDC_HPIL_LOW_SPEED), 1);
								EnableWindow(GetDlgItem(hDlg, IDC_HPIL_MED_SPEED), 1);
								EnableWindow(GetDlgItem(hDlg, IDC_HPIL_HIGH_SPEED), 1);
								EnableWindow(GetDlgItem(hDlg, IDC_HPIL_PIL_Box), 1);
							}
						}
					} while (err == ERROR_SUCCESS);
				}
                err = SendMessage(ctl, CB_INSERTSTRING,-1,(LPARAM)"TCP/IP");
				if (!strcmp("TCP/IP",state_extensions.comPort)) {
					SendMessage(ctl, CB_SETCURSEL, SendMessage(ctl, CB_FINDSTRING, -1, (LPARAM)"TCP/IP"), 0);
					EnableWindow(GetDlgItem(hDlg, IDC_HPIL_IP_ADD), 1);
					EnableWindow(GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_OUT), 1);
					EnableWindow(GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_IN), 1);
				}
				if (state_extensions.highSpeed) {
	                ctl = GetDlgItem(hDlg, IDC_HPIL_HIGH_SPEED);
				}
				else if (state_extensions.medSpeed) {
	                ctl = GetDlgItem(hDlg, IDC_HPIL_MED_SPEED);
				}
				else {
	                ctl = GetDlgItem(hDlg, IDC_HPIL_LOW_SPEED);
				}
				SendMessage(ctl, BM_SETCHECK, BST_CHECKED, 0);
				if (state_extensions.pilBox) {
	                ctl = GetDlgItem(hDlg, IDC_HPIL_PIL_Box);
					SendMessage(ctl, BM_SETCHECK, BST_CHECKED, 0);
				}
                ctl = GetDlgItem(hDlg, IDC_HPIL_IP_ADD);
				SendMessage(ctl, IPM_SETADDRESS, 0, (LPARAM)state_extensions.outIP);
				sprintf(hkeyValue, "%u", state_extensions.outTcpPort);
                ctl = GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_OUT);
				SendMessage(ctl, WM_SETTEXT, 0, (LPARAM)hkeyValue);
				sprintf(hkeyValue, "%u", state_extensions.inTcpPort);
                ctl = GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_IN);
				SendMessage(ctl, WM_SETTEXT, 0, (LPARAM)hkeyValue);
			}
			return true;
		}
        case WM_COMMAND: {
            int cmd = LOWORD(wParam);
            switch (cmd) {
                case IDOK: {
                    // Update the prefs structures from the dialog
                    ctl = GetDlgItem(hDlg, IDC_HPIL_INTERFACE);
					dwindex = SendMessage(ctl, CB_GETCURSEL, 0, 0);
					SendMessage(ctl, CB_GETLBTEXT, dwindex, (LPARAM)state_extensions.comPort);
					state_extensions.highSpeed = false;
					state_extensions.medSpeed = false;
                    ctl = GetDlgItem(hDlg, IDC_HPIL_HIGH_SPEED);
					if (SendMessage(ctl, BM_GETSTATE, 0, 0) == BST_CHECKED) {
						state_extensions.highSpeed = true;
					}
					else {
	                    ctl = GetDlgItem(hDlg, IDC_HPIL_MED_SPEED);
						if (SendMessage(ctl, BM_GETSTATE, 0, 0) == BST_CHECKED) {
							state_extensions.medSpeed = true;
						}
					}
                    ctl = GetDlgItem(hDlg, IDC_HPIL_PIL_Box);
					if (SendMessage(ctl, BM_GETSTATE, 0, 0) == BST_CHECKED) {
						state_extensions.pilBox = true;
					}
					else {
						state_extensions.pilBox = false;
					}
                    ctl = GetDlgItem(hDlg, IDC_HPIL_IP_ADD);
					SendMessage(ctl, IPM_GETADDRESS, 0, (LPARAM)&state_extensions.outIP);
                    ctl = GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_OUT);
					((WORD*)hkeyValue)[0] = 64;
					hkeyValueLen = SendMessage(ctl, EM_GETLINE, 0, (LPARAM)hkeyValue);
					hkeyValue[hkeyValueLen] = 0;
					hkeyValueLen = strtol(hkeyValue, NULL, 10);
					if (hkeyValueLen >= 49152 && hkeyValueLen <= 65535) {
						state_extensions.outTcpPort = hkeyValueLen;
					}
                    ctl = GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_IN);
					((WORD*)hkeyValue)[0] = 64;
					hkeyValueLen = SendMessage(ctl, EM_GETLINE, 0, (LPARAM)hkeyValue);
					hkeyValue[hkeyValueLen] = 0;
					hkeyValueLen = strtol(hkeyValue, NULL, 10);
					if (hkeyValueLen >= 49152 && hkeyValueLen <= 65535) {
						state_extensions.inTcpPort = hkeyValueLen;
					}
					shell_close_port();
					shell_init_port();
                }
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
					break;
                case IDC_HPIL_INTERFACE:
					if (HIWORD(wParam) == CBN_SELCHANGE) {
						ctl = GetDlgItem(hDlg, IDC_HPIL_INTERFACE);
						dwindex = SendMessage(ctl, CB_GETCURSEL, 0, 0);
						SendMessage(ctl, CB_GETLBTEXT, dwindex, (LPARAM)hkeyValue);
						if (!strcmp(hkeyValue," Disable")) {
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_LOW_SPEED), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_MED_SPEED), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_HIGH_SPEED), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_PIL_Box), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_IP_ADD), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_OUT), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_IN), 0);
						}
						else if (!strcmp(hkeyValue,"TCP/IP")) {
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_LOW_SPEED), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_MED_SPEED), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_HIGH_SPEED), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_PIL_Box), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_IP_ADD), 1);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_OUT), 1);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_IN), 1);
						}
						else {
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_LOW_SPEED), 1);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_MED_SPEED), 1);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_HIGH_SPEED), 1);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_PIL_Box), 1);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_IP_ADD), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_OUT), 0);
							EnableWindow(GetDlgItem(hDlg, IDC_HPIL_TCP_PORT_IN), 0);
						}
						return TRUE;
					}
			}		
        }
    }
    return FALSE;
}

void open_extension(char* stateExtFilename) {
ebmlElement_Struct el;
int l, version;

	// try to get ebml state file
	if (_access(stateExtFilename,0) != -1) {
        EbmlStateFile = fopen(stateExtFilename, "rb");
        // open global master document
        el.docId = 0;
        el.elId = EBMLFree42;
        if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
        }
        // use global master document
        el.docId = el.elId;
        el.docLen = el.elLen;
        el.docFirstEl = el.pos;
        // get version
        el.elId = EBMLFree42Version;
        if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
        }
        version = el.elLen;
        // get read version
        el.elId = EBMLFree42ReadVersion;
        if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
        }
        // read version compatibility ?
        if (el.elLen > _EBMLFree42Version) {
            goto openExtensionError;
        }
		// get Extensions document
        el.elId = EBMLFree42Extensions;
        if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
        }
        el.docId = el.elId;
        el.docLen = el.elLen;
        el.docFirstEl = el.pos;
        // get version
        el.elId = EBMLFree42ExtensionsVersion;
        if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
        }
        version = el.elLen;
        // get read version
        el.elId = EBMLFree42ExtensionsReadVersion;
        if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
        }
        // read version compatibility
        if (el.elLen > _EBMLFree42ExtensionsVersion) {
            goto openExtensionError;
        }
        // open hp-il document
        el.elId = EBMLFree42ExtensionsHpil;
        if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
        }
        // use hp-il preferences document
        el.docId = el.elId;
        el.docLen = el.elLen;
        el.docFirstEl = el.pos;
        // get version
        el.elId = EBMLFree42ExtensionsHpilVersion;
        if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
        }
        version = el.elLen;
        // get read version
        el.elId = EBMLFree42ExtensionsHpilReadVersion;
        if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
        }
        // read version compatibility ?
        if (el.elLen > _EBMLFree42ExtensionsHpilVersion) {
            goto openExtensionError;
        }
		// Get extensions parameters
		el.elId = EL_hpil_comPort;
		l = sizeof (state_extensions.comPort);
		if (!ebmlReadElString(&el, state_extensions.comPort, &l)) {
            goto openExtensionError;
		}
		state_extensions.comPort[l] = 0;
		// target IP
		el.elId = EL_hpil_outIP;
		if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
		}
		state_extensions.outIP = el.elLen;
		// target port
		el.elId = EL_hpil_outTcpPort;
		if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
		}
		state_extensions.outTcpPort = el.elLen;
		// listening port
		el.elId = EL_hpil_inTcpPort;
		if (ebmlGetEl(&el) != 1) {
            goto openExtensionError;
		}
		state_extensions.inTcpPort = el.elLen;
		// speeds
		el.elId = EL_hpil_highSpeed;
		if (!ebmlReadElBool(&el, &state_extensions.highSpeed)) {
            goto openExtensionError;
		}
		el.elId = EL_hpil_medSpeed;
		if (!ebmlReadElBool(&el, &state_extensions.medSpeed)) {
            goto openExtensionError;
		}
		// take care of pilBox
		el.elId = EL_hpil_pilBox;
		if (!ebmlReadElBool(&el, &state_extensions.pilBox)) {
            goto openExtensionError;
		}
		// core HP-IL parameters
		el.elId = EL_hpil_selected;
		if (!ebmlReadElInt(&el, &hpil_settings.selected)) {
            goto openExtensionError;
		}
		el.elId = EL_hpil_print;
		if (!ebmlReadElInt(&el, &hpil_settings.print)) {
			goto openExtensionError;
		}
		el.elId = EL_hpil_disk;
		if (!ebmlReadElInt(&el, &hpil_settings.disk)) {
            goto openExtensionError;
		}
		el.elId = EL_hpil_plotter;
		if (!ebmlReadElInt(&el, &hpil_settings.plotter)) {
            goto openExtensionError;
		}
		el.elId = EL_hpil_prtAid;
		if (!ebmlReadElInt(&el, &hpil_settings.prtAid)) {
            goto openExtensionError;
		}
		el.elId = EL_hpil_dskAid;
		if (!ebmlReadElInt(&el, &hpil_settings.dskAid)) {
            goto openExtensionError;
		}
		goto openExtensionDone;
	}
  openExtensionError:
	// (re)init anything 
	state_extensions.comPort[0] = 0;
	state_extensions.outIP = 0;
	state_extensions.outTcpPort = 0;
	state_extensions.inTcpPort = 0;
	state_extensions.medSpeed = state_extensions.highSpeed = state_extensions.pilBox = false;
	hpil_settings.selected = 0;
	hpil_settings.print = 0;
	hpil_settings.prtAid = 0;
	hpil_settings.disk = 0;
	hpil_settings.dskAid = 0;
	hpil_settings.plotter = 0;
  openExtensionDone:
	fclose(EbmlStateFile);
	shell_init_port();
}

void close_extension(char* stateExtFilename) {
	EbmlStateFile = fopen(stateExtFilename, "wb");
    if (EbmlStateFile != NULL) {
	    ebmlWriteMasterHeader();
	    ebmlWriteExtensionsDocument();
	    ebmlWriteExtensionsHpilDocument();
		// i/o config
		ebmlWriteElString(EL_hpil_comPort, strlen(state_extensions.comPort), state_extensions.comPort);
		ebmlWriteElVInt(EL_hpil_outIP, state_extensions.outIP);
		ebmlWriteElVInt(EL_hpil_outTcpPort, state_extensions.outTcpPort);
		ebmlWriteElVInt(EL_hpil_inTcpPort, state_extensions.inTcpPort);
		ebmlWriteElBool(EL_hpil_highSpeed, state_extensions.highSpeed);
		ebmlWriteElBool(EL_hpil_medSpeed, state_extensions.medSpeed);
		ebmlWriteElBool(EL_hpil_pilBox, state_extensions.pilBox);
		// internal HP-IL config
		ebmlWriteElInt(EL_hpil_selected, hpil_settings.selected);
		ebmlWriteElInt(EL_hpil_print, hpil_settings.print);
		ebmlWriteElInt(EL_hpil_disk, hpil_settings.disk);
		ebmlWriteElInt(EL_hpil_plotter, hpil_settings.plotter);
		ebmlWriteElInt(EL_hpil_prtAid, hpil_settings.prtAid);
		ebmlWriteElInt(EL_hpil_dskAid, hpil_settings.dskAid);
		// done
		ebmlWriteEndOfDocument();
		ebmlWriteEndOfDocument();
		ebmlWriteEndOfDocument();
    }
	fclose(EbmlStateFile);
	shell_close_port();
 }

static int shell_init_IP() {
	char ipStrBuf[16];
	char portStrBuf[6];
	struct addrinfo *result = NULL,
					hints;
	char optVal = 1;
	int err;

	// WinSock initialization
	if (WSAStartup(MAKEWORD(2,2), &wsaData)) {
		return 1;
	}
	// find host target
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	// open server socket
	sprintf(portStrBuf, "%u", state_extensions.inTcpPort);
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, portStrBuf, &hints, &result)) {
		WSACleanup();
	}
	serverSocket[0] = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (serverSocket[0] == INVALID_SOCKET) {
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	err = bind(serverSocket[0], result->ai_addr, (int)result->ai_addrlen);
	if (err) {
		err = WSAGetLastError();
		closesocket(serverSocket[0]);
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);
	if (listen(serverSocket[0], 5)) {
		closesocket(serverSocket[0]);
		WSACleanup();
		return 1;
	}
	serverEvent[0] = WSACreateEvent();
	serverEvent[1] = WSACreateEvent();
	if (WSAEventSelect(serverSocket[0], serverEvent[0], FD_ACCEPT)) {
		closesocket(serverSocket[0]);
		WSACleanup();
		return 1;
	}
	// connect to server
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	sprintf(ipStrBuf, "%u.%u.%u.%u", state_extensions.outIP>>24, state_extensions.outIP>>16&0xff, state_extensions.outIP>>8&0xff, state_extensions.outIP&0xff);
	sprintf(portStrBuf, "%u", state_extensions.outTcpPort);
	if (getaddrinfo(ipStrBuf, portStrBuf, &hints, &result)) {
		return 0;
	}
	// open client socket
	clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (clientSocket == INVALID_SOCKET) {
		freeaddrinfo(result);
		return 1;
	}
	setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &optVal, sizeof(optVal));
	if (connect(clientSocket, result->ai_addr, (int)result->ai_addrlen)) {
		closesocket(clientSocket);
		freeaddrinfo(result);
		return 1;
	}
	freeaddrinfo(result);
	return 0;
}

int shell_write_IP(int tx) {
	int err;
	char buf[2];
	// send data
	buf[0] = (char)(tx >> 8 & 0xff);
	buf[1] = (char)(tx & 0xff);
	err = send(clientSocket, buf, sizeof(buf), 0);
	//sprintf(ipStrBuf,"send %04x\n", tx);
	//shell_write_console(ipStrBuf);
	if (err == SOCKET_ERROR) {
		err = 0;
		clientSocket = SOCKET_ERROR;
	}
	else {
		err = 1;
	}
	return err;
}

int shell_read_IP(int *rx) {
	//char ipStrBuf[16];
	DWORD wsaEvent;
	WSANETWORKEVENTS  wsaNetworkEvents;
	SOCKADDR callerAddr;
	int l;
	int frameok=0;
	char buf[2];

	wsaEvent = WSAWaitForMultipleEvents(2, serverEvent, false, 2, false);
	switch (wsaEvent) {
		case WSA_WAIT_FAILED :
		case WAIT_IO_COMPLETION :
		case WSA_WAIT_TIMEOUT :
			break;
		default :
			wsaEvent-= WSA_WAIT_EVENT_0;
			if (WSAEnumNetworkEvents(serverSocket[wsaEvent], serverEvent[wsaEvent], &wsaNetworkEvents) != SOCKET_ERROR) {
				if (wsaNetworkEvents.lNetworkEvents & FD_ACCEPT) {
					l = sizeof(callerAddr);
					serverSocket[wsaEvent+1] = accept(serverSocket[wsaEvent], &callerAddr, &l);
					WSAEventSelect(serverSocket[wsaEvent+1], serverEvent[wsaEvent+1], FD_CLOSE | FD_READ);
				}
				if (wsaNetworkEvents.lNetworkEvents & FD_CLOSE) {
					closesocket(serverSocket[wsaEvent]);
					serverSocket[0] = SOCKET_ERROR;
				}
				if (wsaNetworkEvents.lNetworkEvents & FD_READ) {
					frameok = recv(serverSocket[wsaEvent], buf, sizeof(buf), 0);
					*rx = (buf[0] << 8);
					*rx |= (unsigned char)buf[1];
					//sprintf(ipStrBuf,"recv %04x\n", *rx);
					//shell_write_console(ipStrBuf);
				}
			}
			else {
				serverSocket[0] = SOCKET_ERROR;
			}
	}
	return frameok;
}

static void shell_close_IP() {
	//shutdown(clientSocket, SD_BOTH);
	closesocket(clientSocket);
	//shutdown(serverSocket[1], SD_BOTH);
	//closesocket(serverSocket[1]);
	//shutdown(serverSocket[0], SD_BOTH);
	//closesocket(serverSocket[0]);
	WSACleanup();
}

static int shell_init_serial() {
	DCB dcb;
	COMMTIMEOUTS commtimeouts;
	DWORD lasterror;
	char comport[72] = "\\\\.\\";
	int err = 1;
	
	strcat(comport, state_extensions.comPort);
	hSerial = CreateFileA(comport,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
	);
	if (hSerial == INVALID_HANDLE_VALUE) {
		lasterror = GetLastError();
	}
	else {
		GetCommState(hSerial, &dcb);
		GetCommTimeouts(hSerial, &commtimeouts);
		if (state_extensions.highSpeed) {
			dcb.BaudRate = 230400;
		}
		else if (state_extensions.medSpeed) {
			dcb.BaudRate = CBR_115200;
		}
		else {
			dcb.BaudRate = CBR_9600;
		}
		dcb.fBinary = 1;
		dcb.Parity = 1;
		dcb.ByteSize = 8;
		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;
		commtimeouts.ReadTotalTimeoutConstant = 2;
		SetCommState(hSerial, &dcb);
		SetCommTimeouts(hSerial, &commtimeouts);
		PurgeComm(hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
		err = 0;
	}
	return err;
}

int shell_write_serial(int tx) {
	DWORD result;
	DWORD lasterror;
	char buf[2];
	char buflen = 0;

	if ((tx != 0x496) && (tx != 0x494) && (tx != 0x497)) {
		//aff_frame(data,1);
	} 
	if ((tx & highframemsk) != _lastFrame) {
		//Serial.print(" High part ");
		//Serial.print(data & highframemsk, HEX);
		//sprintf(txt," MSB %02x,",tx & highframemsk);
		//shell_write_console(txt);
		_lastFrame = tx & highframemsk ;
		buf[buflen++] = ((unsigned char)(_lastFrame >> 6) | highbytesig);
	}
	//Serial.print(" Low part ");
	//Serial.print(data & lowframemsk, HEX);
	//sprintf(txt," LSB %02x\n",tx & lowframemsk);
	//shell_write_console(txt);
	buf[buflen++] = ((unsigned char)(tx & lowframemsk) | lowbytesig);
	if (!WriteFile(hSerial, buf, buflen, &result, NULL)) {
		lasterror = GetLastError();
		shell_close_serial();
		hSerial = NULL;
		printf("Error %d sending %s\n", lasterror, buf);
	}
	return 1;
}

int shell_read_serial(int *rx) {
	DWORD result;
	char buf;
	int frame;
	int frameok=0;

	if (hSerial == NULL) {
		return frameok;
	}
	ReadFile(hSerial, &buf, 1, &result, NULL);
	if (result != 0) {
		// emu41 <-> ilser read sync
		if (buf == 0x0d) {
			//Serial.println("Ignore sync");
		}
		// test for msb's frame
		else if ((buf & highbytesigmsk) == highbytesig) {
			//Serial.print(" High part ");
			//Serial.print(buf, HEX);
			_lastFrame = (buf & highbytevalmsk) << 6;
		}
		// test for lsb's frame
		else if ((buf & lowbytesigmsk) == lowbytesig) {
			frame = _lastFrame | (buf & lowbytevalmsk);
			//if ((frame != 0x496) && (frame != 0x494) && (frame != 0x497)) {
				//Serial.print(" Low part ");
				//Serial.print(buf, HEX);
				frameok = 1;
				*rx = frame;
				//aff_frame(_frame,0);
			//}
			// ilser commands
			//else {
				//Serial.println("Bypass");
				//	shell_write_frame(frame);
			//}
		}
	}
	return(frameok); 
}

static void shell_close_serial() {
	if (hSerial != NULL) {
		CloseHandle(hSerial);
	}
}

static void shell_init_port() {
	modeEnabled = false;
	modePIL_Box = false;
	modeIP = false;

	if (!strcmp(state_extensions.comPort," Disabled")) {
	}
	else if (!strcmp(state_extensions.comPort,"TCP/IP")) {
		shell_close_IP();
		if (!shell_init_IP()) {
			modeEnabled = true;
			modeIP = true;
		}
	}
	else {
		shell_close_serial();
		if (!shell_init_serial()) {
			modeEnabled = true;
			modePIL_Box = state_extensions.pilBox;
		}
	}
	hpil_init(modeEnabled, modePIL_Box);
}

int shell_check_connectivity(void) {
	int err = ERR_NONE;
	if (modeIP) {
		if (clientSocket == SOCKET_ERROR || serverSocket[0] == SOCKET_ERROR) {
			shell_close_IP();
			if (shell_init_IP()) {
				err = ERR_BROKEN_LOOP;
			};
		}
	}
	else if (hSerial == NULL) {
		if (shell_init_serial()) {
			err = ERR_BROKEN_LOOP;
		}
	}
	return err;
}

int shell_write_frame(int tx) {
	if (modeIP) {
		return shell_write_IP(tx);
	}
	else {
		return shell_write_serial(tx);
	}
}

int shell_read_frame(int *rx) {
	if (modeIP) {
		return shell_read_IP(rx);
	}
	else {
		return shell_read_serial(rx);
	}
}

static void shell_close_port() {
	hpil_close(modeEnabled, modePIL_Box);
	shell_close_IP();
	shell_close_serial();
}

void shell_write_console (char *msg) {
	OutputDebugString(msg);
}
