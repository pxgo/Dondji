#include "snake.h"

static uint8_t RingIndex(uint16_t index)
{
    if (index >= SNAKE_CAPACITY)
        index = (uint16_t)(index - SNAKE_CAPACITY);
    return (uint8_t)index;
}

uint8_t SNAKE_CellAt(const SnakeState_t *state, uint8_t index)
{
    return state->body[RingIndex((uint16_t)state->tail + index)];
}

static bool Occupied(const SnakeState_t *state, uint8_t cell, uint8_t first)
{
    for (uint8_t i = first; i < state->length; ++i) {
        if (SNAKE_CellAt(state, i) == cell)
            return true;
    }
    return false;
}

static void PlaceFood(SnakeState_t *state)
{
    if (state->length == SNAKE_CAPACITY) {
        state->food = UINT8_MAX;
        state->status = SNAKE_WON;
        return;
    }

    /* Full-period 16-bit LCG; zero is also a valid seed. */
    state->random = (uint16_t)((uint32_t)state->random * 25173u + 13849u);
    uint16_t choice = (uint16_t)(state->random % (SNAKE_CAPACITY - state->length));
    for (uint16_t cell = 0; cell < SNAKE_CAPACITY; ++cell) {
        if (!Occupied(state, (uint8_t)cell, 0)) {
            if (choice == 0) {
                state->food = (uint8_t)cell;
                return;
            }
            --choice;
        }
    }
}

void SNAKE_Init(SnakeState_t *state, uint16_t seed)
{
    *state = (SnakeState_t){0};
    state->length = 3;
    state->direction = SNAKE_RIGHT;
    state->nextDirection = SNAKE_RIGHT;
    state->status = SNAKE_RUNNING;
    state->random = seed;
    for (uint8_t i = 0; i < state->length; ++i)
        state->body[i] = (uint8_t)((SNAKE_HEIGHT / 2u) * SNAKE_WIDTH
                                + SNAKE_WIDTH / 2u - 2u + i);
    PlaceFood(state);
}

bool SNAKE_Turn(SnakeState_t *state, uint8_t direction)
{
    if (state->status != SNAKE_RUNNING || direction > SNAKE_LEFT
        || state->turnPending || direction == state->direction
        || direction == (uint8_t)((state->direction + 2u) % 4u))
        return false;

    state->nextDirection = direction;
    state->turnPending = true;
    return true;
}

void SNAKE_Step(SnakeState_t *state)
{
    if (state->status != SNAKE_RUNNING)
        return;

    if (state->turnPending)
        state->direction = state->nextDirection;
    state->turnPending = false;

    uint8_t head = SNAKE_CellAt(state, (uint8_t)(state->length - 1u));
    uint8_t x = (uint8_t)(head % SNAKE_WIDTH);
    uint8_t y = (uint8_t)(head / SNAKE_WIDTH);
    if ((state->direction == SNAKE_UP && y == 0)
        || (state->direction == SNAKE_RIGHT && x == SNAKE_WIDTH - 1u)
        || (state->direction == SNAKE_DOWN && y == SNAKE_HEIGHT - 1u)
        || (state->direction == SNAKE_LEFT && x == 0)) {
        state->status = SNAKE_LOST;
        return;
    }

    switch (state->direction) {
    case SNAKE_UP:    head = (uint8_t)(head - SNAKE_WIDTH); break;
    case SNAKE_RIGHT: ++head; break;
    case SNAKE_DOWN:  head = (uint8_t)(head + SNAKE_WIDTH); break;
    case SNAKE_LEFT:  --head; break;
    }

    bool grow = head == state->food;
    /* Without growth, the old tail vacates in this same step. */
    if (Occupied(state, head, grow ? 0 : 1)) {
        state->status = SNAKE_LOST;
        return;
    }

    uint8_t insert = RingIndex((uint16_t)state->tail + state->length);
    state->body[insert] = head;
    if (grow) {
        ++state->length;
        ++state->score;
        PlaceFood(state);
    } else {
        state->tail = RingIndex((uint16_t)state->tail + 1u);
    }
}
