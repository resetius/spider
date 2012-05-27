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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/buffer.h>

#include <event2/event.h>
#include <event2/dns.h>

#include <boost/regex.hpp> 
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "utils/config.h"
#include "utils/logger.h"
#include "dnl.h"

#ifdef WIN32
#include <io.h>
#define snprintf _snprintf
#define access _access
#endif

Download::Download(): uri(0)
{
}

Download::~Download()
{
	if (uri) {
		evhttp_uri_free(uri);
	}
}

static void libevent_log(int level, const char * msg)
{
	mylog(level, msg);
}

Dnl::Dnl()
{
	db_env_ = new KV::BEnv("state"); // TODO
	db_env_->set_errcall(mylog);
	if (db_env_->open(/* DB_INIT_LOCK | */ DB_INIT_MPOOL | DB_CREATE /* | DB_THREAD*/ , 0) != 0)
	{
		mylog(LOG_ERR, "cannot open/create state directory");
	}
	ids_ = new Ids(this);
	urls_ = new Urls(this);
	downloaded_ = new Downloaded(this);
	queue_ = new Queue(this);

	event_set_log_callback(libevent_log);

	base_ = event_base_new();
	dns_  = evdns_base_new(base_, 1);
	http_ = evhttp_new(base_);

	downloads_ = 0;

	bytes_ = 0;
	urls_downloaded_  = 0;

	// config
	Config c;
	c.open("spider.ini");

	opt_.max_downloads     = c.get("dnl", "max_downloads", 10);
	opt_.max_downloads_per_host = c.get("dnl", "max_downloads_per_host", 5);

	opt_.accepted_save_url = c.get("dnl", "accepted_save_url", "");
	opt_.rejected_save_url = c.get("dnl", "rejected_save_url", "");

	opt_.accepted_dnl_url  = c.get("dnl", "accepted_dnl_url", "");
	opt_.rejected_dnl_url  = c.get("dnl", "rejected_dnl_url", "");
	opt_.accepted_ctype    = c.get("dnl", "accepted_ctype", "");
	opt_.rejected_ctype    = c.get("dnl", "rejected_ctype", "");
	opt_.user_agent        = c.get("dnl", "user_agent", "Johnnie Walker 1.0");

	opt_.use_nofollow      = c.get("dnl", "use_nofollow", 1);
	opt_.save_make_dirs    = c.get("dnl", "save_make_dirs", 0);
	opt_.save_repl_slash   = c.get("dnl", "save_repl_slash", 1);
}

Dnl::~Dnl()
{
	delete ids_;
	delete urls_;
	delete downloaded_;
	delete queue_;
	delete db_env_;

	evhttp_free(http_);
	evdns_base_free(dns_, 0);
	event_base_free(base_);
}

void Dnl::add(const std::string & url)
{
//	fprintf(stderr, "add url '%s'\n", url.c_str());
	uint64_t id = urls_->id(url);
	queue_->push(id);
}

void Dnl::save_content(const std::string & content, const std::string & url)
{
	std::string path, file;
	fname_from_url(path, file, url);
	std::string fname = path + file;
	mylog(LOG_NOTICE, "save '%s'", fname.c_str());
	boost::filesystem::create_directories(path);

	FILE * f = fopen(fname.c_str(), "wb");
	if (!f) {
		mylog(LOG_ERR, "cannot open '%s'", fname.c_str());
		return;
	}
	fwrite(&content[0], 1, content.length(), f);
	fclose(f);
}

void Dnl::extract_links(
	struct evhttp_request * req,
	const std::string & content,
	Download * d)
{
	const char * ctype = evhttp_find_header(req->input_headers, "Content-Type");
	struct evhttp_uri * src = d->uri;
	std::string & url = d->url;
	uint64_t id       = d->id;

	if (ctype && std::string(ctype).find("text/html") == 0) {
		// extract here 
		LinksExtractor::links_t links = le_.extract(content);
		for (LinksExtractor::links_t::iterator it = links.begin(); it != links.end(); ++it)
		{
			Link & link = *it;

			std::string base;
			if (opt_.use_nofollow && link.keyval["rel"].find("nofollow") != std::string::npos) {
				continue;
			}
			if (link.type == "a") {
				base = link.keyval["href"];
			} else if (link.type == "img") {
				base = link.keyval["src"];
			} else if (link.type == "redir") {
				base = link.keyval["url"];
			}
			struct evhttp_uri * uri = evhttp_uri_parse(base.c_str());
			if (!uri) {
				continue;
			}

			const char * scheme = evhttp_uri_get_scheme(uri);
			if (scheme) { /* absolute */
				if (!strcmp(scheme, "http")) {
					add(base);
				}
				evhttp_uri_free(uri);
				continue;
			}

			size_t pos = url.rfind("/");
			if (pos == std::string::npos) {
				base = std::string("/") + base;
			}

			if (base.empty() || base[0] == '/') {
				std::string new_url;
				new_url  = std::string(evhttp_uri_get_scheme(src));
				new_url += "://";
				new_url += evhttp_uri_get_host(src);
				if (evhttp_uri_get_port(src) > 0) {
					char buf[1024];
					snprintf(buf, 1023, "%d", evhttp_uri_get_port(src));
					new_url += buf;
				}
//				fprintf(stderr, "new url, base: '%s' '%s'\n", new_url.c_str(), base.c_str());
				if (base.empty()) {
					new_url += "/";
				} else {
					new_url += base;
				}
				add(new_url);
				evhttp_uri_free(uri);
				continue;
			}

			std::string new_url = url.substr(0, pos+1);
//			fprintf(stderr, "new url, base: '%s' '%s'\n", new_url.c_str(), base.c_str());

			new_url += base;
			add(new_url);

			evhttp_uri_free(uri);
		}

		if (check_save_url(url, id)) {
			save_content(content, url);
		}
	} else {
		if (ctype && check_content_type(ctype)) {
			save_content(content, url);
		}
	}
}

