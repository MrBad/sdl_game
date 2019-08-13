#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <time.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define  WINDOW_HEIGHT 600
#define DLT 4
#define NUM_STARS 10

#define IMG_STAR_PATH "imgs/star.png"
#define IMG_MAN_PATH "imgs/man.png"

typedef enum {
	DIRECTION_DOWN = 0,
	DIRECTION_LEFT = 1,
	DIRECTION_UP = 2,
	DIRECTION_RIGHT = 3,
} directionType;

typedef struct {
	int x, y, w, h;
	bool walking; // is man walking?
	directionType direction; // in which direction
	short int life;
	char *name;
} Man;

typedef struct {
	int x, y, w, h;
} Star;

typedef struct {
	Man man;
	Star stars[NUM_STARS];
	SDL_Rect manFrames[24];
	SDL_Texture *starTexture;
	SDL_Texture *manTexture;
	SDL_Renderer *renderer;
	int numFrames;
	int manFrame;
} GameState;

typedef enum {
	COLLIDE_NONE = 1 << 0,
	COLLIDE_TOP = 1 << 1,
	COLLIDE_BOTTOM = 1 << 2,
	COLLIDE_LEFT = 1 << 3,
	COLLIDE_RIGHT = 1 << 4,
} collideType;


static SDL_Texture *loadTexture(GameState *game, const char *path)
{
	SDL_Surface *surface = IMG_Load(path);
	SDL_Texture *texture = NULL;
	if (surface == NULL) {
		fprintf(stderr, "Cannot load %s: %s\n", path, IMG_GetError());
		return NULL;
	}
	SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));
	texture = SDL_CreateTextureFromSurface(game->renderer, surface);
	if (!texture) {
		fprintf(stderr, "Cannot create texture for %s: %s\n", path, SDL_GetError());
		return NULL;
	}
	SDL_FreeSurface(surface);
	return texture;
}

bool loadGame(GameState *game)
{
	int i;
	game->starTexture = loadTexture(game, IMG_STAR_PATH);
	game->manTexture = loadTexture(game, IMG_MAN_PATH);
	game->manFrame = 0;

	// create man sprites
	for (i = 0; i < 24; i++) {
		SDL_Rect *r = &game->manFrames[i];
		r->w = 32;
		r->h = 64;
		r->x = (i % 6) * r->w;
		r->y = (i / 6) * r->h;
	}
	// create stars
	for (i = 0; i < NUM_STARS; i++) {
		game->stars[i].w = game->stars[i].h = 64;
		game->stars[i].x = rand() % (WINDOW_WIDTH - game->stars[i].w);
		game->stars[i].y = rand() % (WINDOW_HEIGHT - game->stars[i].h);
	}
	game->numFrames = 0;
	return true;
}

collideType checkCollision(SDL_Rect *a, SDL_Rect *b)
{

	// Minkowsky sum
	double w = (a->w + b->w) / 2;
	double h = (a->h + b->h) / 2;
	int dx = (a->x + a->w / 2) - (b->x + b->w / 2); // aCenterX - bCenterX;
	int dy = (a->y + a->h / 2) - (b->y + b->h / 2); // aCenterY - bCenterY;

	if (abs(dx) <= w && abs(dy) <= h) {
		// collision!!!
		double wy = w * dy;
		double hx = h * dx;

		if (wy > hx) {
			if (wy > -hx) {
				return COLLIDE_TOP;
			} else {
				return COLLIDE_RIGHT;
			}
		} else {
			if (wy > -hx) {
				return COLLIDE_LEFT;
			} else {
				return COLLIDE_BOTTOM;
			}
		}
	}
	return COLLIDE_NONE;
}

