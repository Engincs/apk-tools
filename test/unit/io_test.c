#include <dirent.h>
#include <dlfcn.h>

#include "apk_test.h"
#include "apk_io.h"
#include "apk_balloc.h"
#include "apk_print.h"

#define MOCKFD 9999

static int (*next_openat)(int, const char *, int);
static int (*next_dup)(int);

static void __attribute((constructor)) resolver(void)
{
	next_openat = dlsym(RTLD_NEXT, "openat");
	next_dup = dlsym(RTLD_NEXT, "dup");
}

/* assume shared libapk.so, and override the symbols it depends on */
int openat(int atfd, const char *filename, int flags, ...)
{
	if (atfd != MOCKFD) return next_openat(atfd, filename, flags);
	return MOCKFD;
}

int dup(int fd)
{
	return fd == MOCKFD ? MOCKFD : next_dup(fd);
}

DIR *fdopendir(int dirfd)
{
	assert_int_equal(MOCKFD, dirfd);
	expect_value(closedir, dir, 1);
	return (DIR*) 1;
}

int closedir(DIR *dir)
{
	check_expected(dir);
	return 0;
}

struct dirent *readdir(DIR *dir)
{
	static struct dirent de;
	const char *entry = mock_type(const char *);
	if (!entry) return NULL;
	memset(&de, 0, sizeof de);
	strcpy(de.d_name, entry);
	return &de;
}

static int assert_entry(void *ctx, int dirfd, const char *path, const char *entry)
{
	assert_string_equal(entry, mock_type(const char*));
	return 0;
}

static int assert_path_entry(void *ctx, int dirfd, const char *path, const char *entry)
{
	assert_string_equal(path, mock_type(const char*));
	assert_string_equal(entry, mock_type(const char*));
	return 0;
}

APK_TEST(io_foreach_file_basic) {
	will_return(readdir, "one");
	will_return(readdir, "two");
	will_return(readdir, "three");
	will_return(readdir, NULL);

	will_return(assert_entry, "one");
	will_return(assert_entry, "two");
	will_return(assert_entry, "three");

	assert_int_equal(0, apk_dir_foreach_file(MOCKFD, "path", assert_entry, NULL, NULL));
}

APK_TEST(io_foreach_file_filter) {
	will_return(readdir, "one");
	will_return(readdir, ".two");
	will_return(readdir, "three");
	will_return(readdir, NULL);

	will_return(assert_entry, "one");
	will_return(assert_entry, "three");

	assert_int_equal(0, apk_dir_foreach_file(MOCKFD, "path", assert_entry, NULL, apk_filename_is_hidden));
}

APK_TEST(io_foreach_file_sorted) {
	will_return(readdir, "one");
	will_return(readdir, "two");
	will_return(readdir, "three");
	will_return(readdir, "four");
	will_return(readdir, NULL);

	will_return(assert_entry, "four");
	will_return(assert_entry, "one");
	will_return(assert_entry, "three");
	will_return(assert_entry, "two");

	assert_int_equal(0, apk_dir_foreach_file_sorted(MOCKFD, "path", assert_entry, NULL, apk_filename_is_hidden));
}

APK_TEST(io_foreach_config_file) {
	will_return(readdir, "1-one");
	will_return(readdir, "2-two");
	will_return(readdir, "4-four");
	will_return(readdir, NULL);

	will_return(readdir, "2-two");
	will_return(readdir, "3-three");
	will_return(readdir, "4-four");
	will_return(readdir, NULL);

	will_return(assert_path_entry, "a");
	will_return(assert_path_entry, "1-one");
	will_return(assert_path_entry, "a");
	will_return(assert_path_entry, "2-two");
	will_return(assert_path_entry, "b");
	will_return(assert_path_entry, "3-three");
	will_return(assert_path_entry, "a");
	will_return(assert_path_entry, "4-four");

	assert_int_equal(0, apk_dir_foreach_config_file(MOCKFD, assert_path_entry, NULL, apk_filename_is_hidden, "a", "b", NULL));
}