void Dnl::process(struct evhttp_request * req, Download * d)
{
	int code = req ? evhttp_request_get_response_code(req) : 0;
	mylog(LOG_DEBUG, "process '%s' %d !", d->url.c_str(), code);

	if (code == 200) {
		size_t size = evbuffer_get_length(req->input_buffer);
		std::string content;
		if (size) {
			content.resize(size);
			evbuffer_remove(req->input_buffer, &content[0], size);
		}
		downloaded_->done(d->id);
		downloading_ids_.erase(d->id);

		if (!content.empty()) {
			extract_links(req, content, d);
		}

		bytes_ += size;
		urls_downloaded_  += 1;
		fprintf(stderr, "speed: %lf K\t%lf\r",
				(double)bytes_ / t_.elapsed() / 1024.0,
				(double)urls_downloaded_ / t_.elapsed()
				);
	} else if (code >= 300 && code < 400) {
		const char * location = evhttp_find_header(req->input_headers, "Location");
		//fprintf(stderr, "location: '%s'\n", location);

		downloaded_->done(d->id);
		urls_downloaded_ += 1;

		if (location) {
			add(location);
		}
	}
	downloading_ids_.erase(d->id);

	const char * addr = evhttp_uri_get_host(d->uri);
	downloads_from_host_[addr]--;
//	fprintf(stderr, " del '%s' -> %d of %d\n", addr, downloads_from_host_[addr],  opt_.max_downloads_per_host);

	if (downloads_from_host_[addr] < 0) {
		downloads_from_host_.erase(addr);
	}

	downloads_ --;
	enqueue();
	delete d;
}

void cb(struct evhttp_request * req, void * arg)
{
	uint64_t id = (uint64_t)arg;
	struct evhttp_connection * con = req ? evhttp_request_get_connection(req) : 0;
	Download * d = (Download*)arg;

	d->dnl->process(req, d);

	if (con) {
		evhttp_connection_free(con);
	}
}

bool Dnl::fname_from_url(std::string & path, std::string & file, const std::string & url)
{
	size_t i = url.rfind("/"); 
	if (i == std::string::npos) {
		return false;
	}
	std::string fname = url.substr(i+1);
	struct evhttp_uri * uri = evhttp_uri_parse_with_flags(url.c_str(), 0);
	if (!uri) {
		return false;
	}

	if (opt_.save_make_dirs) {
		if (!evhttp_uri_get_path(uri)) {
			path = "/";
		} else {
			path = evhttp_uri_get_path(uri);
		}
		i = path.rfind("/");
		if (i != std::string::npos) {
			path = path.substr(0, i);
		}
		if (!path.empty()) {
			path = path.substr(1);
			path = path + "/";
		}
		file = fname;
	} else if (opt_.save_repl_slash) {
		file = url;
		boost::algorithm::replace_all(file, "http://", "");
		boost::algorithm::replace_all(file, ":", "");
		boost::algorithm::replace_all(file, "/", "_");
		path = "./out/";
	} else {
		path = "./";
		file = fname;
	}
	evhttp_uri_free(uri);

	return true;
}

