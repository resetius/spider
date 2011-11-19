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

#include "dnl.h"
#include "logger.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

Dnl * d;

typedef void  (*sig_handler_t) (int);

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

void stop(int signo)
{
	d->stop();
}

int main(int argc, char ** argv)
{
	Dnl dnl;
	char buf[32768];
	d = &dnl;
	mylog_init(6, "spider.log");

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

	set_signal(2, stop);
	set_signal(15, stop);

	dnl.run();
	mylog_close();

	return 0;
}
