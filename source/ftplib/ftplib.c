/***************************************************************************/
/*									   */
/* ftplib.c - callable ftp access routines                                 */
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

#include "../common/common.h"
#include <sys/time.h>
#include "../ui/xmftp.h"
#include "../ui/operations.h"

#if defined(__unix__)
#include <unistd.h>
#endif
#if defined(__unix__) || defined(VMS)
#define GLOBALDEF
#define GLOBALREF extern
#elif defined(_WIN32)
#include <windows.h>
#define GLOBALDEF __declspec(dllexport)
#define GLOBALREF __declspec(dllimport)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if defined(__unix__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#elif defined(VMS)
#include <types.h>
#include <socket.h>
#include <in.h>
#include <netdb.h>
#elif defined(_WIN32)
#include <winsock.h>
#endif

#if !defined(_WIN32)
#include "ftplib.h"
#endif

/* added by me */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>      

#if defined(_WIN32)
#define SETSOCKOPT_OPTVAL_TYPE (const char *)
#else
#define SETSOCKOPT_OPTVAL_TYPE (void *)
#endif

#define FTP_BUFSIZ 8192
#define ACCEPT_TIMEOUT 30

netbuf *DefaultNetbuf;

void wakeup(int sig);
int my_get_host_by_name(char *host, struct in_addr *site);

struct NetBuf {
    char *cput,*cget;
    int handle;
    int cavail,cleft;
    char buf[FTP_BUFSIZ];
    char response[256];
};

pid_t xfer_pid;
extern void *to_maincb;

/*
static char *version =
    "ftplib Release 2 3/15/97, copyright 1996, 1997 Thomas Pfau";
    */

GLOBALDEF int ftplib_debug = 0;

#if defined(__unix__) || defined(VMS)
#define net_read read
#define net_write write
#define net_close close
#elif defined(_WIN32)
#define net_read(x,y,z) recv(x,y,z,0)
#define net_write(x,y,z) send(x,y,z,0)
#define net_close closesocket
#endif

#if defined(VMS)
/*
 * VAX C does not supply a memccpy routine so I provide my own
 */
void *memccpy(void *dest, const void *src, int c, size_t n)
{
    int i=0;
    unsigned char *ip=src,*op=dest;
    while (i < n)
    {
	if ((*op++ = *ip++) == c)
	    break;
	i++;
    }
    if (i == n)
	return NULL;
    return op;
}
#endif

/*
 * read a line of text
 *
 * return -1 on error or bytecount
 */
static int readline(char *buf,int max,netbuf *ctl)
{
    int x,retval = 0;
    char *end;
    int eof = 0;

    if (max == 0)
	return 0;
    do
    {
    	if (ctl->cavail > 0)
    	{
	    x = (max >= ctl->cavail) ? ctl->cavail : max-1;
	    end = memccpy(buf,ctl->cget,'\n',x);
	    if (end != NULL)
		x = end - buf;
	    retval += x;
	    buf += x;
	    *buf = '\0';
	    max -= x;
	    ctl->cget += x;
	    ctl->cavail -= x;
	    if (end != NULL)
	    	break;
    	}
    	if (max == 1)
    	{
	    *buf = '\0';
	    break;
    	}
    	if (ctl->cput == ctl->cget)
    	{
	    ctl->cput = ctl->cget = ctl->buf;
	    ctl->cavail = 0;
	    ctl->cleft = FTP_BUFSIZ;
    	}
	if (eof)
	{
	    if (retval == 0)
		retval = -1;
	    break;
	}
    	if ((x = net_read(ctl->handle,ctl->cput,ctl->cleft)) == -1)
    	{
	    perror("read");
	    retval = -1;
	    break;
    	}
	if (x == 0)
	    eof = 1;
    	ctl->cleft -= x;
    	ctl->cavail += x;
    	ctl->cput += x;
    }
    while (1);
    return retval;
}

/*
 * read a response from the server
 *
 * return 0 if first char doesn't match
 * return 1 if first char matches
 */
