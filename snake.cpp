/* 
    ----- Main logic of snake game (pseudocode) ------
while (game is running) {
   take input;
   update snake position;
   check for collisions;
   handle food eating;
   redraw screen;
} */

#include <iostream>
#include <deque>
#include <vector>
#include <conio.h>  // for getting input from keyboard (_kbhit, _getch)
#include <windows.h> // for coordinate system, how to navigate to specific coordinate on screen & Sleep, SetConsoleCursorPosition, SetConsoleTextAttribute
#include <ctime>
#include <cstdlib>
#include <functional>

using namespace std;

const int WIDTH = 40;
const int HEIGHT = 20;

const char SNAKE_HEAD = 'O';
const char SNAKE_BODY = 'o';
const char FOOD_CHAR = '*';
const char SPECIAL_FOOD_CHAR = '@';
const char OBSTACLE_CHAR = '#';
const char EMPTY_CHAR = ' ';

// enum classes for better type safety, we have to use prefix before using the members. eg. Direction::STOP
enum class Direction { STOP, UP, DOWN, LEFT, RIGHT };
enum class Difficulty { EASY, MEDIUM, HARD };

// Initializing position of points and comparing position by operator overloading
struct Position {
    int x, y;
    Position(int x_ = 0, int y_ = 0) : x(x_), y(y_) {}
    bool operator==(const Position& rhs) const { return x == rhs.x && y == rhs.y; }
    bool operator!=(const Position& rhs) const { return !(*this == rhs); }
};

// here we are giving control to user via console
void gotoXY(int x, int y) {
    static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hConsole, pos);
}

// here we are hiding the blinking cursor
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
    void reset() {
        body.clear();
        int startX = WIDTH / 2;
        int startY = HEIGHT / 2;
        for (int i = 0; i < 3; ++i)
            body.push_back(Position(startX - i, startY));
        dir = Direction::RIGHT;
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
        if (checkCollision(head)) return false; // collision with self
        body.push_front(head);
        if (!grow) body.pop_back();
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
    void spawn(const Snake &snake, const vector<Position> &obstacles, bool forceNormal = false,
               const function<int()> &random = rand) {
        special = (!forceNormal) && (random() % 100 < 15); // 15% chance
        spawnTime = clock();
        while (true) {
            pos.x = random() % WIDTH;
            pos.y = random() % HEIGHT;
            if (snake.checkCollision(pos)) continue;
            bool collide = false;
            for (auto& o : obstacles) {
                if (o == pos) {
                    collide = true;
                    break;
                }
            }
            if (!collide) break;
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
    Food food;
    vector<Position> obstacles;
    Difficulty difficulty = Difficulty::EASY;
    int score = 0, highscore = 0;
    int baseSpeed = 200; // milliseconds; easy
    int obstacleCount = 0;
    bool running = true;

    int lastObstacleScore = 0; // cooldown for obstacle spawning
    
    HANDLE hConsole;
    function<int()> random; // seam: every random draw goes through here

public:
    explicit Game(function<int()> random_ = rand) : random(random_) {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    int getScore() const { return score; }
    const Snake& getSnake() const { return snake; }

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
            if (frameTime < (DWORD)baseSpeed) Sleep(baseSpeed - frameTime);
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
            baseSpeed = 200; // easy speed
            obstacleCount = 4;
            break;
        case 2:
            difficulty = Difficulty::MEDIUM;
            baseSpeed = 100; // medium speed
            obstacleCount = 6;
            break;
        case 3:
            difficulty = Difficulty::HARD;
            baseSpeed = 80; // hard speed
            obstacleCount = 12;
            break;
        default:
            difficulty = Difficulty::EASY;
            baseSpeed = 200;
            obstacleCount = 4;
        }
    }

public:
    void reset() {
        snake.reset();
        score = 0;
        obstacles.clear();
        spawnObstacles(obstacleCount);
        food.spawn(snake, obstacles, false, random);
        running = true;
        lastObstacleScore = 0;
    }

private:
    void drawBorder() {
        system("cls");
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // white for border
        gotoXY(0, 0);
        cout << "+";
        for (int i = 0; i < WIDTH; i++) cout << "-";
        cout << "+\n";
        for (int y = 0; y < HEIGHT; ++y) {
            gotoXY(0, y + 1);
            cout << "|";
            for (int x = 0; x < WIDTH; ++x) cout << " ";
            cout << "|";
        }
        gotoXY(0, HEIGHT + 1);
        cout << "+";
        for (int i = 0; i < WIDTH; ++i) cout << "-";
        cout << "+\n";
    }

    void spawnObstacles(int count) {
        while ((int)obstacles.size() < count) {
            Position pos(random() % WIDTH, random() % HEIGHT);
            if (!snake.checkCollision(pos) && pos != food.pos && !isOccupied(pos, obstacles)) {
                obstacles.push_back(pos);
            }
        }
    }

    bool isOccupied(Position pos, const vector<Position>& positions) {
        for (auto& p : positions) if (p == pos) return true;
        return false;
    }

    void handleInput() {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 224 || ch == 0) {
                ch = _getch();
                switch (ch) {
                case 72: snake.setDirection(Direction::UP); break;
                case 80: snake.setDirection(Direction::DOWN); break;
                case 75: snake.setDirection(Direction::LEFT); break;
                case 77: snake.setDirection(Direction::RIGHT); break;
                }
            }
            else {
                switch (ch) {
                case 'w': case 'W': snake.setDirection(Direction::UP); break;
                case 's': case 'S': snake.setDirection(Direction::DOWN); break;
                case 'a': case 'A': snake.setDirection(Direction::LEFT); break;
                case 'd': case 'D': snake.setDirection(Direction::RIGHT); break;
                case 'q': case 'Q': running = false; break;
                }
            }
        }
    }

