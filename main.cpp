#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<cstdlib>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	1200
#define SCREEN_HEIGHT	800
#define ROAD_WIDTH 600
#define VERGE_WIDTH (SCREEN_WIDTH - ROAD_WIDTH)/2
#define SPEED 200
#define FRIENDLIES_NUMBER 6
#define DESTRUCTION_PENALTY 3;
#define INVINCIBILITY_DURATION 3;

struct Point
{
	int x, y;
};

struct colors
{
	int black, green, red, blue;
};

struct Player
{
	int posX = SCREEN_WIDTH / 2;
	int posY = 3 * SCREEN_HEIGHT / 4;
	SDL_Surface* currentSurface;
	SDL_Surface* playerCar;
	double score = 0, distanceY = 0, SpeedY = SPEED, recentDistanceY = 0;
	double timePenalty;
	double InvincibilityDuration;
};

struct Vehicles
{
	SDL_Surface* surface;
	double posY;
	int posX = 0;
	int speed = 75;
	bool visible = false;
};

struct MovingBackground
{
	SDL_Surface* surface;
	double posY = 0;
	int posX = 0;
};

struct explosionAnimation
{
	bool active = false;
	int posX = 0;
	double posY = 0;
	int state = 0;
	double  time;
	SDL_Surface* explosion[6];
};

struct Game {
	colors colors;
	explosionAnimation animation;
	Player Player;
	SDL_Surface* screen;
	SDL_Surface* charset;
	Vehicles Friendlies[FRIENDLIES_NUMBER];
	Vehicles Enemies[3];
	SDL_Surface* truck, * motor, * enemy3, * friendly1, * friendly2;
	MovingBackground road, vergeLeft, vergeRight;
	SDL_Texture* scrtex;
	SDL_Window* window = NULL;
	SDL_Renderer* renderer;
	int t1, t2, quit = 0, frames = 0, rc;
	double delta, worldTime = 0, fpsTimer = 0, fps = 0;
	bool scrollLeft = 0, scrollRight = 0;
};

// narysowanie napisu txt na powierzchni screen, zaczynajπc od punktu (x, y)
// charset to bitmapa 128x128 zawierajπca znaki
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

void DrawString2(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset, int w, int h) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = w;
	d.h = h;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt úrodka obrazka sprite na ekranie
// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};


