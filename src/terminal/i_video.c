#include "../doomdef.h"
#include "../doomstat.h"
#include "../v_video.h"
#include "../command.h"
#include "../screen.h"
#include "../i_system.h"
#include "../i_video.h"

rendermode_t rendermode = render_soft;
rendermode_t chosenrendermode = render_soft;

boolean allow_fullscreen = false;

consvar_t cv_vidwait = CVAR_INIT ("vid_wait", "On", CV_SAVE, CV_OnOff, NULL);

static RGBA_t term_palette[256];

void I_StartupGraphics(void)
{
	CV_RegisterVar (&cv_vidwait);
	printf("\x1b[?1049h");
	printf("\x1b[?25l");
	printf("\x1b[H");
	fflush(stdout);
	VID_SetMode(1);
	graphics_started = true;
}

void I_ShutdownGraphics(void)
{
	printf("\x1b[?25h");
	printf("\x1b[0m");
}

void VID_StartupOpenGL(void){}

void I_SetPalette(RGBA_t *palette)
{
	memcpy(term_palette, palette, sizeof(term_palette));
}

INT32 VID_NumModes(void)
{
	return 0;
}

INT32 VID_GetModeForSize(INT32 w, INT32 h)
{
	(void)w;
	(void)h;
	return 0;
}

void VID_PrepareModeList(void){}

void I_SetResolution(INT32 width, INT32 height) {}

INT32 VID_SetMode(INT32 modenum)
{
	(void)modenum;

	vid.width = 320;
	vid.height = 200;
	vid.rowbytes = vid.width;
	vid.bpp = 1;
	vid.modenum = 17;

	vid.buffer = calloc(1, vid.width * vid.height);

	return 0;
}

boolean VID_CheckRenderer(void)
{
	return false;
}

void VID_CheckGLLoaded(rendermode_t oldrender)
{
	(void)oldrender;
}

const char *VID_GetModeName(INT32 modenum)
{
	(void)modenum;
	return "small";
}

UINT32 I_GetRefreshRate(void) { return 35; }

void I_UpdateNoBlit(void){}

// really stupid 2 pixel thing
static char termbuf[8 * 1024 * 1024];

void I_FinishUpdate(void)
{
	char *out = termbuf;

	out += sprintf(out, "\x1b[H");

	SCR_CalculateFPS();

	if (marathonmode)
		SCR_DisplayMarathonInfo();

	if (cv_closedcaptioning.value)
		SCR_ClosedCaptions();

	if (cv_ticrate.value)
		SCR_DisplayTicRate();

	for (int y = 0; y < vid.height - 1; y += 2)
	{
		for (int x = 0; x < vid.width; x++)
		{
			UINT8 p1 = screens[0][y * vid.width + x];
			UINT8 p2 = screens[0][(y + 1) * vid.width + x];

			RGBA_t c1 = term_palette[p1];
			RGBA_t c2 = term_palette[p2];

			out += sprintf(out,
				"\x1b[38;2;%d;%d;%dm"
				"\x1b[48;2;%d;%d;%dm▀",
				c1.s.red, c1.s.green, c1.s.blue,
				c2.s.red, c2.s.green, c2.s.blue);
		}

		*out++ = '\n';
		*out++ = '\x1b';
		*out++ = '[';
		*out++ = '0';
		*out++ = 'm';
	}

	fwrite(termbuf, 1, out - termbuf, stdout);
	fflush(stdout);
}

void I_UpdateNoVsync(void) {}

void I_WaitVBL(INT32 count)
{
	(void)count;
}

void I_ReadScreen(UINT8 *scr)
{
	(void)scr;
}

void I_BeginRead(void){}

void I_EndRead(void){}

