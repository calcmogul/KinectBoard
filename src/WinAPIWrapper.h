//=============================================================================
//File Name: WinAPIWrapper.h
//Description: Provides wrapper for common WinAPI algorithms called in this
//             program
//Author: Tyler Veness
//=============================================================================

#ifndef WINAPI_WRAPPER_H
#define WINAPI_WRAPPER_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* All INPUT structures are zeroed upon entry to each function.
 * The parameters passed to the function determine what the zeroed struct will then contain.
 */

void moveMouse( INPUT* input ,
        DWORD dx ,
        DWORD dy ,
        DWORD dwFlags // put extra flags here; use either MOUSEEVENTF_MOVE or MOUSEEVENTF_ABSOLUTE
        );

// Sends left click and release to event queue
void leftClick( INPUT* input );

// Sends right click and release to event queue
void rightClick( INPUT* input );

#ifdef __cplusplus
}
#endif

#endif // WINAPI_WRAPPER_H
