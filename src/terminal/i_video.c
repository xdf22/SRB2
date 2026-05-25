#include "../doomdef.h"
#include "../doomstat.h"
#include "../v_video.h"
#include "../command.h"
#include "../screen.h"
#include "../i_system.h"
#include "../i_video.h"

#include <ncurses.h>
#include <panel.h>

rendermode_t rendermode = render_soft;
rendermode_t chosenrendermode = render_soft;

boolean allow_fullscreen = false;

consvar_t cv_vidwait = CVAR_INIT ("vid_wait", "On", CV_SAVE, CV_OnOff, NULL);

static RGBA_t term_palette[256];
static short real_palette[256];

void I_StartupGraphics(void)
{
    initscr();

    start_color();
    use_default_colors();

	cbreak();
	noecho();
	nodelay(stdscr, TRUE);
	keypad(stdscr, TRUE);

	VID_SetMode(1);
    graphics_started = true;
}

void I_ShutdownGraphics(void)
{
	printf("\x1b[?25h");
	printf("\x1b[0m");
	endwin();
}

void VID_StartupOpenGL(void){}

void I_SetPalette(RGBA_t *palette)
{
    memcpy(term_palette, palette, sizeof(term_palette));

	for (int i = 0; i < 256; i++)
	{
		// normally its really dark so this is my hack (?)
		int r = term_palette[i].s.red * 1000 / 255;
		int g = term_palette[i].s.green * 1000 / 255;
		int b = term_palette[i].s.blue * 1000 / 255;

		init_color(i, r, g, b);
		init_pair(i + 1, i, -1);

		real_palette[i] = i + 1;
	}
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

// can this be improved?
void I_FinishUpdate(void)
{
    erase();

	SCR_CalculateFPS();

	if (marathonmode)
		SCR_DisplayMarathonInfo();

	if (cv_closedcaptioning.value)
		SCR_ClosedCaptions();

	if (cv_ticrate.value)
		SCR_DisplayTicRate();

	if (cv_showping.value && netgame && consoleplayer != serverplayer)
		SCR_DisplayLocalPing();

    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);

    // cool scaling thing :sunglasses:
    for (int y = 0; y < maxy; y++)
    {
        int sy = y * vid.height / maxy;

        for (int x = 0; x < maxx; x++)
        {
            int sx = x * vid.width / maxx;

            UINT8 p = screens[0][sy * vid.width + sx];

            attron(COLOR_PAIR(real_palette[p]));
            mvaddstr(y, x, "█");
            attroff(COLOR_PAIR(real_palette[p]));
        }
    }
    refresh();
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

