#ifdef HAVE_QDBM
#include <depot.h>
#include <string.h>
#include "tool_db.h"


int QDb::open(int flags /*READ|WRITE|CREATE*/) {
	int fl = 0;

	if (flags & CREATE) {
		fl |= DP_OCREAT;
	}

	if (flags & WRITE) {
		fl |= DP_OWRITER; //TODO
	}

	if (flags & READ) {
		fl |= DP_OREADER;
	}
 
	depot_ = dpopen((env_.path() + "/" + name_).c_str(), fl, -1);
	
	if (depot_ == 0) {
		return -1;
	}
	
	return 0;
}

int QDb::get(void * key, size_t klen, boost::shared_ptr < uchar > & p, size_t & size) {
	if (!depot_) {
		return -1;
	}

	char * val = dpget(depot_, (char*)key, klen, 0, -1, NULL);

	if (!val) {
		return -1;
	}

	size = strlen(val);
	p.reset((uchar*)val, free);

	return 0;
}

int QDb::put(void * key, size_t klen, void * data, size_t len) {
	if (!depot_) {
		return -1;
	}

	if (!dpput(depot_, (char*)key, klen, (char*)data, len, DP_DOVER)) {
		return -1;
	}

	return 0;
}

class QDbIterator: public DbIterator {
public:
	DEPOT * depot_;

	QDbIterator(DEPOT * depot): depot_(depot) {
		dpiterinit(depot_);
	}

	virtual ~QDbIterator() {}

	bool next(std::string & k, boost::shared_ptr < uchar > & p, size_t & size) {
		char * key, * val;
		if ((key = dpiternext(depot_, NULL))) {
			if ((val = dpget(depot_, key, -1, 0, -1, NULL))) {
				p.reset((uchar*)val, free);
				k = key;
				size = strlen(val);
				return true;
			} else {
				free(key);
				return false;
			}
		} else {
			return false;
		}
	}
};

DbIterator * QDb::begin()
{
	if (!depot_) return 0;

	QDbIterator * it = new QDbIterator(depot_);
	
	return it;
}
#endif /*HAVE_QDBM*/

