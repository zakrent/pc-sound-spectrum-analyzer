#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <windowsx.h>
#include <dsound.h>
#include <gl/gl.h>

#include "ext/glext.h"
#include "ext/wglext.h"
#include "ext/stb_easy_font.h"

#include "common.h"
#include "system.h"

#define GLLoad(type, name) PFNGL##type##PROC gl##name;
#include "opengl_list.inc"
#undef GLLoad

#define SAMP_RATE 48000
#define CAPTURE_BUFFER_SIZE SAMP_RATE
#define FRAME_TIME (1/60.0)

#include "ext/kiss_fft.c"
#include "ext/kiss_fftr.c"
#include "memory_arena.c"
#include "render.c"
#include "ui.c"
#include "program.c"

static bool Running;
static u32 Width;
static u32 Height;

r32 MouseX;
r32 MouseY;
u32 MouseEvent;

LRESULT CALLBACK Win32WindowProc(HWND   hwnd,
				UINT   uMsg,
				WPARAM wParam,
				LPARAM lParam ){
	LRESULT Result = 0;
	switch(uMsg){
		case WM_SIZE: {
			RECT WindowRect;
			GetClientRect(hwnd, &WindowRect);
			Width = WindowRect.right - WindowRect.left;
			Height = WindowRect.bottom - WindowRect.top;
		}
		break;
		case WM_DESTROY:
		case WM_CLOSE: {
			Running = 0;
		}
		break;
		case WM_LBUTTONDOWN: {
			if(MouseEvent == ME_None){
				MouseEvent = ME_LDown;
				MouseX = 2.0f*(GET_X_LPARAM(lParam)*1.0/Width-0.5f); 
				MouseY = 2.0f*(GET_Y_LPARAM(lParam)*1.0/Height-0.5f); 
			}
		}
		break;
		case WM_LBUTTONUP: {
			MouseEvent = ME_LUp;
			MouseX = 2.0f*(GET_X_LPARAM(lParam)*1.0/Width-0.5f); 
			MouseY = 2.0f*(GET_Y_LPARAM(lParam)*1.0/Height-0.5f); 
		}
		break;
		case WM_MOUSEMOVE: {
			if(MouseEvent == ME_None){
				MouseX = 2.0f*(GET_X_LPARAM(lParam)*1.0/Width-0.5f); 
				MouseY = 2.0f*(GET_Y_LPARAM(lParam)*1.0/Height-0.5f); 
			}
		}
		break;
		case WM_ACTIVATEAPP: {
		}
		break;
		default: {
			Result = DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		break;
	}
	return Result;
}

bool Win32InitOpenGL(HINSTANCE hInstance, HWND *Window){
	WNDCLASSA WindowClass = {
		.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
		.lpfnWndProc   = Win32WindowProc,
		.hInstance     = hInstance,
		.lpszClassName = "SierraWC",
	};
	
	if(!RegisterClass(&WindowClass)){
		return 0;
	}

	HWND FakeWindow = CreateWindow("SierraWC",
									"Fake Window",
									WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
									0, 0,
									1, 1,
									0, 0,
									hInstance, 0);
	if(!FakeWindow){
		return 0;
	}
	
	HDC FakeDC = GetDC(FakeWindow);
	PIXELFORMATDESCRIPTOR FakePixelFormat;
	FakePixelFormat = (PIXELFORMATDESCRIPTOR){
			.nSize = sizeof(FakePixelFormat),
			.nVersion = 1,
			.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
			.iPixelType = PFD_TYPE_RGBA,
			.cColorBits = 32,
			.cAlphaBits = 8,
			.cDepthBits = 24,
			.iLayerType = PFD_MAIN_PLANE
	};

	int FakePixelFormatIdx = ChoosePixelFormat(FakeDC, &FakePixelFormat);
	if(!FakePixelFormatIdx){
		return 0;
	}

	if(!SetPixelFormat(FakeDC, FakePixelFormatIdx, &FakePixelFormat)){
		return 0;
	}

	HGLRC FakeRC = wglCreateContext(FakeDC);
	if(!FakeRC){
		return 0;
	}

	if(!wglMakeCurrent(FakeDC, FakeRC)){
		return 0;
	}

	PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB    = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC )wglGetProcAddress("wglCreateContextAttribsARB");
	if(!wglChoosePixelFormatARB || !wglCreateContextAttribsARB){
		return 0;
	}
	
	*Window = CreateWindowExA(0,
							WindowClass.lpszClassName,
							"Sierra",
							WS_OVERLAPPEDWINDOW | WS_VISIBLE,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							0,
							0,
							hInstance,
							0);
	HDC DC = GetDC(*Window);

	const int PixelAttribs[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
		WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
		WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB,     32,
		WGL_ALPHA_BITS_ARB,     8,
		0
	};
	
	int PixelFormatIdx;
	UINT FormatsCount;
	if(!wglChoosePixelFormatARB(DC, PixelAttribs, 0, 1, &PixelFormatIdx, &FormatsCount) || !FormatsCount){
		return 0;	
	}

	PIXELFORMATDESCRIPTOR PixelFormat;
	DescribePixelFormat(DC, PixelFormatIdx, sizeof(PixelFormat), &PixelFormat);
	SetPixelFormat(DC, PixelFormatIdx, &PixelFormat);

	int  ContextAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	
	HGLRC RC = wglCreateContextAttribsARB(DC, 0, ContextAttribs);
	if(!RC){
		return 0;
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(FakeRC);
	ReleaseDC(FakeWindow, FakeDC);
	DestroyWindow(FakeWindow);
	if (!wglMakeCurrent(DC, RC)) {
		return 0;
	}

#define GLLoad(type, name) gl##name = (PFNGL##type##PROC)wglGetProcAddress("gl" #name);
#include "opengl_list.inc"
#undef GLLoad

	char *glVersionString = glGetString(GL_VERSION);
	char WindowTitle[128];
	snprintf(WindowTitle, 128, "Sierra (OpenGL: %s)", glVersionString);
	WindowTitle[127] = 0;

	SetWindowTextA(*Window, WindowTitle);

	RECT WindowRect;
	GetClientRect(*Window, &WindowRect);
	Width = WindowRect.right - WindowRect.left;
	Height = WindowRect.bottom - WindowRect.top;

	return 1;
}

bool Win32InitDirectSoundCapture(LPDIRECTSOUNDCAPTUREBUFFER* CaptureBuffer){
	LPDIRECTSOUNDCAPTURE DSC; 
	if(!SUCCEEDED(DirectSoundCaptureCreate8(0, &DSC, 0))){
		return 0;
	}

	WAVEFORMATEX CaptureWaveFormat = {
		.wFormatTag = WAVE_FORMAT_PCM,
		.nChannels = 1,
		.nSamplesPerSec = SAMP_RATE,
		.nAvgBytesPerSec = SAMP_RATE*2,
		.nBlockAlign = 2,
		.wBitsPerSample = 16,
	};

	DSCBUFFERDESC CaptureBufferDesc;
	CaptureBufferDesc = (DSCBUFFERDESC){
		.dwSize = sizeof(CaptureBufferDesc),
		.dwFlags = 0,
		.dwBufferBytes = CAPTURE_BUFFER_SIZE*2,
		.lpwfxFormat = &CaptureWaveFormat,
	};

	if(!SUCCEEDED(IDirectSoundCapture_CreateCaptureBuffer(DSC, &CaptureBufferDesc, CaptureBuffer, 0))){
		return 0;
	}
	return 1;
}

static LPDIRECTSOUNDCAPTUREBUFFER CaptureBuffer;
static u32 ReadCursor;

bool ReadCaptureBuffer(i16 *Buffer, u32 Bytes){
	u32 ReadPosition, CapturePosition;
	IDirectSoundCaptureBuffer_GetCurrentPosition(CaptureBuffer, &CapturePosition, &ReadPosition);

	u32 ReadEnd = ReadCursor + Bytes - 1;
	bool Wrap = false;
	if(ReadEnd >= CAPTURE_BUFFER_SIZE*sizeof(i16)){
		ReadEnd -= CAPTURE_BUFFER_SIZE*sizeof(i16);
		Wrap = true;
	}
	
	bool CanRead = false;
	if(!Wrap){
		CanRead = !(ReadPosition >= ReadCursor && ReadPosition <= ReadEnd);
	}
	else{
		CanRead = !(ReadPosition <= ReadEnd || ReadPosition >= ReadCursor);
	}

	if(!CanRead){
		return false;
	}
	
	void *area1;
	void *area2;
	u32 size1;
	u32 size2;
	HRESULT Result = IDirectSoundCaptureBuffer_Lock(CaptureBuffer, ReadCursor, Bytes, 
													&area1, &size1, 
													&area2, &size2, 0);

	if(Result == DS_OK){
		ReadCursor = ReadEnd+1;
		if(ReadCursor >= CAPTURE_BUFFER_SIZE*sizeof(i16)){
			ReadCursor -= CAPTURE_BUFFER_SIZE*sizeof(i16);
		}
		CopyMemory(Buffer, area1, size1);
		if(area2){
			//TODO: check off by one
			CopyMemory((u8*)(Buffer)+size1, area2, size2);
		}
	}
	else {
		MessageBox(NULL, "Capture device failed!", NULL, MB_OK | MB_ICONERROR);
		exit(-1);
	}

	IDirectSoundCaptureBuffer_Unlock(CaptureBuffer, area1, size1, area2, size2);

	return Result == DS_OK;	
}

INT WinMain(HINSTANCE hInstance, 
			HINSTANCE hPrevInstance,
			PSTR lpCmdLine, 
			INT nCmdShow){
	HWND Window;
				
	if(!Win32InitOpenGL(hInstance, &Window)){
		MessageBox(NULL, "Failed to initialize OpenGL!", NULL, MB_OK | MB_ICONERROR);
		exit(-1);
	}
	
	if(!Win32InitDirectSoundCapture(&CaptureBuffer)){
		while (1) {
			i32 action = MessageBox(NULL, "Cannot open capture device!", NULL, MB_RETRYCANCEL | MB_ICONERROR);
			if (action == IDRETRY) {
				if (Win32InitDirectSoundCapture(&CaptureBuffer))
					break;
			}
			else {
				exit(-1);
			}
		}
	}

	u64 MemorySize = MEBIBYTES(1);
	void* Memory = malloc(MemorySize);
	
	Running = 1;

	IDirectSoundCaptureBuffer_Start(CaptureBuffer, DSCBSTART_LOOPING);
	
	LARGE_INTEGER PerformanceFrequency;
	QueryPerformanceFrequency(&PerformanceFrequency);
	
	LARGE_INTEGER LastCounter;
	QueryPerformanceCounter(&LastCounter);

	r32 TargetFrameTime = FRAME_TIME;

	MSG Message;
	while(Running){
		while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)){
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}

		HDC WindowDC = GetDC(Window);
		Frame(Memory, MemorySize, Width, Height, MouseX, MouseY, MouseEvent);
		SwapBuffers(WindowDC);
		ReleaseDC(Window, WindowDC);

		MouseEvent = ME_None;

		LARGE_INTEGER CurrentCounter;
		QueryPerformanceCounter(&CurrentCounter);
		r32 TimeElapsed = (CurrentCounter.QuadPart-LastCounter.QuadPart)*1.0f/PerformanceFrequency.QuadPart;
		if(TimeElapsed < TargetFrameTime){
			Sleep((u32)((TargetFrameTime-TimeElapsed)*1000.0f));
		}

		LastCounter = CurrentCounter;
	}

	return 0;
}
