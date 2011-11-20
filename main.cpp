/* -*- charset: utf-8 -*- */
/*$Id$*/

/* Copyright (c) 2011 Alexey Ozeritsky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <event2/http.h>
#include <event2/event.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#ifndef WIN32
#include <unistd.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#endif

#include "dnl.h"
#include "logger.h"

static Dnl * d;

typedef void  (*sig_handler_t) (int);

#ifdef WIN32
void set_signal(int signo, sig_handler_t hndl)
{
	signal(signo, hndl);
}
#else
void set_signal(int signo, sig_handler_t hndl)
{
	struct sigaction sa;
	sa.sa_handler = hndl;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, signo);
	sa.sa_flags = 0;

	if (sigaction(signo, &sa, NULL) < 0) {
		fprintf(stderr, "cannot set %d signal! %s\n", signo, strerror(errno));
		exit(-1);
	}
}
#endif

void stop(int signo)
{
	d->stop();
}

int main(int argc, char ** argv)
{
	mylog_init(6, "spider.log");

#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		fprintf(stderr, "WSAStartup failed with error: %d\n", err);
		return 1;
	}
#endif

	Dnl dnl;
	char buf[32768];
	d = &dnl;

	set_signal(2, stop);
	set_signal(15, stop);

	if (argc > 1) {
		FILE * f = fopen(argv[1], "rb");
		if (!f) {
			fprintf(stderr, "cannot open %s\n", argv[1]);
		}
		while (fgets(buf, sizeof(buf), f)) {
			if (!strlen(buf)) {
				continue;
			}
			if (buf[strlen(buf) - 1] == '\n') {
				buf[strlen(buf) - 1] = 0;
			}
			if (buf[strlen(buf) - 1] == '\r') {
				buf[strlen(buf) - 1] = 0;
			}
			fprintf(stdout, "adding '%s'\n", buf);
			dnl.add(buf);
		}
		fclose(f);
		dnl.run();
	} else {
		fprintf(stderr, "usage: %s urls.txt\n", argv[0]);
	}

	mylog_close();

#ifdef WIN32
	 WSACleanup();
#endif

	return 0;
}

