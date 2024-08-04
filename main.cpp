#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <chrono>

#define STDLIB_H
#define TIME_H
#define DEBUG
#undef DEBUG

// Modify this macro to change the input filename
#define FILE_NAME "test2.ch8"

#include <stdio.h>
#include "fries8.h"

const unsigned int cycle_delay = 3;


unsigned char buffer[MAX_ROM_SIZE] = {0};
void read_rom(char const* filename)
{
	FILE *file = fopen(filename, "rb");
	if (file == NULL)
	{
		printf("Error: failed to open file '%s',\n", filename);
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < MAX_ROM_SIZE; ++i)
	{
		buffer[i] = fgetc(file);
	}
	fclose(file);
}

#define WINDOW_WIDTH 	640
#define WINDOW_HEIGHT 	480

#define DISPLAY_AREA_OFFSET 32
#define DISPLAY_AREA_WIDTH 64
#define DISPLAY_AREA_HEIGHT 32
#define DISPLAY_AREA_SIZE 8

void get_input(unsigned char *keypad)
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				exit(EXIT_SUCCESS);
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						exit(EXIT_SUCCESS);
						break;
					case SDLK_x:
						keypad[0] = 1;
						break;
					case SDLK_1:
						keypad[1] = 1;
						break;
					case SDLK_2:
						keypad[2] = 1;
						break;
					case SDLK_3:
						keypad[3] = 1;
						break;
					case SDLK_q:
						keypad[4] = 1;
						break;
					case SDLK_w:
						keypad[5] = 1;
						break;
					case SDLK_e:
						keypad[6] = 1;
						break;
					case SDLK_a:
						keypad[7] = 1;
						break;
					case SDLK_s:
						keypad[8] = 1;
						break;
					case SDLK_d:
						keypad[9] = 1;
						break;
					case SDLK_z:
						keypad[10] = 1;
						break;
					case SDLK_c:
						keypad[11] = 1;
						break;
					case SDLK_4:
						keypad[12] = 1;
						break;
					case SDLK_r:
						keypad[13] = 1;
						break;
					case SDLK_f:
						keypad[14] = 1;
						break;
					case SDLK_v:
						keypad[15] = 1;
						break;
				}
				break;
			case SDL_KEYUP:
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						exit(EXIT_SUCCESS);
						break;
					case SDLK_x:
						keypad[0] = 0;
						break;
					case SDLK_1:
						keypad[1] = 0;
						break;
					case SDLK_2:
						keypad[2] = 0;
						break;
					case SDLK_3:
						keypad[3] = 0;
						break;
					case SDLK_q:
						keypad[4] = 0;
						break;
					case SDLK_w:
						keypad[5] = 0;
						break;
					case SDLK_e:
						keypad[6] = 0;
						break;
					case SDLK_a:
						keypad[7] = 0;
						break;
					case SDLK_s:
						keypad[8] = 0;
						break;
					case SDLK_d:
						keypad[9] = 0;
						break;
					case SDLK_z:
						keypad[10] = 0;
						break;
					case SDLK_c:
						keypad[11] = 0;
						break;
					case SDLK_4:
						keypad[12] = 0;
						break;
					case SDLK_r:
						keypad[13] = 0;
						break;
					case SDLK_f:
						keypad[14] = 0;
						break;
					case SDLK_v:
						keypad[15] = 0;
						break;
				}
				break;
			}
			break;
		}
}
int WinMain()
{
        struct Chip8Machine Machine = {0};		// Instantiate the machine

	SDL_Renderer *renderer;
	SDL_Window *window;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC, &window, &renderer);

	read_rom(FILE_NAME);
	init_machine(&Machine, buffer, MAX_ROM_SIZE);
	SDL_Rect rectangles[64][32];

#ifdef DEBUG
	for (int i = 0; i < MAX_ROM_SIZE; i+=2)
		printf("%x%x\t", buffer[i], buffer[i + 1]);
#endif

    	for (int i = 0; i < DISPLAY_AREA_WIDTH; i++)
    	{
        	for (int j = 0; j < DISPLAY_AREA_HEIGHT; j++)
        	{
            		SDL_Rect rect = {DISPLAY_AREA_OFFSET + i * DISPLAY_AREA_SIZE, DISPLAY_AREA_OFFSET + j * DISPLAY_AREA_SIZE, DISPLAY_AREA_SIZE, DISPLAY_AREA_SIZE};
            		rectangles[i][j] = rect;
        	}
    	}


	std::chrono::high_resolution_clock::time_point previous_cycle_time = std::chrono::high_resolution_clock::now();
	while (1)
	{

		get_input(Machine.keypad);
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		float delay_time = std::chrono::duration<float, std::chrono::milliseconds::period>(now - previous_cycle_time).count();

		if (delay_time > cycle_delay)
		{
			for (int i = 0; i < 64; ++i)
			{
				for (int j = 0; j < 32; ++j)
				{
					if (Machine.display[(j * 64) + i])
					{
			                    SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, SDL_ALPHA_OPAQUE);
					}
					else
					{
			                    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
					}

					SDL_RenderFillRect(renderer, &rectangles[i][j]);
				}
			}

			previous_cycle_time = now;
			do_cycle(&Machine);
			SDL_RenderPresent(renderer);
		}
	}
	return EXIT_SUCCESS;
};
