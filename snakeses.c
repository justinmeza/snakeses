/*
 * snakeses.c
 * Justin J. Meza
 * 2007-2011
 */

/* Use this to build on Windows with wincurses */
#undef WINDOWS

#include <time.h>
#include <stdlib.h>
#ifdef WINDOWS
#include "wincurses.h"
#else
#include <ncurses.h>
#endif

#define CHAR_ARENA ' '
#define CHAR_SNAKE '<'
#define CHAR_FOOD 'o'
#define CHAR_WALL 'X'

#define NUM_FOOD 5
#define NUM_WALL 25


/* For colors */
typedef enum colors {
	COLOR_ARENA = 1,
	COLOR_SNAKE,
	COLOR_FOOD,
	COLOR_WALL,
	COLOR_MESSAGE
};

typedef enum dir {
	NONE,
	UP,
	RIGHT,
	DOWN,
	LEFT
} dir_t;

typedef enum status {
	ALIVE,
	DEAD
} status_t;

typedef enum itemtype {
	ITEM_FOOD,
	ITEM_WALL
} itemtype_t;

typedef struct segment {
	short y, x;
	struct segment_t *prev, *next;
} segment_t;

typedef struct snake {
	dir_t dir;
	status_t status;
	segment_t *head, *tail;
} snake_t;

typedef struct item {
	itemtype_t type;
	short y, x;
	struct item_t *next;
} item_t;

typedef struct arena {
	short top, right, bottom, left;
	item_t *items;
} arena_t;


int add_item(short y, short x, itemtype_t type, arena_t *arena) {
	item_t *cur, *temp;

	temp = (item_t *)malloc(sizeof(item_t));
	temp->y = y;
	temp->x = x;
	temp->type = type;
	temp->next = 0;

	if (arena->items) {
		/* Find the end */
		for (cur = arena->items; cur->next != 0; cur = cur-> next) ;
		cur->next = temp;
	} else {
		arena->items = temp;
	}

	return 1;
}

void remove_item(item_t *item, arena_t *arena) {
	if (item && arena) {
		item_t *cur, *prev;
		for (prev = 0, cur = arena->items; cur != 0; prev = cur, cur = cur->next) {
			if (item->y == cur->y && item->x == cur->x) break;
		}
		if (prev) {
			prev->next = cur->next;
		} else {
			arena->items = cur->next;
		}
		free(item);
	}
}

int add_segment(snake_t *snake) {
	segment_t *newseg = malloc(sizeof(segment_t));
	newseg->y = snake->tail->y;
	newseg->x = snake->tail->x;
	newseg->prev = snake->tail;
	newseg->next = 0;

	snake->tail->next = newseg;
	snake->tail = newseg;
	return 0;
}

void die(snake_t *snake) {
	snake->status = DEAD;
}

int on_snake_head(short y, short x, snake_t *snake) {
	if (y == snake->head->y && x == snake->head->x) return 1;
	return 0;
}

int on_snake_body(short y, short x, snake_t *snake) {
	segment_t *cur;

	for (cur = snake->head->next; cur != snake->tail && cur; cur = cur->next)
		if (y == cur->y && x == cur->x) return 1;

	return 0;
}

int in_bounds(short y, short x, arena_t *arena) {
	return (y < arena->top || x > arena->right ||
			y > arena->bottom || x < arena->left) ? 0 : 1;
}

int can_move(snake_t *snake, arena_t *arena) {
	if (in_bounds(snake->head->y, snake->head->x, arena) &&
			!on_snake_body(snake->head->y, snake->head->x,
			snake))
		return 1;

	return 0;
}

int move_snake(snake_t *snake, arena_t *arena) {
	segment_t *temp = snake->tail;

	/* Move the tail to the head */
	if (snake->head != snake->tail) {
		/* Detach the tail from the end of the snake */
		snake->tail = temp->prev;
		snake->tail->next = 0;

		/* Move it to be the head */
		temp->y = snake->head->y;
		temp->x = snake->head->x;

		temp->prev = 0;
		temp->next = snake->head;
		snake->head->prev = temp;
		snake->head = temp;
	}

	/* Move the head */
	switch (snake->dir) {
		case UP:
			snake->head->y--;
			break;
		case RIGHT:
			snake->head->x++;
			break;
		case DOWN:
			snake->head->y++;
			break;
		case LEFT:
			snake->head->x--;
			break;
	}

	if (!can_move(snake, arena)) die(snake);

	return 0;
}

