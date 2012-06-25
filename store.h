#ifndef STORE_H
#define STORE_H

#include <string.h>

#include <string>
#include <stdexcept>

#include <boost/shared_ptr.hpp>

#include <db_cxx.h>

namespace detail
{
	template < typename T >
	class Key
	{
	public:
		static Dbt dbt4put(const T & v)
		{
			Dbt dbt((void*)&v, sizeof(v));
			dbt.set_ulen(sizeof(v));
			dbt.set_flags(DB_DBT_USERMEM);
			return dbt;			
		}

		static Dbt dbt4get(const T & k)
		{
			Dbt dbt((void*)&k, sizeof(k));
			return dbt;
		}

		void dbt2val(T & v, Dbt & dbt) {}
	};

	template <>
	class Key<std::string>
	{
	public:
		Dbt dbt4put(const std::string & k)
		{
			Dbt dbt((void*)k.c_str(), k.length());
			dbt.set_flags(DB_DBT_MALLOC);
			return dbt;
		}

		Dbt dbt4get(const std::string & k)
		{
			Dbt dbt((void*)k.c_str(), k.length());
			dbt.set_ulen(k.length());
			dbt.set_flags(DB_DBT_USERMEM);
			return dbt;
		}

		void dbt2val(std::string & v, Dbt & dbt) {
			v.clear();
			v.resize(dbt.get_size());
			memcpy((void*)v.c_str(), dbt.get_data(), dbt.get_size());
			free(dbt.get_data());
		}
	};

	template < typename T >
	class Val
	{
	public:
		Dbt dbt4put(const T & v)
		{
			Dbt dbt((void*)&v, sizeof(v));
			dbt.set_ulen(sizeof(v));
			dbt.set_flags(DB_DBT_USERMEM);
			return dbt;
		}

		Dbt dbt4get(const T & v) {
			Dbt dbt((void*)&v, sizeof(v));
			dbt.set_ulen(sizeof(v));
			dbt.set_flags(DB_DBT_USERMEM);
			return dbt;
		}

		void dbt2val(T & v, Dbt & dbt) {}
	};

	template <>
	class Val<std::string>
	{
	public:
		Dbt dbt4put(const std::string & k)
		{
			Dbt dbt((void*)k.c_str(), k.length());
			dbt.set_ulen(k.length());
			dbt.set_flags(DB_DBT_USERMEM);
			return dbt;
		}

		Dbt dbt4get(const std::string & v) {
			Dbt dbt;
			dbt.set_flags(DB_DBT_MALLOC);
			return dbt;
		}

		void dbt2val(std::string & v, Dbt & dbt) {
			v.clear();
			v.resize(dbt.get_size());
			memcpy((void*)v.c_str(), dbt.get_data(), dbt.get_size());
			free(dbt.get_data());
		}
	};
}

template < typename K, typename V >
class Store: public Db
{

	detail::Key<K> kt_;
	detail::Val<V> vt_;

public:
	class Iterator {
		Store<K, V> * parent_;
		Dbc * cursor_;

	public:
		Iterator(Store<K, V> * db): parent_(db), cursor_(0)
		{
			if (parent_->cursor(0, &cursor_, 0) != 0) {
				throw std::runtime_error("cannot open cursor");
			}
		}

		~Iterator()
		{
			cursor_->close();
		}

		bool next(K & k, V & v, uint32_t flags = DB_NEXT)
		{
			Dbt key = parent_->kt_.dbt4put(k);
			Dbt val = parent_->vt_.dbt4put(v);

			int r;
			if ((r = cursor_->get(&key, &val, flags)) == 0) {
				parent_->kt_.dbt2val(k, val);
				parent_->vt_.dbt2val(v, val);
				return true;
			}
			return false;
		}

		int del(uint32_t flags = 0)
		{
			return cursor_->del(flags);
		}
	};

	typedef boost::shared_ptr<Iterator> iterator;
	friend class Iterator;

	Store(DbEnv *env, uint32_t flags = 0): Db(env, flags)
	{
	}

	int put(const K & k, const V & v, uint32_t flags = 0)
	{
		Dbt key = kt_.dbt4get(k);
		Dbt val = vt_.dbt4put(v);
		return Db::put(0, &key, &val, flags);
	}

	int get(const K & k, V & v, uint32_t flags = 0)
	{
		Dbt key = kt_.dbt4get(k);
		Dbt val = vt_.dbt4get(v);
		int r = 0;
		if ((r = Db::get(0, &key, &val, flags)) == 0) {
			vt_.dbt2val(v, val);
		}
		return r;
	}

	int exists(const K & k, uint32_t flags = 0)
	{
		Dbt key = kt_.dbt4get(k);
		return Db::exists(0, &key, flags);
	}

	boost::shared_ptr <Iterator> begin()
	{
		boost::shared_ptr <Iterator> it(new Iterator(this));
		return it;
	}
};

#endif /*STORE_H*/
