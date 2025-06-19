#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>
#include <vector>
#include <windows.h> 
#include <climits> 
#include <cstdlib> 
#include <ctime> 
#include <io.h>
#include <fcntl.h> 
#include <codecvt>

constexpr auto ROW_MAX = 22;
constexpr auto COL_MAX = 42;
constexpr auto ENEMY_CNT = 5;

using namespace std;

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
public:
    static int enemyMoveCnt;
    static int invincibleCnt;
public:
    Game() : pacmanX{ 20 }, pacmanY{ 7 }, gameOver{ false }, score{ 0 }, foodCnt{ 0 }
    {
        map.resize(ROW_MAX, vector<wchar_t>(COL_MAX, L' ')); 
        LoadMapFromFile(L"mapFile.txt");        
        map[pacmanY][pacmanX] = L'@';
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SpawnEnemies();  
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

private: 
    vector<vector<wchar_t>> map;    
    int pacmanX, pacmanY;
    bool gameOver;
    int score;
    int foodCnt;
    HANDLE hConsole;
    vector<Enemy> enemies;

    void LoadMapFromFile(const wstring& filename)
    {
        wifstream file(filename);
        file.imbue(locale(locale(), new codecvt_utf8<wchar_t>)); 

        if (!file)
        {
            wcerr << L"맵 파일을 찾을 수 없습니다: " << filename << endl;
            return;
        }

        wstring line;
        int row = 0;
        while (getline(file, line) && row < ROW_MAX)
        {
            for (int col = 0; col < COL_MAX && col < line.size(); col++)
            {
                map[row][col] = line[col];
                if (map[row][col] == L'*')
                    foodCnt++;
            }
            row++;
        }
        file.close();
    }
  
    void Render()
    {
        COORD cursorPosition = { 0, 0 };
        SetConsoleCursorPosition(hConsole, cursorPosition);

        for (int row = 0; row < ROW_MAX; row++)
        {
            for (int col = 0; col < COL_MAX; col++)
            {         
                wchar_t ch = map[row][col]; 

                if (ch == '#') 
                    ch = L'⬛';
                else if (map[row][col] == '@' && invincibleCnt > 0)
                    ch = L'😎'; 
                else if (map[row][col] == '@')
                    ch = L'😃';
                else if (map[row][col] == 'M')
                    ch = L'👾'; 
                else if (map[row][col] == '*')
                    ch = L'🍒'; 
                else if (ch == ' ') 
                    ch = L'　';  
                
                wcout << ch; 
            }
            wcout << endl;
        }        
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

            if (map[newY][newX] != '#')
            {
                if (map[newY][newX] == 'o')
                {
                    score++;
                    foodCnt--;
                }

                map[pacmanY][pacmanX] = ' ';
                pacmanX = newX;
                pacmanY = newY;
                map[pacmanY][pacmanX] = '@';

                // 모든 먹이를 먹었으면 승리 처리 
                // foodCnt bug 때문에 이중처리
                if (foodCnt == 0)
                {
                    gameOver = true;
                }

                // 무적 모드 활성화
                if (score == 20)
                {
                    invincibleCnt = 200; // 20초 (게임 루프당 100ms이므로 200번 실행)
                }
            }
        }
    }

    void SpawnEnemy()
    {
        int ex, ey;
        do
        {
            ex = rand() % COL_MAX;
            ey = rand() % ROW_MAX;
        } while (map[ey][ex] != ' ');
        enemies.push_back({ ex, ey, ' ', false });
        map[ey][ex] = 'M';
    }

    void SpawnEnemies()
    {
        enemies.clear();
        for (int i = 0; i < ENEMY_CNT; i++)
        {
            SpawnEnemy();
        }
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

            } while (map[newY][newX] == '#'); // 새 위치가 벽이면 이동할 위치 다시 설정 

            enemy.prevChar = map[newY][newX]; // 이동할 위치의 기존 문자 저장 

            map[newY][newX] = 'M'; // 새로운 위치에 `M` 배치 

            // 이전 프레임 때 o을 M이 덮어 쓴 경우 
            if (enemy.wasFood == true)
                map[enemy.y][enemy.x] = 'o';
            else
                map[enemy.y][enemy.x] = ' ';

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
            return; // 무적 상태면 충돌 체크 안 함

        for (const auto& enemy : enemies)
        {
            if (enemy.x == pacmanX && enemy.y == pacmanY)
            {
                gameOver = true;
            }
        }
    }

    void RenderUx() const 
    {
        COORD scorePosition = { 0, ROW_MAX + 1 };
        SetConsoleCursorPosition(hConsole, scorePosition);
        wcout << L"Score: " << score << endl;

        if (invincibleCnt > 0)
        {
            wcout << L"Invincible Time Left: " << invincibleCnt / 10 << L" sec" << endl;
            invincibleCnt--;
        }

        if (gameOver)
        {
            if (foodCnt == 0)
                wcout << L"\n[ Game Clear! ]\n" << endl;
            else
                wcout << L"\n[ Game Over! ]\n" << endl;
        }
    } 
};

int Game::enemyMoveCnt = 0;
int Game::invincibleCnt = 0;

int main()
{
    (void)_setmode(_fileno(stdout), _O_U16TEXT);
    srand(time(nullptr));
    Game game;
    game.Run();
    system("pause");
    return 0;
}