static int readresp(char c, netbuf *nControl)
{
    char match[5];

    signal(SIGALRM, wakeup);
    alarm(NET_TIMEOUT);
    if (readline(nControl->response,256,nControl) == -1)
    {
      alarm(0);
	perror("Control socket read failed");
	return 0;
    }
    alarm(0);
    if (ftplib_debug > 1)
	fprintf(stderr,"%s",nControl->response);
    if (nControl->response[3] == '-')
    {
	strncpy(match,nControl->response,3);
	match[3] = ' ';
	match[4] = '\0';
	do
	{
	  signal(SIGALRM, wakeup);	
	  alarm(NET_TIMEOUT);
	    if (readline(nControl->response,256,nControl) == -1)
	    {
	      alarm(0);
		perror("Control socket read failed");
		return 0;
	    }
	    alarm(0);
	    if (ftplib_debug > 1)
		fprintf(stderr,"%s",nControl->response);
	}
	while (strncmp(nControl->response,match,4));
    }
    if (nControl->response[0] == c)
	return 1;
    return 0;
}

/*
 * FtpInit for stupid operating systems that require it (Windows NT)
 */
GLOBALDEF void FtpInit(void)
{
#if defined(_WIN32)
    WORD wVersionRequested;
    WSADATA wsadata;
    int err;
    wVersionRequested = MAKEWORD(1,1);
    if ((err = WSAStartup(wVersionRequested,&wsadata)) != 0)
	fprintf(stderr,"Network failed to start: %d\n",err);
#endif
}

/*
 * FtpLastResponse - return a pointer to the last response received
 */
GLOBALDEF char *FtpLastResponse(netbuf *nControl)
{
    return nControl->response;
}

/*
 * FtpConnect - connect to remote server
 *
 * return 1 if connected, 0 if not
 */
/* modified */
GLOBALDEF int FtpConnect(char *host, netbuf **nControl, int port)
{
    int sControl;
    struct sockaddr_in sin;
    struct hostent *phe;
    struct servent *pse;
    int on=1;
    netbuf *ctrl;

    memset(&sin,0,sizeof(sin));
    sin.sin_family = AF_INET;
#if defined(VMS)
    sin.sin_port = htons(21);
#else
    if(port < 0) {
      if ((pse = getservbyname("ftp","tcp")) == NULL)
	{
	  perror("getservbyname");
	  return 0;
	}
      sin.sin_port = pse->s_port;
    }
    else
      sin.sin_port = htons(port);   
#endif
#ifdef BAD_GETHOSTBYNAME
    if(!my_get_host_by_name(host, &sin.sin_addr))
      return 0;
#else
    if ((phe = gethostbyname(host)) == NULL)
    {
	perror("gethostbyname");
	return 0;
    }
    memcpy((char *)&sin.sin_addr, phe->h_addr, phe->h_length);
#endif

    sControl = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sControl == -1)
    {
	perror("socket");
	return 0;
    }
    if (setsockopt(sControl,SOL_SOCKET,SO_REUSEADDR,
		   SETSOCKOPT_OPTVAL_TYPE &on, sizeof(on)) == -1)
    {
	perror("setsockopt");
	close(sControl);
	return 0;
    }
    signal(SIGALRM, wakeup);
    alarm(NET_TIMEOUT);
    if (connect(sControl, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
      alarm(0);
	perror("connect");
	close(sControl);
	return 0;
    }
    alarm(0);

    ctrl = calloc(1,sizeof(netbuf));
    if (ctrl == NULL)
    {
	perror("calloc");
	close(sControl);
	return 0;
    }
    ctrl->handle = sControl;
    if (readresp('2', ctrl) == 0)
    {
	close(sControl);
	free(ctrl);
	return 0;
    }
    *nControl = ctrl;
    return 1;
}

/*
 * FtpSendCmd - send a command and wait for expected response
 *
 * return 1 if proper response received, 0 otherwise
 */
int FtpSendCmd(char *cmd, char expresp, netbuf *nControl)
{
    char buf[256];
    if (ftplib_debug > 2)
	fprintf(stderr,"%s\n",cmd);
    sprintf(buf,"%s\r\n",cmd);
    signal(SIGALRM, wakeup);
    alarm(NET_TIMEOUT);
    if (net_write(nControl->handle,buf,strlen(buf)) <= 0)
    {
      alarm(0);
      perror("write");
	return 0;
    }    
    alarm(0);
    return readresp(expresp, nControl);
}

