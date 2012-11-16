//=============================================================================
//File Name: WinAPIWrapper.cpp
//Description: Provides wrapper for common WinAPI algorithms called in this program
//Author: Tyler Veness
//=============================================================================

#include "WinAPIWrapper.h"

void moveMouse( INPUT& input , DWORD dx , DWORD dy , DWORD dwFlags ) {
	ZeroMemory( &input , sizeof(INPUT) );
	input.type = INPUT_MOUSE;
	if ( dx != 0 )
		input.mi.dx = dx;
	if ( dy != 0 )
		input.mi.dy = dy;
	input.mi.dwFlags |= MOUSEEVENTF_MOVE | dwFlags;

	SendInput( 1 , &input , sizeof(input) );
}

void leftClick( INPUT& input ) {
	// left mouse button down
	ZeroMemory( &input , sizeof(INPUT) );
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	SendInput( 1 , &input , sizeof(input) );

	// left mouse button up
	ZeroMemory( &input , sizeof(INPUT) );
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput( 1 , &input , sizeof(input) );
}

void rightClick( INPUT& input ) {
    // right mouse button down
    ZeroMemory( &input , sizeof(INPUT) );
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    SendInput( 1 , &input , sizeof(input) );

    // right mouse button up
    ZeroMemory( &input , sizeof(INPUT) );
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput( 1 , &input , sizeof(input) );
}
