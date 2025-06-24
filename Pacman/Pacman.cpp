/* 
#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>
#include <vector>
#include <windows.h> 
#include <climits> 
#include <cstdlib> 
#include <ctime> 
#include <codecvt> 
constexpr auto ROW_MAX = 14;
constexpr auto COL_MAX = 40;
constexpr auto ENEMY_CNT = 5;
using namespace std;

enum class ObjType
{
    Way = 0, Wall, Coin, Enemy, Pacman
};

class Enemy
{
public:
    int x;
    int y;
    char prevChar;
    bool wasFood;
};

class Game
{
private:
    vector<vector<ObjType>> map;
    int pacmanX, pacmanY;
    bool gameOver;
    int score;
    int coinCnt;
    HANDLE hConsole;
    vector<Enemy> enemies;

public:
    static int enemyMoveCnt;
    static int invincibleCnt;

public:
    Game() : pacmanX{ 20 }, pacmanY{ 7 }, gameOver{ false }, score{ 0 }, coinCnt{ 0 }
    {
        map.resize(ROW_MAX, vector<ObjType>(COL_MAX, ObjType::Way)); 
        LoadMapFromFile("mapFile.txt");
        map[pacmanY][pacmanX] = ObjType::Pacman; 
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SpawnEnemies();
    }

    void SpawnEnemy()
    {
        int ex, ey;
        do
        {
            ex = rand() % COL_MAX;
            ey = rand() % ROW_MAX;
        } while (map[ey][ex] != ObjType::Way); // 통로가 아니면 다시 생성
        enemies.push_back({ ex, ey, '0', false });
        map[ey][ex] = ObjType::Enemy;
    }

    void SpawnEnemies()
    {
        enemies.clear();
        for (int i = 0; i < ENEMY_CNT; i++)
        {
            SpawnEnemy();
        }
    }

    void Run()
    {
        while (!gameOver)
        {
            Update();
            Render();

            if (enemyMoveCnt < INT_MAX)
                enemyMoveCnt++;
            else
                enemyMoveCnt = 0;

            if (enemyMoveCnt % 4 == 0) // 적 특정 프레임마다 동작  
            {
                MoveEnemies();
                CheckCollision();
            }

            if (enemyMoveCnt % 100 == 0) // 10초마다 적 추가
            {
                SpawnEnemy();
            }

            RenderUx();
            Sleep(100); // 게임 루프 자체를 지연하여 전체 속도 제어
        }
    }

    void LoadMapFromFile(const string& filename)
    {
        ifstream file(filename);
        if (!file)
        {
            cerr << "맵 파일을 찾을 수 없습니다: " << filename << endl;
            return;
        }

        string line;
        int row = 0;
        while (getline(file, line) && row < ROW_MAX)
        {
            for (int col = 0; col < COL_MAX; col++)
            {
                map[row][col] = static_cast<ObjType>(line[col]); 
                if (map[row][col] == ObjType::Coin) 
                    coinCnt++; 
            }
            row++;
        }
        file.close();
    }

    void Update()
    {
        if (_kbhit())
        {
            char key = _getch();
            int newX = pacmanX, newY = pacmanY;

            switch (key)
            {
            case 'w': newY = max(0, pacmanY - 1); break;
            case 's': newY = min(ROW_MAX - 1, pacmanY + 1); break;
            case 'a': newX = max(0, pacmanX - 1); break;
            case 'd': newX = min(COL_MAX - 1, pacmanX + 1); break;
            case 'q': gameOver = true; return;
            }

            if (map[newY][newX] != ObjType::Wall)
            {
                if (map[newY][newX] == ObjType::Coin)
                {
                    score++;
                    coinCnt--;
                }

                map[pacmanY][pacmanX] = ObjType::Way; 
                pacmanX = newX;
                pacmanY = newY;
                map[pacmanY][pacmanX] = ObjType::Pacman; 

                if (coinCnt == 0)
                {
                    gameOver = true;
                }

                if (score == 20)
                {
                    invincibleCnt = 200; // 20초 (게임 루프당 100ms이므로 200번 실행)
                }
            }
        }
    } 

    void Render()
    {
        COORD cursorPosition = { 0, 0 };
        SetConsoleCursorPosition(hConsole, cursorPosition);

        std::ios_base::sync_with_stdio(false);
        std::cin.tie(nullptr);

        char ch{};

        for (int row = 0; row < ROW_MAX; row++)
        {
            for (int col = 0; col < COL_MAX; col++)
            {
                if (map[row][col] == ObjType::Way)
                {
                    SetConsoleTextAttribute(hConsole, 0);  // 기본 색상
                    ch = ' ';
                }
                if (map[row][col] == ObjType::Wall)
                {
                    SetConsoleTextAttribute(hConsole, 7);  // 기본 색상
                    ch = '#';
                }
                else if (map[row][col] == ObjType::Coin)
                {
                    SetConsoleTextAttribute(hConsole, 14); // 노란색 
                    ch = 'o';
                }
                else if (map[row][col] == ObjType::Enemy)
                {
                    SetConsoleTextAttribute(hConsole, 13); // 마젠타
                    ch = 'M';
                }
                else if (map[row][col] == ObjType::Pacman && invincibleCnt > 0)
                {
                    SetConsoleTextAttribute(hConsole, 9); // 파란색 (무적 모드) 
                    ch = '@';
                }
                else if (map[row][col] == ObjType::Pacman)
                {
                    SetConsoleTextAttribute(hConsole, 10); // 연두색 (기본)
                    ch = '@';
                }
                cout << ch;
                SetConsoleTextAttribute(hConsole, 7);  // 기본 색상
            }
            cout << endl;
        }
        SetConsoleTextAttribute(hConsole, 7);
    }

    void MoveEnemies()
    {
        for (auto& enemy : enemies)
        {
            int direction, newX, newY;
            do
            {
                direction = rand() % 4;
                newX = enemy.x;
                newY = enemy.y;

                switch (direction)
                {
                case 0: newY = max(0, enemy.y - 1); break; // 위로 이동
                case 1: newY = min(ROW_MAX - 1, enemy.y + 1); break; // 아래로 이동
                case 2: newX = max(0, enemy.x - 1); break; // 왼쪽 이동
                case 3: newX = min(COL_MAX - 1, enemy.x + 1); break; // 오른쪽 이동
                }

            } while (map[newY][newX] == ObjType::Wall); // 새 위치가 벽이면 이동할 위치 다시 설정 
            if (map[newY][newX] == ObjType::Coin)
            {
                enemy.prevChar = 'o'; 
            }
            else
            {
                enemy.prevChar = ' '; 
            }
            map[newY][newX] = ObjType::Enemy; 

            // 이전 프레임 때 o을 M이 덮어 쓴 경우 
            if (enemy.wasFood == true)
                map[enemy.y][enemy.x] = ObjType::Coin;
            else
                map[enemy.y][enemy.x] = ObjType::Way; 

            // 에너미 위치를 갱신한다 
            enemy.x = newX;
            enemy.y = newY;

            if (enemy.prevChar == 'o')
                enemy.wasFood = true;
            else
                enemy.wasFood = false;
        }
    }

    void CheckCollision()
    {
        if (invincibleCnt > 0)
            return;

        for (const auto& enemy : enemies)
        {
            if (enemy.x == pacmanX && enemy.y == pacmanY)
            {
                gameOver = true;
            }
        }
    }

    void RenderUx()
    {
        COORD scorePosition = { 0, ROW_MAX + 1 };
        SetConsoleCursorPosition(hConsole, scorePosition);
        cout << "Score: " << score << endl;

        if (invincibleCnt > 0)
        {
            cout << "Invincible Time Left: " << invincibleCnt / 10 << " sec" << endl;
            invincibleCnt--;
        }

        if (gameOver)
        {
            if (coinCnt == 0)
                cout << "\n[ Game Clear! ]\n" << endl;
            else
                cout << "\n[ Game Over! ]\n" << endl;
        }
    }
};

int Game::enemyMoveCnt = 0;
int Game::invincibleCnt = 0;

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    srand(time(nullptr));
    Game game;
    game.Run();
    system("pause");
    return 0;
} 
*/