/*
 * FtpLogin - log in to remote server
 *
 * return 1 if logged in, 0 otherwise
 *
 * modified by Viraj Alankar (kaos@magg.net)
 */
GLOBALDEF int FtpLogin(char *user, char *pass, netbuf *nControl)
{
    char tempbuf[64];
    sprintf(tempbuf,"user %s",user);
    if (!FtpSendCmd(tempbuf,'3',nControl)) {
	return 0;
    }
    sprintf(tempbuf,"pass %s",pass);
    return FtpSendCmd(tempbuf,'2',nControl);
}

/*
 * FtpPort - set up data connection
 *
 * return 1 if successful, 0 otherwise
 */
static int FtpPort(netbuf *nControl)
{
    int sData;
    union {
	struct sockaddr sa;
	struct sockaddr_in in;
    } sin;
    struct linger lng = { 0, 0 };
    int l;
    int on=1;
    char *cp;
    int v[6];

    l = sizeof(sin);
    memset(&sin, 0, l);
    sin.in.sin_family = AF_INET;
    if (!FtpSendCmd("pasv",'2',nControl))
	return -1;
    cp = strchr(nControl->response,'(');
    if (cp == NULL)
	return -1;
    cp++;
    sscanf(cp,"%d,%d,%d,%d,%d,%d",&v[2],&v[3],&v[4],&v[5],&v[0],&v[1]);
    sin.sa.sa_data[2] = v[2];
    sin.sa.sa_data[3] = v[3];
    sin.sa.sa_data[4] = v[4];
    sin.sa.sa_data[5] = v[5];
    sin.sa.sa_data[0] = v[0];
    sin.sa.sa_data[1] = v[1];
    sData = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (sData == -1)
    {
	perror("socket");
	return -1;
    }
    if (setsockopt(sData,SOL_SOCKET,SO_REUSEADDR,
		   SETSOCKOPT_OPTVAL_TYPE &on,sizeof(on)) == -1)
    {
	perror("setsockopt");
	net_close(sData);
	return -1;
    }
    if (setsockopt(sData,SOL_SOCKET,SO_LINGER,
		   SETSOCKOPT_OPTVAL_TYPE &lng,sizeof(lng)) == -1)
    {
	perror("setsockopt");
	net_close(sData);
	return -1;
    }
    if (connect(sData, &sin.sa, sizeof(sin.sa)) == -1)
    {
	perror("connect");
	net_close(sData);
	return -1;
    }
    return sData;
}

/*
 * FtpSite - send a SITE command
 *
 * return 1 if command successful, 0 otherwise
 */
GLOBALDEF int FtpSite(char *cmd, netbuf *nControl)
{
    char buf[256];
    sprintf(buf,"site %s",cmd);
    if (!FtpSendCmd(buf,'2',nControl))
	return 0;
    return 1;
}

/*
 * FtpMkdir - create a directory at server
 *
 * return 1 if successful, 0 otherwise
 */
GLOBALDEF int FtpMkdir(char *path, netbuf *nControl)
{
    char buf[256];
    sprintf(buf,"mkd %s",path);
    if (!FtpSendCmd(buf,'2', nControl))
	return 0;
    return 1;
}

/*
 * FtpChdir - change path at remote
 *
 * return 1 if successful, 0 otherwise
 */
GLOBALDEF int FtpChdir(char *path, netbuf *nControl)
{
    char buf[256];
    sprintf(buf,"cwd %s",path);
    if (!FtpSendCmd(buf,'2',nControl))
	return 0;
    return 1;
}

GLOBALDEF int FtpRmdir(char *path, netbuf *nControl)
{
    char buf[256];
    sprintf(buf,"RMD %s",path);
    if (!FtpSendCmd(buf,'2',nControl))
	return 0;
    return 1;
}

/*
 * FtpNlst - issue an NLST command and write response to output
 *
 * return 1 if successful, 0 otherwise
 */
