#include "../common/common.h"
#include "../ui/xmftp.h"

/* If you are having a problem with gethostbyname uncomment this */
/* #define BAD_GETHOSTBYNAME */

/***************************************************************************/
/*									   */
/* ftplib.h - header file for callable ftp access routines                 */
/* Copyright (C) 1996, 1997 Thomas Pfau, pfau@cnj.digex.net                */
/*	73 Catherine Street, South Bound Brook, NJ, 08880		   */
/*									   */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.  				   */
/*									   */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	   */
/* GNU General Public License for more details. 			   */
/*									   */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software  	   */
/* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.    	   */
/*									   */
/***************************************************************************/

#include <sys/types.h>
#include <signal.h>

#if defined(__unix__) || defined(VMS)
#define GLOBALDEF
#define GLOBALREF extern
#elif defined(_WIN32)
#define GLOBALDEF __declspec(dllexport)
#define GLOBALREF __declspec(dllimport)
#endif

typedef struct NetBuf netbuf;

extern netbuf *DefaultNetbuf; /* defined in ftplib/ftplib.c */

#define ftplib_lastresp FtpLastResponse(DefaultNetbuf)
#define ftpInit FtpInit
#define ftpOpen(x) FtpConnect(x, &DefaultNetbuf)
#define ftpLogin(x,y) FtpLogin(x, y, DefaultNetbuf)
#define ftpSite(x) FtpSite(x, DefaultNetbuf)
#define ftpMkdir(x) FtpMkdir(x, DefaultNetbuf)
#define ftpChdir(x) FtpChdir(x, DefaultNetbuf)
#define ftpRmdir(x) FtpRmdir(x, DefaultNetbuf)
#define ftpNlst(x, y) FtpNlst(x, y, DefaultNetbuf)
#define ftpDir(x, y) FtpDir(x, y, DefaultNetbuf)
#define ftpGet(x, y, z) FtpGet(x, y, z, DefaultNetbuf)
#define ftpPut(x, y, z) FtpPut(x, y, z, DefaultNetbuf)
#define ftpRename(x, y) FtpRename(x, y, DefaultNetbuf)
#define ftpDelete(x) FtpDelete(x, DefaultNetbuf)
#define ftpQuit() FtpQuit(DefaultNetbuf)

pid_t FtpXferPID(void);
GLOBALREF int ftplib_debug;
GLOBALREF void FtpInit(void);
GLOBALREF char *FtpLastResponse(netbuf *nControl);
GLOBALDEF int FtpConnect(char *host, netbuf **nControl, int port);
GLOBALREF int FtpLogin(char *user, char *pass, netbuf *nControl);
GLOBALREF int FtpSite(char *cmd, netbuf *nControl);
GLOBALREF int FtpMkdir(char *path, netbuf *nControl);
GLOBALREF int FtpChdir(char *path, netbuf *nControl);
GLOBALREF int FtpRmdir(char *path, netbuf *nControl);
GLOBALREF int FtpNlst(char *output, char *path, netbuf *nControl);
GLOBALREF int FtpDir(char *output, char *path, netbuf *nControl);
GLOBALREF int FtpGet(char *output, char *path, char mode, netbuf *nControl);
GLOBALDEF int FtpResume(char *outputfile, char *path, 
			unsigned long int offset, char mode, 
			netbuf *nControl);
GLOBALREF int FtpPut(char *input, char *path, char mode, netbuf *nControl);
GLOBALREF int FtpRename(char *src, char *dst, netbuf *nControl);
GLOBALREF int FtpDelete(char *fnm, netbuf *nControl);
GLOBALREF void FtpQuit(netbuf *nControl);

/* added by Viraj Alankar */

int FtpSendCmd(char *cmd, char expresp, netbuf *nControl);
int FtpSiteFP(char *cmd, netbuf *nControl, FILE *fp);
int FtpSendCmdFP(char *cmd, char expresp, netbuf *nControl, FILE *fp);
int readrespFP(char c, netbuf *nControl, FILE *fp);
int FtpConnectFP(char *host, netbuf **nControl, int port, FILE *fp);
int FtpLoginFP(char *user, char *pass, netbuf *nControl, FILE *fp);
int myFtpDir(char *outputfile, char *path, netbuf *nControl);


