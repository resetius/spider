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
#include "links_extractor.h"

int main(int argc, char ** argv)
{
	char buf[32768];
	std::string content;
	size_t size;

	while ((size = fread(buf, 1, sizeof(buf), stdin)) > 0) 
	{
		content.insert(content.end(), buf, buf + size);
	}

	LinksExtractor le;
	LinksExtractor::links_t links = le.extract(content);

	for (LinksExtractor::links_t::iterator it = links.begin(); it != links.end(); ++it) 
	{
		Link & link = *it;
		fprintf(stderr, "type: '%s'\n", link.type.c_str());

		for (Link::keyval_t::iterator jt = link.keyval.begin(); jt != link.keyval.end(); ++jt) {
			fprintf(stderr, "  '%s':'%s'\n", jt->first.c_str(), jt->second.c_str());
		}
	}
}

