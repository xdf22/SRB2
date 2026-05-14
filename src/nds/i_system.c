#include <nds.h>
#include <fat.h>

#include "../doomdef.h"
#include "../netcode/d_clisrv.h"
#include "../d_main.h"
#include "../filesrch.h"
#include "../m_misc.h"
#include "../i_system.h"
#include "../i_video.h"
#include "../i_sound.h"
#include "../i_joy.h"
#include "../z_zone.h"

FILE *logstream = NULL;

UINT8 keyboard_started = 0;

static volatile tic_t ticcount;

#define timers2ms(tlow,thigh) ((tlow>>5)+(thigh<<11))

// Handy DSdev.org timer functions
u32 GetTicks(void)
{
	return timers2ms(TIMER0_DATA, TIMER1_DATA);
} 

void Pause(u32 ms)
{
	u32 now;
	now=timers2ms(TIMER0_DATA, TIMER1_DATA);
	while((u32)timers2ms(TIMER0_DATA, TIMER1_DATA)<now+ms);
}

void I_Sleep(UINT32 ms)
{
	Pause(ms/1000);
}

int ms_to_next_tick;

int TimeFunction(int requested_frequency)
{
	static UINT64 basetime = 0;
		   UINT64 ticks = GetTicks();

	if (!basetime)
		basetime = ticks;

	ticks -= basetime;

	ticks = (ticks*requested_frequency);

	ticks = (ticks/1000);

	return ticks;
}

int I_GetTimeMicros(void)
{
	return TimeFunction(1000000);
}

extern char __ewram_start;
extern char __ewram_end;

size_t I_GetFreeMem(size_t *total)
{
	if (total)
		*total = 12<<20;
	return 12<<20;
}

void I_GetEvent(void)
{
    static touchPosition last_touch_position;
	scanKeys();
	u16 keys = keysDown();
	
	event_t e_w;

	if (keys & KEY_A) {
		event_t event;
		event.type = ev_keydown;
		event.key = KEY_ENTER;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_B) {
		event_t event;
		event.type = ev_keydown;
		event.key = KEY_LSHIFT;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_START) {
		event_t event;
		event.type = ev_keydown;
		event.key = KEY_ESCAPE;
		D_PostEvent(&event);
	}

	if (keys & KEY_UP) {
		event_t event;
		event.type = ev_keydown;
		event.key = KEY_UPARROW;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_DOWN) {
		event_t event;
		event.type = ev_keydown;
		event.key = KEY_DOWNARROW;
		D_PostEvent(&event);
	}

	if (keys & KEY_LEFT) {
		event_t event;
		event.type = ev_keydown;
		event.key = KEY_LEFTARROW;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_RIGHT) {
		event_t event;
		event.type = ev_keydown;
		event.key = KEY_RIGHTARROW;
		D_PostEvent(&event);
	}

    // lowk took this from srb2_3ds
	if(keysHeld() & KEY_TOUCH) {
        event_t event;
		touchPosition current_touch_position;
		touchRead(&current_touch_position);
		if (!(keysDown() & KEY_TOUCH)) {
			event.type = ev_mouse;
			event.key = 0;
			event.x = current_touch_position.px - last_touch_position.px;
			event.y = current_touch_position.py - last_touch_position.py;
			D_PostEvent(&event);
		}
		last_touch_position = current_touch_position;
	}
	
	keys = keysUp();
	
	if (keys & KEY_A) {
		event_t event;
		event.type = ev_keyup;
		event.key = KEY_ENTER;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_B) {
		event_t event;
		event.type = ev_keyup;
		event.key = KEY_LSHIFT;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_START) {
		event_t event;
		event.type = ev_keyup;
		event.key = KEY_ESCAPE;
		D_PostEvent(&event);
	}

	if (keys & KEY_UP) {
		event_t event;
		event.type = ev_keyup;
		event.key = KEY_UPARROW;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_DOWN) {
		event_t event;
		event.type = ev_keyup;
		event.key = KEY_DOWNARROW;
		D_PostEvent(&event);
	}

	if (keys & KEY_LEFT) {
		event_t event;
		event.type = ev_keyup;
		event.key = KEY_LEFTARROW;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_RIGHT) {
		event_t event;
		event.type = ev_keyup;
		event.key = KEY_RIGHTARROW;
		D_PostEvent(&event);
	}

	// keyboard
	int16_t c = keyboardUpdate();
	if (c != -1)
	{
		event_t event;

		// backspace
		if (c == '\b')
		{
			event.type = ev_keydown;
			event.key = KEY_BACKSPACE;
			D_PostEvent(&event);

			event.type = ev_keyup;
			event.key = KEY_BACKSPACE;
			D_PostEvent(&event);
		}
		else if (c >= 32)
		{
			// key down
			event.type = ev_keydown;
			event.key = c;
			D_PostEvent(&event);

			// key up
			event.type = ev_keyup;
			event.key = c;
			D_PostEvent(&event);
		}
	}
}

void I_OsPolling(void)
{
	I_GetEvent();
}

void I_SleepDuration(precise_t duration)
{
	(void)duration;
}

precise_t I_GetPreciseTime(void)
{
    return (precise_t)GetTicks();
}

UINT64 I_GetPrecisePrecision(void)
{
	return 1000000;
}

ticcmd_t *I_BaseTiccmd(void)
{
	return NULL;
}

ticcmd_t *I_BaseTiccmd2(void)
{
	return NULL;
}

void I_Quit(void)
{
	exit(0);
}

void I_Error(const char *error, ...)
{
    // Format the error string
    va_list args;
    va_start(args, error);

    int len = vsnprintf(NULL, 0, error, args);
    va_end(args);

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), error, args);
    printf("SRB2 "VERSIONSTRING" Error: %s", buffer);


    M_SaveConfig(NULL);
    D_QuitNetGame();
    I_ShutdownGraphics();
    I_ShutdownSound();
    I_ShutdownMusic();
    I_ShutdownSystem();

    while(1) 
	{
        swiWaitForVBlank();
    }
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

	int len = vsnprintf(NULL, 0, error, args);
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
	return -1;
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
    chdir("nitro:/");
	return "nitro:/";
}

void I_GetJoystickEvents(void){}

void I_GetJoystick2Events(void){}

void I_GetMouseEvents(void){}

void I_UpdateMouseGrab(void){}

char *I_GetEnv(const char *name)
{
	(void)name;
	return NULL;
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

void I_lock_mutex(void **) {}
void I_unlock_mutex (void *) {}

int I_can_thread(...)
{
    return false;
}

void I_spawn_thread (const char *name, I_thread_fn, void *userdata) {}
void I_hold_cond(void **cond_anchor, void *mutex_id) {}
void I_wake_all_cond(void **anchor) {}

#include "../sdl/dosstr.c"