public:
    void updateGame() {
        Position nextHead = snake.getHead();
        switch (snake.getDirection()) {
        case Direction::UP: nextHead.y--; break;
        case Direction::DOWN: nextHead.y++; break;
        case Direction::LEFT: nextHead.x--; break;
        case Direction::RIGHT: nextHead.x++; break;
        default: break;
        }

        if (nextHead.x < 0 || nextHead.x >= WIDTH || nextHead.y < 0 || nextHead.y >= HEIGHT) {
            running = false;
            return;
        }
        if (snake.checkCollision(nextHead)) {
            running = false;
            return;
        }
        for (auto& o : obstacles) {
            if (o == nextHead) {
                running = false;
                return;
            }
        }

        bool grow = false;
        if (nextHead == food.pos) {
            score += food.value();
            grow = true;
            food.spawn(snake, obstacles, false, random);
        }

        if (food.expired()) {
            food.spawn(snake, obstacles, true, random);
        }

        if (!snake.move(grow)) {
            running = false;
        }

        int milestone = score / 50;
        if (milestone > 0 && milestone > lastObstacleScore) {
            lastObstacleScore = milestone;
            int toAdd = 0;
            switch (difficulty) {
            case Difficulty::EASY: toAdd = 1; break;
            case Difficulty::MEDIUM: toAdd = 2; break;
            case Difficulty::HARD: toAdd = 3; break;
            }
            spawnObstacles((int)obstacles.size() + toAdd);
        }
    }

private:
    void draw() {
        static bool firstDraw = true;
        if (firstDraw) {
            drawBorder();
            firstDraw = false;
        }

        // draw everything inside board
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                char ch = EMPTY_CHAR;
                WORD colorCode = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // default white

                // obstacles
                for (auto& o : obstacles) {
                    if (o.x == x && o.y == y) {
                        ch = OBSTACLE_CHAR;
                        colorCode = FOREGROUND_INTENSITY; // gray
                        break;
                    }
                }

                const auto& body = snake.getBody();
                for (auto it = body.begin(); it != body.end(); ++it) {
                    if (it->x == x && it->y == y) {
                        if (it == body.begin()) {
                            ch = SNAKE_HEAD;
                            colorCode = FOREGROUND_GREEN | FOREGROUND_INTENSITY; // bright green head
                        }
                        else {
                            ch = SNAKE_BODY;
                            colorCode = FOREGROUND_GREEN; // green body
                        }
                        break;
                    }
                }

                if (food.pos.x == x && food.pos.y == y) {
                    ch = food.special ? SPECIAL_FOOD_CHAR : FOOD_CHAR;
                    colorCode = food.special ? (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY) : FOREGROUND_RED | FOREGROUND_INTENSITY; // special food yellow, normal food bright red
                }

                gotoXY(x + 1, y + 1); // +1 for left border
                SetConsoleTextAttribute(hConsole, colorCode);
                cout << ch;
            }
        }

        const WORD scoreColor = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;    // bright cyan
        const WORD highscoreColor = FOREGROUND_RED | FOREGROUND_INTENSITY;                   // bright red
        const WORD levelColor = FOREGROUND_GREEN | FOREGROUND_INTENSITY;                     // bright green

        gotoXY(0, HEIGHT + 3);
        SetConsoleTextAttribute(hConsole, scoreColor);
        cout << "Score: " << score << "    ";

        SetConsoleTextAttribute(hConsole, highscoreColor);
        cout << "High Score: " << highscore << "    ";

        SetConsoleTextAttribute(hConsole, levelColor);
        cout << "Level: " << levelName() << "    ";

        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // reset to white
    }

    string levelName() {
        switch (difficulty) {
        case Difficulty::EASY: return "Easy";
        case Difficulty::MEDIUM: return "Medium";
        case Difficulty::HARD: return "Hard";
        default: return "Unknown";
        }
    }

    void gameOverScreen() {
        if (score > highscore) highscore = score;
        system("cls");
        cout << "-------------> GAME OVER <-------------\n";
        cout << "Better luck next time.";
        cout << "Final Score: " << score << "\n";
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
