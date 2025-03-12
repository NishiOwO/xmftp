/***************************************************************************/
/*									   */
/* qftp.c - command line driven ftp file transfer program                  */
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

#if defined(__unix__)
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <winsock.h>
#include <io.h>
#include "getopt.h"
#endif

#include "ftplib.h"

#if !defined(S_ISDIR)
#define S_ISDIR(m) ((m&S_IFMT) == S_IFDIR)
#endif

/* exit values */
#define EX_SYNTAX 2 	/* command syntax errors */
#define EX_NETDB 3	/* network database errors */
#define EX_CONNECT 4	/* network connect errors */
#define EX_LOGIN 5	/* remote login errors */
#define EX_REMCMD 6	/* remote command errors */
#define EX_SYSERR 7	/* system call errors */

#define FTP_SEND 1
#define FTP_GET 2
#define FTP_DIR 3
#define FTP_RM 4

static int logged_in = 0;
static char *host = NULL;
static char *user = NULL;
static char *pass = NULL;
static char mode = 'I';
static int action = 0;
static char *invocation;

void usage(void)
{
    printf("\
usage:  %s <host>\n\
\t[ -l user [ -p pass ] ]  defaults to anonymous/user@hostname\n\
\t[\n\
\t  [ -v level ]           debug level\n\
\t  [ -r rootpath ]        chdir path\n\
\t  [ -m umask ]           umask for created files\n\
\t  [ -a | -i ] ]          ascii/image transfer file\n\
\t  [ file ]               file spec for directory or file to transfer\n\
\t]...\n\n\
If no files are specified on command line, the program will read file\n\
names from stdin.\n", invocation);
}

void ftp_connect(void)
{
    if (host == NULL)
    {
	fprintf(stderr,"Host name not specified\n");
	usage();
	exit(EX_SYNTAX);
    }
    if (!logged_in)
    {
    	if (user == NULL)
    	{
	    user = "anonymous";
	    if (pass == NULL)
	    {
	    	char *u,h[64];
	    	u = getenv("USER");
	    	if (gethostname(h,64) < 0)
	    	{
		    perror("gethostname");
		    exit(EX_NETDB);
	    	}
	    	if ((u != NULL) && (h != NULL))
	    	{
		    static char xxx[64];
		    sprintf(xxx,"%s@%s",u,h);
		    pass = xxx;
	    	}
	    }
    	}
	else if (pass == NULL)
#if defined(_WIN32)
	    exit(EX_LOGIN);
#else
	    if ((pass = getpass("Password: ")) == NULL)
		exit(EX_SYSERR);
#endif
    	if (!ftpOpen(host))
    	{
	    fprintf(stderr,"Unable to connect to node %s\n%s",host,ftplib_lastresp);
	    exit(EX_CONNECT);
    	}
    	if (!ftpLogin(user,pass))
    	{
	    fprintf(stderr,"Login failure\n%s",ftplib_lastresp);
	    exit(EX_LOGIN);
    	}
	logged_in++;
    }
}

void change_directory(char *root)
{
    ftp_connect();
    if (!ftpChdir(root))
    {
	fprintf(stderr,"Change directory failed\n%s",ftplib_lastresp);
	exit(EX_REMCMD);
    }
}

void process_file(char *fnm)
{
    int i;

    ftp_connect();
    if ((action == FTP_SEND) || (action == FTP_GET))
    {
	if (action == 1)
	{
	    struct stat info;
	    if (stat(fnm,&info) == -1)
	    {
	    	perror(fnm);
		return;
	    }
	    if (S_ISDIR(info.st_mode))
	    {
		if (!ftpMkdir(fnm))
		    fprintf(stderr,"mkdir %s failed\n%s",fnm,ftplib_lastresp);
		else
		    if (ftplib_debug)
			fprintf(stderr,"Directory %s created\n",fnm);
		return;
	    }
	}
    }
    switch (action)
    {
      case FTP_DIR :
	i = ftpDir(NULL, fnm);
	break;
      case FTP_SEND :
	i = ftpPut(fnm,fnm,mode);
	if (ftplib_debug && i)
	    printf("%s sent\n",fnm);
	break;
      case FTP_GET :
	i = ftpGet(fnm,fnm,mode);
	if (ftplib_debug && i)
	    printf("%s retrieved\n",fnm);
	break;
      case FTP_RM :
	i = ftpDelete(fnm);
	if (ftplib_debug && i)
	    printf("%s deleted\n", fnm);
	break;
    }
    if (!i)
	printf("ftp error\n%s\n",ftplib_lastresp);
}

void set_umask(char *m)
{
    char buf[80];
    sprintf(buf,"umask %s", m);
    ftp_connect();
    ftpSite(buf);
}

void main(int argc, char *argv[])
{
    int files_processed = 0;
    int i;
    int opt;

    invocation = argv[0];
    optind = 1;
    if (strstr(argv[0],"send") != NULL)
	action = FTP_SEND;
    else if (strstr(argv[0],"get") != NULL)
	action = FTP_GET;
    else if (strstr(argv[0],"dir") != NULL)
	action = FTP_DIR;
    else if (strstr(argv[0],"rm") != NULL)
	action = FTP_RM;
    if (action == 0)
    {
	if (strcmp(argv[1],"send") == 0)
	    action = FTP_SEND;
    	else if (strcmp(argv[1],"get") == 0)
	    action = FTP_GET;
    	else if (strcmp(argv[1],"dir") == 0)
	    action = FTP_DIR;
    	else if (strcmp(argv[1],"rm") == 0)
	    action = FTP_RM;
	if (action)
	    optind++;
    }
    if (action == 0)
    {
	usage();
	exit(EX_SYNTAX);
    }

    ftpInit();

    while (argv[optind] != NULL)
    {
	if (argv[optind][0] != '-')
	{
	    if (host == NULL)
		host = argv[optind++];
	    else
	    {
		process_file(argv[optind++]);
		files_processed++;
	    }
	    continue;
	}
	opt = getopt(argc,argv,"ail:m:p:r:v:");
	switch (opt)
	{
	  case '?' :
	    usage();
	    exit(EX_SYNTAX);
	  case ':' :
	    usage();
	    exit(EX_SYNTAX);
	  case 'a' : mode = 'A'; break;
	  case 'i' : mode = 'I'; break;
	  case 'l' : user = optarg; break;
	  case 'm' : set_umask(optarg); break;
	  case 'p' : pass = optarg; break;
	  case 'r' : change_directory(optarg); break;
	  case 'v' :
	    if (opt == ':')
		ftplib_debug++;
	    else
		ftplib_debug = atoi(optarg);
	    break;
	}
    }

    if (files_processed == 0)
    {
	ftp_connect();
	if (action == FTP_DIR)
	    process_file(NULL);
	else
	{
	    char fnm[256];
	    do
	    {
		if (isatty(fileno(stdin)))
		    printf("file> ");
		if (gets(fnm) == NULL)
		    break;
		process_file(fnm);
	    }
	    while (1);
	}
    }
    exit(0);
}
