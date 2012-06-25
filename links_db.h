#ifndef LINKS_DB_H
#define LINKS_DB_H
/* -*- charset: utf-8 -*- */
/*$Id$*/

/* Copyright (c) 2012 Alexey Ozeritsky
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

#include <stdint.h>

#include <string>

#include "store.h"

class LinksDb
{
	DbEnv * env_;
	Store<uint32_t, uint64_t> * queue_;
	Store<uint64_t, uint8_t> * downloaded_;
	Store<uint64_t, uint8_t> * downloading_;
	Store<std::string, uint64_t> * urls_;
	Store<uint64_t, std::string> * ids_;

	uint64_t max_id_;

public:
	LinksDb(const std::string & path);
	~LinksDb();
	
	void push_link(uint64_t id);
	uint64_t pop_link();

	bool is_downloaded(uint64_t id);
	void mark_downloaded(uint64_t id);

	bool is_downloading(uint64_t id);
	void mark_downloading(uint64_t id);

	// id -> string
	std::string url(uint64_t id);
	// string -> url
	uint64_t id(const std::string & url);

	// move back downloading into queue
	void restore_downloading();
};

#endif /* LINKS_DB_H */

