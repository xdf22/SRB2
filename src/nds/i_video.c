#include "../doomdef.h"
#include "../command.h"
#include "../i_video.h"
#include "../screen.h"

#include <nds.h>

rendermode_t rendermode = render_none;
rendermode_t chosenrendermode = render_none;

boolean highcolor = false;

boolean allow_fullscreen = false;
UINT8 graphics_started = false;

consvar_t cv_vidwait = CVAR_INIT("vid_wait", "On", CV_SAVE, CV_OnOff, NULL);

UINT16 ds_palette[256];

void I_StartupGraphics(void)
{
    CV_RegisterVar (&cv_vidwait);
	VID_SetMode(1);
    graphics_started = true;
}

void I_ShutdownGraphics(void){}

void VID_StartupOpenGL(void){}

void I_SetPalette(RGBA_t *palette)
{
    for (int i = 0; i < 256; i++)
    {
        u8 r = palette[i].s.red>>3;
        u8 g = palette[i].s.green>>3;
        u8 b = palette[i].s.blue>>3;

        ds_palette[i] = RGB15(r, g, b);
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

INT32 VID_SetMode(INT32 modenum)
{
	vid.width = 320;
	vid.height = 200;
	vid.bpp = 1;
	vid.rowbytes = vid.width * vid.bpp;
	vid.recalc = true;
	
	vid.modenum = 0; 
    vid.buffer = calloc(NUMSCREENS, vid.rowbytes*vid.height);
	
	videoSetMode(MODE_VRAM_A);
    vramSetBankA(VRAM_A_LCD);

	return 0;
}

boolean VID_CheckRenderer(void) { return false; }

void VID_CheckGLLoaded(rendermode_t oldrender)
{
	(void)oldrender;
}

const char *VID_GetModeName(INT32 modenum)
{
	(void)modenum;
	return NULL;
}

UINT32 I_GetRefreshRate(void) { return 35; }

void I_UpdateNoBlit(void){}

void I_FinishUpdate(void)
{
    u16* framebuffer = (u16*)VRAM_A;

    const int src_w = vid.width;
    const int src_h = vid.height;
    const int dst_w = 256;
    const int dst_h = 192;

    if (cv_fullscreen.value) // squished
    {
        for (int y = 0; y < dst_h; y++)
        {
            int src_y = (y * src_h) / dst_h;

            for (int x = 0; x < dst_w; x++)
            {
                int src_x = (x * src_w) / dst_w;

                framebuffer[y * dst_w + x] =
                    ds_palette[vid.buffer[src_y * src_w + src_x]];
            }
        }
    }
    else // cropped
    {
        int x_offset = (src_w - dst_w) / 2; // 32
        int y_offset = (src_h - dst_h) / 2; // 4

        for (int y = 0; y < dst_h; y++)
        {
            int src_y = y + y_offset;

            for (int x = 0; x < dst_w; x++)
            {
                int src_x = x + x_offset;

                framebuffer[y * dst_w + x] =
                    ds_palette[vid.buffer[src_y * src_w + src_x]];
            }
        }
    }
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

