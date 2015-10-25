#ifndef FTC_JTAG_INTERNAL_H
#define FTC_JTAG_INTERNAL_H

#define MAX_NUM_DEVICES 50

#ifndef _WIN32
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>

typedef pthread_mutex_t CRITICAL_SECTION;
#define InitializeCriticalSection(pm) pthread_mutex_init(pm, NULL)
#define DeleteCriticalSection(pm) pthread_mutex_destroy(pm)
#define EnterCriticalSection(pm) pthread_mutex_lock(pm)
#define LeaveCriticalSection(pm) pthread_mutex_unlock(pm)

#define Sleep(ms) usleep((ms) * 1000)

#define GetCurrentProcessId getpid

#define GetLocalTime(tp) gettimeofday(tp, NULL)

char* strupr( char* );
#endif

#endif // !FTC_JTAG_INTERNAL_H
