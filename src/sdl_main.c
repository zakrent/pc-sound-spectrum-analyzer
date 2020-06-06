#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "ext/stb_easy_font.h"

#include "common.h"
#include "system.h"

#define SAMP_RATE 48000
#define CAPTURE_BUFFER_SIZE SAMP_RATE
#define FRAME_TIME (1/60.0)

#include "ext/kiss_fft.c"
#include "ext/kiss_fftr.c"
#include "memory_arena.c"
#include "render.c"
#include "ui.c"
#include "program.c"

SDL_AudioDeviceID dev;
bool ReadCaptureBuffer(i16 *Buffer, u32 Bytes){
#if 0
	if(SDL_GetQueuedAudioSize(dev) < Bytes){
		return 0;
	}
#endif
	u32 BytesGot = SDL_DequeueAudio(dev, (void*)Buffer, Bytes);
	return BytesGot > 0;
}

int main(){
	srand(0);
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* window = SDL_CreateWindow("Spectrum",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1000, 500,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_GL_CreateContext(window);

	GLenum glew_status = glewInit();
	if (glew_status != GLEW_OK) {
		return EXIT_FAILURE;
	}

	SDL_AudioSpec want, have;

	SDL_memset(&want, 0, sizeof(want));
	want.freq = SAMP_RATE;
	want.format = AUDIO_S16;
	want.channels = 1;
	want.samples = MaxReadSize*2;
	want.callback = NULL;

	dev = SDL_OpenAudioDevice(NULL, 1, &want, &have, 0);
	SDL_PauseAudioDevice(dev, 0);

	u64 MemorySize = MEBIBYTES(32);
	void *Memory = malloc(MemorySize);

	bool running = true;
	SDL_Event e;
	u32 mouseEvent = ME_None;
	r32 mouseX = 0.0f;
	r32 mouseY = 0.0f;
	i32 windowWidth  = 0.0f;
	i32 windowHeight = 0.0f;
	while(running){
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);

		while(SDL_PollEvent(&e)){
			switch(e.type){
				case SDL_QUIT:
				{
					running = false;
				}
				break;
				case SDL_MOUSEBUTTONDOWN:
				{
					if(e.button.button == SDL_BUTTON_LEFT){
						mouseEvent = ME_LDown;
					}
				}
				break;
				case SDL_MOUSEBUTTONUP:
				{
					if(e.button.button == SDL_BUTTON_LEFT){
						mouseEvent = ME_LUp;
					}
				}
				break;
				case SDL_MOUSEMOTION:
				{
					mouseX = 2.0f*(e.motion.x*1.0f/windowWidth-0.5f);
					mouseY = -2.0f*(e.motion.y*1.0f/windowHeight-0.5f);
				}
				break;
				default:
				break;
			}
			if(e.type == SDL_QUIT){
			}
		}
		bool shouldExit = Frame(Memory, MemorySize, windowWidth, windowHeight, mouseX, mouseY, mouseEvent);
		if(shouldExit){
			running = false;
		}
		SDL_GL_SwapWindow(window);

		mouseEvent = ME_None;
	}

	return EXIT_SUCCESS;
}
