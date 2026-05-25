#include "../doomdef.h"
#include "../doomtype.h"
#include "../d_main.h"
#include "../keys.h"
#include "../m_misc.h"
#include "../m_argv.h"
#include "../i_system.h"
#include "../i_video.h"
#include "../i_time.h"
#include "../i_threads.h"

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <ncurses.h>

FILE *logstream = NULL;

UINT8 graphics_started = 0;

UINT8 keyboard_started = 0;

#ifdef __linux__
#define MEMINFO_FILE "/proc/meminfo"
#define MEMTOTAL "MemTotal:"
#define MEMAVAILABLE "MemAvailable:"
#define MEMFREE "MemFree:"
#define CACHED "Cached:"
#define BUFFERS "Buffers:"
#define SHMEM "Shmem:"

/* Parse the contents of /proc/meminfo (in buf), return value of "name"
 * (example: MemTotal) */
static long get_entry(const char* name, const char* buf)
{
	long val;
	char* hit = strstr(buf, name);
	if (hit == NULL) {
		return -1;
	}

	errno = 0;
	val = strtol(hit + strlen(name), NULL, 10);
	if (errno != 0) {
		CONS_Alert(CONS_ERROR, M_GetText("get_entry: strtol() failed: %s\n"), strerror(errno));
		return -1;
	}
	return val;
}
#endif

size_t I_GetFreeMem(size_t *total)
{
#ifdef FREEBSD
	u_int v_free_count, v_page_size, v_page_count;
	size_t size = sizeof(v_free_count);
	sysctlbyname("vm.stats.vm.v_free_count", &v_free_count, &size, NULL, 0);
	size = sizeof(v_page_size);
	sysctlbyname("vm.stats.vm.v_page_size", &v_page_size, &size, NULL, 0);
	size = sizeof(v_page_count);
	sysctlbyname("vm.stats.vm.v_page_count", &v_page_count, &size, NULL, 0);

	if (total)
		*total = v_page_count * v_page_size;
	return v_free_count * v_page_size;
#elif defined (SOLARIS)
	/* Just guess */
	if (total)
		*total = 32 << 20;
	return 32 << 20;
#elif defined (_WIN32)
	MEMORYSTATUS info;

	info.dwLength = sizeof (MEMORYSTATUS);
	GlobalMemoryStatus( &info );
	if (total)
		*total = (size_t)info.dwTotalPhys;
	return (size_t)info.dwAvailPhys;
#elif defined (__linux__)
	/* Linux */
	char buf[1024];
	char *memTag;
	size_t freeKBytes;
	size_t totalKBytes;
	INT32 n;
	INT32 meminfo_fd = -1;
	long Cached;
	long MemFree;
	long Buffers;
	long Shmem;
	long MemAvailable = -1;

	meminfo_fd = open(MEMINFO_FILE, O_RDONLY);
	n = read(meminfo_fd, buf, 1023);
	close(meminfo_fd);

	if (n < 0)
	{
		// Error
		if (total)
			*total = 0L;
		return 0;
	}

	buf[n] = '\0';
	if ((memTag = strstr(buf, MEMTOTAL)) == NULL)
	{
		// Error
		if (total)
			*total = 0L;
		return 0;
	}

	memTag += sizeof (MEMTOTAL);
	totalKBytes = (size_t)atoi(memTag);

	if ((memTag = strstr(buf, MEMAVAILABLE)) == NULL)
	{
		Cached = get_entry(CACHED, buf);
		MemFree = get_entry(MEMFREE, buf);
		Buffers = get_entry(BUFFERS, buf);
		Shmem = get_entry(SHMEM, buf);
		MemAvailable = Cached + MemFree + Buffers - Shmem;

		if (MemAvailable == -1)
		{
			// Error
			if (total)
				*total = 0L;
			return 0;
		}
		freeKBytes = MemAvailable;
	}
	else
	{
		memTag += sizeof (MEMAVAILABLE);
		freeKBytes = atoi(memTag);
	}

	if (total)
		*total = totalKBytes << 10;
	return freeKBytes << 10;
#else
	// Guess 48 MB.
	if (total)
		*total = 48<<20;
	return 48<<20;
#endif
}

