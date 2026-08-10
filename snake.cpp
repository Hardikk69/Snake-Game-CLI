/*
    ----- Main logic of snake game (pseudocode) ------
while (game is running) {
   take input;
   update snake position;
   check for collisions;
   handle food eating;
   redraw screen;
}
*/

#include <iostream>
#include <deque>
#include <vector>
#include <conio.h>
#include <windows.h>
#include <ctime>
#include <cstdlib>

using namespace std;

const int WIDTH = 40;
const int HEIGHT = 20;

const char SNAKE_HEAD = 'O';
const char SNAKE_BODY = 'o';
const char FOOD_CHAR = '*';
const char SPECIAL_FOOD_CHAR = '@';
const char OBSTACLE_CHAR = '#';
const char EMPTY_CHAR = ' ';

enum class Direction { STOP, UP, DOWN, LEFT, RIGHT };
enum class Difficulty { EASY, MEDIUM, HARD };

struct Position {
    int x, y;
    Position(int x_ = 0, int y_ = 0) : x(x_), y(y_) {}
    bool operator==(const Position& rhs) const { return x == rhs.x && y == rhs.y; }
    bool operator!=(const Position& rhs) const { return !(*this == rhs); }
};

void gotoXY(int x, int y) {
    static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hConsole, pos);
}

void hideCursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hOut, &cci);
    cci.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cci);
}

class Snake {
    deque<Position> body;
    Direction dir;

public:
    Snake() {
        reset();
    }

    void reset(int startX = WIDTH / 2,
               int startY = HEIGHT / 2,
               Direction startDir = Direction::RIGHT) {
        body.clear();

        for (int i = 0; i < 3; ++i)
            body.push_back(Position(startX - i, startY));

        dir = startDir;
    }

    void setDirection(Direction d) {
        if ((dir == Direction::LEFT && d == Direction::RIGHT) ||
            (dir == Direction::RIGHT && d == Direction::LEFT) ||
            (dir == Direction::UP && d == Direction::DOWN) ||
            (dir == Direction::DOWN && d == Direction::UP)) return;

        dir = d;
    }

    Direction getDirection() const {
        return dir;
    }

    const deque<Position>& getBody() const {
        return body;
    }

    Position getHead() const {
        return body.front();
    }

    bool move(bool grow) {
        Position head = getHead();

        switch (dir) {
        case Direction::UP: head.y--; break;
        case Direction::DOWN: head.y++; break;
        case Direction::LEFT: head.x--; break;
        case Direction::RIGHT: head.x++; break;
        default: break;
        }

        if (checkCollision(head)) return false;

        body.push_front(head);

        if (!grow)
            body.pop_back();

        return true;
    }

    bool checkCollision(const Position &pos) const {
        for (auto& part : body) {
            if (part == pos) return true;
        }

        return false;
    }
};

struct Food {
    Position pos;
    bool special;
    clock_t spawnTime;

    Food() {
        pos = Position(0, 0);
        special = false;
        spawnTime = 0;
    }

    void spawn(const Snake &snake,
               const Snake &snake2,
               const vector<Position> &obstacles,
               bool forceNormal = false) {
        special = (!forceNormal) && (rand() % 100 < 15);
        spawnTime = clock();

        while (true) {
            pos.x = rand() % WIDTH;
            pos.y = rand() % HEIGHT;

            if (snake.checkCollision(pos))
                continue;

            if (snake2.checkCollision(pos))
                continue;

            bool collide = false;

            for (auto& o : obstacles) {
                if (o == pos) {
                    collide = true;
                    break;
                }
            }

            if (!collide)
                break;
        }
    }

    bool expired() const {
        if (!special) return false;

        clock_t now = clock();
        double elapsed = double(now - spawnTime) / CLOCKS_PER_SEC;

        return (elapsed >= 5.0);
    }

    int value() const {
        return special ? 30 : 10;
    }
};

class Game {
    Snake snake;
    Snake snake2;

    Food food;
    vector<Position> obstacles;

    Difficulty difficulty = Difficulty::EASY;

