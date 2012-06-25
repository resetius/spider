#include <string.h>
#include <boost/filesystem.hpp>

#include "links_db.h"
#include "logger.h"

using namespace std;

static const string path = "test_state";  

void test_queue()
{
	boost::filesystem::remove_all(path);

	LinksDb ldb(path);
	ldb.push_link(1);
	ldb.push_link(2);
	ldb.push_link(3);
	ldb.push_link(4);
	ldb.push_link(5);

	if (ldb.pop_link() != 1) {
		fprintf(stderr, "failed (1)");
		exit(1);
	}
	if (ldb.pop_link() != 2) {
		fprintf(stderr, "failed (2)");
		exit(1);
	}
	if (ldb.pop_link() != 3) {
		fprintf(stderr, "failed (3)");
		exit(1);
	}
	if (ldb.pop_link() != 4) {
		fprintf(stderr, "failed (4)");
		exit(1);
	}
	if (ldb.pop_link() != 5) {
		fprintf(stderr, "failed (5)");
		exit(1);
	}
}

void test_flags()
{
	boost::filesystem::remove_all(path);

	LinksDb ldb(path);

	uint64_t id1 = ldb.id("url1");
	uint64_t id2 = ldb.id("url2");
	if (id1 == -1 || id2 == -1) {
		fprintf(stderr, "failed (1)");
		exit(1);
	}

	ldb.mark_downloading(id1);
	if (ldb.is_downloading(id1) == false) {
		fprintf(stderr, "failed (2)");
		exit(1);
	}
	if (ldb.is_downloading(id2) == true) {
		fprintf(stderr, "failed (3)");
		exit(1);
	}
	ldb.mark_downloaded(id1);
	if (ldb.is_downloaded(id1) == false) {
		fprintf(stderr, "failed (4)");
		exit(1);
	}	
	if (ldb.is_downloading(id1) == true) {
		fprintf(stderr, "failed (2)");
		exit(1);
	}
}

int main(int argc, char ** argv)
{
	if (argc < 2) {
		return -1;
	}
	mylog_init(11, "test_spider.log");

	if (!strcmp(argv[1], "queue")) {
		test_queue();
	} else if (!strcmp(argv[1], "flags")) {
		test_flags();
	}

	mylog_close();

	return 0;
}
