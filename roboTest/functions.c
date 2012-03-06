#include "header.h"

//****************************//
// GAME CONTROL FUNCTIONS //
//****************************//

// Handle button presses and analog positioning:
void delayInput() { for (i = 0; i <= 10; i++) { sceDisplayWaitVblankStart(); } }

void optionControl() {
	if (titleScreen & !gameSetup & !gameRunning) optTotal = 4;
	else if (!titleScreen & gameSetup & !gameRunning) optTotal = 2;
	else if (!titleScreen & !gameSetup & gameRunning & paused) optTotal = 3;

	//* Option Selection:
	if (newInput.Buttons & PSP_CTRL_UP) { if (optSelect > 1) { optSelect--; delayInput(); } }
	else if (newInput.Buttons & PSP_CTRL_DOWN) { if (optSelect < optTotal) { optSelect ++; delayInput(); } }
}

void boostActive() {
	if ((boost > 0) | roaming) {
		boost -= (0.5 - ((boostAdder * 2) / stamina)); refreshRate = 3;
		switch(spriteRow) {
			case 1: spriteRow = 5; y -= (spriteSpeed * boostSpeed); break;
			case 2: spriteRow = 6; y += (spriteSpeed * boostSpeed); break;
			case 3: spriteRow = 7; x -= (spriteSpeed * boostSpeed); break;
			case 4: spriteRow = 8; x += (spriteSpeed * boostSpeed); break;
		}
	} else if (boost <= 0) {
		boost = 0; refreshRate = 6;
		switch(spriteRow) {
			case 5: spriteRow = 1; break;
			case 6: spriteRow = 2; break;
			case 7: spriteRow = 3; break;
			case 8: spriteRow = 4; break;
		}
	}
}

void boostInactive() {
	refreshRate = 6;
	if (boost < boostTotal) boost += (0.25 + ((boostAdder * 5) / stamina)); else if (boost > boostTotal) boost = boostTotal;
	switch(spriteRow) {
		case 5: spriteRow = 1; break;
		case 6: spriteRow = 2; break;
		case 7: spriteRow = 3; break;
		case 8: spriteRow = 4; break;
	}
}