void DrawSurface2(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x; // -sprite->w;
	dest.y = y; //-sprite->h;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

void DrawSurface3(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x;
	dest.y = y - sprite->h;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};


// rysowanie linii o d≥ugoúci l w pionie (gdy dx = 0, dy = 1) 
// bπdü poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};


// rysowanie prostokπta o d≥ugoúci bokÛw l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};


bool CheckBitmap(SDL_Surface* bmp, Game Game, const char* fileName) {
	if (bmp == NULL) {
		printf("SDL_LoadBMP(./\n.bmp) error: %s\n", fileName, SDL_GetError());
		SDL_FreeSurface(Game.charset);
		SDL_FreeSurface(Game.screen);
		SDL_DestroyTexture(Game.scrtex);
		SDL_DestroyWindow(Game.window);
		SDL_DestroyRenderer(Game.renderer);
		SDL_Quit();
		return false;
	};
	return true;
}


// This function checks if two rectangles (l1,r1) and (l2, r2) collide with each other and by what side
bool doOverlap(Point l1, Point r1, Point l2, Point r2, bool& front)
{
	if (l1.x == r1.x || l1.y == r1.y || r2.x == l2.x || l2.y == r2.y)
		return false;
	if (l1.x > r2.x || l2.x > r1.x)
		return false;
	if (r1.y < l2.y || r2.y < l1.y)
		return false;
	if (l1.x == r2.x || l2.x == r1.x)
		front = false;
	return true;
}

void CreateFreePosition(int& posX, double& posY, int index, Game Game) {
	int randomX = 300 + (rand() % (601 - Game.Friendlies[index].surface->w));
	int randomY = 0 + (rand() % SCREEN_HEIGHT + 1);
	Point vehicleLeftTop, vehicleRightBottom;
	vehicleLeftTop.x = randomX;
	vehicleLeftTop.y = randomY - Game.Friendlies[index].surface->h;
	vehicleRightBottom.x = randomX + Game.Friendlies[index].surface->w;
	vehicleRightBottom.y = randomY;
	bool front = true;
	for (int i = 0; i < index; i++)
	{
		Point recentLeftTop, recentRightBottom;
		recentLeftTop.x = Game.Friendlies[i].posX;
		recentLeftTop.y = Game.Friendlies[i].posY - Game.Friendlies[i].surface->h - 20;
		recentRightBottom.x = Game.Friendlies[i].posX + Game.Friendlies[i].surface->w;
		recentRightBottom.y = Game.Friendlies[i].posY + 20;
		if (doOverlap(vehicleLeftTop, vehicleRightBottom, recentLeftTop, recentRightBottom, front))
		{
			CreateFreePosition(posX, posY, index, Game);
			return;
		}
	}
	posX = randomX;
	posY = randomY;
	return;
}

//This function creates random bystander cars with random sprites assigned to them
void CreateFriendlies(Game& Game) {
	for (int i = 0; i < FRIENDLIES_NUMBER; i++)
	{
		int random = 1 + rand() % 4;
		if (random == 1) {
			Game.Friendlies[i].surface = Game.friendly1;
		}
		else if (random == 2) {
			Game.Friendlies[i].surface = Game.friendly2;
		}
		else if (random == 3) {
			Game.Friendlies[i].surface = Game.motor;
		}
		else
			Game.Friendlies[i].surface = Game.truck;
		CreateFreePosition(Game.Friendlies[i].posX, Game.Friendlies[i].posY, i, Game);
		int a = 5;
	}
}

bool Load(Game& Game) {
	int w = 0, h = 0;
	Game.charset = SDL_LoadBMP("./cs8x8.bmp");
	if (!CheckBitmap(Game.charset, Game, "charset")) return 0;
	SDL_SetColorKey(Game.charset, true, 0x000000);


	Game.road.surface = SDL_LoadBMP("./road.bmp");
	if (!CheckBitmap(Game.road.surface, Game, "road")) return 0;
	Game.road.posX = (SCREEN_WIDTH - ROAD_WIDTH) / 2;
	Game.road.posY = 0;

	Game.vergeLeft.surface = SDL_LoadBMP("./sideleft2.bmp");
	if (!CheckBitmap(Game.vergeLeft.surface, Game, "sideleft2")) return 0;
	Game.vergeLeft.posX = 0;
	Game.vergeLeft.posY = 0;

	Game.vergeRight.surface = SDL_LoadBMP("./sideright2.bmp");
	if (!CheckBitmap(Game.vergeRight.surface, Game, "sideright2")) return 0;
	Game.vergeRight.posX = SCREEN_WIDTH * 0.75;
	Game.vergeRight.posY = 0;

	Game.Player.playerCar = SDL_LoadBMP("./playercar.bmp");
	if (!CheckBitmap(Game.Player.playerCar, Game, "playercar")) return 0;
	Game.Player.currentSurface = Game.Player.playerCar;

	Game.truck = SDL_LoadBMP("./truck.bmp");
	if (!CheckBitmap(Game.truck, Game, "truck")) return 0;

	Game.motor = SDL_LoadBMP("./motor.bmp");
	if (!CheckBitmap(Game.motor, Game, "motor")) return 0;

	Game.friendly1 = SDL_LoadBMP("./friendlyCar1.bmp");
	if (!CheckBitmap(Game.friendly1, Game, "friendly1")) return 0;

	Game.friendly2 = SDL_LoadBMP("./friendlyCar2.bmp");
	if (!CheckBitmap(Game.friendly2, Game, "friendlyCar2")) return 0;

	Game.animation.explosion[0] = SDL_LoadBMP("./explosion1.bmp");
	if (!CheckBitmap(Game.animation.explosion[0], Game, "explosion1")) return 0;

	Game.animation.explosion[1] = SDL_LoadBMP("./explosion2.bmp");
	if (!CheckBitmap(Game.animation.explosion[1], Game, "explosion2")) return 0;

	Game.animation.explosion[2] = SDL_LoadBMP("./explosion3.bmp");
	if (!CheckBitmap(Game.animation.explosion[2], Game, "explosion3")) return 0;

	Game.animation.explosion[3] = SDL_LoadBMP("./explosion4.bmp");
	if (!CheckBitmap(Game.animation.explosion[3], Game, "explosion4")) return 0;

	Game.animation.explosion[5] = SDL_LoadBMP("./explosion6.bmp");
	if (!CheckBitmap(Game.animation.explosion[5], Game, "explosion6")) return 0;

	Game.animation.explosion[4] = SDL_LoadBMP("./explosion5.bmp");
	if (!CheckBitmap(Game.animation.explosion[4], Game, "explosion5")) return 0;

	CreateFriendlies(Game);

	return 1;
}

bool Init(Game& Game) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return false;
	}
	if (Load(Game) == 0)
		return false;

	Game.rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		&Game.window, &Game.renderer);
	if (Game.rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return false;
	};
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(Game.renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(Game.renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(Game.window, "Spy hunter");

	Game.screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	Game.scrtex = SDL_CreateTexture(Game.renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_ShowCursor(SDL_DISABLE);

	return true;
}

void NewGame(Game& Game)
{
	Game.worldTime = 0;
	Game.Player.score = 0;
	Game.Player.distanceY = 0;
	Game.Player.posX = SCREEN_WIDTH / 2;
	Game.Player.SpeedY = SPEED;
	Game.road.posX = VERGE_WIDTH;
	Game.road.posY = 0;
	Game.vergeLeft.posX = 0;
	Game.vergeLeft.posY = 0;
	Game.vergeRight.posX = VERGE_WIDTH + ROAD_WIDTH;
	Game.vergeRight.posY = 0;
	Game.t1 = SDL_GetTicks();
	Game.t2 = 0;
	for (int i = 0; i < FRIENDLIES_NUMBER; i++) {
		Game.Friendlies[i].visible = false;
	}
	CreateFriendlies(Game);
	Game.animation.active = false;
	Game.animation.state = 0;
	Game.animation.time = 0;
	Game.scrollLeft = 0, Game.scrollRight = 0;
}

int FindFreePosition(Game Game, int index) {
	int randomX = 300 + (rand() % (601 - Game.Friendlies[index].surface->w));
	int posY = 0;
	Point vehicleLeftTop, vehicleRightBottom;
	vehicleLeftTop.x = randomX;
	vehicleLeftTop.y = posY - Game.Friendlies[index].surface->h;
	vehicleRightBottom.x = randomX + Game.Friendlies[index].surface->w;
	vehicleRightBottom.y = posY;
	bool front = true;
	for (int i = 0; i < index; i++)
	{
		if (i == index)
			continue;
		Point recentLeftTop, recentRightBottom;
		recentLeftTop.x = Game.Friendlies[i].posX;
		recentLeftTop.y = Game.Friendlies[i].posY - Game.Friendlies[i].surface->h - 20;
		recentRightBottom.x = Game.Friendlies[i].posX + Game.Friendlies[i].surface->w;
		recentRightBottom.y = Game.Friendlies[i].posY + 20;
		if (doOverlap(vehicleLeftTop, vehicleRightBottom, recentLeftTop, recentRightBottom, front))
		{
			return FindFreePosition(Game, index);;
		}
	}
	return randomX;
}

//This function checks if anything has happen to vehicles driving down the road
void FriendlyCheck(Game& Game, Vehicles& vehicle, int index) {
	if ((vehicle.visible == true) && (vehicle.posY > SCREEN_HEIGHT + vehicle.surface->w))
	{
		vehicle.visible = false;
		return;
	}
	else if ((vehicle.visible == true) && ((vehicle.posX < VERGE_WIDTH - vehicle.surface->w) || vehicle.posX > SCREEN_WIDTH - VERGE_WIDTH))
	{
		Game.animation.active = true;
		Game.animation.posX = vehicle.posX;
		Game.animation.posY = vehicle.posY;
		Game.animation.time = SDL_GetTicks() * 0.001;
		vehicle.visible = false;
		Game.Player.timePenalty = DESTRUCTION_PENALTY;
	}
	else if ((vehicle.visible == false) && (vehicle.posY > 800) && Game.animation.active == false) {
		vehicle.posY = 0;
		vehicle.posX = FindFreePosition(Game, index);
		vehicle.visible = true;
		DrawSurface3(Game.screen, vehicle.surface, vehicle.posX, vehicle.posY);
		return;
	}
	else if (vehicle.visible == true)
	{
		double distance = Game.delta * vehicle.speed;
		vehicle.posY += distance;
		DrawSurface3(Game.screen, vehicle.surface, vehicle.posX, vehicle.posY);
		return;
	}
	else
	{
		double distance = Game.delta * vehicle.speed;
		vehicle.posY += distance;
		return;
	}
	return;
}

//This function spawns all vehicles except player's
void FriendliesSpawner(Game& Game) {
	for (int i = 0; i < FRIENDLIES_NUMBER; i++)
		FriendlyCheck(Game, Game.Friendlies[i], i);
	return;
}

//This function checks if vehicle that player's car collides with collides with any other vehicle
void CollisionsOthers(Game& Game, int index) {
	Point vehicleLeftTop, vehicleRightBottom;
	bool front = true;
	int move = 0;
	vehicleLeftTop.x = Game.Friendlies[index].posX;
	vehicleLeftTop.y = Game.Friendlies[index].posY - Game.Friendlies[index].surface->h;
	vehicleRightBottom.x = Game.Friendlies[index].posX + Game.Friendlies[index].surface->w;
	vehicleRightBottom.y = Game.Friendlies[index].posY;
	for (int i = 0; i < FRIENDLIES_NUMBER; i++) {
		if ((Game.Friendlies[i].visible == true) && (i != index)) {
			Point recentLeftTop, recentRightBottom;
			recentLeftTop.x = Game.Friendlies[i].posX;
			recentLeftTop.y = Game.Friendlies[i].posY - Game.Friendlies[i].surface->h;
			recentRightBottom.x = Game.Friendlies[i].posX + Game.Friendlies[i].surface->w;
			recentRightBottom.y = Game.Friendlies[i].posY;
			if (doOverlap(vehicleLeftTop, vehicleRightBottom, recentLeftTop, recentRightBottom, front)) {
				if (front == true) {
					Game.animation.active = true;
					Game.animation.posX = vehicleLeftTop.x + (Game.Friendlies[index].surface->w / 2);
					Game.animation.posY = vehicleLeftTop.y + (Game.Friendlies[index].surface->h / 2);
					Game.animation.time = SDL_GetTicks() * 0.001;
					Game.Friendlies[index].visible = false;
					Game.Friendlies[i].visible = false;
					Game.Player.timePenalty = DESTRUCTION_PENALTY;
					return;
				}
				else if ((front == false) && (vehicleLeftTop.x == recentRightBottom.x))
					move = -1;
				else
					move = 1;
				Game.Friendlies[i].posX += move;
			}
		}
	}
	return;
}

void Collisions(Game& Game) {
	Point playerLeftTop, playerRightBottom;
	bool front = true;
	int move = 0;
	playerLeftTop.x = Game.Player.posX - (Game.Player.currentSurface->w / 2);
	playerLeftTop.y = Game.Player.posY - (Game.Player.currentSurface->h / 2);
	playerRightBottom.x = Game.Player.posX + (Game.Player.currentSurface->w / 2);
	playerRightBottom.y = Game.Player.posY + (Game.Player.currentSurface->h / 2);
	for (int i = 0; i < FRIENDLIES_NUMBER; i++) {
		if (Game.Friendlies[i].visible == true) {
			Point vehicleLeftTop, vehicleRightBottom;
			vehicleLeftTop.x = Game.Friendlies[i].posX;
			vehicleLeftTop.y = Game.Friendlies[i].posY - Game.Friendlies[i].surface->h;
			vehicleRightBottom.x = Game.Friendlies[i].posX + Game.Friendlies[i].surface->w;
			vehicleRightBottom.y = Game.Friendlies[i].posY;
			if (doOverlap(playerLeftTop, playerRightBottom, vehicleLeftTop, vehicleRightBottom, front)) {
				if ((front == true) && (Game.Player.InvincibilityDuration <= 0)) {
					Game.animation.active = true;
					Game.animation.posX = Game.Player.posX;
					Game.animation.posY = Game.Player.posY;
					Game.animation.time = SDL_GetTicks() * 0.001;
					Game.Player.currentSurface = NULL;
					Game.Friendlies[i].visible = false;
					Game.Player.SpeedY = 0;
					return;
				}
				else if ((front == false) && (playerLeftTop.x == vehicleRightBottom.x))
					move = -1;
				else
					move = 1;
				Game.Friendlies[i].posX += move;
				CollisionsOthers(Game, i);
			}
		}
	}
	return;
}

void CheckCollisions(Game& Game) {
	if (Game.Player.currentSurface == NULL)
		return;
	int index = 0;
	int move = 0;
	Collisions(Game);
}

//This function does the animation of explosion
void Explosion(Game& Game) {
	if (Game.animation.active == true)
	{
		if (Game.animation.state <= 5)
		{
			DrawSurface(Game.screen, Game.animation.explosion[Game.animation.state], Game.animation.posX, Game.animation.posY);
			if ((Game.animation.state == 5) && (Game.Player.currentSurface == NULL)) {
				Game.Player.SpeedY = SPEED;
				Game.Player.posX = SCREEN_WIDTH / 2;
				Game.Player.currentSurface = Game.Player.playerCar;
				Game.Player.InvincibilityDuration = INVINCIBILITY_DURATION;
			}
			if (Game.animation.time > 0.150)
			{
				Game.animation.state++;
				Game.animation.time = 0;
			}
			Game.animation.time += Game.delta;
			Game.frames++;
		}
		else if (Game.animation.posY - (Game.animation.explosion[Game.animation.state - 1]->h / 2) <= SCREEN_HEIGHT)
		{
			Game.animation.posY += Game.Player.recentDistanceY;
			DrawSurface(Game.screen, Game.animation.explosion[Game.animation.state - 1], Game.animation.posX, Game.animation.posY);
		}
		else
		{
			Game.animation.active = false;
			Game.animation.state = 0;
		}
	}
	return;
}

void Draw(Game& Game) {
	char text[128];
	if (int(Game.Player.distanceY) % SCREEN_HEIGHT == 0) {
		DrawSurface2(Game.screen, Game.road.surface, Game.road.posX + 150, Game.road.posY);
		DrawSurface2(Game.screen, Game.vergeLeft.surface, Game.vergeLeft.posX, Game.vergeLeft.posY);
		DrawSurface2(Game.screen, Game.vergeRight.surface, Game.vergeRight.posX, Game.vergeRight.posY);
	}
	else {
		double distance = Game.Player.recentDistanceY;
		if (fmod(Game.Player.distanceY, SCREEN_HEIGHT) <= Game.Player.recentDistanceY)
			double distance = fmod(Game.Player.distanceY, SCREEN_HEIGHT);
		DrawSurface2(Game.screen, Game.road.surface, Game.road.posX+ 150, Game.road.posY + distance);
		DrawSurface2(Game.screen, Game.vergeLeft.surface, Game.vergeLeft.posX, Game.vergeLeft.posY + distance);
		DrawSurface2(Game.screen, Game.vergeRight.surface, Game.vergeRight.posX, Game.vergeRight.posY + distance);
		DrawSurface3(Game.screen, Game.road.surface, Game.road.posX + 150, Game.road.posY + distance);
		DrawSurface3(Game.screen, Game.vergeLeft.surface, Game.vergeLeft.posX, Game.vergeLeft.posY + distance);
		DrawSurface3(Game.screen, Game.vergeRight.surface, Game.vergeRight.posX, Game.vergeRight.posY + distance);
		Game.road.posY = fmod(Game.road.posY + distance, SCREEN_HEIGHT);
		Game.vergeLeft.posY = fmod(Game.vergeLeft.posY + distance, SCREEN_HEIGHT);
		Game.vergeRight.posY = fmod(Game.vergeRight.posY + distance, SCREEN_HEIGHT);
	}
	Explosion(Game);
	FriendliesSpawner(Game);
	if (Game.Player.currentSurface != NULL)
	{
		DrawSurface(Game.screen, Game.Player.currentSurface, Game.Player.posX, Game.Player.posY);
	}
	DrawRectangle(Game.screen, 4, 4, SCREEN_WIDTH - 8, 36, Game.colors.red, Game.colors.blue);
	sprintf(text, "Jan Ludwicki  188899, SCORE: %d czas trwania = %.1lf s  %.0lf klatek / s", int(Game.Player.score), Game.worldTime, Game.fps);
	DrawString(Game.screen, Game.screen->w / 2 - strlen(text) * 8 / 2, 10, text, Game.charset);
	sprintf(text, "Esc - leave, \32 - move left, \033 - move right, n - new game, p - pause");
	DrawString(Game.screen, Game.screen->w / 2 - strlen(text) * 8 / 2, 26, text, Game.charset);
	sprintf(text, "abcdefhk");
	DrawString(Game.screen, Game.screen->w - strlen(text) * 8, SCREEN_HEIGHT - 10, text, Game.charset);
	SDL_UpdateTexture(Game.scrtex, NULL, Game.screen->pixels, Game.screen->pitch);
	SDL_RenderClear(Game.renderer);
	SDL_RenderCopy(Game.renderer, Game.scrtex, NULL, NULL);
	SDL_RenderPresent(Game.renderer);
}

void Pause(Game& Game, SDL_Event event) {
	bool leave = false;
	while (!leave)
	{
		SDL_FillRect(Game.screen, NULL, Game.colors.black);
		DrawString(Game.screen, SCREEN_WIDTH / 2 - 32, SCREEN_HEIGHT / 2, "PAUSE", Game.charset);
		SDL_UpdateTexture(Game.scrtex, NULL, Game.screen->pixels, Game.screen->pitch);
		SDL_RenderClear(Game.renderer);
		SDL_RenderCopy(Game.renderer, Game.scrtex, NULL, NULL);
		SDL_RenderPresent(Game.renderer);
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) leave = true, Game.quit = 1;
				else if (event.key.keysym.sym == SDLK_n) leave = true, NewGame(Game);
				else if (event.key.keysym.sym == SDLK_p) leave = true, Game.t1 = SDL_GetTicks();
				break;
			case SDL_QUIT:
				leave = true;
				Game.quit = 1;
				break;
			};
		}
	}
}

