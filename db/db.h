#ifndef KV_DB_H
#define KV_DB_H
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
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include <db_cxx.h>

namespace KV {

class Tx {
public:
	virtual ~Tx() {}
	virtual void commit() = 0;
	virtual void rollback() = 0;
};

class Env {
protected:
	std::string dir_;

public:
	Env(const std::string & dir): dir_(dir) {
		boost::filesystem::create_directories(dir);
	}

	virtual ~Env() {}

	virtual int open(int flags, int mode) = 0;
	virtual Tx * transaction() = 0;
	const std::string & path() { return dir_; }
	virtual void set_errcall(void (*cb)(int level, const char * message, ...)) = 0;
};

class BEnv: public Env, DbEnv {

	static void error(const DbEnv * env, const char * p, const char * m)
	{
		const BEnv * benv = dynamic_cast < const BEnv * >(env);
		if (!benv) return;
		benv->log(5, "%s: %s", p, m);
	}

	static void dummy_log(int, const char * message, ...)
	{
	}

public:
	void (*log)(int level, const char * message, ...);

	BEnv(const std::string & dir, int gbytes = 1, int bytes = 0, int noc = 2): Env(dir), 
		DbEnv(0), log(dummy_log)
	{
		set_cachesize(gbytes, 0, 4);
	}

	virtual ~BEnv() {}

	int open(int flags, int mode) {
		return DbEnv::open(dir_.c_str(), flags, mode);
	}

	DbEnv * get() { return this; }
	Tx * transaction();

	void set_errcall(void (*cb)(int level, const char * message, ...)) {
		log = cb;
		DbEnv::set_errcall(error);
	}
};

class Iterator {
public:
	typedef boost::shared_ptr < unsigned char > value_type;

	virtual ~Iterator() {}
	virtual bool next(std::string & key, value_type & p, size_t & size) = 0;
	virtual void del() = 0;
};

class Db {
protected:
	Env & env_;
	std::string name_;

public:
	typedef boost::shared_ptr < unsigned char > value_type;

	enum {
		READ   = 1,
		WRITE  = 2,
		CREATE = 4,
	};

	Db(Env & env, const std::string & name): env_(env), name_(name) {}
	virtual ~Db() {}

	virtual int open(int flags /*READ|WRITE|CREATE*/) = 0;

	template < typename T > 
	int get(Tx * tx, const std::string & key, T & value) {
		value_type p;
		size_t size;
		int r = get(tx, key.c_str(), p, size);
		if (r != 0) {
			return r;
		}
		memcpy(&value, p.get(), size);
		return 0;
	}

	template < typename T > 
	int put(Tx * tx, const std::string & key, T & value) {
		return put(tx, key, &value, sizeof(value));
	}

	int get(Tx * tx, const std::string & key, value_type & p, size_t & size) {
		return get(tx, (void*)key.c_str(), key.length(), p, size);
	}

	int put(Tx * tx, const std::string & key, void * data, size_t len) {
		return put(tx, (void*)key.c_str(), key.length(), data, len);
	}

	template < typename T >
	int put(Tx * tx, T t, const std::string & value) {
		return put(tx, &t, sizeof(t), value);
	}

	template < typename T >
	int get(Tx * tx, T t, std::string & value) {
		return get(tx, &t, sizeof(t), value);
	}

	int get(Tx * tx, void * key, size_t klen, std::string & value) {
		value_type val;
		size_t size;
		int r = get(tx, key, klen, val, size);
		if (r != 0) {
			return r;
		}
		value.resize(size);
		memcpy(&value[0], val.get(), size);
		return r;
	}

	int put(Tx * tx, void * key, size_t klen, const std::string & value) {
		return put(tx, key, klen, (void*)&value[0], value.length());
	}

	virtual int get(Tx * tx, void * key, size_t klen, value_type & p, size_t & size) = 0;
	virtual int put(Tx * tx, void * key, size_t klen, void * data, size_t len) = 0;
	virtual int del(Tx * tx, void * key, size_t klen) = 0;
	virtual Iterator * begin(Tx * tx) = 0; 
};

class BDb: public KV::Db, public ::Db {
	BEnv & env_;
	int type_;

public:
	typedef unsigned (*hash_t)(DB *, const void * bytes, uint32_t length);

	enum DbType {
		BTREE = 0,
		HASH  = 1,
		RECNO = 2,
		QUEUE = 3,
	};

	BDb(BEnv & env, const std::string & name, DbType type): 
		KV::Db(env, name), ::Db(env_.get(), 0), env_(env), type_((int)type) 
	{
	}

	virtual ~BDb() {}

	int open(int flags /*READ|WRITE|CREATE*/);

	int get(Tx * tx, void * key, size_t klen, value_type & p, size_t & size);
	int put(Tx * tx, void * key, size_t klen, void * data, size_t len);
	int del(Tx * tx, void * key, size_t klen);

	Iterator * begin(Tx * tx);
};
}

#endif  /* KV_DB_H */