GLOBALDEF int FtpNlst(char *outputfile, char *path, netbuf *nControl)
{
    int sData,l;
    char buf[256];
    char *dbuf;
    FILE *output;

    if (!FtpSendCmd("type A",'2', nControl))
	return 0;
    if ((sData = FtpPort(nControl)) == -1)
	return 0;
    strcpy(buf,"nlst");
    if (path != NULL)
	sprintf(buf+strlen(buf)," %s",path);
    if (!FtpSendCmd(buf,'1', nControl))
	return 0;
    if (outputfile == NULL)
	output = stdout;
    else
    {
	output = fopen(outputfile,"w");
	if (output == NULL)
	{
	    perror(outputfile);
	    output = stdout;
	}
    }
    dbuf = malloc(FTP_BUFSIZ);
    while ((l = net_read(sData,dbuf,FTP_BUFSIZ)) > 0)
	fwrite(dbuf,1,l,output);
    if (outputfile != NULL)
	fclose(output);
    free(dbuf);
    shutdown(sData,2);
    net_close(sData);
    return readresp('2', nControl);
}

/*
 * FtpDir - issue a LIST command and write response to output
 *
 * return 1 if successful, 0 otherwise
 */
GLOBALDEF int FtpDir(char *outputfile, char *path, netbuf *nControl)
{
    int sData,l;
    char buf[256];
    char *dbuf;
    FILE *output;

    if (!FtpSendCmd("type A",'2', nControl))
	return 0;
    if ((sData = FtpPort(nControl)) == -1)
	return 0;
    strcpy(buf,"list");
    if (path != NULL)
	sprintf(buf+strlen(buf)," %s",path);
    if (!FtpSendCmd(buf,'1', nControl))
	return 0;
    if (outputfile == NULL)
	output = stdout;
    else
    {
	output = fopen(outputfile,"w");
	if (output == NULL)
	{
	    perror(outputfile);
	    output = stdout;
	}
    }
    dbuf = malloc(FTP_BUFSIZ);
    while ((l = net_read(sData,dbuf,FTP_BUFSIZ)) > 0)
	fwrite(dbuf,1,l,output);
    if (outputfile != NULL)
	fclose(output);
    free(dbuf);
    shutdown(sData,2);
    net_close(sData);
    return readresp('2', nControl);
}

/*
 * FtpGet - issue a GET command and write received data to output
 *
 * return 1 if successful, 0 otherwise
 *
 * modified by Viraj Alankar (kaos@magg.net) to return fd of pipe to
 * monitor transfer status.. still very buggy, not much is taken care of
 */
GLOBALDEF int FtpGet(char *outputfile, char *path, char mode, netbuf *nControl)
{
    int sData, sendval;
    int l;
    char buf[256];
    char *dbuf;
    FILE *output;
    int size = 0;
    int fd[2];
    pid_t child_pid;

    pipe(fd);
    if(!(child_pid = fork())) { 
      /* child */
      close(fd[0]);
      
      sprintf(buf,"type %c",mode);
      if (!FtpSendCmd(buf,'2', nControl)) {
	write(fd[1], (void *) -1, sizeof(int));
	exit(1);
      }
      
      if ((sData = FtpPort(nControl)) == -1) {
	write(fd[1], (void *) -1, sizeof(int));
	exit(1);
      }

      sprintf(buf,"retr %s",path);
      if (!FtpSendCmd(buf,'1',nControl)) {
	write(fd[1], (void *) -1, sizeof(int));
	exit(1);
      }
      if ((dbuf = strchr(nControl->response,'(')))
	{
	  dbuf++;
	  size = atoi(dbuf);
	}
      if (outputfile == NULL)
	output = stdout;
      else
	{
	  output = fopen(outputfile,"w");
	  if (output == NULL)
	    {
	      perror(outputfile);
	      output = stdout;
	    }
	}
      dbuf = malloc(FTP_BUFSIZ);
      if (mode == 'A')
	{
	  netbuf *nData;
	  int dl;
	  nData = calloc(1,sizeof(netbuf));
	  if (nData == NULL)
	    {
	      perror("calloc");
	      net_close(sData);
	      write(fd[1], (void *) -1, sizeof(int));
	      exit(1);
	    }
	  nData->handle = sData;
	  while ((dl = readline(dbuf,FTP_BUFSIZ,nData))!= -1)
	    {
	      if (strcmp(&dbuf[dl-2],"\r\n") == 0)
		{
		  dl -= 2;
		  dbuf[dl++] = '\n';
		  dbuf[dl] = '\0';
		}
	      fwrite(dbuf,1,dl,output);
	      write(fd[1], &dl, sizeof(int));
	    }
	}
      else
	while ((l = net_read(sData,dbuf,FTP_BUFSIZ)) > 0) {
	  fwrite(dbuf,1,l,output);
	  if((write(fd[1], &l, sizeof(int))) == -1)
	    break;
	}
      if (outputfile != NULL)
    	fclose(output);
      free(dbuf);
      shutdown(sData,2);
      net_close(sData);
      if(readresp('2', nControl)) {
	sendval = -2;
	write(fd[1], &sendval, sizeof(int));
	exit(0);
      }
      else {
	sendval = -3;
	write(fd[1], &sendval, sizeof(int));
	exit(2);
      }
    }
    else {
      close(fd[1]);
      xfer_pid = child_pid;
      return fd[0];
    }
}

