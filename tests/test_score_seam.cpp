// Part D: rule 4 through the random-number seam in Game's constructor.
#define main snake_main
#include "../snake.cpp"
#undef main
#include "check.h"
#include <initializer_list>
#include <memory>

// Stub: answers each random() call with the next scripted value, then 0.
static function<int()> scripted(std::initializer_list<int> values) {
    auto queue = std::make_shared<deque<int>>(values);
    return [queue] {
        if (queue->empty()) return 0;
        int v = queue->front();
        queue->pop_front();
        return v;
    };
}

// Draws Food::spawn makes: special roll (<15 is special), then x, then y.
// Snake starts at (20,10) heading RIGHT, so food at (21,10) is eaten on the first tick.
// The second triple is the replacement food, parked out of the way at (0,0).

static void test_normal_food_adds_ten() {
    Game g(scripted({99, 21, 10, 99, 0, 0}));
    g.reset();
    g.updateGame();
    CHECK(g.getScore() == 10, "eating normal food must add exactly 10");
}

static void test_special_food_adds_thirty() {
    Game g(scripted({0, 21, 10, 99, 0, 0}));
    g.reset();
    g.updateGame();
    CHECK(g.getScore() == 30, "eating special food must add exactly 30");
}

static void test_eating_grows_snake_by_one() {
    Game g(scripted({99, 21, 10, 99, 0, 0}));
    g.reset();
    g.updateGame();
    CHECK(g.getSnake().getBody().size() == 4, "snake of length 3 must be 4 after eating");
}

static void test_no_food_no_score() {
    Game g(scripted({99, 0, 0}));  // food far away at (0,0)
    g.reset();
    g.updateGame();
    CHECK(g.getScore() == 0, "moving onto an empty cell must not change the score");
    CHECK(g.getSnake().getBody().size() == 3, "moving onto an empty cell must not grow the snake");
}

int main() {
    test_normal_food_adds_ten();
    test_special_food_adds_thirty();
    test_eating_grows_snake_by_one();
    test_no_food_no_score();
    return REPORT();
}