bool Dnl::check_dnl_url(const std::string & url, uint64_t id)
{
	if (downloaded_->check(id)) {
		mylog(LOG_NOTICE, "url '%s' already downloaded", url.c_str());
		return false;
	}
	std::string path, file;
	if (!fname_from_url(path, file, url)) {
		mylog(LOG_NOTICE, "bad url '%s'", url.c_str());
		return false;
	}
	std::string full = path + file;
	int mode = 0644;
#if WIN32
	mode = 00;
#endif
	if (access(full.c_str(), mode) == 0) {
		// already exists
		mylog(LOG_NOTICE, "file '%s'exists", full.c_str());
		return false;
	}

	if (!opt_.rejected_dnl_url.empty()) {
		static boost::regex rejected_dnl_url_re(opt_.rejected_dnl_url);
		if (boost::regex_match(url, rejected_dnl_url_re)) {
			mylog(LOG_DEBUG, "'%s' match '%s'", url.c_str(), opt_.rejected_dnl_url.c_str());
			return false;
		}
	}

	if (opt_.accepted_dnl_url.empty()) {
		return true;
	}

	static boost::regex accepted_dnl_url_re(opt_.accepted_dnl_url);
	if (boost::regex_match(url, accepted_dnl_url_re, boost::regex_constants::match_perl)) {
		return true;
	}

	return false;
}

bool Dnl::check_save_url(const std::string & url, uint64_t id)
{
	if (!opt_.rejected_save_url.empty()) {
		static boost::regex rejected_save_url_re(opt_.rejected_save_url);
		if (boost::regex_match(url, rejected_save_url_re)) {
			return false;
		}
	}

	if (opt_.accepted_save_url.empty()) {
		return true;
	}

	static boost::regex accepted_save_url_re(opt_.accepted_save_url);
	if (boost::regex_match(url, accepted_save_url_re)) {
		return true;
	}
	return false;
}

bool Dnl::check_content_type(const std::string & ctype)
{
	if (!opt_.rejected_ctype.empty()) {
		static boost::regex rejected_ctype_re(opt_.rejected_ctype);
		if (boost::regex_match(ctype, rejected_ctype_re)) {
			mylog(LOG_DEBUG, "match '%s' '%s'", ctype.c_str(), opt_.rejected_ctype.c_str());
			return false;
		}
	}
	
	if (opt_.accepted_ctype.empty()) {
		return true;
	}
	static boost::regex accepted_ctype_re(opt_.accepted_ctype);
	if (boost::regex_match(ctype, accepted_ctype_re)) {
		return true;
	}
	mylog(LOG_DEBUG, "!match '%s' '%s'", ctype.c_str(), opt_.accepted_ctype.c_str());

	return false;
}

bool Dnl::enqueue_single()
{
	uint64_t id = queue_->pop();
	if (id == (uint64_t)(-1)) {
		mylog(LOG_NOTICE, "empty queue");
		return false;
	}
	if (downloading_ids_.find(id) != downloading_ids_.end()) {
		mylog(LOG_DEBUG, "url %lu already downloading", id);
		return true;
	}
	std::string url = ids_->url(id);
	if (url.empty()) {
		mylog(LOG_DEBUG, "empty url");
		return true;
	}
	if (!check_dnl_url(url, id)) {
		mylog(LOG_DEBUG, "url '%s' bad check", url.c_str());
		return true;
	}

	struct evhttp_uri * uri = evhttp_uri_parse_with_flags(url.c_str(), 0);
	if (!uri) {
		mylog(LOG_DEBUG, "cannot parse uri '%s'", url.c_str());
		return true;
	}
	const char * scheme = evhttp_uri_get_scheme(uri);
	if (!scheme || strcmp(scheme, "http")) {
		evhttp_uri_free(uri);
		return true;
	}

	const char * addr = evhttp_uri_get_host(uri);
	int port    = evhttp_uri_get_port(uri);
	if (port == -1) {
		port = 80;
	}
	if (downloads_from_host_.find(addr) != downloads_from_host_.end()) {
		if (downloads_from_host_[addr] > opt_.max_downloads_per_host) {
			mylog(LOG_DEBUG, " host '%s' limit reached ", addr);
			queue_->push(id);
			evhttp_uri_free(uri);
			// TODO: not optimal
			//
			if (fake_ == -1) {
				fake_ = id;
				return true;
			}
			if (fake_ == id) {
				return false;
			}
			return true;
		}
	}
	Download * rq = new Download;
	rq->id  = id;
	rq->url = url;
	rq->dnl = this;
	rq->uri = uri;

	mylog(LOG_DEBUG, "make connection: '%s' %d", addr, port);
	struct evhttp_connection * con = evhttp_connection_base_new(
			base_, dns_, addr, port);
	if (!con) {
		mylog(LOG_ERR, "cannot create connection '%s' %d", addr, port);
		exit(-1);
	}

	downloading_ids_.insert(id);

	struct evhttp_request * req = evhttp_request_new(cb, rq);
	if (!req) {
		mylog(LOG_ERR, "cannot create request '%s' %d", addr, port);
		exit(-1);
	}
	evhttp_add_header(
			req->output_headers, 
			"User-Agent", opt_.user_agent.c_str());
	evhttp_add_header(
			req->output_headers,
			"Accept", "*/*");
	evhttp_add_header(
			req->output_headers,
			"Host", addr);

	mylog(LOG_DEBUG, "start: '%s'", url.c_str());
	
	std::string query;
	if (evhttp_uri_get_path(uri)) {
		query += evhttp_uri_get_path(uri);
	}
	if (evhttp_uri_get_query(uri)) {
		query += "?";
		query += evhttp_uri_get_query(uri);
	}

	if (query.empty()) {
		query = "/";
	}
	evhttp_make_request(con, req, EVHTTP_REQ_GET, query.c_str());

	downloads_from_host_[addr] ++;
	//fprintf(stderr, " add '%s' -> %d of %d (%s)\n", addr, downloads_from_host_[addr],  opt_.max_downloads_per_host, query.c_str());
	downloads_++;

	return true;
}

