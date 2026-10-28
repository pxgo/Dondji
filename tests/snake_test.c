#include "../App/app/snake.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void CheckState(const SnakeState_t *state)
{
    bool seen[SNAKE_CAPACITY] = {false};
    assert(state->tail < SNAKE_CAPACITY);
    assert(state->length >= 1 && state->length <= SNAKE_CAPACITY);
    assert(state->direction <= SNAKE_LEFT);
    assert(state->nextDirection <= SNAKE_LEFT);
    for (uint8_t i = 0; i < state->length; ++i) {
        uint8_t cell = SNAKE_CellAt(state, i);
        assert(cell < SNAKE_CAPACITY);
        assert(!seen[cell]);
        seen[cell] = true;
    }
    if (state->status == SNAKE_WON) {
        assert(state->length == SNAKE_CAPACITY);
        assert(state->food == UINT8_MAX);
    } else {
        assert(state->food < SNAKE_CAPACITY);
        assert(!seen[state->food]);
    }
}

static SnakeState_t MakeState(const uint8_t *cells, uint8_t length,
                             uint8_t direction, uint8_t tail)
{
    SnakeState_t state = {0};
    state.length = length;
    state.direction = direction;
    state.nextDirection = direction;
    state.tail = tail;
    state.food = 200;
    state.status = SNAKE_RUNNING;
    for (uint16_t i = 0; i < length; ++i)
        state.body[((uint16_t)tail + i) % SNAKE_CAPACITY] = cells[i];
    return state;
}

static void TestInitAndFood(void)
{
    for (uint32_t seed = 0; seed <= UINT16_MAX; ++seed) {
        SnakeState_t state;
        SNAKE_Init(&state, (uint16_t)seed);
        assert(state.length == 3 && state.tail == 0 && state.score == 0);
        assert(state.direction == SNAKE_RIGHT);
        assert(state.nextDirection == SNAKE_RIGHT && !state.turnPending);
        assert(state.status == SNAKE_RUNNING);
        assert(SNAKE_CellAt(&state, 0) == 130);
        assert(SNAKE_CellAt(&state, 1) == 131);
        assert(SNAKE_CellAt(&state, 2) == 132);
        CheckState(&state);

        state.food = 133;
        SNAKE_Step(&state);
        assert(state.length == 4 && state.score == 1 && state.tail == 0);
        assert(SNAKE_CellAt(&state, 3) == 133);
        CheckState(&state);
    }
    SnakeState_t a, b;
    SNAKE_Init(&a, 0);
    SNAKE_Init(&b, 0);
    assert(a.food == b.food && a.random == b.random);
}

static void TestWalls(void)
{
    static const uint8_t cells[4][3] = {
        {48, 24, 0}, {21, 22, 23}, {168, 192, 216}, {26, 25, 24}
    };
    for (uint8_t direction = SNAKE_UP; direction <= SNAKE_LEFT; ++direction) {
        SnakeState_t state = MakeState(cells[direction], 3, direction, 239);

        uint8_t head = SNAKE_CellAt(&state, 2);
        SNAKE_Step(&state);
        assert(state.status == SNAKE_LOST);
        assert(state.length == 3 && state.tail == 239 && state.score == 0);
        assert(SNAKE_CellAt(&state, 2) == head);
        CheckState(&state);
    }
}

static void TestBodyAndOldTail(void)
{
    static const uint8_t collision[] = {25, 26, 27, 51, 50};
    SnakeState_t state = MakeState(collision, 5, SNAKE_UP, 238);
    SNAKE_Step(&state);
    assert(state.status == SNAKE_LOST && state.tail == 238);
    assert(SNAKE_CellAt(&state, 4) == 50);
    CheckState(&state);

    static const uint8_t square[] = {25, 26, 50, 49};
    state = MakeState(square, 4, SNAKE_UP, 239);
    SNAKE_Step(&state);
    assert(state.status == SNAKE_RUNNING && state.tail == 0);
    assert(state.length == 4 && state.score == 0);
    assert(SNAKE_CellAt(&state, 0) == 26);
    assert(SNAKE_CellAt(&state, 3) == 25);
    CheckState(&state);
}