void I_Sleep(UINT32 ms)
{
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
	struct timespec ts = {
		.tv_sec = ms / 1000,
		.tv_nsec = ms % 1000 * 1000000,
	};
	int status;
	do status = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts);
	while (status == EINTR);
#elif defined (_WIN32)
	Sleep(ms);
#else
	(void)ms;
#warning No sleep function for this system!
#endif
}

void I_SleepDuration(precise_t duration)
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__HAIKU__)
	UINT64 precision = I_GetPrecisePrecision();
	precise_t dest = I_GetPreciseTime() + duration;
	precise_t slack = (precision / 5000); // 0.2 ms slack
	if (duration > slack)
	{
		duration -= slack;
		struct timespec ts = {
			.tv_sec = duration / precision,
			.tv_nsec = duration * 1000000000 / precision % 1000000000,
		};
		int status;
		do status = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts);
		while (status == EINTR);
	}

	// busy-wait the rest
	while (((INT64)dest - (INT64)I_GetPreciseTime()) > 0);
#else
	UINT64 precision = I_GetPrecisePrecision();
	INT32 sleepvalue = cv_sleep.value;
	UINT64 delaygranularity;
	precise_t cur;
	precise_t dest;

	{
		double gran = round(((double)(precision / 1000) * sleepvalue * MIN_SLEEP_DURATION_MS));
		delaygranularity = (UINT64)gran;
	}

	cur = I_GetPreciseTime();
	dest = cur + duration;

	// the reason this is not dest > cur is because the precise counter may wrap
	// two's complement arithmetic is our friend here, though!
	// e.g. cur 0xFFFFFFFFFFFFFFFE = -2, dest 0x0000000000000001 = 1
	// 0x0000000000000001 - 0xFFFFFFFFFFFFFFFE = 3
	while ((INT64)(dest - cur) > 0)
	{
		// If our cv_sleep value exceeds the remaining sleep duration, use the
		// hard sleep function.
		if (sleepvalue > 0 && (dest - cur) > delaygranularity)
		{
			I_Sleep(sleepvalue);
		}

		// Otherwise, this is a spinloop.

		cur = I_GetPreciseTime();
	}
#endif
}

precise_t I_GetPreciseTime(void)
{
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (precise_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
#elif defined (_WIN32)
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return (precise_t)counter.QuadPart;
#else
	return 0;
#endif
}

UINT64 I_GetPrecisePrecision(void)
{
    return 1000000;
}

static boolean key_down[512];
static int key_hold[512];

// i prefer using multiple short functions instead of one long one
static int mapKey(int ch)
{
    switch (ch)
    {
        case 'w': return 'w';
        case 'a': return 'a';
        case 's': return 's';
        case 'd': return 'd';

        case KEY_UP:    return KEY_UPARROW;
        case KEY_DOWN:  return KEY_DOWNARROW;
        case KEY_LEFT:  return KEY_LEFTARROW;
        case KEY_RIGHT: return KEY_RIGHTARROW;

        case 10:
        case 13:
        case KEY_ENTER: return KEY_ENTER;

        case ' ': return KEY_SPACE;
        case 27:  return KEY_ESCAPE;
    }
    return 0;
}

void I_GetEvent(void)
{
    int ch;

    while ((ch = getch()) != ERR)
    {
        int key = mapKey(ch);
        if (!key) continue;

        key_hold[key] = 15;

        event_t ev;
        ev.type = ev_keydown;
        ev.repeated = false;
        ev.key = key;
        D_PostEvent(&ev);
    }
}

void I_OsPolling(void)
{
    I_GetEvent();

    for (int i = 0; i < 512; i++)
    {
        if (key_hold[i] > 0)
        {
            key_hold[i]--;
        }
        else if (key_hold[i] == 0)
        {
            key_hold[i] = -1;

            event_t ev;
            ev.type = ev_keyup;
            ev.repeated = false;
            ev.key = i;
            D_PostEvent(&ev);
        }
    }
}

/**	\brief empty ticcmd for player 1
*/
static ticcmd_t emptycmd;

ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

/**	\brief empty ticcmd for player 2
*/
static ticcmd_t emptycmd2;

ticcmd_t *I_BaseTiccmd2(void)
{
	return &emptycmd2;
}

void I_Quit(void) {}

void I_Error(const char *error, ...)
{
    // Format the error string
    va_list args;
    va_start(args, error);
    va_end(args);

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), error, args);
    printf("SRB2 "VERSIONSTRING" Error: %s", buffer);

    M_SaveConfig(NULL);
    I_ShutdownGraphics();
    I_ShutdownSound();
    I_ShutdownMusic();
    I_ShutdownSystem();
}

