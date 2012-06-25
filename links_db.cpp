#include <db_cxx.h>

#include "links_db.h"
#include "utils/logger.h"

#if 0
	db_env_ = new KV::BEnv("state"); // TODO
	db_env_->set_errcall(mylog);
	if (db_env_->open(DB_INIT_MPOOL | DB_CREATE, 0) != 0)
	{
		mylog(LOG_ERR, "cannot open/create state directory");
	}
#endif

static void db_error(const DbEnv * env, const char * p, const char * m)
{
	mylog(5, "%s: %s", p, m);
}

LinksDb::LinksDb(const std::string & path)
{
	env_ = new DbEnv(0);
	env_->set_errcall(db_error);
	if (env_->open(path.c_str(), DB_INIT_MPOOL | DB_CREATE, 0) != 0)
	{
		mylog(LOG_ERR, "cannot open/create state directory");
		exit(1);
	}

	queue_      = new Store<uint32_t, uint64_t>(env_);
	downloaded_ = new Store<uint64_t, uint8_t>(env_);
	urls_       = new Store<std::string, uint64_t>(env_);
	ids_        = new Store<uint64_t, std::string>(env_);

	if (queue_->open(0, "queue", 0, DB_QUEUE, DB_CREATE, 0) != 0) {
		mylog(LOG_ERR, "cannot open/create database");
		exit(1);		
	}
	if (downloaded_->open(0, "downloaded", 0, DB_HASH, DB_CREATE, 0) != 0) {
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
}

uint64_t LinksDb::pop_link()
{
}

bool LinksDb::is_downloaded(uint64_t id)
{
}

void LinksDb::mark_downloaded(uint64_t id)
{
}

// id -> string
std::string LinksDb::url(uint64_t id)
{
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
