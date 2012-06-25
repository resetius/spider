#ifndef LINKS_DB_H
#define LINKS_DB_H

#include <stdint.h>

#include <string>

#include "store.h"

class LinksDb
{
	DbEnv * env_;
	Store<uint32_t, uint64_t> * queue_;
	Store<uint64_t, uint8_t> * downloaded_;
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

	// id -> string
	std::string url(uint64_t id);
	// string -> url
	uint64_t id(const std::string & url);
};

#endif /* LINKS_DB_H */

