// Part B: tests written against lab4-base without touching snake.cpp.
// snake.cpp has no header, so it is included; its main() is renamed out of the way.
#define main snake_main
#include "../snake.cpp"
#undef main
#include "check.h"

// Rule 1: the snake dies when its head enters its own body.
static void test_head_into_body_kills_snake() {
    Snake s;                                   // body (20,10) (19,10) (18,10), heading RIGHT
    s.move(true); s.move(true);                // grow to 5, head (22,10)
    s.setDirection(Direction::UP);    s.move(true);  // head (22,9)
    s.setDirection(Direction::LEFT);  s.move(true);  // head (21,9)
    s.setDirection(Direction::DOWN);
    CHECK(!s.move(false), "moving head onto (21,10), a body cell, must report death");
}

// Rule 2: pressing the opposite direction does not reverse the snake.
static void test_opposite_direction_is_ignored() {
    Snake s;                                   // heading RIGHT
    s.setDirection(Direction::LEFT);
    CHECK(s.getDirection() == Direction::RIGHT, "LEFT while heading RIGHT must be ignored");
    s.move(false);
    CHECK(s.getHead() == Position(21, 10), "snake must keep moving right");
}

// Rule 5: special food disappears after 5 seconds; normal food never does.
static void test_special_food_expires_after_five_seconds() {
    Food f;
    f.special = true;
    f.spawnTime = clock();
    CHECK(!f.expired(), "freshly spawned special food must not be expired");
    f.spawnTime = clock() - 5 * CLOCKS_PER_SEC;
    CHECK(f.expired(), "special food spawned 5 s ago must be expired");
    f.special = false;
    CHECK(!f.expired(), "normal food must never expire");
}

int main() {
    test_head_into_body_kills_snake();
    test_opposite_direction_is_ignored();
    test_special_food_expires_after_five_seconds();
    return REPORT();
}
