#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <chrono>

#include "i8080.h"

// Emulation settings
bool autostep = false;
float gameSpeed = 1.0;

// Window settings
const bool drawRam = true;
const uint8_t gameScale = 2;
const int gameWidth = 224 * gameScale;
const int gameHeight = 256 * gameScale;
int width = gameWidth;
int height = gameHeight;
const int ramWidth = 800;
const int ramHeight = 640;
//RAM drawing settings
int8_t ramPage = 0;
uint8_t rowCount = 32;
uint8_t colCount = 32;
uint8_t pageCount = 8;
uint16_t pageElementCount;
int ramYOffset = 100;


// Emulation vars
SDL_Rect pixel = {0, 0, gameScale, gameScale};
int sWidth;
int sHeight;
unsigned int steps = 0;
unsigned int stepsToRun = 0;
unsigned int deltaLastStep;
unsigned int frameNumber = 1;

std::chrono::high_resolution_clock::time_point cycleStart;
std::chrono::high_resolution_clock::time_point cycleEnd;
std::chrono::nanoseconds cycleTime;
unsigned int netCyclens;
std::chrono::high_resolution_clock::time_point drawStart;
std::chrono::high_resolution_clock::time_point drawEnd;
std::chrono::nanoseconds drawTime;
unsigned int deltaLastDraw = 0;

SDL_Surface* gWindowSurface = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Window* gWindow = NULL;

TTF_Font* font;
SDL_Color fontColour;
SDL_Surface* surfaceMessage;
SDL_Texture* message;

bool checkPCBreakpoint(i8080);
bool setupGraphics();
void drawGraphics(i8080);
enum screenHalf { upper, lower };
void drawGameHalf(i8080, screenHalf);
template<typename T>
void emplaceDataInArray(char*, int, T);
void close();