    int score = 0;
    int score2 = 0;
    int highscore = 0;

    int baseSpeed = 200;
    int obstacleCount = 0;

    bool running = true;

    int lastObstacleScore = 0;

    int losingPlayer = 0;

    HANDLE hConsole;

public:
    Game() {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    void run() {
        srand((unsigned)time(nullptr));
        hideCursor();
        gatherDifficulty();
        reset();
        drawBorder();

        while (running) {
            DWORD startTick = GetTickCount();

            handleInput();
            updateGame();
            draw();

            if (!running) break;

            DWORD frameTime = GetTickCount() - startTick;

            if (frameTime < (DWORD)baseSpeed)
                Sleep(baseSpeed - frameTime);
        }

        gameOverScreen();
    }

private:
    void gatherDifficulty() {
        system("cls");

        cout << "Select Difficulty:\n1. Easy\n2. Medium\n3. Hard\nChoose (1-3): ";

        int choice = 1;
        cin >> choice;

        switch (choice) {
        case 1:
            difficulty = Difficulty::EASY;
            baseSpeed = 200;
            obstacleCount = 4;
            break;

        case 2:
            difficulty = Difficulty::MEDIUM;
            baseSpeed = 100;
            obstacleCount = 6;
            break;

        case 3:
            difficulty = Difficulty::HARD;
            baseSpeed = 80;
            obstacleCount = 12;
            break;

        default:
            difficulty = Difficulty::EASY;
            baseSpeed = 200;
            obstacleCount = 4;
        }
    }

    void reset() {
        // Player 1 keeps the existing arrow-key controls.
        snake.reset(WIDTH / 2 - 8, HEIGHT / 2, Direction::RIGHT);

        // Player 2 starts separately and uses W/A/S/D.
        snake2.reset(WIDTH / 2 + 8, HEIGHT / 2, Direction::LEFT);

        score = 0;
        score2 = 0;
        losingPlayer = 0;

        obstacles.clear();

        spawnObstacles(obstacleCount);

        food.spawn(snake, snake2, obstacles);

        running = true;
        lastObstacleScore = 0;
    }

    void drawBorder() {
        system("cls");

        SetConsoleTextAttribute(
            hConsole,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
        );

        gotoXY(0, 0);

        cout << "+";

        for (int i = 0; i < WIDTH; i++)
            cout << "-";

        cout << "+\n";

        for (int y = 0; y < HEIGHT; ++y) {
            gotoXY(0, y + 1);

            cout << "|";

            for (int x = 0; x < WIDTH; ++x)
                cout << " ";

            cout << "|";
        }

        gotoXY(0, HEIGHT + 1);

        cout << "+";

        for (int i = 0; i < WIDTH; ++i)
            cout << "-";

        cout << "+\n";
    }

    void spawnObstacles(int count) {
        while ((int)obstacles.size() < count) {
            Position pos(rand() % WIDTH, rand() % HEIGHT);

            if (!snake.checkCollision(pos) &&
                !snake2.checkCollision(pos) &&
                pos != food.pos &&
                !isOccupied(pos, obstacles)) {

                obstacles.push_back(pos);
            }
        }
    }

    bool isOccupied(Position pos, const vector<Position>& positions) {
        for (auto& p : positions)
            if (p == pos)
                return true;

        return false;
    }

    void handleInput() {
        if (_kbhit()) {
            int ch = _getch();

            if (ch == 224 || ch == 0) {
                ch = _getch();

                // Player 1: existing arrow-key controls
                switch (ch) {
                case 72:
                    snake.setDirection(Direction::UP);
                    break;

                case 80:
                    snake.setDirection(Direction::DOWN);
                    break;

                case 75:
                    snake.setDirection(Direction::LEFT);
                    break;

                case 77:
                    snake.setDirection(Direction::RIGHT);
                    break;
                }
            }
            else {
                // Player 2: W A S D
                switch (ch) {
                case 'w':
                case 'W':
                    snake2.setDirection(Direction::UP);
                    break;

                case 's':
                case 'S':
                    snake2.setDirection(Direction::DOWN);
                    break;

                case 'a':
                case 'A':
                    snake2.setDirection(Direction::LEFT);
                    break;

                case 'd':
                case 'D':
                    snake2.setDirection(Direction::RIGHT);
                    break;

                case 'q':
                case 'Q':
                    running = false;
                    break;
                }
            }
        }
    }

    void updateGame() {
        Position nextHead = snake.getHead();

        switch (snake.getDirection()) {
        case Direction::UP:
            nextHead.y--;
            break;

        case Direction::DOWN:
            nextHead.y++;
            break;

        case Direction::LEFT:
            nextHead.x--;
            break;

        case Direction::RIGHT:
            nextHead.x++;
            break;

        default:
            break;
        }

        Position nextHead2 = snake2.getHead();

        switch (snake2.getDirection()) {
        case Direction::UP:
            nextHead2.y--;
            break;

        case Direction::DOWN:
            nextHead2.y++;
            break;

        case Direction::LEFT:
            nextHead2.x--;
            break;

        case Direction::RIGHT:
            nextHead2.x++;
            break;

        default:
            break;
        }

        // Player 1 hits wall.
        if (nextHead.x < 0 ||
            nextHead.x >= WIDTH ||
            nextHead.y < 0 ||
            nextHead.y >= HEIGHT) {

            running = false;
            losingPlayer = 1;
            return;
        }

        // Player 2 hits wall.
        if (nextHead2.x < 0 ||
            nextHead2.x >= WIDTH ||
            nextHead2.y < 0 ||
            nextHead2.y >= HEIGHT) {

            running = false;
            losingPlayer = 2;
            return;
        }

        // Player 1 hits itself.
        if (snake.checkCollision(nextHead)) {
            running = false;
            losingPlayer = 1;
            return;
        }

        // Player 2 hits itself.
        if (snake2.checkCollision(nextHead2)) {
            running = false;
            losingPlayer = 2;
            return;
        }

        // Player 1 hits Player 2.
        if (snake2.checkCollision(nextHead)) {
            running = false;
            losingPlayer = 1;
            return;
        }

        // Player 2 hits Player 1.
        if (snake.checkCollision(nextHead2)) {
            running = false;
            losingPlayer = 2;
            return;
        }

        // Player 1 hits obstacle.
        for (auto& o : obstacles) {
            if (o == nextHead) {
                running = false;
                losingPlayer = 1;
                return;
            }
        }

        // Player 2 hits obstacle.
        for (auto& o : obstacles) {
            if (o == nextHead2) {
                running = false;
                losingPlayer = 2;
                return;
            }
        }

        bool grow = false;
        bool grow2 = false;

        // Both snakes use the same food.
        // Player 1 gets it first if both somehow reach it in
        // the same update.
        if (nextHead == food.pos) {
            score += food.value();
            grow = true;

            food.spawn(snake, snake2, obstacles);
        }
        else if (nextHead2 == food.pos) {
            score2 += food.value();
            grow2 = true;

            food.spawn(snake, snake2, obstacles);
        }

        if (food.expired()) {
            food.spawn(snake, snake2, obstacles, true);
        }

        if (!snake.move(grow)) {
            running = false;
            losingPlayer = 1;
            return;
        }

        if (!snake2.move(grow2)) {
            running = false;
            losingPlayer = 2;
            return;
        }

        int milestone = score / 50;

        if (milestone > 0 && milestone > lastObstacleScore) {
            lastObstacleScore = milestone;

            int toAdd = 0;

            switch (difficulty) {
            case Difficulty::EASY:
                toAdd = 1;
                break;

            case Difficulty::MEDIUM:
                toAdd = 2;
                break;

            case Difficulty::HARD:
                toAdd = 3;
                break;
            }

            spawnObstacles((int)obstacles.size() + toAdd);
        }
    }

    void draw() {
        static bool firstDraw = true;

        if (firstDraw) {
            drawBorder();
            firstDraw = false;
        }

        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {

                char ch = EMPTY_CHAR;

                WORD colorCode =
                    FOREGROUND_RED |
                    FOREGROUND_GREEN |
                    FOREGROUND_BLUE;

                // obstacles
                for (auto& o : obstacles) {
                    if (o.x == x && o.y == y) {
                        ch = OBSTACLE_CHAR;
                        colorCode = FOREGROUND_INTENSITY;
                        break;
                    }
                }

                // Player 1
                const auto& body = snake.getBody();

                for (auto it = body.begin(); it != body.end(); ++it) {
                    if (it->x == x && it->y == y) {

                        if (it == body.begin()) {
                            ch = SNAKE_HEAD;
                            colorCode =
                                FOREGROUND_GREEN |
                                FOREGROUND_INTENSITY;
                        }
                        else {
                            ch = SNAKE_BODY;
                            colorCode = FOREGROUND_GREEN;
                        }

                        break;
                    }
                }

                // Player 2
                const auto& body2 = snake2.getBody();

                for (auto it = body2.begin(); it != body2.end(); ++it) {
                    if (it->x == x && it->y == y) {

                        if (it == body2.begin()) {
                            ch = SNAKE_HEAD;
                            colorCode =
                                FOREGROUND_BLUE |
                                FOREGROUND_INTENSITY;
                        }
                        else {
                            ch = SNAKE_BODY;
                            colorCode = FOREGROUND_BLUE;
                        }

                        break;
                    }
                }

                // food
                if (food.pos.x == x && food.pos.y == y) {
                    ch = food.special
                        ? SPECIAL_FOOD_CHAR
                        : FOOD_CHAR;

                    colorCode =
                        food.special
                        ? (FOREGROUND_RED |
                           FOREGROUND_GREEN |
                           FOREGROUND_INTENSITY)
                        : (FOREGROUND_RED |
                           FOREGROUND_INTENSITY);
                }

                gotoXY(x + 1, y + 1);

                SetConsoleTextAttribute(hConsole, colorCode);

                cout << ch;
            }
        }

        const WORD scoreColor =
            FOREGROUND_BLUE |
            FOREGROUND_GREEN |
            FOREGROUND_INTENSITY;

        const WORD highscoreColor =
            FOREGROUND_RED |
            FOREGROUND_INTENSITY;

        const WORD levelColor =
            FOREGROUND_GREEN |
            FOREGROUND_INTENSITY;

        gotoXY(0, HEIGHT + 3);

        SetConsoleTextAttribute(hConsole, scoreColor);

        cout << "P1 Score: " << score << "    ";
        cout << "P2 Score: " << score2 << "    ";

        SetConsoleTextAttribute(hConsole, highscoreColor);

        cout << "High Score: " << highscore << "    ";

        SetConsoleTextAttribute(hConsole, levelColor);

        cout << "Level: " << levelName() << "    ";

        SetConsoleTextAttribute(
            hConsole,
            FOREGROUND_RED |
            FOREGROUND_GREEN |
            FOREGROUND_BLUE
        );
    }

    string levelName() {
        switch (difficulty) {
        case Difficulty::EASY:
            return "Easy";

        case Difficulty::MEDIUM:
            return "Medium";

        case Difficulty::HARD:
            return "Hard";

        default:
            return "Unknown";
        }
    }

    void gameOverScreen() {
        if (score > highscore)
            highscore = score;

        system("cls");

        cout << "-------------> GAME OVER <-------------\n";

        cout << "Player " << losingPlayer << " lost.\n\n";

        cout << "Player 1 Score: " << score << "\n";
        cout << "Player 2 Score: " << score2 << "\n";

        cout << "High Score: " << highscore << "\n";

        cout << "Press R to Restart or Q to Quit\n";

        while (true) {
            int ch = _getch();

            if (ch == 'r' || ch == 'R') {
                reset();
                run();
                break;
            }
            else if (ch == 'q' || ch == 'Q') {
                break;
            }
        }
    }
};

int main() {
    Game game;
    game.run();

    return 0;
}