bool processEvents(GameState *game)
{
	const Uint8 *kbState;
	SDL_Event e;
	Man *man;
	bool gameOver = false;

	while (SDL_PollEvent(&e) != 0) {
		if (e.type == SDL_QUIT) {
			gameOver = true;
		} else if (e.type == SDL_KEYDOWN) {
			if (e.key.keysym.sym == SDLK_ESCAPE) {
				gameOver = true;
			}
			game->man.walking = true;
			switch (e.key.keysym.sym) {
				case SDLK_LEFT:
					game->man.direction = DIRECTION_LEFT;
					break;
				case SDLK_UP:
					game->man.direction = DIRECTION_UP;
					break;
				case SDLK_RIGHT:
					game->man.direction = DIRECTION_RIGHT;
					break;
				case SDLK_DOWN:
					game->man.direction = DIRECTION_DOWN;
					break;
				default:
					game->man.direction = DIRECTION_RIGHT;
			}
		} else if(e.type == SDL_KEYUP) {
			game->man.walking = false;
		}
	}

	// shortcut
	man = &game->man;

	kbState = SDL_GetKeyboardState(NULL);
	if (kbState[SDL_SCANCODE_LEFT]) {
		if (man->x >= DLT) man->x -= DLT;
	} else if (kbState[SDL_SCANCODE_RIGHT]) {
		if (man->x <= WINDOW_WIDTH - DLT - man->w) man->x += DLT;
	} else if (kbState[SDL_SCANCODE_UP]) {
		if (man->y >= DLT) man->y -= DLT;
	} else if (kbState[SDL_SCANCODE_DOWN]) {
		if (man->y <= WINDOW_HEIGHT - DLT - man->h) man->y += DLT;
	}

	// stars collision detection //
	for (int i = 0; i < NUM_STARS; i++) {
		SDL_Rect m = {man->x, man->y, man->w, man->h};
		SDL_Rect s = {game->stars[i].x, game->stars[i].y, game->stars[i].w, game->stars[i].h};
		collideType clt = checkCollision(&m, &s);
		if (clt != COLLIDE_NONE) {
			printf("Game Over\n");
			switch (clt) {
				case COLLIDE_TOP:
					printf("top\n");
					break;
				case COLLIDE_BOTTOM:
					printf("bottom\n");
					break;
				case COLLIDE_LEFT:
					printf("left\n");
					break;
				case COLLIDE_RIGHT:
					printf("right\n");
					break;
				default:
					printf("Donno\n");
			}
			gameOver = true;
		}
	}
	return gameOver;
}

void renderGame(GameState *game)
{
	SDL_Rect rect, frame;
	SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(game->renderer);

	SDL_SetRenderDrawColor(game->renderer, 0x66, 0xFF, 0x66, 0xFF);

	// Show man sprite //
	//rect = {game->man.x, game->man.y, game->man.w, game->man.h};
	rect.x = game->man.x; rect.y = game->man.y; rect.w = game->man.w; rect.h = game->man.h;
	if(game->man.walking)
		frame = game->manFrames[6 * game->man.direction + game->manFrame];
	else {
		frame = game->manFrames[6 * game->man.direction];
	}
	SDL_RenderCopy(game->renderer, game->manTexture, &frame, &rect);

	// Show stars
	for (int i = 0; i < NUM_STARS; i++) {
		//rect = {game->stars[i].x, game->stars[i].y, 64, 64};
		rect.x = game->stars[i].x; rect.y = game->stars[i].y; rect.h = rect.w = 64;
		SDL_RenderCopy(game->renderer, game->starTexture, NULL, &rect);
	}
	SDL_RenderPresent(game->renderer);
	game->numFrames++;
	if (game->numFrames % 5 == 0) {
		game->manFrame++;
		if (game->manFrame == 6) game->manFrame = 1;
	}

}

int main()
{
	SDL_Window *window;
	bool gameOver = false;
	GameState game;

	game.man.x = game.man.y = 0;
	game.man.w = 32;
	game.man.h = 64;
	game.man.direction = DIRECTION_RIGHT;
	game.man.walking = false;

	srand((unsigned int) time(NULL));

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Cannot sdl init: %s\n", SDL_GetError());
		return 1;
	}
	if (!(window = SDL_CreateWindow(
			"Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN))) {
		fprintf(stderr, "Cannot create window: %s\n", SDL_GetError());
		return 1;
	}
	game.renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (game.renderer == NULL) {
		fprintf(stderr, "Cannot create renderer:%s\n", SDL_GetError());
		return 1;
	}

	if (!loadGame(&game)) {
		return 1;
	}

	while (!gameOver) {

		gameOver = processEvents(&game);

		renderGame(&game);
	}
	SDL_Delay(1000);

	SDL_DestroyTexture(game.starTexture);
	SDL_DestroyRenderer(game.renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