void Dnl::stop()
{
	mylog(LOG_DEBUG, "break loop");
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1;
	event_base_loopexit(base_, &tv);
}

void Dnl::enqueue()
{
	fake_ = -1;
	while (downloads_ < opt_.max_downloads && enqueue_single());

	if (downloads_ == 0) {
		stop();
	}
}

void Dnl::run()
{
	enqueue();
	t_.restart();
	event_base_loop(base_, 0);
}

Ids::Ids(Dnl * dnl)
{
	id2url_ = new KV::BDb(*dnl->db_env(), "ids", KV::BDb::HASH);
	id2url_->open(KV::BDb::READ|KV::Db::WRITE|KV::BDb::CREATE);
}

Ids::~Ids()
{
	delete id2url_;
}

std::string Ids::url(uint64_t id)
{
	std::string ret;
	id2url_->get(0, id, ret);
	return ret;
}

void Ids::url(uint64_t id, const std::string & u)
{
	if (id2url_->put(0, id, u) != 0) {
		mylog(LOG_ERR, "cannot put url");
	}
}

Urls::Urls(Dnl * dnl): ids_(dnl->ids())
{
	url2id_ = new KV::BDb(*dnl->db_env(), "urls", KV::BDb::HASH);
	url2id_->open(KV::BDb::READ|KV::BDb::WRITE|KV::BDb::CREATE);
	max_id_ = 0;
	url2id_->get(0, "", max_id_);
}

Urls::~Urls()
{
	delete url2id_;
}

uint64_t Urls::id(const std::string & url)
{
	uint64_t ret = 0;
	if (url2id_->get(0, url, ret) == 0) {
		return ret;
	} else {
		ret = max_id_++;
		if (url2id_->put(0, url, ret) != 0) {
			mylog(LOG_ERR, "cannot put '%s'", url.c_str());
		}
		if (url2id_->put(0, "", max_id_) != 0) {
			mylog(LOG_ERR, "cannot put special key");
		}
		ids_->url(ret, url);
	}
	return ret;
}

Downloaded::Downloaded(Dnl * dnl)
{
	downloaded_ = new KV::BDb(*dnl->db_env(), "downloaded", KV::BDb::BTREE);
	if (downloaded_->open(KV::BDb::READ|KV::BDb::WRITE|KV::BDb::CREATE) != 0) {
		mylog(LOG_ERR, "cannot open downloaded");
	}
}

Downloaded::~Downloaded()
{
	delete downloaded_;
}

bool Downloaded::check(uint64_t id)
{
	KV::Db::value_type p;
	size_t size;
	if (downloaded_->get(0, (void*)&id, sizeof(id), p, size) == 0) {
		return true;
	}
	return false;
}

void Downloaded::done(uint64_t id)
{
	if (downloaded_->put(0, id, "") != 0) {
		mylog(LOG_ERR, "cannot commit url");
	}
}

Queue::Queue(Dnl * dnl)
{
	queue_ = new KV::BDb(*dnl->db_env(), "queue", KV::BDb::QUEUE);
	queue_->set_re_len(8);
	if (queue_->open(KV::BDb::READ|KV::BDb::WRITE|KV::BDb::CREATE) != 0) {
		mylog(LOG_ERR, "cannot open queue!");
	}
}

Queue::~Queue()
{
	delete queue_;
}

void Queue::push(uint64_t id)
{
	uint32_t fake = 1;
	if (queue_->put(0, &fake, sizeof(fake), (void*)&id, sizeof(id)) != 0) {
		mylog(LOG_ERR, "cannot push %d", id);
	}
}

uint64_t Queue::pop()
{
	uint64_t ret = -1;
	KV::Iterator * it = queue_->begin(0);
	if (!it) {
		return ret;
	}
	KV::Db::value_type p;
	size_t size;
	std::string key;
	if (!it->next(key, p, size)) {
		return ret;
	}
	ret = 0;
	memcpy(&ret, p.get(), size);
	it->del();
	delete it;
	return ret;
}

