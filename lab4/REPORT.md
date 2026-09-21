# Lab 4 — Group ___

| | |
|---|---|
| Repository | https://github.com/Hardikk69/Snake-Game-CLI (own snake game) |
| Base tag | `lab4-base` at commit `0bc4078` |
| Pull request | ___ |

Build and run the tests (MinGW g++, from the repo root):

```
g++ --coverage -O0 -g tests/test_rules.cpp -o test_rules && ./test_rules
g++ --coverage -O0 -g tests/test_score_seam.cpp -o test_score_seam && ./test_score_seam
```

`snake.cpp` has no header and defines `main`, so each test file does
`#define main snake_main` and `#include "../snake.cpp"`. That is a new test file, not a
change to the game.

---

## 1. Five rules — [5]

| # | Rule |
|---|---|
| 1 | The snake dies when its head enters its own body. |
| 2 | Pressing the direction opposite to the current one is ignored — the snake cannot reverse into itself. |
| 3 | The game ends when the snake's head moves off the 40×20 board. |
| 4 | Eating food makes the snake one segment longer and adds exactly 10 to the score (30 for special `@` food). |
| 5 | Special food disappears after 5 seconds; normal food never disappears. |

All five could be stated from how the game plays (and they match the README's
"Gameplay" section). The one I could not state precisely without looking was *when*
obstacles are added — the README table says "per 30 pts" in one place and "every 50 points"
in another — so I left it out.

---

## 2. What you could test, and what stopped you — [10]

No source changes in this part. Every `file:line` below is a line in `lab4-base`.
Tests: [tests/test_rules.cpp](tests/test_rules.cpp), built against `git show lab4-base:snake.cpp`.

| # | Rule | Test written? | Blocking dependency (`file:line` + what it is) |
|---|---|---|---|
| 1 | Head into body kills | **Yes** — `test_head_into_body_kills_snake` | None for the rule itself: `Snake` is public and `Snake::move` (snake.cpp:90) returns `false` on self-collision at snake.cpp:99. |
| 2 | No reversing | **Yes** — `test_opposite_direction_is_ignored` | None: `Snake::setDirection` (snake.cpp:74-80) is public and deterministic. |
| 3 | Wall ends the game | **No** | snake.cpp:294-296 — the bounds check lives in `Game::updateGame`, which is `private` (section starts snake.cpp:188) and sets the `private` flag `running` (snake.cpp:157). The only public way in is `Game::run` (snake.cpp:168), which blocks on `cin >> choice` (snake.cpp:193), polls the keyboard with `_kbhit()/_getch()` (snake.cpp:261-262), sleeps every frame with `Sleep()` (snake.cpp:183), and after death blocks forever on `_getch()` (snake.cpp:420). Even if a tick could be driven, `spawnObstacles` places obstacles with unseeded-by-test `rand()` (snake.cpp:248), so an obstacle may sit in the path and kill the snake before the wall. |
| 4 | Eating: +1 length, +10 / +30 | **No** | snake.cpp:311 — `score += food.value()` is inside `private` `Game::updateGame` and `score` is `private` (snake.cpp:154), with the same `run()` blockers as rule 3 (snake.cpp:193, 261-262, 183, 420). And the food is placed by `rand() % WIDTH` / `rand() % HEIGHT` (snake.cpp:125-126), special-ness by `rand() % 100 < 15` (snake.cpp:122): a test cannot put food in front of the head. `Food::value()` (snake.cpp:144) is public, but asserting 10/30 on it tests a constant, not that eating adds it. |
| 5 | Special food expires after 5 s | **Yes, partly** — `test_special_food_expires_after_five_seconds` | `Food`'s fields are public, so the test sets `special = true` and back-dates `spawnTime` instead of waiting. What is still blocked: `Food::expired` reads the real clock with `clock()` (snake.cpp:140), so the boundary (4.999 s vs 5.000 s) cannot be pinned, and whether spawned food is special comes from `rand()` (snake.cpp:122). |

> **Rules testable without modifying the source: 3 / 5** (rule 5 only by back-dating a public field against the real clock)

Rules 1 and 2 are testable because they live in `Snake`, which has no I/O, no randomness
and no clock. Everything decided inside `Game` is behind `private` plus a game loop that
reads the keyboard, sleeps and draws.

---

## 3. Coverage, and what it missed — [6]

| | |
|---|---|
| Line coverage | 17.20 % of 279 lines of `snake.cpp` |
| Branch coverage | 14.72 % taken at least once (17.75 % executed) of 231 branches |
| Command used | `g++ --coverage -O0 -g tests/test_rules.cpp -o test_rules && ./test_rules && gcov -b test_rules-test_rules.gcno` (on `lab4-base`; `snake.cpp` is included by the test, so it is compiled into the same object) |

