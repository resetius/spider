#include <db_cxx.h>

#include <boost/filesystem.hpp>

#include "links_db.h"
#include "utils/logger.h"

static void db_error(const DbEnv * env, const char * p, const char * m)
{
	mylog(5, "%s: %s", p, m);
}

LinksDb::LinksDb(const std::string & path)
{
	env_ = new DbEnv(0);
	env_->set_errcall(db_error);
	boost::filesystem::create_directories(path);
	if (env_->open(path.c_str(), DB_INIT_MPOOL | DB_CREATE, 0) != 0)
	{
		mylog(LOG_ERR, "cannot open/create state directory");
		exit(1);
	}

	queue_       = new Store<uint32_t, uint64_t>(env_);
	downloaded_  = new Store<uint64_t, uint8_t>(env_);
	downloading_ = new Store<uint64_t, uint8_t>(env_);
	urls_        = new Store<std::string, uint64_t>(env_);
	ids_         = new Store<uint64_t, std::string>(env_);

	queue_->set_re_len(8);

	if (queue_->open(0, "queue", 0, DB_QUEUE, DB_CREATE, 0) != 0) {
		mylog(LOG_ERR, "cannot open/create database");
		exit(1);		
	}
	if (downloaded_->open(0, "downloaded", 0, DB_HASH, DB_CREATE, 0) != 0) {
		mylog(LOG_ERR, "cannot open/create database");
		exit(1);		
	}
	if (downloading_->open(0, "downloading", 0, DB_HASH, DB_CREATE, 0) != 0) {
		mylog(LOG_ERR, "cannot open/create database");
		exit(1);		
	}
	if (urls_->open(0, "urls", 0, DB_HASH, DB_CREATE, 0) != 0) {
		mylog(LOG_ERR, "cannot open/create database");
		exit(1);		
	}
	if (ids_->open(0, "ids", 0, DB_HASH, DB_CREATE, 0) != 0) {
		mylog(LOG_ERR, "cannot open/create database");
		exit(1);		
	}

	max_id_ = 0;
	urls_->get("", max_id_);
}

LinksDb::~LinksDb()
{
	queue_->close(0);
	downloaded_->close(0);
	urls_->close(0);
	ids_->close(0);

	delete queue_;
	delete downloaded_;
	delete urls_;
	delete ids_;

	env_->close(0);
	delete env_;
}

void LinksDb::push_link(uint64_t id)
{
	uint32_t fake = 1;

	if (queue_->put(fake, id, DB_APPEND) != 0) {
		mylog(LOG_ERR, "cannot push %d", id);
	}
}

uint64_t LinksDb::pop_link()
{
	uint64_t ret = -1;
	
	Store<uint32_t, uint64_t>::iterator it = queue_->begin();
	uint32_t fake;
	if (!it->next(fake, ret)) {
		//empty
		return ret;
	}
	it->del(); // delete value and return
	//TODO: can loose id here
	return ret;
}

bool LinksDb::is_downloaded(uint64_t id)
{
	return downloaded_->exists(id) == 0;
}

bool LinksDb::is_downloading(uint64_t id)
{
	return downloading_->exists(id) == 0;
}

void LinksDb::mark_downloaded(uint64_t id)
{
	if (downloaded_->put(id, 1) != 0) {
		mylog(LOG_ERR, "cannot commit url");
	}
	downloading_->del(id);
}

void LinksDb::mark_downloading(uint64_t id)
{
	if (downloading_->put(id, 1) != 0) {
		mylog(LOG_ERR, "cannot commit url");
	}
}

// id -> string
std::string LinksDb::url(uint64_t id)
{
	std::string ret;
	ids_->get(id, ret);
	return ret;
}

// string -> url
uint64_t LinksDb::id(const std::string & url)
{
	uint64_t ret = 0;
	if (urls_->get(url, ret) == 0) {
		return ret;
	} else {
		ret = max_id_++;
		//TODO: atomic
		if (urls_->put(url, ret) != 0) {
			mylog(LOG_ERR, "cannot put '%s'", url.c_str());
		}
		if (urls_->put("", max_id_) != 0) {
			mylog(LOG_ERR, "cannot put special key");
		}
		if (ids_->put(ret, url) != 0) {
			mylog(LOG_ERR, "cannot put url");
		}
	}
	return ret;
}
