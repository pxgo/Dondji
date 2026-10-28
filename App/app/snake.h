#ifndef APP_SNAKE_H
#define APP_SNAKE_H

#include <stdbool.h>
#include <stdint.h>

#define SNAKE_WIDTH 24u
#define SNAKE_HEIGHT 10u
#define SNAKE_CAPACITY (SNAKE_WIDTH * SNAKE_HEIGHT)

typedef enum {
    SNAKE_UP = 0,
    SNAKE_RIGHT = 1,
    SNAKE_DOWN = 2,
    SNAKE_LEFT = 3
} SnakeDirection_t;

typedef enum {
    SNAKE_RUNNING,
    SNAKE_PAUSED,
    SNAKE_LOST,
    SNAKE_WON
} SnakeStatus_t;

typedef struct {
    uint8_t body[SNAKE_CAPACITY];
    uint8_t tail;
    uint8_t length;
    uint8_t direction;
    uint8_t nextDirection;
    uint8_t food;
    uint8_t status;
    bool turnPending;
    uint16_t score;
    uint16_t random;
} SnakeState_t;

_Static_assert(SNAKE_CAPACITY <= UINT8_MAX, "Snake cells must fit in uint8_t");
_Static_assert(sizeof(SnakeState_t) <= 320u, "Snake state exceeds RAM budget");

void SNAKE_Init(SnakeState_t *state, uint16_t seed);
/* True only for a newly queued turn; same-direction input does not consume it. */
bool SNAKE_Turn(SnakeState_t *state, uint8_t direction);
void SNAKE_Step(SnakeState_t *state);
/* index < length, ordered from tail to head. WON has food == UINT8_MAX. */
uint8_t SNAKE_CellAt(const SnakeState_t *state, uint8_t index);

#endif
