#include "functions.c"

int main() {
	srand(sceKernelLibcTime((void *) 0));
	initializeGame();

	//* Main game loop:
	while (!gameExit) {
		clearScreen(black); handleInput(); checkBoundaries();
		if (!paused) animateSprites();

		if (titleScreen & !gameSetup & !gameRunning) { 
			//* Title menu:
			drawBackground(); showOptions(); tempShowStats(); idleTime++;
			if (idleTime > 420) aiRoam(); else roaming = 0;
			blitAlphaImageToScreen(spriteX, spriteY, spriteW, spriteH, playerSprite, x, y);
		}
		else if (!titleScreen & gameSetup & !gameRunning) {
			//* Game Setup:
			drawBackground(); showOptions();
			blitAlphaImageToScreen(spriteX, spriteY, spriteW, spriteH, playerSprite, 229, 122);
		}
		else if (!titleScreen & !gameSetup & gameRunning) { checkEnergy(); checkBattLevel();
			drawBackground(); showVitals(); tempShowStats();
			if (idleTime > 420) aiRoam(); else roaming = 0;
			blitAlphaImageToScreen(spriteX, spriteY, spriteW, spriteH, playerSprite, x, y);
			if (paused) showOptions();
		}

		if (checkErrors()) showErrors();
		flipScreen(); sceDisplayWaitVblankStart();
	}

	//* Exit to eLoader:
	exitGame(); return 0;
}
