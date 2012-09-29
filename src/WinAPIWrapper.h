//=============================================================================
//File Name: WinAPIWrapper.h
//Description: Provides wrapper for common WinAPI algorithms called in this program
//Author: Tyler Veness
//=============================================================================

#ifndef WINAPI_WRAPPER_H
#define WINAPI_WRAPPER_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

/* All INPUT structures are zeroed upon entry to each function.
 * The parameters passed to the function determine what the zeroed struct will then contain.
 */

void moveMouse( INPUT& input , DWORD dx , DWORD dy , DWORD dwFlags = 0 ); // put extra flags here; MOUSEEVENTF_MOVE is added automatically
void leftClick( INPUT& input ); // sends left click and release to event queue

#endif // WINAPI_WRAPPER_H