static void TestTurns(void)
{
    SnakeState_t state;
    SNAKE_Init(&state, 0);
    state.food = 0;
    assert(!SNAKE_Turn(&state, SNAKE_LEFT));
    assert(!SNAKE_Turn(&state, SNAKE_RIGHT));
    assert(!SNAKE_Turn(&state, 4));
    assert(!SNAKE_Turn(&state, UINT8_MAX));
    assert(!state.turnPending);
    assert(SNAKE_Turn(&state, SNAKE_UP));
    assert(state.direction == SNAKE_RIGHT && state.nextDirection == SNAKE_UP);
    assert(!SNAKE_Turn(&state, SNAKE_LEFT));
    assert(!SNAKE_Turn(&state, SNAKE_DOWN));
    assert(!SNAKE_Turn(&state, SNAKE_UP));
    assert(state.nextDirection == SNAKE_UP && state.turnPending);
    SNAKE_Step(&state);
    assert(state.direction == SNAKE_UP && !state.turnPending);
    assert(SNAKE_CellAt(&state, 2) == 108);
    assert(!SNAKE_Turn(&state, SNAKE_DOWN));
    assert(!SNAKE_Turn(&state, SNAKE_UP));
    assert(SNAKE_Turn(&state, SNAKE_LEFT));
    SNAKE_Step(&state);
    assert(state.direction == SNAKE_LEFT && !state.turnPending);
    assert(SNAKE_CellAt(&state, 2) == 107);
    CheckState(&state);
}

static void TestInactive(void)
{
    static const uint8_t statuses[] = {SNAKE_PAUSED, SNAKE_LOST, SNAKE_WON};
    for (uint8_t i = 0; i < sizeof(statuses) / sizeof(statuses[0]); ++i) {
        SnakeState_t state;
        SNAKE_Init(&state, 0);
        assert(SNAKE_Turn(&state, SNAKE_UP));
        state.status = statuses[i];
        SnakeState_t before;
        memcpy(&before, &state, sizeof(state));
        assert(!SNAKE_Turn(&state, SNAKE_DOWN));
        SNAKE_Step(&state);
        assert(memcmp(&before, &state, sizeof(state)) == 0);
        if (state.status == SNAKE_PAUSED) {
            state.status = SNAKE_RUNNING;
            SNAKE_Step(&state);
            assert(state.direction == SNAKE_UP && !state.turnPending);
            assert(SNAKE_CellAt(&state, 2) == 108);
        }
    }
}

/* Hamiltonian cycle: top row, serpentine inner rows, then the left column. */
static uint8_t CycleCell(uint16_t index)
{
    if (index < SNAKE_WIDTH)
        return (uint8_t)index;
    index = (uint16_t)(index - SNAKE_WIDTH);
    if (index < (SNAKE_HEIGHT - 1u) * (SNAKE_WIDTH - 1u)) {
        uint16_t y = (uint16_t)(1u + index / (SNAKE_WIDTH - 1u));
        uint16_t offset = (uint16_t)(index % (SNAKE_WIDTH - 1u));
        uint16_t x = (uint16_t)((y & 1u) ? SNAKE_WIDTH - 1u - offset : 1u + offset);
        return (uint8_t)(y * SNAKE_WIDTH + x);
    }
    index = (uint16_t)(index - (SNAKE_HEIGHT - 1u) * (SNAKE_WIDTH - 1u));
    return (uint8_t)((SNAKE_HEIGHT - 1u - index) * SNAKE_WIDTH);
}

static uint8_t DirectionTo(uint8_t from, uint8_t to)
{
    if ((uint16_t)from + SNAKE_WIDTH == to)
        return SNAKE_DOWN;
    if ((uint16_t)to + SNAKE_WIDTH == from)
        return SNAKE_UP;
    if ((uint16_t)from + 1u == to) {
        assert(from / SNAKE_WIDTH == to / SNAKE_WIDTH);
        return SNAKE_RIGHT;
    }
    assert((uint16_t)to + 1u == from);
    assert(from / SNAKE_WIDTH == to / SNAKE_WIDTH);
    return SNAKE_LEFT;
}