void Explode(Game& Game) {
	Game.animation.active = true;
	Game.animation.posX = Game.Player.posX;
	Game.animation.posY = Game.Player.posY;
	Game.animation.time = SDL_GetTicks() * 0.001;
	Game.Player.currentSurface = NULL;
	Game.Player.SpeedY = 0;
	return;
}

void IncrementValues(Game& Game) {
	Game.t2 = SDL_GetTicks();
	Game.delta = (Game.t2 - Game.t1) * 0.001;
	Game.t1 = Game.t2;
	Game.worldTime += Game.delta;
	Game.Player.recentDistanceY = Game.Player.SpeedY * Game.delta;
	Game.Player.distanceY += Game.Player.recentDistanceY;
	if ((Game.Player.SpeedY == 0) && (!Game.animation.active))
	{
		Game.Player.SpeedY = SPEED;
		Game.Player.posX = SCREEN_WIDTH / 2;
		Game.Player.currentSurface = Game.Player.playerCar;
		Game.Player.InvincibilityDuration = INVINCIBILITY_DURATION;
	}
	if (Game.Player.timePenalty > 0)
		Game.Player.timePenalty -= Game.delta;
	if (Game.Player.InvincibilityDuration > 0)
		Game.Player.InvincibilityDuration -= Game.delta;
	if ((Game.Player.posX >= (SCREEN_WIDTH - ROAD_WIDTH) / 2) && (Game.Player.posX <= (SCREEN_WIDTH - ROAD_WIDTH) / 2 + ROAD_WIDTH) && (Game.Player.timePenalty < 0)
		&& (Game.Player.currentSurface != NULL))
		Game.Player.score += Game.delta * 50;
	SDL_FillRect(Game.screen, NULL, Game.colors.black);
	Game.fpsTimer += Game.delta;
	if (Game.fpsTimer > 0.5) {
		Game.fps = Game.frames * 2;
		Game.frames = 0;
		Game.fpsTimer -= 0.5;
	};
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
	Game Game;
	SDL_Event event;

	if (Init(Game) == 0)
		return -1;

	Game.colors.black = SDL_MapRGB(Game.screen->format, 0x00, 0x00, 0x00);
	Game.colors.green = SDL_MapRGB(Game.screen->format, 0x00, 0xFF, 0x00);
	Game.colors.red = SDL_MapRGB(Game.screen->format, 0xFF, 0x00, 0x00);
	Game.colors.blue = SDL_MapRGB(Game.screen->format, 0x11, 0x11, 0xCC);
	Game.t1 = SDL_GetTicks();

	while (!Game.quit) {
		IncrementValues(Game);
		Draw(Game);
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) Game.quit = 1;
				else if (event.key.keysym.sym == SDLK_LEFT) Game.scrollLeft = 1;
				else if (event.key.keysym.sym == SDLK_RIGHT) Game.scrollRight = 1;
				else if (event.key.keysym.sym == SDLK_n) NewGame(Game);
				else if (event.key.keysym.sym == SDLK_p) Pause(Game, event);
				break;
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_LEFT) Game.scrollLeft = 0;
				else if (event.key.keysym.sym == SDLK_RIGHT) Game.scrollRight = 0;
				break;
			case SDL_QUIT:
				Game.quit = 1;
				break;
			};
		};
		if ((Game.scrollRight) && (Game.Player.posX < SCREEN_WIDTH))
		{
			Game.Player.posX++;
			if ((Game.Player.posX == SCREEN_WIDTH) && (Game.Player.InvincibilityDuration <= 0))
			{
				Explode(Game);
			}
		}
		if ((Game.scrollLeft) && (Game.Player.posX > 0))
		{
			Game.Player.posX--;
			if ((Game.Player.posX == 0) && (Game.Player.InvincibilityDuration <= 0))
			{
				Explode(Game);
			}
		}
		Game.frames++;
		CheckCollisions(Game);
	};
	SDL_FreeSurface(Game.charset);
	SDL_FreeSurface(Game.screen);
	SDL_DestroyTexture(Game.scrtex);
	SDL_DestroyRenderer(Game.renderer);
	SDL_DestroyWindow(Game.window);
	SDL_Quit();
	return 0;
};
