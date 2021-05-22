#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <chrono>

#include "i8080.h"

// Emulation settings
bool autostep = false;
double deltaLastStep;
double cycleUs;
double netCycleUs;

// Window settings
const uint8_t gameScale = 2;
const int gameWidth = 256 * gameScale;
const int gameHeight = 224 * gameScale;
const int ramWidth = 800;
const int ramHeight = 640;
const int width = gameWidth + ramWidth;
const int height = (gameHeight > ramHeight)?gameHeight:ramHeight;
//const int width = 1056;
//const int height = 640;

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
int steps = 0;
int grossSteps = 0;

std::chrono::high_resolution_clock::time_point cycleStart;
std::chrono::high_resolution_clock::time_point cycleEnd;
std::chrono::duration<double> cycleTime;
std::chrono::high_resolution_clock::time_point drawStart;
std::chrono::high_resolution_clock::time_point drawEnd;
std::chrono::duration<double> drawTime;
double deltaLastDraw = 0;

SDL_Surface* gWindowSurface = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Window* gWindow = NULL;

TTF_Font* font;
SDL_Color fontColour;
SDL_Surface* surfaceMessage;
SDL_Texture* message;

bool setupGraphics();
void drawGraphics(i8080);
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

				case(SDLK_SPACE):
					My8080.emulateCycle();
					printf("step! %i\n", ++grossSteps);
					break;
				case(SDLK_p):
					My8080.printState();
					break;
				case(SDLK_g):
					for(int i=0;i<10;++i){
						My8080.emulateCycle();
						printf("step! %i\n", ++grossSteps);
						//My8080.printState();
					}
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
				}
			} else if (e.type == SDL_KEYUP){

			}
		} // end input processing
	
	if(autostep && (deltaLastStep >= 0.000001)){
		My8080.emulateCycle();
		++steps;
		//My8080.printState();
		printf("step! %i\n", ++grossSteps);
		deltaLastStep -= 0.000001;
	}

	if(deltaLastDraw >= 0.06){
		drawGraphics(My8080);
	}

	// Timer stuff
	cycleEnd = std::chrono::high_resolution_clock::now();
	cycleTime = std::chrono::duration_cast<std::chrono::duration<double>>(cycleEnd - cycleStart);
	if(deltaLastDraw >= 0.06){
		deltaLastDraw = 0;
	} else {
		deltaLastDraw += cycleTime.count();
	}
	deltaLastStep += cycleTime.count(); 
	netCycleUs += 1000000 * cycleTime.count();
	//printf("Cycle time: %f us \n", 1000000 * cycleTime.count() );
	//printf("Delta time: %f us \n", 1000000 * deltaLastStep );
	//printf("NSus time: %f us \n", netCycleUs );
	if(netCycleUs >= 1000000){
		//printf("Steps completed this second: %d\n", steps);
		netCycleUs = 0;
		steps = 0;
	}

	}
	close();	
	return 0;
	
}

bool setupGraphics(){
	bool success = true;

	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		printf("Could not initialise SDL: %S\n", SDL_GetError());
		success = false;
	} else { 
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
	
	//printf("Draw graphics \n");
	/*
	drawStart = std::chrono::high_resolution_clock::now();
	*/	

	// reset screen
	SDL_SetRenderDrawColor(gRenderer, 0x20, 0x20, 0x20, 0xFF);
	SDL_RenderClear(gRenderer);

	// draw game
	//printf("Game draw \n");
	for(int y = 0; y < 224; ++y){
		for(int x = 0; x < 32; ++x){
			// change colour here

			uint8_t pixelSet = system.fetchGFXPixel(y*32+x);
			for(uint8_t shift = 0; shift < 8; ++shift){
				if(pixelSet & (0x80 >> shift)){
					pixel.x = (x * 8 + shift) * gameScale;
					pixel.y = y * gameScale;
					SDL_RenderFillRect(gRenderer, &pixel);
				}
			}
		}
	}
	
	char text[6 + 2 + (3 * colCount)];
	text[(sizeof(text)/sizeof(text[0]))] = '\0';

	// Draw register info


	text[0]='P';text[1]='C';text[2]=':';emplaceDataInArray(text,3, system.fetchRegPair('P'));text[7]=' ';
	text[8]='O';text[9]='P';text[10]=':';emplaceDataInArray(text,11,system.fetchReg('O'));text[13]=' ';
	text[14]='S';text[15]='P';text[16]=':';emplaceDataInArray(text,17,system.fetchRegPair('S'));text[21]=' ';

	//text[8]={'q','w'};
	//emplaceDataInArray(text, 8,'q');
	const char registerArray[8] = {'A', 'B', 'C', 'D', 'E', 'H', 'L', 'P'};
	for(int i = 0; i < 8; ++i){
		uint8_t offset = i * 5 + 22;
		text[offset] = registerArray[i];
		text[offset + 1] = ':';
		emplaceDataInArray(text, offset+2, system.fetchReg(registerArray[i]));
		text[offset+4]=' ';
	}
	text[22+5*8] = '\0';
//   text[24]='A';text[25]=':';emplaceDataInArray(text, 26, system.fetchReg('A'));text[28]=' ';
//   text[29]='B';text[30]=':';emplaceDataInArray(text, 31, system.fetchReg('B'));text[33]=' ';
//   text[34]='C';text[35]=':';emplaceDataInArray(text, 28, system.fetchReg('C'));text[38]=' ';
//   text[39]='D';text[40]=':';emplaceDataInArray(text, 33, system.fetchReg('D'));text[43]=' ';
//   text[44]='E';text[45]=':';emplaceDataInArray(text, 38, system.fetchReg('E'));text[48]=' ';
//   text[49]='H';text[50]=':';emplaceDataInArray(text, 43, system.fetchReg('H'));text[53]=' ';
//   text[54]='L';text[55]=':';emplaceDataInArray(text, 48, system.fetchReg('L'));text[58]=' ';
//   text[59]='P';text[60]=':';emplaceDataInArray(text, 53, system.fetchReg('P'));text[63]='\0';


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


	SDL_RenderPresent(gRenderer);
	
	/*
	drawEnd = std::chrono::high_resolution_clock::now();
	drawTime = std::chrono::duration_cast<std::chrono::duration<double>>(drawEnd - drawStart);
	printf("Draw time: %f ms\n", (1000 * drawTime.count()) );
	*/
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
