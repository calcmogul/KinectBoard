//=============================================================================
//File Name: Main.cpp
//Description: Handles processing Kinect input
//Author: Tyler Veness
//=============================================================================

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <SFML/Window/Event.hpp>

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "WinAPIWrapper.h"
#include "Kinect.hpp"

sf::RenderWindow mainWin;
bool CLOSE_THREADS = false;

LRESULT CALLBACK OnEvent( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam );

INT WINAPI WinMain( HINSTANCE Instance , HINSTANCE , LPSTR , INT ) {
	const char* mainClassName = "KinectBoard";

	HICON appIcon = LoadIcon( Instance , "mainIcon" );

	// Define a class for our main window
	WNDCLASS WindowClass;
	WindowClass.style         = 0;
	WindowClass.lpfnWndProc   = &OnEvent;
	WindowClass.cbClsExtra    = 0;
	WindowClass.cbWndExtra    = 0;
	WindowClass.hInstance     = Instance;
	WindowClass.hIcon         = appIcon;
	WindowClass.hCursor       = NULL;
	WindowClass.hbrBackground = reinterpret_cast<HBRUSH>( COLOR_BACKGROUND );
	WindowClass.lpszMenuName  = NULL;
	WindowClass.lpszClassName = mainClassName;
	RegisterClass(&WindowClass);

	// Calibration window
	HWND Window = CreateWindow( mainClassName , "KinectBoard" , WS_VISIBLE | WS_THICKFRAME | WS_POPUP | WS_MAXIMIZE , 0 , 0 , GetSystemMetrics(SM_CXSCREEN) * 2 , GetSystemMetrics(SM_CYSCREEN) * 2 , NULL , NULL , Instance , NULL );

	mainWin.create( Window );
	mainWin.setMouseCursorVisible( false );

	sf::Event event;

	MSG Message;
	INPUT input = { 0 };

	Kinect projectorKinect;

	/* ===== Create and draw the calibration graphic ===== */
	// Construct pixel data
	sf::Image testImage;
	testImage.create( 40 , 40 , sf::Color( 0 , 0 , 0 ) );

	// Change top-left block to white
	for ( unsigned int yPos = 0 ; yPos < 20 ; yPos++ ) {
		for ( unsigned int xPos = 20 ; xPos < 40 ; xPos++ )
			testImage.setPixel( xPos , yPos , sf::Color( 255 , 255 , 255 ) );
	}

	// Change bottom-right block to white
	for ( unsigned int yPos = 20 ; yPos < 40 ; yPos++ ) {
		for ( unsigned int xPos = 0 ; xPos < 20 ; xPos++ )
			testImage.setPixel( xPos , yPos , sf::Color( 255 , 255 , 255 ) );
	}

	// Prepare the test pattern for drawing
	sf::Texture testTexture;
	testTexture.loadFromImage( testImage );
	testTexture.setRepeated( true );

	sf::Sprite calibrationSprite( testTexture );
	calibrationSprite.setTextureRect( sf::IntRect( 0 , 0 , mainWin.getSize().x , mainWin.getSize().y ) );

	// Create red border for window
	sf::RectangleShape border( sf::Vector2f( mainWin.getSize().x - 40.f , mainWin.getSize().y - 40.f ) );
	border.setFillColor( sf::Color( 0 , 0 , 0 , 0 ) );
	border.setOutlineThickness( 20 );
	border.setOutlineColor( sf::Color( 0 , 255 , 0 ) );

	// Set graphics positions before drawing them
	calibrationSprite.setPosition( 20.f , 20.f );
	border.setPosition( 20.f , 20.f );

	// Draw graphics to window
	mainWin.clear( sf::Color( 0 , 0 , 0 ) );
	mainWin.draw( calibrationSprite );
	mainWin.draw( border );
	mainWin.display();
	/* =================================================== */

	Sleep( 100 );

	while ( projectorKinect.hasNewImage() != Kinect::ImageStatus::Full ) {
		projectorKinect.fillImage();
	}

	// TODO process Kinect image to find location of Kinect in 3D space

	/* ===== Make a new window that isn't fullscreen ===== */
	mainWin.close();
	DestroyWindow( Window );

	// Create a new window to be used for the lifetime of the application
	Window = CreateWindow( mainClassName , "KinectBoard" , WS_SYSMENU | WS_VISIBLE | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME , ( GetSystemMetrics(SM_CXSCREEN) - 800 ) / 2 , ( GetSystemMetrics(SM_CYSCREEN) - 600 ) / 2 , 800 , 600 , NULL , NULL , Instance , NULL );
	mainWin.create( Window );
	/* =================================================== */

	while ( mainWin.isOpen() ) {
		if ( PeekMessage( &Message , NULL , 0 , 0 , PM_REMOVE ) ) {
			// If a message was waiting in the message queue, process it
			TranslateMessage( &Message );
			DispatchMessage( &Message );
		}
		else {
			while ( mainWin.pollEvent( event ) ) {
				if ( event.type == sf::Event::Closed )
					mainWin.close();
			}

			mainWin.clear( sf::Color( 40 , 40 , 40 ) );

			// TODO get images from Kinect, process them, and move mouse and press mouse buttons

			//moveMouse( input , -5 , 0 ); // move mouse cursor 5 pixels left
			//moveMouse( input , 5 , 0 ); // move mouse cursor 5 pixels right

			//leftClick( input ); // left click for testing

			mainWin.display();

			Sleep( 50 );
		}
	}

	// Clean up window
	DestroyWindow( Window );
	UnregisterClass( mainClassName , Instance );

	return EXIT_SUCCESS;
}

LRESULT CALLBACK OnEvent( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam ) {
	switch ( Message ) {
	case WM_SIZE: {
		mainWin.clear( sf::Color( 40 , 40 , 40 ) );
		mainWin.display();

		break;
	}
	case WM_WINDOWPOSCHANGED: {
		mainWin.clear( sf::Color( 40 , 40 , 40 ) );
		mainWin.display();

		break;
	}
	// Quit when we close the main window
	case WM_CLOSE: {
		PostQuitMessage(0);
		break;
	}
	default: {
		return DefWindowProc(Handle, Message, WParam, LParam);
	}
	}

	return 0;
}
