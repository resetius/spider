#ifndef LINKS_DB_H
#define LINKS_DB_H

#include <stdint.h>

#include <string>

class LinksDb
{
public:
	void push_link(uint64_t id);
	uint64_t pop_link();

	void is_downloaded(uint64_t id);
	void mark_downloaded(uint64_t id);

	// id -> string
	std::string url(uint64_t id);
	// string -> url
	uint64_t id(const std::string & url);
};

#endif /* LINKS_DB_H */

