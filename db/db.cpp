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
#include <iostream>
#include <db_cxx.h>
#include "db.h"

using namespace std;

class BTransaction: public KV::Tx
{
	DbEnv * env;
	DbTxn * tid;

public:
	BTransaction(KV::BEnv * e): env(e->get())
	{
		if (env->txn_begin(0, &tid, 0) != 0) {
			fprintf(stderr, "cannot create transaction \n");
		}
	}

	void commit()
	{
		if (tid->commit(0) != 0) {
			fprintf(stderr, "cannot commit transaction\n");
		}
	}

	void rollback()
	{
		if (tid->abort() != 0) {
			fprintf(stderr, "cannot rollback transaction\n");
		}
	}
};

KV::Tx * KV::BEnv::transaction()
{
	return new BTransaction(this);
}

int KV::BDb::open(int flags /*READ|WRITE|CREATE*/)
{
	try {
		DBTYPE t;
		switch (type_) {
		case BTREE:
			t = DB_BTREE;
			break;
		case HASH:
			t = DB_HASH;
			break;
		case RECNO:
			t = DB_RECNO;
			break;
		case QUEUE:
			t = DB_QUEUE;
			break;
		default:
			t= DB_BTREE;
			break;
		}

		int fl = 0;
		if (flags & CREATE) {
			fl |= DB_CREATE;
		}

		if (flags & WRITE) {
			; //TODO
		}

		if ( (flags & READ) && !(flags & WRITE) ) {
			fl |= DB_RDONLY; //TODO
		}

		return ::Db::open(0, name_.c_str(), 0, t, fl, 0644);
	} catch (const DbException & ex) {
		env_.log(1, "cannot open %s/%s: %s", env_.path().c_str(), name_.c_str(), ex.what());
	}

	return -1;
}

int KV::BDb::get(KV::Tx * tx, void * key, size_t klen, 
		KV::Db::value_type & p, size_t & size) 
{
	try {
		Dbt k(key, klen);
		Dbt val;
		val.set_flags(DB_DBT_MALLOC);

		if (::Db::get(0, &k, &val, 0) == 0) {
			p.reset((unsigned char*)val.get_data(), free);
			size = val.get_size();
			return 0;
		} else {
			return -1;
		}
	} catch (const DbException & ex) {
		env_.log(1, "error: %s", ex.what());
		return -1;
	}

	return -1;
}

int KV::BDb::del(KV::Tx * tx, void * key, size_t klen)
{
	try {
		Dbt k(key, klen);
		return ::Db::del(0, &k, 0);
	} catch (const DbException & ex) {
		env_.log(1, "error: %s", ex.what());
		return -1;
	}

	return -1;
}

int KV::BDb::put(KV::Tx * tx, void * key, size_t klen, void * data, size_t len) {
	try {
		Dbt k(key, klen);
		Dbt v(data, len);
		int flags = 0;
		if (type_ == QUEUE) {
//			k.set_flags(DB_DBT_USERMEM);
//			k.set_ulen(klen);
			flags |= DB_APPEND;
		}

		k.set_flags(DB_DBT_USERMEM);
		k.set_ulen(klen);
		v.set_flags(DB_DBT_USERMEM);
		v.set_ulen(len);

		return ::Db::put(0, &k, &v, flags);
	} catch (const DbException & ex) {
		env_.log(1, "error: %s", ex.what());
		return -1;
	}

	return -1;
}

class BIterator: public KV::Iterator {
public:
	Dbc *cursor;
	int type;

	BIterator(int t): cursor(0), type(t) {}

	virtual ~BIterator() { if (cursor) cursor->close(); }

	bool next(std::string & key, KV::Db::value_type & p, size_t & size) {
		Dbt k;
		Dbt v;
		v.set_flags(DB_DBT_MALLOC);
		int r = cursor->get(&k, &v, DB_NEXT);
		if (r == 0) {
			key = (char*)k.get_data();
			p.reset((unsigned char*)v.get_data(), free);
			size = v.get_size();
			return true;
		}

		return false;
	}

	void del() {
		cursor->del(0);
	}
};

KV::Iterator * KV::BDb::begin(KV::Tx * tx) {
	BIterator * it = new BIterator(type_);
	::Db::cursor(0, &it->cursor, 0);

	return it;
}

