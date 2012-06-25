#ifndef STORE_H
#define STORE_H

#include <db_cxx.h>

template < typename K, typename V >
class Store: public Db
{
public:
	Store(DbEnv *env, uint32_t flags = 0): Db(env, flags)
	{
	}

	int put(const K & k, const V & v, uint32_t flags = 0)
	{
	}

	int get(const K & k, V & v, uint32_t flags = 0)
	{
	}
};

#endif /*STORE_H*/
