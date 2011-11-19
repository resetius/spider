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

#include <algorithm>
#include <boost/regex.hpp>
#include <stdio.h>

#include "links_extractor.h"

LinksExtractor::LinksExtractor():
	re1("<(img|a|meta http-equiv=\"REFRESH\")([^>]+?)>", 
			boost::regex_constants::normal | boost::regex_constants::icase),
	re2("([^\\s'\"]+)\\s*=\\s*[\"']([^'\"#]+)[\"']|#")
{
}

LinksExtractor::~LinksExtractor()
{
}

LinksExtractor::links_t LinksExtractor::extract(const std::string & content)
{
	links_t ret;
	boost::match_results<std::string::const_iterator> what;
	typedef std::vector<std::string> raw_t;
	raw_t raw;
	boost::match_flag_type flags = boost::match_default;

	std::string::const_iterator start = content.begin();
	std::string::const_iterator end   = content.end();

	while (regex_search(start, end, what, re1, flags)) {
		std::string type(what[1].first, what[1].second);
		std::string raw(what[2].first, what[2].second);
		raw += " "; // wtf?
		std::transform(type.begin(), type.end(), type.begin(), ::tolower);
		start = what[0].second;

//		fprintf(stderr, "type '%s'\n", type.c_str());
//		fprintf(stderr, "raww '%s'\n", raw.c_str());


		Link link;
		if (type.find("img") != std::string::npos) {
			link.type = "img";
		} else if (type.find("meta") != std::string::npos) {
			link.type = "redir";
		} else {
			link.type = "a";
		}

		std::string::const_iterator s = raw.begin();
		std::string::const_iterator e = raw.end();
		boost::match_results<std::string::const_iterator> w;

		while (regex_search(s, e, w, re2, flags)) {
			std::string key(w[1].first, w[1].second);
			std::string val(w[2].first, w[2].second);
			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
//			fprintf(stderr, "  val '%s'\n", val.c_str());

			s = w[0].second;

			link.keyval.insert(std::make_pair(key, val));
		}

		ret.push_back(link);
	}

	return ret;
}