static void TestFullBoard(void)
{
    uint8_t cells[SNAKE_CAPACITY];
    for (uint16_t i = 0; i < SNAKE_CAPACITY - 1u; ++i)
        cells[i] = CycleCell((uint16_t)(i + 1u));
    SnakeState_t state = MakeState(cells, 239, SNAKE_UP, 239);
    state.food = 0;
    state.score = 236;
    CheckState(&state);
    SNAKE_Step(&state);
    assert(state.status == SNAKE_WON && state.length == 240);
    assert(state.score == 237 && state.tail == 239);
    for (uint16_t i = 0; i < SNAKE_CAPACITY; ++i)
        assert(SNAKE_CellAt(&state, (uint8_t)i)
               == CycleCell((uint16_t)((i + 1u) % SNAKE_CAPACITY)));
    CheckState(&state);
    SnakeState_t before;
    memcpy(&before, &state, sizeof(state));
    SNAKE_Step(&state);
    assert(!SNAKE_Turn(&state, SNAKE_RIGHT));
    assert(memcmp(&before, &state, sizeof(state)) == 0);
}

static void TestRingWrap(void)
{
    static const uint8_t square[] = {25, 26, 50, 49};
    static const uint8_t turns[] = {SNAKE_UP, SNAKE_RIGHT, SNAKE_DOWN, SNAKE_LEFT};
    SnakeState_t state = MakeState(square, 4, SNAKE_LEFT, 239);
    for (uint16_t step = 0; step < 960; ++step) {
        assert(SNAKE_Turn(&state, turns[step % 4u]));
        SNAKE_Step(&state);
        assert(state.status == SNAKE_RUNNING && state.length == 4);
        assert(state.tail == (239u + step + 1u) % SNAKE_CAPACITY);
        assert(SNAKE_CellAt(&state, 3) == square[step % 4u]);
        CheckState(&state);
    }
    assert(state.tail == 239);
}

static void TestLongGames(void)
{
    static const uint16_t seeds[] = {0, 1, 12345, UINT16_MAX};
    static const uint8_t start[] = {0, 1, 2};
    for (uint8_t s = 0; s < sizeof(seeds) / sizeof(seeds[0]); ++s) {
        SnakeState_t state = MakeState(start, 3, SNAKE_RIGHT, 237);
        state.food = 3;
        state.random = seeds[s];
        uint16_t headIndex = 2;
        uint32_t steps = 0;
        while (state.status == SNAKE_RUNNING && steps < 40000u) {
            uint8_t head = CycleCell(headIndex);
            headIndex = (uint16_t)((headIndex + 1u) % SNAKE_CAPACITY);
            uint8_t next = CycleCell(headIndex);
            uint8_t direction = DirectionTo(head, next);
            bool accepted = SNAKE_Turn(&state, direction);
            assert(accepted == (direction != state.direction));
            uint8_t oldLength = state.length;
            uint16_t oldScore = state.score;
            bool eating = next == state.food;
            SNAKE_Step(&state);
            assert(state.status == SNAKE_RUNNING || state.status == SNAKE_WON);
            assert(state.length == oldLength + (eating ? 1u : 0u));
            assert(state.score == oldScore + (eating ? 1u : 0u));
            assert(SNAKE_CellAt(&state, (uint8_t)(state.length - 1u)) == next);
            CheckState(&state);
            ++steps;
        }
        assert(state.status == SNAKE_WON && state.score == 237);
    }
}

int main(void)
{
    TestInitAndFood();
    TestWalls();
    TestBodyAndOldTail();
    TestTurns();
    TestInactive();
    TestFullBoard();
    TestRingWrap();
    TestLongGames();
    printf("snake tests passed (state: %zu bytes)\n", sizeof(SnakeState_t));
    return 0;
}
