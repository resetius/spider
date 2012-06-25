#ifndef DNL_QUEUE_H
#define DNL_QUEUE_H
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

#include <string>
#include <vector>
#include <stdint.h>

#include <unordered_map>
#include <unordered_set>
#include <queue>

#include "utils/timer.h"
#include "links_extractor.h"
#include "links_db.h"

class Urls;
class Ids;
class Downloaded;
class Queue;
class Dnl;

struct evhttp_connection;
struct evhttp_uri;

struct Download
{
	uint64_t id;
	std::string url;
	evhttp_uri * uri;
	Dnl * dnl;
	Download();
	~Download();
};

class Dnl
{
	LinksDb ldb_;
	LinksExtractor le_;

	struct event_base * base_;
	struct evdns_base * dns_;
	struct evhttp * http_;

	int downloads_;
	uint64_t bytes_;
	uint64_t urls_downloaded_;
	Timer t_;

	uint64_t fake_; // TODO: remove me

	std::unordered_set < uint64_t > downloading_ids_; //TODO: store in db
	std::unordered_map < std::string, int > downloads_from_host_;

	struct Options {
		std::string accepted_save_url;
		std::string rejected_save_url;
		std::string accepted_dnl_url;
		std::string rejected_dnl_url;
		std::string accepted_ctype;
		std::string rejected_ctype;
		std::string user_agent;

		int use_nofollow;
		int save_make_dirs;
		int save_repl_slash;
		int max_downloads;
		int max_downloads_per_host;
	} opt_;

	void enqueue();
	bool enqueue_single();

	bool fname_from_url(std::string & path, std::string & file, const std::string & url);

	bool check_dnl_url(const std::string & url, uint64_t id);
	bool check_save_url(const std::string & url, uint64_t id);
	bool check_content_type(const std::string & ctype);
	void save_content(const std::string & content, const std::string & url);

	void extract_links(
			struct evhttp_request * req, 
			const std::string & content, 
			Download * d);

public:

	Dnl();
	~Dnl();

	void process(struct evhttp_request * req, Download * d);

	void add(const std::string & );
	void run();
	void stop();
};

#endif /*DNL_QUEUE_H*/

