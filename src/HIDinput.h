//=============================================================================
//File Name: HIDinput.h
//Description: Provides wrapper for emulating Human Interface Device commands
//Author: Tyler Veness
//=============================================================================

#ifndef HID_INPUT_H
#define HID_INPUT_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* All INPUT structures are zeroed upon entry to each function. The parameters
 * passed to the function determine what the zeroed struct will then contain.
 */

// Use either MOUSEEVENTF_MOVE or MOUSEEVENTF_ABSOLUTE for dwFlags
void moveMouse(INPUT* input, DWORD dx, DWORD dy, DWORD dwFlags);

// Sends left click and release to event queue
void leftClick(INPUT* input);

// Sends right click and release to event queue
void rightClick(INPUT* input);

#ifdef __cplusplus
}
#endif

#endif // HID_INPUT_H