int main(int argc, char** argv){

	i8080 My8080;
	const char* romname = "rom";
	if( argc == 2){
		My8080.loadROM(argv[1]);
	} else {
		My8080.loadROM(romname);
	}
	if(!setupGraphics()){return -1;}
	screenHalf drawHalf = upper;
	drawGraphics(My8080);//initial window fill-in
	pageElementCount = rowCount * colCount;
	printf("Setup done!\n");

	bool quit = false;
	SDL_Event e;
	while(!quit){
		cycleStart = std::chrono::high_resolution_clock::now();
		while(SDL_PollEvent(&e) != 0){
			if(e.type == SDL_QUIT){
				quit=true;
				break;
			} else if (e.type == SDL_KEYDOWN){
				switch(e.key.keysym.sym){

				case(SDLK_p):
					My8080.printState();
					break;
				case(SDLK_v):
					stepsToRun += 1;
					break;
				case(SDLK_b):
					stepsToRun += 10;
					break;
				case(SDLK_n):
					stepsToRun += 100;
					break;
				case(SDLK_m):
					stepsToRun += 1000;
					break;
				case(SDLK_o):
					autostep = !autostep;
					break;
				case(SDLK_q):
					quit = true;
					break;
				//ram scrolling
				case(SDLK_l):
					ramPage++;
					if(ramPage == pageCount){ ramPage = 0; }
					break;
				case(SDLK_h):
					ramPage--;
					if(ramPage < 0){ ramPage = (pageCount - 1); }
					break;
				// system input

				// Port 1
				case(SDLK_c): //coin
					My8080.port1 = My8080.port1 | 0x01;
					break;
				case(SDLK_2): //p2start
					My8080.port1 = My8080.port1 | 0x02;
					break;
				case(SDLK_1): //p1start
					My8080.port1 = My8080.port1 | 0x04;
					break;
				// 0x08 unused
				case(SDLK_SPACE): //p1 shoot
					My8080.port1 = My8080.port1 | 0x10;
					break;
				case(SDLK_LEFT): //p1 left
					My8080.port1 = My8080.port1 | 0x20;
					break;
				case(SDLK_RIGHT): //p1 right
					My8080.port1 = My8080.port1 | 0x40;
					break;
				//0x80 unused

				// Port 2
				case(SDLK_KP_0): //life dipswitch upper bit
					My8080.port2 = My8080.port2 ^ 0x01;
					break;
				case(SDLK_KP_1): //life dipswitch lower bit
					My8080.port2 = My8080.port2 ^ 0x02;
					break;
				case(SDLK_KP_2): //tilt
					My8080.port2 = My8080.port2 | 0x04;
					break;
				case(SDLK_KP_3): //life bonus
					My8080.port2 = My8080.port2 ^ 0x08;
					break;
				case(SDLK_KP_4): //p2 shoot
					My8080.port2 = My8080.port2 | 0x10;
					break;
				case(SDLK_a): //p2 left
					My8080.port2 = My8080.port2 | 0x20;
					break;
				case(SDLK_d): //p2 right
					My8080.port2 = My8080.port2 | 0x40;
					break;
				case(SDLK_7): //coin info
					My8080.port2 = My8080.port2 ^ 0x80;
					break;
				}
			} else if (e.type == SDL_KEYUP){
				switch(e.key.keysym.sym){
				// My8080 input

					// Port 1
					case(SDLK_c): //coin
						My8080.port1 = My8080.port1 ^ 0x01;
						break;
					case(SDLK_2): //p2start
						My8080.port1 = My8080.port1 ^ 0x02;
						break;
					case(SDLK_1): //p1start
						My8080.port1 = My8080.port1 ^ 0x04;
						break;
					// 0x08 unused
					case(SDLK_SPACE): //p1 shoot
						My8080.port1 = My8080.port1 ^ 0x10;
						break;
					case(SDLK_LEFT): //p1 left
						My8080.port1 = My8080.port1 ^ 0x20;
						break;
					case(SDLK_RIGHT): //p1 right
						My8080.port1 = My8080.port1 ^ 0x40;
						break;
					//0x80 unused

					// Port 2
					case(SDLK_KP_0): //life dipswitch upper bit
						My8080.port2 = My8080.port2 ^ 0x01;
						break;
					case(SDLK_KP_1): //life dipswitch lower bit
						My8080.port2 = My8080.port2 ^ 0x02;
						break;
					case(SDLK_KP_2): //tilt
						My8080.port2 = My8080.port2 ^ 0x04;
						break;
					case(SDLK_KP_3): //life bonus
						My8080.port2 = My8080.port2 ^ 0x08;
						break;
					case(SDLK_KP_4): //p2 shoot
						My8080.port2 = My8080.port2 ^ 0x10;
						break;
					case(SDLK_a): //p2 left
						My8080.port2 = My8080.port2 ^ 0x20;
						break;
					case(SDLK_d): //p2 right
						My8080.port2 = My8080.port2 ^ 0x40;
						break;
					case(SDLK_7): //coin info
						My8080.port2 = My8080.port2 ^ 0x80;
						break;
				}
			}
		} // end input processing

		while(stepsToRun > 0){
			stepsToRun--;
			My8080.emulateCycle();
			steps++;

			// check if interrupts / drawing needs to happen
			switch(steps){
	
			// mid-screen interrupt
			case 16666:
				My8080.dataBus = 0x08;	
				My8080.signalInt();
				drawHalf = upper;
				drawGameHalf(My8080, drawHalf); 
				break;
			// end of screen interrupt
			case 33333:
				My8080.dataBus = 0x10;	
				My8080.signalInt();
				steps = 1;
				frameNumber++;
				drawHalf = lower;
				drawGameHalf(My8080, drawHalf); 
				drawGraphics(My8080);
				break;
			}
			// ensures that when manually stepping, visual will update
			// low value check on stepsToRun prevents slowdown with high sTR values
			if(!autostep && stepsToRun < 5){
				drawHalf = upper;
				drawGameHalf(My8080, drawHalf);
				drawHalf = lower;
				drawGameHalf(My8080, drawHalf);
				drawGraphics(My8080);
			}
		}
		
		//account for 16666.66666 being an even division of cycles 
		// (2MHz / 60FPS / 2 interrupts
		if(frameNumber == 60){
			for(int i=0;i<20;i++){
				My8080.emulateCycle();
			}	
			steps = 1;
			frameNumber = 1;
		}

		// Timer stuff
		cycleEnd = std::chrono::high_resolution_clock::now();
		cycleTime = std::chrono::duration_cast<std::chrono::nanoseconds>(cycleEnd - cycleStart);
		//printf("Cycletime: %d\n", cycleTime.count());
		deltaLastStep += cycleTime.count();
		if(autostep && deltaLastStep >= (16666666/gameSpeed) ){
			stepsToRun += 33333;
		} 

	} // end of while loop
	close();	
	return 0;
	
}