void I_Tactile(FFType Type, const JoyFF_t *Effect)
{
	(void)Type;
	(void)Effect;
}

void I_Tactile2(FFType Type, const JoyFF_t *Effect)
{
	(void)Type;
	(void)Effect;
}

void I_JoyScale(void){}

void I_JoyScale2(void){}

void I_InitJoystick(void){}

void I_InitJoystick2(void){}

INT32 I_NumJoys(void)
{
	return 0;
}

const char *I_GetJoyName(INT32 joyindex)
{
	(void)joyindex;
	return NULL;
}

#ifndef NOMUMBLE
void I_UpdateMumble(const mobj_t *mobj, const listener_t listener)
{
	(void)mobj;
	(void)listener;
}
#endif

void I_OutputMsg(const char *error, ...)
{
	va_list args;
	va_start(args, error);
	va_end(args);

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), error, args);
}

void I_StartupMouse(void){}

void I_StartupMouse2(void){}

INT32 I_GetKey(void)
{
	return 0;
}

void I_StartupTimer(void){}

void I_AddExitFunc(void (*func)())
{
	(void)func;
}

void I_RemoveExitFunc(void (*func)())
{
	(void)func;
}

INT32 I_StartupSystem(void)
{
	I_start_threads();
	return 0;
}

void I_ShutdownSystem(void){}

void I_GetDiskFreeSpace(INT64* freespace)
{
	*freespace = 0;
}

char *I_GetUserName(void)
{
	return NULL;
}

INT32 I_mkdir(const char *dirname, INT32 unixright)
{
	(void)dirname;
	(void)unixright;
	return -1;
}

const CPUInfoFlags *I_CPUInfo(void)
{
	return NULL;
}

const char *I_LocateWad(void)
{
	return I_GetEnv("SRB2WADDIR");
}

void I_GetJoystickEvents(void){}

void I_GetJoystick2Events(void){}

void I_GetMouseEvents(void){}

void I_UpdateMouseGrab(void){}

char *I_GetEnv(const char *name)
{
	return getenv(name);
}

INT32 I_PutEnv(char *variable)
{
	(void)variable;
	return -1;
}

INT32 I_ClipboardCopy(const char *data, size_t size)
{
	(void)data;
	(void)size;
	return -1;
}

const char *I_ClipboardPaste(void)
{
	return NULL;
}

size_t I_GetRandomBytes(char *destination, size_t amount)
{
	(void)destination;
	(void)amount;
	return 0;
}

void I_RegisterSysCommands(void){}

void I_GetCursorPosition(INT32 *x, INT32 *y)
{
	(void)x;
	(void)y;
}

const char *I_GetSysName(void)
{
	return NULL;
}

void I_SetTextInputMode(boolean active)
{
	(void)active;
}

boolean I_GetTextInputMode(void)
{
	return false;
}

#include "../sdl/dosstr.c"