pid_t FtpXferPID(void)
{
    return xfer_pid;
}

GLOBALDEF int FtpResume(char *outputfile, char *path, 
			unsigned long int offset, char mode, 
			netbuf *nControl)
{
    int sData;
    int l, sendval;
    char buf[256];
    char *dbuf;
    FILE *output;
    int size = 0;
    int fd[2];
    pid_t child_pid;

    pipe(fd);
    if(!(child_pid = fork())) { 
      /* child */
      close(fd[0]);
      
      sprintf(buf,"type %c",mode);
      if (!FtpSendCmd(buf,'2', nControl)) {
	write(fd[1], (void *) -1, sizeof(int));
	exit(1);
      }
      
      if ((sData = FtpPort(nControl)) == -1) {
	write(fd[1], (void *) -1, sizeof(int));
	exit(1);
      }

      sprintf(buf, "rest %ld", offset);
      if (!FtpSendCmd(buf,'3',nControl)) {
	write(fd[1], (void *) -1, sizeof(int));
	exit(1);
      }

      sprintf(buf, "retr %s",path);
      if (!FtpSendCmd(buf,'1',nControl)) {
	sendval = -1;
	write(fd[1], &sendval, sizeof(int));
	exit(1);
      }
      if ((dbuf = strchr(nControl->response,'(')))
	{
	  dbuf++;
	  size = atoi(dbuf);
	}
      if (outputfile == NULL)
	output = stdout;
      else
	{
	  output = fopen(outputfile,"a");
	  if (output == NULL)
	    {
	      perror(outputfile);
	      output = stdout;
	    }
	}
      dbuf = malloc(FTP_BUFSIZ);
      if (mode == 'A')
	{
	  netbuf *nData;
	  int dl;
	  nData = calloc(1,sizeof(netbuf));
	  if (nData == NULL)
	    {
	      perror("calloc");
	      net_close(sData);
	      write(fd[1], (void *) -1, sizeof(int));
	      exit(1);
	    }
	  nData->handle = sData;
	  while ((dl = readline(dbuf,FTP_BUFSIZ,nData))!= -1)
	    {
	      if (strcmp(&dbuf[dl-2],"\r\n") == 0)
		{
		  dl -= 2;
		  dbuf[dl++] = '\n';
		  dbuf[dl] = '\0';
		}
	      fwrite(dbuf,1,dl,output);
	      write(fd[1], &dl, sizeof(int));
	    }
	}
      else
	while ((l = net_read(sData,dbuf,FTP_BUFSIZ)) > 0) {
	  fwrite(dbuf,1,l,output);
	  if((write(fd[1], &l, sizeof(int))) == -1)
	    break;
	}
      if (outputfile != NULL)
    	fclose(output);
      free(dbuf);
      shutdown(sData,2);
      net_close(sData);
      if(readresp('2', nControl)) {
	sendval = -2;
	write(fd[1], &sendval, sizeof(int));
	exit(0);
      }
      else {
	sendval = -3;
	write(fd[1], &sendval, sizeof(int));
	exit(2);
      }
    }
    else {
      close(fd[1]);
      xfer_pid = child_pid;
      return fd[0];
    }
}