bool checkPCBreakpoint(i8080 system){
	return false;
	uint16_t PC = system.fetchRegPair('P');
	// we care about this
	switch(PC){
		case 0x0003:
			printf("The Beginning\n");
			autostep = false;
			printf("\n ===Breakpoint!=== \n");
			system.printState();
			return true;
		case 0x191A:
			printf("Begin DrawScorHead\n");
			autostep = false;
			printf("\n ===Breakpoint!=== \n");
			system.printState();
			return true;
			break;	
		case 0x1925:
			printf("Begin p1 score\n");
			autostep = false;
			printf("\n ===Breakpoint!=== \n");
			system.printState();
			return true;
			break;
		case 0x192B:
			printf("Begin p2 score\n");
			autostep = false;
			printf("\n ===Breakpoint!=== \n");
			system.printState();
			return true;
			break;
		default:
			return false;
	}
}

bool setupGraphics(){
	bool success = true;

	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		printf("Could not initialise SDL: %S\n", SDL_GetError());
		success = false;
	} else { 
		if(drawRam){
		   width = gameWidth + ramWidth;
		   height = (gameHeight > ramHeight)?gameHeight:ramHeight;
		}	
		gWindow = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
		if(gWindow == NULL){
			printf("Could not create window: %S\n", SDL_GetError());
			success = false;
		} else {
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
			if(gRenderer == NULL){
				printf("Could not create renderer: %S\n", SDL_GetError());
				success = false;
			}
		}
	}
	
	TTF_Init();

	font = TTF_OpenFont("font.ttf", 12);
	fontColour = {200, 200, 255};

	SDL_SetRenderDrawColor(gRenderer, 0x00, 0x20, 0x20, 0xFF);
	SDL_RenderClear(gRenderer);
	return success;
}

void drawGraphics(i8080 system){
	
	if(drawRam){
	char text[6 + 2 + (3 * colCount)];
	text[(sizeof(text)/sizeof(text[0]))] = '\0';

	// Draw register info


	text[0]='P';text[1]='C';text[2]=':';emplaceDataInArray(text,3, system.fetchRegPair('P'));text[7]=' ';
	text[8]='O';text[9]='P';text[10]=':';emplaceDataInArray(text,11,system.fetchReg('O'));text[13]=' ';
	text[14]='S';text[15]='P';text[16]=':';emplaceDataInArray(text,17,system.fetchRegPair('S'));text[21]=' ';

	const char registerArray[8] = {'A', 'B', 'C', 'D', 'E', 'H', 'L', 'P'};
	for(int i = 0; i < 8; ++i){
		uint8_t offset = i * 5 + 22;
		text[offset] = registerArray[i];
		text[offset + 1] = ':';
		emplaceDataInArray(text, offset+2, system.fetchReg(registerArray[i]));
		text[offset+4]=' ';
	}
	text[22+5*8] = '\0';


	surfaceMessage = TTF_RenderText_Solid(font, text, fontColour);
	message = SDL_CreateTextureFromSurface(gRenderer, surfaceMessage);

	sWidth = surfaceMessage->w;
	sHeight = surfaceMessage->h;
	SDL_Rect mRect;
	mRect.x = gameWidth;
	mRect.y = 0;
	mRect.w = sWidth;
	mRect.h = sHeight;
	SDL_FreeSurface(surfaceMessage);
	SDL_RenderCopy(gRenderer, message, NULL, &mRect);
	SDL_DestroyTexture(message);

	//printf("RAM address draw \n");
	// draw RAM
	// addr + ": " + "val "
	text[0] = ' ';
	text[1] = ' ';
	text[2] = ' ';
	text[3] = ' ';
	text[4] = ' ';
	text[5] = ' ';
	text[6] = ' ';
	text[7] = ' ';
	for(int col = 0; col < colCount; ++col){
		char upperByte = (col & 0xF0) >> 4;
		char lowerByte = (col & 0x0F);
		upperByte += (upperByte < 10)? 48 : 55;
		lowerByte += (lowerByte < 10)? 48 : 55;
		text[8 + 3*col] = upperByte;
		text[8 + 3*col + 1] = lowerByte; 
		text[8 + 3*col + 2] = ' '; 
	}

	surfaceMessage = TTF_RenderText_Solid(font, text, fontColour);
	message = SDL_CreateTextureFromSurface(gRenderer, surfaceMessage);

	sWidth = surfaceMessage->w;
	sHeight = surfaceMessage->h;
	mRect;
	mRect.x = gameWidth;
	mRect.y = 0 + ramYOffset;
	mRect.w = sWidth;
	mRect.h = sHeight;
	SDL_FreeSurface(surfaceMessage);
	SDL_RenderCopy(gRenderer, message, NULL, &mRect);
	SDL_DestroyTexture(message);

	//printf("RAM draw \n");
	uint16_t startAddr = 0x2000;
	for(int row = 0; row < rowCount; ++row){
		uint16_t trueRow = (row * colCount) + (pageElementCount * ramPage) + startAddr;
		text[0]='0';text[1]='x';
		text[2]=((trueRow & 0xF000)>>12) + ((((trueRow & 0xF000) >> 12) < 10) ? 48 : 55);
		text[3]=((trueRow & 0x0F00)>>8) + ((((trueRow & 0x0F00) >> 8) < 10) ? 48 : 55);
		text[4]=((trueRow & 0x00F0)>>4) + ((((trueRow & 0x00F0) >> 4) < 10) ? 48 : 55);
		text[5]=((trueRow & 0x000F)) + ((((trueRow & 0x000F)) < 10) ? 48 : 55);
		text[6]=':';text[7]=' ';
		for(int col = 0; col < colCount; ++col){
			uint16_t ramAddress = trueRow + col;
			uint8_t value = system.fetchRAM(ramAddress);
			//printf("reading address: %X = %d\n", ramAddress, value);
			char upperByte = (value & 0xF0) >> 4;
			char lowerByte = (value & 0x0F);
			upperByte += (upperByte < 10)? 48 : 55;
			lowerByte += (lowerByte < 10)? 48 : 55;
			text[8 + 3*col] = upperByte;
			text[8 + 3*col + 1] = lowerByte; 
			text[8 + 3*col + 2] = ' '; 
		}
		surfaceMessage = TTF_RenderText_Solid(font, text, fontColour);
		message = SDL_CreateTextureFromSurface(gRenderer, surfaceMessage);
		
		int sWidth = surfaceMessage->w;
		int sHeight = surfaceMessage->h;
		SDL_Rect mRect;
		mRect.x = gameWidth;
		mRect.y = row * 12 + 12 + ramYOffset;
		mRect.w = sWidth;
		mRect.h = sHeight;
		SDL_FreeSurface(surfaceMessage);
		SDL_RenderCopy(gRenderer, message, NULL, &mRect);
		SDL_DestroyTexture(message);

	}
	}

	SDL_RenderPresent(gRenderer);
	
	/*
	drawEnd = std::chrono::high_resolution_clock::now();
	drawTime = std::chrono::duration_cast<std::chrono::duration<int>>(drawEnd - drawStart);
	printf("Draw time: %f ms\n", (1000 * drawTime.count()) );
	*/
}

