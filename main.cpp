#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>


HGLRC		hRC		= NULL; //Perament Rendering Context
HDC			hDC		= NULL; //Private GDI Device Context
HWND		hWnd	= NULL; //Holds the Window Handle
HINSTANCE	hInstance;		//Holds the Instace of the Application

bool keys[256];				//Array used for Keyboard Routine
bool active			= TRUE;	//Window Active Flag
bool fullscreen		= TRUE;	//Fullscreen Flag

//WndProc receives all input directed at the window
//This bottom line simply declares WndProc. This is needed because
//When we use CreateGLWindow() it has a reference to WndProc but WndProc
//Comes after CreateGLWindow()
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//The function below rezies our OpenGL scene whenever the window has been
//Resized. It also is run on set-up to initialize the window. Sets up the screen
//for a perspective view (things in the distance get smaller). Uses a 45 degree
//viewing angle with 0.1f and 100.0f being the starting and ending points
//respectivly for how deep we can draw to the screen
GLvoid ResizeGLScene(GLsizei width, GLsizei height)
{
	if (height == 0)					//Prevent devide by zero by setting height = 1
		height = 1;

	glViewport(0, 0, width, height);	//This resets the current viewport

	glMatrixMode(GL_PROJECTION);		//Loads the projection matrix which is responsible for adding perspective to the scene
	glLoadIdentity();					//Resets the projection matrix

	gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);

	glMatrixMode(GL_MODELVIEW);			//Loads the ModelView matrix
	glLoadIdentity();					//Resets the ModelView matrix
}

//OpenGL set-up. Will not be called until our OpenGL window has been created
int InitGL(GLvoid)
{
	glShadeModel(GL_SMOOTH);							//Enables smooth shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				//Clears the screen colour to black
	glClearDepth(1.0f);									//Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							//Enable depth testing
	glDepthFunc(GL_LEQUAL);								//Sets what type of depth test to do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	//The best perspective correction set-up
	
	return TRUE;										//Init when A-OK (Add error code later)
}

//This function is responsible for drawing anything you want to the screen.
//Do ALL the drawing here
int DrawGLScene(GLvoid) 
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	//Clear the screen and the depth buffer
	glLoadIdentity();									//Reset current ModelView matrix
	
	return TRUE;										//Drawing went A-OK
}