void handleInput() {
	sceCtrlReadBufferPositive(&newInput, 1);

	if (titleScreen & !gameSetup & !gameRunning) {
		optionControl();
		if (newInput.Buttons & PSP_CTRL_CROSS) {
			switch (optSelect) {
				case 1: gameSetup = 1; titleScreen = 0; break;
				case 2: printTextScreen(5, 10, "OPTIONS - UNDER CONSTRUCTION", red); break;
				case 3: printTextScreen(5, 10, "CREDITS - UNDER CONSTRUCTION", red); break;
				case 4: gameExit = 1; break;
			}
			delayInput();
		}
	} else if (!titleScreen & gameSetup & !gameRunning) {
		optionControl();
		if (newInput.Buttons & PSP_CTRL_LEFT) {
			if ((optSelect == 1) & (charSelect > 1)) { charSelect--; delayInput(); } else if ((optSelect == 2) & (lvlSelect > 1)) { lvlSelect--; delayInput(); }
		} else if (newInput.Buttons & PSP_CTRL_RIGHT) {
			if ((optSelect == 1) & (charSelect < 3)) { charSelect++; delayInput(); } else if ((optSelect == 2) & (lvlSelect < 3)) { lvlSelect++; delayInput(); }
		}
		//* Select character:
		switch (charSelect) {
			case 1: playerSprite = roboGreen; break;
			case 2: playerSprite = dracoBlue; break;
			case 3: playerSprite = tankBrown; break;
		}
		//* Select level:
		switch (lvlSelect) {
			case 1: bgTile = brickTile; break;
			case 2: bgTile = grassTile; break;
			case 3: bgTile = waterTile; break;
		}
		if (newInput.Buttons & PSP_CTRL_CROSS) {
			//* Start game:
			boostTotal = boostStart + (stamina /(boostAdder * 2.5)); boost = boostTotal;
			energyTotal = energyStart + (stamina /(energyAdder * 10)); energy = energyTotal;
			idleTime = 0; x = 229; y = 122; gameRunning = 1; gameSetup = 0;
			delayInput();
		}
	} else if (!titleScreen & !gameSetup & gameRunning) {
		//* Pause or unpause game and pause options:
		if (newInput.Buttons & PSP_CTRL_START) { if (paused) { paused = 0; delayInput(); } else { optSelect = 1; paused = 1; delayInput(); }; }
		if (paused) {
			optionControl();
			if (newInput.Buttons & PSP_CTRL_CROSS) {
				switch (optSelect) {
					case 1: paused = 0; break;
					case 2: spriteRow = 2; titleScreen = 1; gameRunning = 0; paused = 0; break;
					case 3: gameExit = 1; break;
				}
				delayInput();
			}
		} else {
			//* Basic d-Pad movement: (Disabled for better use)
			//*if (newInput.Buttons & PSP_CTRL_UP) { spriteRow = 1;  y--; } else if (newInput.Buttons & PSP_CTRL_DOWN) { spriteRow = 2; y++; }
			//*if (newInput.Buttons & PSP_CTRL_LEFT) { spriteRow = 3;  x--; } else if (newInput.Buttons & PSP_CTRL_RIGHT) { spriteRow = 4;  x++; }

			//* 3-Speed Variable analog movement:
			if (newInput.Ly < 100) { idleTime = 0; spriteRow = 1; y -= spriteSpeed; if (newInput.Ly < 55) { y -= spriteSpeed; if (newInput.Ly < 10) { y -= spriteSpeed; }; }; }
			else if (newInput.Ly > 155) { idleTime = 0; spriteRow = 2; y += spriteSpeed; if (newInput.Ly > 210) { y += spriteSpeed; if (newInput.Ly > 245) { y += spriteSpeed; }; }; }
			else { idleTime++; }
			if (newInput.Lx < 100) { idleTime = 0; spriteRow = 3; x -= spriteSpeed; if (newInput.Lx < 55) { x -= spriteSpeed; if (newInput.Lx < 10) { x -= spriteSpeed; }; }; } 
			else if (newInput.Lx > 155) { idleTime = 0; spriteRow = 4; x += spriteSpeed; if (newInput.Lx > 210) { x += spriteSpeed; if (newInput.Lx > 245) { x += spriteSpeed; }; }; }
			else { idleTime++; }

			//* Speed boost:
			if (newInput.Buttons & PSP_CTRL_CROSS) { idleTime = 0; boostActive(); } else { boostInactive(); }
		}
	}
}

//****************************//
//    GAME-PLAY FUNCTIONS      //
//****************************//

//* Tile background image to the screen:
void drawBackground() {
	for (bgX = 0; bgX <= 480; bgX += 32 ) { for (bgY = 0; bgY <= 272; bgY += 32 ) { blitAlphaImageToScreen(0, 0, 32, 32, bgTile, bgX, bgY); } }
}

//* Handle sprite animation and selection:
void animateSprites() {
	if (i > refreshRate) i = 0; else i++;
	if (i == refreshRate) { if (forward) frame++; else frame--; }
	if (frame == frameStart) forward = 1; else if (frame == frameEnd) forward = 0;
	if (frame == 1) { spriteX = spriteXpadding; } else { spriteX = (spriteXpadding * frame) + (spriteW * (frame - 1)); }
	spriteY = (spriteYpadding * spriteRow) + (spriteH * (spriteRow - 1));
}

