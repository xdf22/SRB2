// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2026 by xdf
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  dll_load.c
/// \brief holy shit bro

#include "dll_load.h"
#include "console.h"
#include <dlfcn.h>

void *handle;

void (*DLL_Drawer)(void);
void (*DLL_Ticker)(void);

void DLL_Load(const char *filename)
{
    // just have one
    if (handle)
    {
        dlclose(handle);
        handle = NULL;
    }

    handle = dlopen(filename, RTLD_NOW | RTLD_GLOBAL);

    if (!handle)
    {
        CONS_Printf("Failed to load DLL %s\n", dlerror());
        return;
    }

    void (*main)(void);

    main = dlsym(handle, "SRB2_main"); // runs once
    DLL_Drawer = dlsym(handle, "SRB2_drawer"); // runs while the game is ticking
    DLL_Ticker = dlsym(handle, "SRB2_ticker"); // runs while HU_Drawer is running

    if (main)
        main();
}
