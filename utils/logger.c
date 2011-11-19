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
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "logger.h"

#ifdef WIN32
#define flockfile _lock_file
#define funlockfile _unlock_file
#endif

static const char * fn         = 0;
static const char * timeformat = "%d.%m.%Y-%H:%M:%S";
static int level         = 5;
static FILE * fd; 

static void mylog_renew_file()
{
	if (fd) {
		fclose(fd); fd = 0;
	}

	if (fn) {
		fd = fopen(fn, "a");
		if (fd) {
			setbuf(fd, 0);
		}
	}
}

void mylog_init(int max_level, const char * fname)
{
	fn    = fname;
	level = max_level;


	mylog_renew_file();
}

void mylog_close()
{
	if (fd) {
		fclose(fd); fd = 0;
	}
}

void mylog(int type, const char * format, ...)
{
	char time_buf[1024];
	va_list ap;
	struct tm * tm = 0;
	time_t t;

	if (type > level) {
		return;
	}

	time(&t);
	tm = localtime(&t);

	if (timeformat) {
		strftime(time_buf, sizeof(time_buf), timeformat, tm);
		time_buf[sizeof(time_buf) - 1] = 0;
	}

	va_start(ap, format);
	flockfile(fd);

	if (timeformat) {
		fprintf(fd, "%s: ", time_buf);
	}

	switch (type) {
	case LOG_EMERG:
		fprintf(fd, "EMERGE:\t");
		break;
	case LOG_ALERT:
		fprintf(fd, "ALERT:\t");
		break;
	case LOG_CRIT:
		fprintf(fd, "CRIT:\t");
		break;
	case LOG_ERR:
		fprintf(fd, "ERR:\t");
		break;
	case LOG_WARNING:
		fprintf(fd, "WARN:\t");
		break;
	case LOG_NOTICE:
		fprintf(fd, "NOTICE:\t");
		break;
	case LOG_INFO:
		fprintf(fd, "INFO:\t");
		break;
	case LOG_DEBUG:
	default:
		fprintf(fd, "DEBUG:\t");
		break;
	}

	vfprintf(fd, format, ap);
	fprintf(fd, "\n");	

	funlockfile(fd);
	va_end(ap);
}