//* Display game options:
void showOptions() {
	if (titleScreen & !gameSetup & !gameRunning) {
		printTextScreen(191, 232, "START", black); if (optSelect == 1) printTextScreen(190, 231, "START!", red); else printTextScreen(190, 231, "START", white);
		printTextScreen(181, 242, "OPTIONS", black); if (optSelect == 2) printTextScreen(180, 241, "OPTIONS", red); else printTextScreen(180, 241, "OPTIONS", white);
		printTextScreen(181, 252, "CREDITS", black); if (optSelect == 3) printTextScreen(180, 251, "CREDITS", red); else printTextScreen(180, 251, "CREDITS", white);
		printTextScreen(191, 262, "EXIT", black); if (optSelect == 4) printTextScreen(190, 261, "EXIT", red); else printTextScreen(190, 261, "EXIT", white);
	} else if (!titleScreen & gameSetup & !gameRunning) {
		drawScreenRect(black, 198, 153, 150, 50); fillScreenRect(white50, 199, 154, 149, 49);
		sprintf(stringBuffer, "Sprite: %s", spriteNames[charSelect]);
		printTextScreen(201, 156, stringBuffer, black); if (optSelect == 1) printTextScreen(200, 155, stringBuffer, red); else printTextScreen(200, 155, stringBuffer, white);
		sprintf(stringBuffer, "Level: %s", levelNames[lvlSelect]);
		printTextScreen(201, 166, stringBuffer, black); if (optSelect == 2) printTextScreen(200, 165, stringBuffer, red); else printTextScreen(200, 165, stringBuffer, white);
	} else if (!titleScreen & !gameSetup & gameRunning & paused) {
		drawScreenRect(black, 170, 127, 140, 75); fillScreenRect(light_gray, 171, 128, 139, 74);
		printTextScreen(221, 132, "PAUSED", black); printTextScreen(220, 131, "PAUSED", yellow);
		printTextScreen(218, 152, "UNPAUSE", black); if (optSelect == 1) printTextScreen(217, 151, "UNPAUSE", red); else printTextScreen(217, 151, "UNPAUSE", white);
		printTextScreen(229, 162, "QUIT", black); if (optSelect == 2) printTextScreen(228, 161, "QUIT", red); else printTextScreen(228, 161, "QUIT", white);
		printTextScreen(229, 172, "EXIT", black); if (optSelect == 3) printTextScreen(228, 171, "EXIT", red); else printTextScreen(228, 171, "EXIT", white);

		switch (optSelect) {
			case 1: sprintf(optionText, "Continue Playing"); break;
			case 2: sprintf(optionText, "Save and Quit"); break;
			case 3: sprintf(optionText, "Exit to eLoader"); break;
		}
		optTextX = 240 - ((strlen(optionText) * 8) / 2);
		printTextScreen(optTextX, 192, optionText, black); printTextScreen(optTextX, 191, optionText, orange);
	}
}

//* Handle sprite energy:
void checkEnergy() {
	if ((energy > 1) & (i == refreshRate)) energy -= 0.01;
	else if (energy <= 1) energy = 1;
}

//* Show player vital stats:
void showVitals() {
	drawScreenRect(black, 2, 2, (energyTotal + 5), 13); fillScreenRect(white50, 3, 3, (energyTotal + 4), 12);
	fillScreenRect(red, 5, 5, energyTotal, 5); fillScreenRect(green, 5, 5, energy, 5);
	fillScreenRect(yellow, 5, 11, boostTotal, 2); fillScreenRect(cyan, 5, 11, boost, 2);
}

//* Show game information (Temporary debug only):
void tempShowStats() {
	sprintf(stringBuffer, "(%d, %d) %d", aiAction, aiRoamSpeed, aiActionTime); printTextScreen(5, 232, stringBuffer, black);
	sprintf(stringBuffer, "(%d, %d)", x, y); printTextScreen(5, 240, stringBuffer, black);
	sprintf(stringBuffer, "%0.2f", energy); printTextScreen(5, 248, stringBuffer, black);
	sprintf(stringBuffer, "%0.2f", boost); printTextScreen(5, 256, stringBuffer, black);
	sprintf(stringBuffer, "%d", idleTime); printTextScreen(5, 264, stringBuffer, black);
}

//* AI: Roam around the screen:
void aiRoamChange() {
	aiAction = random(8);
	aiRoamSpeed = random(spriteSpeed * boostSpeed); if (aiRoamSpeed == 0) aiRoamSpeed = 1;
	aiActionTime = idleTime + random(1000);
}

void aiRoam() {
	roaming = 1;
	if (idleTime >= aiActionTime) { aiRoamChange(); }
	if (aiRoamSpeed >= (spriteSpeed * boostSpeed)) boostActive(); else boostInactive();
	switch (aiAction) {
		case 0: spriteRow = 1; y -= aiRoamSpeed; break; //* Up
		case 1: spriteRow = 2; y += aiRoamSpeed; break; //* Down
		case 2: spriteRow = 3; x -= aiRoamSpeed; break; //* Left
		case 3: spriteRow = 4; x += aiRoamSpeed; break; //* Right
		case 4: spriteRow = 3; x -= aiRoamSpeed; y -= aiRoamSpeed; break; //* Up-Left
		case 5: spriteRow = 3; x -= aiRoamSpeed; y += aiRoamSpeed; break; //* Down-Left
		case 6: spriteRow = 4; x += aiRoamSpeed; y += aiRoamSpeed; break; //* Down-Right
		case 7: spriteRow = 4; x += aiRoamSpeed; y -= aiRoamSpeed; break; //* Up-Right
		case 8: idleTime++; break; //* Nothing
	}
}

