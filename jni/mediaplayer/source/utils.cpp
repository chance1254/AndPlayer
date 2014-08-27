#include <stddef.h>
#include <stdint.h>
#include <time.h>
#if HAVE_GETTIMEOFDAY
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
int64_t av_gettime(void)
{
#if HAVE_GETTIMEOFDAY
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
#elif HAVE_GETSYSTEMTIMEASFILETIME
	FILETIME ft;
	int64_t t;
	GetSystemTimeAsFileTime(&ft);
	t = (int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
	return t / 10 - 11644473600000000; /* Jan 1, 1601 */
#else
	return -1;
#endif
}