void drawGameHalf(i8080 system, screenHalf sideToDraw){
	int offset = 0;
	if(sideToDraw == upper){
		offset = 0;
		// reset screen
		SDL_SetRenderDrawColor(gRenderer, 0x20, 0x20, 0x20, 0xFF);
		SDL_RenderClear(gRenderer);
	} else {
		offset = 128;
	}	

	SDL_SetRenderDrawColor(gRenderer, 0xFF, 0x02, 0x70, 0xFF);
	// Game draw
	for(int y=0+offset;y<128+offset;y+=8){
		for(int x=0;x<224;++x){
			//printf("Accessing %d, %d = %d\n", x, y, (32*x+0x241F-y/8));
			uint8_t pixelCol = system.fetchGFXPixel( (32*x+0x241F-y/8));
			for(uint8_t shift = 0; shift < 8; ++shift){
				if(pixelCol & (0x80 >> shift)){
					pixel.x = x * gameScale;
					pixel.y = (y+shift) * gameScale;
					SDL_RenderFillRect(gRenderer, &pixel);
				}
			}
		}
	}
}

template<typename T>
void emplaceDataInArray(char* array, int offset, T data){
	uint8_t nibble = sizeof(T) * 2;

	for(int i = 0; i < nibble; ++i){
		//Shift ammount (# of hex digits, offset into 0 index, reverse ordering * bits in nibble )
		uint8_t sa = (nibble - i - 1) * 4;
		//uint8_t hexChar = ((data & (0xF<<sa))>>sa);
		uint8_t hexChar = data>>sa & 0xF;
		array[offset + i] = hexChar + ((hexChar < 10) ? 48 : 55);
	}
}

void close(){
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	SDL_DestroyRenderer(gRenderer);
	gRenderer = NULL;
	SDL_Quit();
}