//* Check battery level:
void checkBattLevel() {
	if (scePowerIsBatteryExist()) {
		battLife = scePowerGetBatteryLifeTime();
		sprintf(stringBuffer, "Charge: %d%% (%02dh%02dm)", scePowerGetBatteryLifePercent(), battLife/60, battLife-(battLife/60*60));
		printTextScreen(5, 260, stringBuffer, black);
	}
}

//* Handle screen boundaries:
void checkBoundaries() {
	if (x + spriteW > 480) { x = 480 - spriteW; if (roaming) { aiRoamChange(); }; }
	else if (x < 0){ x = 0; if (roaming) { aiRoamChange(); }; }
	if (y + spriteH > 272) { y = 272 - spriteH; if (roaming) { aiRoamChange(); }; }
	else if (y < 0){ y = 0; if (roaming) { aiRoamChange(); }; }
	
}

//* Display "Game Over" screen and exit game:
void exitGame() {
	clearScreen(black); printTextScreen(220, 131, "Good Bye!", white); flipScreen();
	sceKernelExitGame(); sceKernelSleepThread();
}

//****************************//
// GAME LOADING FUNCTIONS //
//****************************//

//* Load image files:
void loadImages() {
	//* Load characters:
	roboGreen = loadImage("sprites/roboGreen.png");
	dracoBlue = loadImage("sprites/dracoBlue.png");
	tankBrown = loadImage("sprites/tankBrown.png");
	playerSprite = roboGreen;

	//* Load levels:
	brickTile = loadImage("levels/brick/tile.png");
	grassTile = loadImage("levels/grass/tile.png");
	waterTile = loadImage("levels/water/tile.png");
	bgTile = grassTile;

	//* Load objects:
	woodSign = loadImage("objects/sign.png");
	treeStump = loadImage("objects/stump.png");
}

//* Initialize Game:
void initializeGame() {
	//* Initialize standard game functions:
	scePowerSetClockFrequency(333, 333, 166);
	SetupCallbacks(); pspDebugScreenInit(); initGraphics(); loadImages(); aiRoamChange();
	sceCtrlSetSamplingCycle(0); sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

//* Check image load failures:
int checkErrors() { if (!roboGreen | !dracoBlue | !tankBrown | !brickTile | !grassTile | !waterTile) return 0; else return 1; }
void showErrors() {
	if (!roboGreen) printf("roboSprite load failed!\n");
	if (!dracoBlue) printf("dracoSprite load failed!\n");
	if (!tankBrown) printf("tankSprite load failed!\n");
	if (!brickTile) printf("brickTile load failed!\n");
	if (!grassTile) printf("grassTile load failed!\n");
	if (!waterTile) printf("waterTile load failed!\n");
}

//* Get Free Ram:
u32 ramAvailableLineareMax (void) { 
	u32 size, sizeblock; u8 *ram;  size = 0; sizeblock = RAM_BLOCK; 
	while (sizeblock) {
		size += sizeblock; ram = malloc(size);
		if (!(ram)) { size -= sizeblock; sizeblock >>= 1; } else { free(ram); }
	}
	return size; 
} 

u32 ramAvailable (void) {
	u8 **ram, **temp;
	u32 size, count, x;
	ram = NULL; size = 0; count = 0; 

	for (;;) {
		if (!(count % 10)) { 
			temp = realloc(ram,sizeof(u8 *) * (count + 10)); if (!(temp)) break;
			ram = temp; size += (sizeof(u8 *) * 10);
		}

		x = ramAvailableLineareMax(); if (!(x)) break;
		ram[count] = malloc(x); if (!(ram[count])) break; 
		size += x; count++; 
	}

	if (ram) { for (x=0;x<count;x++) free(ram[x]); free(ram); } 
	return size; 
}