/*
 * FtpPut - issue a PUT command and send data from input
 *
 * return 1 if successful, 0 otherwise
 */
GLOBALDEF int FtpPut(char *inputfile, char *path, char mode, netbuf *nControl)
{
    int sData;
    int l;
    char buf[256];
    char *dbuf;
    FILE *input;
    int fd[2];
    pid_t child_pid;
    

    pipe(fd);
    if(!(child_pid = fork())) {
      /* child */
      close(fd[0]);

      if (inputfile == NULL)
	{
	  fprintf(stderr,"Must specify a file name to send\n");
	  write(fd[1], (void *) -1, sizeof(int));
	  exit(1);
	}

      input = fopen(inputfile,"r");
      if (input == NULL)
	{
	  perror(inputfile);
	  write(fd[1], (void *) -1, sizeof(int));
	  exit(1);
	}

      sprintf(buf,"type %c",mode);
      if (!FtpSendCmd(buf,'2',nControl)) {
	write(fd[1], (void *) -1, sizeof(int));
	exit(1);
      }
      
      if ((sData = FtpPort(nControl)) == -1) {
	write(fd[1], (void *) -1, sizeof(int));
	exit(1);
      }
      
      sprintf(buf,"stor %s",path);
      if (!FtpSendCmd(buf,'1',nControl)) {
	write(fd[1], (void *) -1, sizeof(int));
	exit(1);
      }

      dbuf = malloc(FTP_BUFSIZ);
      if (mode == 'A')
	{
	  netbuf *nData;
	  int dl;
	  nData = calloc(1,sizeof(netbuf));
	  if (nData == NULL)
	    {
	      perror("calloc");
	      net_close(sData);
	      fclose(input);
	      write(fd[1], (void *) -1, sizeof(int));
	      exit(1);
	    }
	  nData->handle = fileno(input);
	while ((dl = readline(dbuf,FTP_BUFSIZ-1,nData))!= -1)
	{
	    if (dbuf[dl-1] == '\n')
	    {
		dbuf[dl-1] = '\r';
		dbuf[dl++] = '\n';
		dbuf[dl] = '\0';
	    }
	    if (net_write(sData,dbuf,dl) == -1)
	    {
		perror("write");
		break;
	    }
	    write(fd[1], &dl, sizeof(int));
	}
	free(nData);
    }
    else
      while ((l = fread(dbuf,1,FTP_BUFSIZ,input)) != 0) {
	if (net_write(sData,dbuf,l) == -1)
	  {
	    perror("write");
	    break;
	  }
	if((write(fd[1], &l, sizeof(int))) == -1)
	  break;
      }
      fclose(input);
      free(dbuf);
      if (shutdown(sData,2) == -1)
	perror("shutdown");
      if (net_close(sData) == -1)
	perror("close");
      if(readresp('2', nControl)) {
	write(fd[1], (void *) -2, sizeof(int));
	exit(0);
      }
      else {
	write(fd[1], (void *) -3, sizeof(int));
	exit(2);
      }
    }
    else {
      close(fd[1]);
      return fd[0];
    }
}

/*
 * FtpRename - rename a file at remote
 *
 * return 1 if successful, 0 otherwise
 */
GLOBALDEF int FtpRename(char *src, char *dst, netbuf *nControl)
{
    char cmd[256];
    sprintf(cmd,"RNFR %s",src);
    if (!FtpSendCmd(cmd,'3',nControl))
	return 0;
    sprintf(cmd,"RNTO %s",dst);
    if (!FtpSendCmd(cmd,'2',nControl))
	return 0;
    return 1;
}

/*
 * FtpDelete - delete a file at remote
 *
 * return 1 if successful, 0 otherwise
 */
