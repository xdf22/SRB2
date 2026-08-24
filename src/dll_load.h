// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2026 by xdf
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  dll_load.h
/// \brief holy shit bro

// dont need to extern main
extern void (*DLL_Drawer)(void);
extern void (*DLL_Ticker)(void);

void DLL_Load(const char *filename);
const char *DLL_GetLoaded(void);
void DLL_Unload(void);