**One rule that is executed by the suite but not verified by it:**

| | |
|---|---|
| Rule | 4 — eating makes the snake one segment longer |
| Line that runs | snake.cpp:101 — `if (!grow) body.pop_back();` runs 5 times; the `grow == true` branch (tail kept) is taken 4 times, by the four `s.move(true)` calls in `test_head_into_body_kills_snake` (tests/test_rules.cpp:11-13) |
| The assertion that is missing | `CHECK(s.getBody().size() == 5, ...)` after the two `move(true)` calls. The test uses growth to build a long enough snake, but only asserts the final `move` returns `false`; if `move(true)` stopped growing the snake, the setup would silently change and nothing would say growth broke. |

Coverage says the growth path is 80 % taken. Nothing checks it.

---

## 4. The seam — [10]

| | |
|---|---|
| Rule made testable | 4 — eating food adds 10 (30 for special) and grows the snake by one |
| Commit 1 (seam) | `7bf9e9e` Introduce random-number seam in Game |
| Commit 2 (test) | (the commit after `7bf9e9e`) Add tests for rule 4 through the seam, Part B tests and report |
| Seam kind | **object** |
| Enabling point | The `Game` constructor: `explicit Game(function<int()> random_ = rand)`. `main()` still writes `Game game;` and gets `rand`; a test writes `Game g(stub)`. Every draw in `Food::spawn` and `Game::spawnObstacles` now goes through that member instead of calling `rand()` directly. |
| What production code gave up | (1) **Encapsulation**: `reset()` and `updateGame()` moved from `private` to `public`, and `getScore()` / `getSnake()` were added, so any caller can now tick the game or reset it mid-game — the class no longer guarantees that only `run()` drives it. (2) **A guarantee about randomness**: `Game` used to be certain its numbers came from `rand()`. Now a caller can hand it any `function<int()>`; a constant one makes `Food::spawn`'s `while (true)` retry loop (snake.cpp:126 after the seam) spin forever if that cell is taken. (3) A tiny cost: each draw is an indirect call through `std::function` instead of a direct `rand()`, irrelevant next to a 80–200 ms frame. |

Rule 4 was the hardest in Part B: it has all of rule 3's blockers (private, `cin`, `_getch`,
`Sleep`) *plus* two random draws that decide where the food is and whether it is special.
One seam — the random source — plus making the tick reachable removes all of them for a
single `updateGame()` call: the test never calls `run()`, so `cin`, `_getch` and `Sleep`
are never reached, and with `gatherDifficulty()` not called `obstacleCount` stays 0.

Commit 1 changes no behaviour: with the default argument every call site still draws from
`rand()` in the same order, seeded by the same `srand(time(nullptr))` in `run()`.

---

## 5. The double — [4]

| | |
|---|---|
| What you passed through the seam | **stub** |
| The method under test | `Game::updateGame()` (via `Food::spawn`) |

The collaborator — the random source — is only ever **asked a question** ("give me the next
number") and its answer steers the code; it is never told to do anything, so there is no
call to verify and nothing to assert on it, which makes it a stub (canned answers:
`scripted({99, 21, 10, 99, 0, 0})`) rather than a spy or mock. The test checks the
consequence of those answers through `getScore()` and the snake's length, not the
interaction with the double.

---

## 6. Two smells in your own tests — [5]

| | Smell | `file:line` | One-line fix |
|---|---|---|---|
| 1 | **Mystery guest** — `scripted({99, 21, 10, 99, 0, 0})` only means "normal food at (21,10), then food at (0,0)" if you know `Food::spawn` draws special-roll, x, y in that order, that `Snake::reset` starts at (20,10) heading right, and that `obstacleCount` is 0 because `gatherDifficulty()` was never called. None of that is visible in the test; a comment (lines 20-22) is standing in for it. | tests/test_score_seam.cpp:25 (same at :32, :39, :46) | Name it: a helper `foodAt(x, y, special)` that returns the draw triple, and place the food at `g.getSnake().getHead()` + one cell to the right instead of the literal `21, 10`. |
| 2 | **Eager test** — one function checks three behaviours: fresh special food is not expired, 5-second-old special food is expired, normal food never expires. If the last `CHECK` fails you learn "expiry broke", not which half of rule 5. | tests/test_rules.cpp:28-37 | Keep the shared `Food f` arrangement, split the three acts into three test functions. |

Also found: the same test is **resource optimism** — it uses the real `clock()`
(tests/test_rules.cpp:31, :33) and assumes the machine is fast enough that no real time
passes between the two reads; on a stalled CI box the "fresh" check could pass the 5 s
line. Fix: put a clock behind the same kind of seam as the random source.
