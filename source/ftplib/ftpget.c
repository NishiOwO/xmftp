/***************************************************************************/
/*									   */
/* ftpget.c - retrieve a remote file via ftp                               */
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
#elif defined(_WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "ftplib.h"

void usage(char *cmd)
{
    printf("%s <host> [-d] [-a] [-l <username>] [-p <password] [-r path]\n",cmd);
    exit(2);
}

int ftpget(char *host, char *user, char *pass, char *root, char mode)
{
    char fnm[256];

    if (!ftpOpen(host))
    {
	fprintf(stderr,"Unable to connect to node %s\n%s",host,ftplib_lastresp);
	return 0;
    }
    if (!ftpLogin(user,pass))
    {
	fprintf(stderr,"Login failure\n%s",ftplib_lastresp);
	return 0;
    }
    if (root)
	if (!ftpChdir(root))
	{
	    fprintf(stderr,"Change directory to %s failed\n%s",
		    root,ftplib_lastresp);
	    return 0;
	}
    while (gets(fnm) != NULL)
    {
	if (!ftpGet(fnm,fnm,mode))
	{
	fprintf(stderr,"Get of %s failed\n%s",fnm,ftplib_lastresp);
	}
	else
	{
	if (ftplib_debug > 1)
		fprintf(stderr,"File %s received\n",fnm);
	}
    }
    ftpQuit();
    return 1;
}

void main(int argc, char *argv[])
{
    char *host = NULL;
    char *user = NULL;
    char *pass = NULL;
    char *root = NULL;
    char mode = 'I';
    int i;
    for (i=1;i<argc;i++)
    {
	if (*argv[i] == '-')
	{
	    switch (argv[i][1])
	    {
	      case 'l':
		user = argv[++i];
		break;
	      case 'p':
		pass = argv[++i];
		break;
	      case 'd':
		ftplib_debug++;
		break;
	      case 'r':
		root = argv[++i];
		break;
	      case 'a':
		mode = 'A';
		break;
	      case 'i':
		mode = 'I';
		break;
	      default:
		usage(argv[0]);
	    }
	}
	else if (host == NULL)
	    host = argv[i];
	else
	    usage(argv[0]);
    }
    ftpInit();
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
		exit(2);
	    }
	    if ((u != NULL) && (h != NULL))
	    {
		static char xxx[64];
		sprintf(xxx,"%s@%s",u,h);
		pass = xxx;
	    }
	}
    }
    if ((host == NULL) || (user == NULL) || (pass == NULL))
	usage(argv[0]);

    if (!ftpget(host,user,pass,root,mode))
	exit(2);
    exit(0);
}