GLOBALDEF int FtpDelete(char *fnm, netbuf *nControl)
{
    char cmd[256];
    sprintf(cmd,"DELE %s",fnm);
    if (!FtpSendCmd(cmd,'2', nControl))
	return 0;
    return 1;
}

/*
 * FtpQuit - disconnect from remote
 *
 * return 1 if successful, 0 otherwise
 */
GLOBALDEF void FtpQuit(netbuf *nControl)
{
    FtpSendCmd("quit",'2',nControl);
    net_close(nControl->handle);
    free(nControl);
}

#ifdef BAD_GETHOSTBYNAME
int my_get_host_by_name(char *host, struct in_addr *site)
{
  FILE *fp;
  char templine[80];
  char *addr, *end;
  int numeric_ip = 1;
  char *errormsg = "*** Can't find";

  for(addr = host; *addr; addr++) {
    if(isalpha(*addr)) {
      numeric_ip = 0;
      break;
    }
  }

  if(numeric_ip) 
    /* looks like num ip, forget about nslookup */
    return(inet_aton(host, site));
 
  
  sprintf(templine, "nslookup %s 2>&1", host);
  fp = popen(templine, "r");

  fgets(templine, sizeof(templine), fp);
  if(strlen(templine) >= strlen(errormsg)) {
    if(!strncmp(templine, errormsg, strlen(errormsg))) {
      /* Error, nslookup will halt so we must kill */
      pclose(fp);
      return 0;
    }
  }

  fgets(templine, sizeof(templine), fp);
  fgets(templine, sizeof(templine), fp);
  
  fgets(templine, sizeof(templine), fp);
  if(strncmp(templine, "Name", 4)) {
    fgets(templine, sizeof(templine), fp);
    if(strncmp(templine, "Name", 4)) {
      pclose(fp);
      return 0;
    }
  }

  fgets(templine, sizeof(templine), fp);
  if(strncmp(templine, "Address", 7)) {
    pclose(fp);
    return 0;
  }
  pclose(fp);

  if((addr = strchr(templine, '\n')))
    *addr = '\0';

  for(addr = templine; *addr && !isdigit(*addr); addr++);
  for(end = addr; *end && (isdigit(*end) || *end == '.'); end++);

  if(!(*addr))
    return 0;
  
  *end = '\0';

  return(inet_aton(addr, site));
}
#endif
  
int FtpSiteFP(char *cmd, netbuf *nControl, FILE *fp)
{
  char buf[256];
  sprintf(buf,"site %s",cmd);
  if (!FtpSendCmdFP(buf,'2',nControl, fp))
    return 0;
  return 1;
}
  
int FtpSendCmdFP(char *cmd, char expresp, netbuf *nControl, FILE *fp)
{
    char buf[256];
    if (ftplib_debug > 2)
	fprintf(stderr,"%s\n",cmd);
    sprintf(buf,"%s\r\n",cmd);
    if (net_write(nControl->handle,buf,strlen(buf)) <= 0)
    {
	perror("write");
	return 0;
    }    
    return readrespFP(expresp, nControl, fp);
}

int readrespFP(char c, netbuf *nControl, FILE *fp)
{
  char match[5];
  char *ch_ptr;

  if (readline(nControl->response,256,nControl) == -1) {
    perror("Control socket read failed");
    return 0;
  }
  while((ch_ptr = strpbrk(nControl->response, "\r\n")))
    *ch_ptr = '\0';
  if(fp)
    fprintf(fp, "%s\n", nControl->response);
  if (ftplib_debug > 1)
    fprintf(stderr,"%s",nControl->response);
  
  if (nControl->response[3] == '-') {
    strncpy(match,nControl->response,3);
    match[3] = ' ';
    match[4] = '\0';
    do
      {
	if (readline(nControl->response,256,nControl) == -1)
	  {
	    perror("Control socket read failed");
	    return 0;
	  }
	if (ftplib_debug > 1)
	  fprintf(stderr,"%s",nControl->response);
	while((ch_ptr = strpbrk(nControl->response, "\r\n")))
	  *ch_ptr = '\0';
	if(fp)
	  fprintf(fp, "%s\n", nControl->response);
      }
    while (strncmp(nControl->response,match,4));
  }
  if (nControl->response[0] == c)
    return 1;
  return 0;
}