//This function is responsible for releasing the rendering context, the device context
//and the window handle
GLvoid KillGLWindow(GLvoid) 
{
	if (fullscreen)						//Check if we are in fullscreen
	{
		ChangeDisplaySettings(NULL, 0); //Switch back to desktop if we are in fullscreen
		ShowCursor(TRUE);				//Show the mouse cursor
	}

	if (hRC)							//Checks if we have a rendering context
	{
		if (!wglMakeCurrent(NULL, NULL))//Check if we can release DC and RC
			MessageBox(NULL, "Release of DC and RC Failed", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);

		if (!wglDeleteContext(hRC))		//Checks if we are able to delete the RC
			MessageBox(NULL, "Release Rendering Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		hRC = NULL;						//Set the render context to null
	}

	if (hDC && !ReleaseDC(hWnd, hDC))	//Check to see if we have device context and we are not able to relase it
	{
		MessageBox(NULL, "Release Device Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		hDC = NULL;						//Set the device context to nulll
	}

	if (hWnd && !DestroyWindow(hWnd))	//Check to see if we have a window handle and are we able to destroy it
	{
		MessageBox(NULL, "Could Not Release hWnd.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		hWnd = NULL;					//Set window handle to null

		if (!UnregisterClass("OpenGL", hInstance))	//Check to see if we can unregister then window class
		{
			MessageBox(NULL, "Could Not Unregister Class.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
			hInstance = NULL;			//Set the instance of our application to null
		}
	}
}

BOOL CreateGLWindow(const char* title, int width, int height, int bits, bool fullscreenFlag) 
{
	GLuint pixelFormat;					//After windows finds a match for a pixel format the value is stored here
	WNDCLASS wc;						//Windows Class structure which holds info about the window
	DWORD dwExStyle;					//Extended window style
	DWORD dwStyle;						//Normal window style

	RECT windowRect;					//Create a rectangle that is the size of the input values
	windowRect.left		= (long)0;
	windowRect.right	= (long)width;	
	windowRect.top		= (long)0;
	windowRect.bottom	= (long)height;

	fullscreen			= fullscreenFlag;						//Set the global fullscreen flag to the input value

	hInstance			= GetModuleHandle(NULL);				//Grabs an application instance for the window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	//Redraw on window move or resize. CS_OWNDC creates a private device context
	wc.lpfnWndProc		= (WNDPROC)WndProc;						//WndProc handles messages in the application
	wc.cbClsExtra		= 0;									//Don't use extra window data
	wc.cbWndExtra		= 0;									//Don't use extra window data
	wc.hInstance		= hInstance;							//Set the instance of the application
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			//Loads the default window icon (No icon)
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			//Loads the default cursor
	wc.hbrBackground	= NULL;									//Set no background
	wc.lpszMenuName		= NULL;									//Set no menu
	wc.lpszClassName	= "OpenGL";								//Set the window class name

	if (!RegisterClass(&wc))									//Attempt to register the window class
	{
		MessageBox(NULL, "Failed To Register The Window Class.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;											//Exit if we unsucessful
	}

	if (fullscreen)																		//Do we want fullscreen?
	{
		DEVMODE dmScreenSettings;														//Device mode variable
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));							//Clear memory at the device mode address
		dmScreenSettings.dmSize			= sizeof(dmScreenSettings);						//Set the size to be the size of the DEVMODE struct
		dmScreenSettings.dmPelsWidth	= width;										//Set the screen width 
		dmScreenSettings.dmPelsHeight	= height;										//Set the screen height
		dmScreenSettings.dmBitsPerPel	= bits;											//Set the bits per pixel
		dmScreenSettings.dmFields		= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;	

		//Tries to change to display mode set in dmScreenSettings. CDS_FULLSCREEN flag is used to remove taskbar and
		//so other windows are not moved to resized when fullscreen mode is toggled
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) 
		{
			//If switching to fullscreen is unsucessfull, give two options: Switch to windowed or exit application
			if (MessageBox(NULL, "The requested fullscreen mode is not supported by\nyour video card. Use windowed mode instead?",
				"OpenGL", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) 
			{
				fullscreen = FALSE;		//Switch to windowed mode
			}
			else
			{
				MessageBox(NULL, "Program will now close.", "ERROR", MB_OK | MB_ICONSTOP);
				return FALSE;			//Otherwise exit applicaiton		
			}
		}
	}

	if (fullscreen)						//Check to see if we are still in fullscreen mode
	{
		//Set the extended style. WS_EX_APPWINDOW forces the current top level 
		//window down to the taskbar once the application window is visable
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;				//Creates a WS_POPUP window which has no border
		ShowCursor(FALSE);				//Don't show mouse cursor
	}
	else 
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;	//Adds a window edge to the application window
		dwStyle = WS_OVERLAPPEDWINDOW;					//Creates a window with a title bar, sizing border, window menu, and min/max buttons
	}

	//Ajusts the window to requested size. AjustWindowRextEx is used since it will create a window
	//that is a bit larger to account for the width of the border so none of the OpenGL scene is 
	//covered by the border. If in fullscreen mode this line doesn't do anything.
	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

	//Create the window and check to see if it was created properly
	if (!(hWnd = CreateWindowEx(
		dwExStyle,										//Extended style of the window
		"OpenGL",										//Name of the window class (same the window class name defined above)
		title,											//The title of the window
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle,	//
		0, 0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,											//Set no parent window
		NULL,											//Set no menu
		hInstance,										//Set the instance of the application 
		NULL)))											//Nothing passed to WM_CREATE
	{
		KillGLWindow();									//Kill the application window if the window was not created sucessfully
		MessageBox(NULL, "Window Creation Error.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;									//Exit application if window wasn't created properly
	}

	//Sets pixel format
	static PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),	//Size of pixel format descriptor struct
		1,								//Version of data structure
		PFD_DRAW_TO_WINDOW |			//Be able to draw to a window							
		PFD_SUPPORT_OPENGL |			//Support OpenGL drawing
		PFD_DOUBLEBUFFER,				//Support double buffering
		PFD_TYPE_RGBA,					//RGBA pixels
		bits,							//Number of colour bits (Colour depth)
		0, 0, 0, 0, 0, 0,				//Ignore all colour bits
		0,								//Ignore alpha buffer
		0,								//Ignore shift bit
		0,								//No accumulation buffer
		0, 0, 0, 0,						//Ignore accumulation bits
		16,								//Set z-buffer to 16 bits
		0,								//Don't use stencil buffer
		0,								//Don't use auxiliary buffer
		PFD_MAIN_PLANE,					//Use the main drawing layer (shared with usual graphics layer)
		0,								
		0, 0, 0							//Ignore layer masks
	};

	//No errors creating the device window, attempt to get an OpenGL device context
	if (!(hDC = GetDC(hWnd))) 
	{
		KillGLWindow();	//Kill window if grabing a device context was unsucessful
		MessageBox(NULL, "Was not able to create a GL Device Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;	//Exit application
	}

	//Attempt to find a pixel format that matches described one
	if (!(pixelFormat = ChoosePixelFormat(hDC, &pfd))) 
	{
		KillGLWindow();	//Kill window if windows couldn't find a matching pixel format
		MessageBox(NULL, "Can't find a suitable pixel format.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;	//Exit application
	}

	//After finding a matching pixel format try to set this format
	if (!SetPixelFormat(hDC, pixelFormat, &pfd))
	{
		KillGLWindow();	//Kill window if not able to set the pixel format
		MessageBox(NULL, "Can't set the pixel format.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;	//Exit application
	}

	//After setting the pixel format try to get a rendering context
	if (!(hRC = wglCreateContext(hDC)))
	{
		KillGLWindow();	//Kill the window if not able to get a rendering context
		MessageBox(NULL, "Can't create a GL rendering context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;	//Exit application
	}

	//Try to set the redering context active after getting it
	if (!wglMakeCurrent(hDC, hRC)) 
	{
		KillGLWindow();	//Kill the window if not able to get 
		MessageBox(NULL, "Can't activate the GL rendering context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;	//Exit application
	}

	//EVERYTHING WENT A-OK do the rest of the stuff

	ShowWindow(hWnd, SW_SHOW);		//Show the window
	SetForegroundWindow(hWnd);		//Set the window as the foreground window
	SetFocus(hWnd);					//Set Keyboard focus to the window
	ResizeGLScene(width, height);	//Set up perspective GL screen

	//Init OpenGL!
	if (!InitGL()) 
	{
		KillGLWindow();				//Kill the window if initalization failed
		MessageBox(NULL, "Initialization Failed.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;				//Exit application
	}

	return TRUE;					//Window creation sucessful!
}

//Processes messages that are sent to the window
LRESULT CALLBACK WndProc(	HWND hWnd,		//Window Handle
							UINT uMsg,		//Window message
							WPARAM wParam,	//Additional Message info that depend on uMsg
							LPARAM lParam)	//More message info	that depend on uMsg
{
	switch (uMsg) 
	{
	case WM_ACTIVATE:			//Check for active window message
		if (!HIWORD(wParam))	//Check minimization state
			active = TRUE;		//Set active if not minimized
		else
			active = FALSE;		//Set not active if minimized
		return 0;
	case WM_SYSCOMMAND:			//Did we get a system call?
		if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER)	//Check to see if the screen saver is starting or of the monitor is going into powersave
			return 0;			//If it is then stop it from happening
		break;
	case WM_CLOSE:				//Check for close message
		PostQuitMessage(0);		//Send a quit message
		return 0;
	case WM_KEYDOWN:			//Check to see if a key is being held down
		keys[wParam] = TRUE;	//Mark the key that is being held down as true
		return 0;
	case WM_KEYUP:				//Check if a key has been released
		keys[wParam] = FALSE;	//Set the key that was released to false
		return 0;
	case WM_SIZE:				//Check if the window has be re-sized
		ResizeGLScene(LOWORD(lParam), HIWORD(lParam));	//Read the Lo word and Hi word values of lParam to find the new window dimentions
		return 0;
	}

	//Hand off all other messages that we don't care about to the default window
	//procedure
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//Entry point of the application. Call window creation routine here. hInstance = cuurent instance of the
//application, hPrevInstance = previous instance of the application, lpCmdLine = command line params,
//nCmdShow = window show state
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	MSG msg;				//Windows message struct to check if there are any waiting messages
	BOOL done = FALSE;		//Vairable to check if the application is finished running or not

	//Ask if the user would like to run the application in fullscreen mode
	if (MessageBox(NULL, "Would you like to run in fullscreen mode?", "Start FullScreen?",
		MB_YESNO | MB_ICONQUESTION) == IDNO)
		fullscreen = FALSE;	//Set to windowed mode if the user answers no
	
	//Create the window. On failure Exit the application
	if (!CreateGLWindow("OpenGL Window", 640, 480, 16, fullscreen))
		return 0;

	while (!done) 
	{
		//Check to see if any messages are waiting. Using Peekmessage can check the
		//messages without haulting the application. The PM_REMOVE flag removes the 
		//messages once they are processed by PeekMessage
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
		{
			if (msg.message == WM_QUIT)		//Check to see if we received a quit message
			{
				done = TRUE;				//Set done to true if we want to quit
			}	
			else
			{
				//Translate virtual-key messages into charater messages
				//the character messages are then posted to the calling thread's
				//message queue next time PeekMessage is called
				TranslateMessage(&msg);
				//Dispatches the message to a window procedure. Retrieved by the PeekMessage
				//funtion
				DispatchMessage(&msg);		
			}
		}
		else	//No messages were sent
		{
			//Draw the scene watching for ESC keys
			if (active)
			{
				if (keys[VK_ESCAPE])	//Check to see if the escape key has been pressed
				{
					done = TRUE;		//If ESC has been pressed exit the application;
				}
				else 
				{
					DrawGLScene();		//Draw the scene
					//Swapping buffers works by drawing the next scene to a hidden "screen" and then the 
					//Two are swapped. This achieves smooth animation.
					SwapBuffers(hDC);
				}
			}
		}
	}
	KillGLWindow();			//Kill the window
	return (msg.wParam);	//Exit the application
}