item_t *on_arena_item(short y, short x, arena_t *arena) {
	item_t *cur;
	for (cur = arena->items; cur != 0; cur = cur->next) {
		if (y == cur->y && x == cur->x) return cur;
	}
	return 0;
}

void newpos(short *y, short *x, snake_t *snake, arena_t *arena) {
	do {
		*y = (rand() % (arena->bottom - arena->top)) + arena->top;
		*x = (rand() % (arena->right - arena->left)) + arena->left;
	} while (on_snake_body(*y, *x, snake) || on_arena_item(*y, *x, arena));
}

void free_snake(snake_t *snake) {
	segment_t *cur, *next;
	for (cur = snake->head; cur != 0; cur = next) {
		next = cur->next;
		free(cur);
	}
	free(snake);
}


int main(void)
{
	arena_t *arena;
	snake_t *snake;
	char *title1 = "SNAKESES";
	char *title2 = "(A SNAKE-like game made with curSES)";
	char *by = "Justin J. Meza";
	char *start = "Press any key to start!";
	char *end = "GAME OVER!";

	srand(time(0));

	initscr();
	if (has_colors()) {
		start_color();
		init_pair(COLOR_ARENA, COLOR_WHITE, COLOR_BLACK);
		init_pair(COLOR_SNAKE, COLOR_GREEN, COLOR_GREEN);
		init_pair(COLOR_FOOD, COLOR_YELLOW, COLOR_YELLOW);
		init_pair(COLOR_WALL, COLOR_RED, COLOR_RED);
		init_pair(COLOR_MESSAGE, COLOR_WHITE, COLOR_RED);
	}
	curs_set(0);
	keypad(stdscr, TRUE);
	noecho();

	/* Create an arena */
	arena = (arena_t *)malloc(sizeof(arena_t));
	arena->top = 0;
	arena->right = COLS - 1;
	arena->bottom = LINES - 1;
	arena->left = 0;
	arena->items = 0;

	/* Set up title screen */
	{
		/* Clear the screen */
		{
			int y, x;
			if (has_colors()) attron(COLOR_PAIR(COLOR_ARENA));
			for (y = arena->top; y <= arena->bottom; y++) {
				for (x = arena->left; x <= arena->right; x++) {
					mvaddch(y, x, CHAR_ARENA);
				}
			}
			if (has_colors()) attroff(COLOR_PAIR(COLOR_ARENA));
		}

		attron(COLOR_PAIR(COLOR_ARENA));
		mvprintw(LINES / 2 - 2, COLS / 2 - (int)strlen(title1) / 2, "%s", title1);
		mvprintw(LINES / 2 - 1, COLS / 2 - (int)strlen(title2) / 2, "%s", title2);
		mvprintw(LINES / 2 - 0, COLS / 2 - (int)strlen(by) / 2, "%s", by);
		attron(A_BOLD);
		mvprintw(LINES / 2 - 1, COLS / 2 - (int)strlen(title2) / 2 + 3, "%s", "SNAKE");
		mvprintw(LINES / 2 - 1, COLS / 2 - (int)strlen(title2) / 2 + (int)strlen(title2) - 4, "%s", "SES");
		attroff(A_BOLD);
		attroff(COLOR_PAIR(COLOR_ARENA));

		attron(COLOR_PAIR(COLOR_MESSAGE));
		mvprintw(LINES / 2 + 3, COLS / 2 - (int)strlen(start) / 2, "%s", start);
		attroff(COLOR_PAIR(COLOR_MESSAGE));

		refresh();
		getch();
		nodelay(stdscr, TRUE);
	}

	/* Create a snake */
	snake = (snake_t *)malloc(sizeof(snake_t));
	snake->dir = NONE;
	snake->status = ALIVE;
	
	/* Create a new segment for the snake's head */
	snake->head = (segment_t *)malloc(sizeof(segment_t));
	snake->head->prev = 0;
	snake->head->next = 0;
	snake->tail = snake->head;

	/* Create walls */
	{
		short y, x, n;

		/* Arena boundary */
		for (y = 0; y < LINES; y++) {
			for (x = 0; x < COLS; x++) {
				if (y == 0 || x == COLS - 1 || y == LINES - 1 || x == 0)
					add_item(y, x, ITEM_WALL, arena);
			}
		}

		for (n = 0; n < NUM_WALL; n++) {
			newpos(&y, &x, snake, arena);
			add_item(y, x, ITEM_WALL, arena);
		}
	}

	/* Create a food item */
	{
		short y, x, n;
		for (n = 0; n < NUM_FOOD; n++) {
			newpos(&y, &x, snake, arena);
			add_item(y, x, ITEM_FOOD, arena);
		}
	}

	newpos(&snake->head->y, &snake->head->x, snake, arena);

	do {
		move_snake(snake, arena);

		/* Check for any items */
		{
			item_t *cur;
			cur = on_arena_item(snake->head->y, snake->head->x, arena);
			if (cur) {
				switch (cur->type) {
					case ITEM_FOOD:
						add_segment(snake);
						remove_item(cur, arena);
						{
							short y, x;
							newpos(&y, &x, snake, arena);
							add_item(y, x, ITEM_FOOD, arena);
						}
						break;
					case ITEM_WALL:
						die(snake);
						break;
				}
			}
		}

		/* Clear the screen */
		{
			int y, x;
			if (has_colors()) attron(COLOR_PAIR(COLOR_ARENA));
			for (y = arena->top; y <= arena->bottom; y++) {
				for (x = arena->left; x <= arena->right; x++) {
					mvaddch(y, x, CHAR_ARENA);
				}
			}
			if (has_colors()) attroff(COLOR_PAIR(COLOR_ARENA));
		}

		/* Draw the snake */
		{
			segment_t *cur = snake->head;
			if (has_colors()) attron(COLOR_PAIR(COLOR_SNAKE));
			do {
				mvaddch(cur->y, cur->x, CHAR_SNAKE);
				cur = cur->next;
			} while (cur);
			if (has_colors()) attroff(COLOR_PAIR(COLOR_SNAKE));
		}

		/* Draw items */
		{
			item_t *cur;
			for (cur = arena->items; cur != 0; cur = cur->next) {
				switch (cur->type) {
					case ITEM_FOOD:
						if (has_colors()) attron(COLOR_PAIR(COLOR_FOOD));
						mvaddch(cur->y, cur->x, CHAR_FOOD);
						if (has_colors()) attroff(COLOR_PAIR(COLOR_FOOD));
						break;
					case ITEM_WALL:
						if (has_colors()) attron(COLOR_PAIR(COLOR_WALL));
						mvaddch(cur->y, cur->x, CHAR_WALL);
						if (has_colors()) attroff(COLOR_PAIR(COLOR_WALL));
						break;
				}
			}
		}

		refresh();

		/* Get input */
		{
			int ch = getch();

			switch (ch) {
				case 'k':
				case 'w':
				case KEY_UP:
					if (snake->dir != DOWN) snake->dir = UP;
					break;
				case 'l': case KEY_RIGHT:
					if (snake->dir != LEFT) snake->dir = RIGHT;
					break;
				case 'j': case KEY_DOWN:
					if (snake->dir != UP) snake->dir = DOWN;
					break;
				case 'h': case KEY_LEFT:
					if (snake->dir != RIGHT) snake->dir = LEFT;
					break;
				case 'q':
					die(snake);
					break;
			}
		}

		/* Apply a delay */
#ifdef WINDOWS
		Sleep(50);
#else
		usleep(50000);
#endif
	} while (snake->status != DEAD);

	free_snake(snake);
	free(arena);

	nodelay(stdscr, FALSE);
	attron(COLOR_PAIR(COLOR_MESSAGE));
	mvprintw(LINES / 2, COLS / 2 - (int)strlen(end) / 2, "%s", end);
	attroff(COLOR_PAIR(COLOR_MESSAGE));
	refresh();
	while(getch() != 'q') ;

	endwin();
	return 0;
}