void wakeup(int sig)
{
}

int FtpConnectFP(char *host, netbuf **nControl, int port, FILE *fp)
{
    int sControl;
    struct sockaddr_in sin;
    struct hostent *phe;
    struct servent *pse;
    int on=1;
    netbuf *ctrl;

    memset(&sin,0,sizeof(sin));
    sin.sin_family = AF_INET;
#if defined(VMS)
    sin.sin_port = htons(21);
#else
    if(port < 0) {
      if ((pse = getservbyname("ftp","tcp")) == NULL)
	{
	  perror("getservbyname");
	  return 0;
	}
      sin.sin_port = pse->s_port;
    }
    else
      sin.sin_port = htons(port);   
#endif
#ifdef BAD_GETHOSTBYNAME
    if(!my_get_host_by_name(host, &sin.sin_addr))
      return 0;
#else
    if ((phe = gethostbyname(host)) == NULL)
    {
	perror("gethostbyname");
	return 0;
    }
    memcpy((char *)&sin.sin_addr, phe->h_addr, phe->h_length);
#endif

    sControl = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sControl == -1)
    {
	perror("socket");
	return 0;
    }
    if (setsockopt(sControl,SOL_SOCKET,SO_REUSEADDR,
		   SETSOCKOPT_OPTVAL_TYPE &on, sizeof(on)) == -1)
    {
	perror("setsockopt");
	close(sControl);
	return 0;
    }
    signal(SIGALRM, wakeup);
    alarm(NET_TIMEOUT);
    if (connect(sControl, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
      alarm(0);
	perror("connect");
	close(sControl);
	return 0;
    }
    alarm(0);

    ctrl = calloc(1,sizeof(netbuf));
    if (ctrl == NULL)
    {
	perror("calloc");
	close(sControl);
	return 0;
    }
    ctrl->handle = sControl;
    if (readrespFP('2', ctrl, fp) == 0)
    {
	close(sControl);
	free(ctrl);
	return 0;
    }
    *nControl = ctrl;
    return 1;
}

int FtpLoginFP(char *user, char *pass, netbuf *nControl, FILE *fp)
{
    char tempbuf[64];
    sprintf(tempbuf,"user %s",user);
    if (!FtpSendCmdFP(tempbuf,'3',nControl, fp)) {
	return 0;
    }
    sprintf(tempbuf,"pass %s",pass);
    return FtpSendCmdFP(tempbuf,'2',nControl, fp);
}

/*
int myFtpDir(char *outputfile, char *path, netbuf *nControl)
{
    int sData,l;
    char buf[256];
    char *dbuf;
    FILE *output;
    int siggo = 1;
    fd_set rfds;
    struct timeval tv;
    int retval;

    if (!FtpSendCmd("type A",'2', nControl))
	return 0;
    if ((sData = FtpPort(nControl)) == -1)
	return 0;
    strcpy(buf,"list");
    if (path != NULL)
	sprintf(buf+strlen(buf)," %s",path);
    if (!FtpSendCmd(buf,'1', nControl))
	return 0;
    if (outputfile == NULL)
	output = stdout;
    else
    {
	output = fopen(outputfile,"w");
	if (output == NULL)
	{
	    perror(outputfile);
	    output = stdout;
	}
    }
    dbuf = malloc(FTP_BUFSIZ);
    
    while(siggo) {
      FD_ZERO(&rfds);
      FD_SET(sData, &rfds);
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      
      retval = select(sData + 1, &rfds, NULL, NULL, &tv);      
      if(retval > 0) {
	if(FD_ISSET(sData, &rfds)) {
	  if((l = net_read(sData,dbuf,FTP_BUFSIZ)) > 0) {
	    fwrite(dbuf,1,l,output);
	  }
	  if(!l)
	    break;
	}
      }
      else
	if(retval < 0)
	  break;
      InterruptSigGoUI(to_maincb, &siggo);
    }
    if (outputfile != NULL)
	fclose(output);
    free(dbuf);
    shutdown(sData,2);
    net_close(sData);
}
*/
