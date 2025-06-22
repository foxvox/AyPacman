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
#include <random>
#include <algorithm>  

constexpr auto ROW_MAX = 22;
constexpr auto COL_MAX = 42;
constexpr auto ENEMY_CNT = 5;

using namespace std; 

enum class Dir 
{
    Up, Down, Left, Right, None  
}; 

struct Pos 
{ 
    int x; 
    int y; 
};

struct Enemy
{
    Pos pos; 
    wchar_t prevChar; 
    bool wasCoin;   
}; 

class Pacman
{
private:
	Pos pos;  // 현재 위치 
	bool invincible; // 무적 상태 여부 
    Dir dir;
public:
    Pacman(const Pos& _pos) : pos{}, invincible{ false }, dir{ Dir::None } 
    {
        pos = _pos; 
    }

    Pos GetPos() const { return pos; }     
    void SetPos(const Pos& _pos) { pos = _pos; } 
    Dir GetDir() const { return dir; } 
    void SetDir(const Dir& _dir) { dir = _dir; } 
    
    void ActivateInvincibility()
    {
        invincible = true; 
    }
    
    void DeactivateInvincibility() 
    {
        invincible = false; 
    }
    
    bool IsInvincible() const
    {
        return invincible;
    }
};

class Game
{
private:
    vector<vector<wchar_t>> map; 
    vector<Enemy> enemies;
    Pacman* pm; 
    bool gameOver;
    int score;
    int coinCnt;
    HANDLE hConsole;   
    wchar_t wch;
    mt19937 randEngine; 
    HANDLE hScreen[2];
    int screenIndex;
public:
    static int enemyMoveCnt;
    static int invincibleCnt;
public:
    Game() : pm{}, gameOver{ false }, score{}, coinCnt{}, hConsole{}, wch{ L' ' }, hScreen{}, screenIndex {}
    {
        // 랜덤 시드 설정 
        random_device   rd; 
        mt19937         gen(rd()); 
        randEngine = gen; 

        map.resize(ROW_MAX, vector<wchar_t>(COL_MAX, L' ')); 
        LoadMapFromFile(L"mapFile.txt");                
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE); 
        InitDoubleBuffer(); 
        SpawnPacman(); 
        SpawnEnemies();  
    } 

    void InitDoubleBuffer()
    {
        CONSOLE_CURSOR_INFO cci = { 1, FALSE };

        hScreen[0] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
        hScreen[1] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

        SetConsoleCursorInfo(hScreen[0], &cci);
        SetConsoleCursorInfo(hScreen[1], &cci);

        SetConsoleActiveScreenBuffer(hScreen[screenIndex]);
    } 

    void FlipBuffer()
    {
        screenIndex ^= 1; // XOR 연산으로 토글 
        SetConsoleActiveScreenBuffer(hScreen[screenIndex]);
    }

    int GetRand(int max)
    {
        uniform_int_distribution<> dist(0, max); 
        return dist(randEngine);  
    }

    void SpawnPacman()
    {
        Pos pos{ 20, 10 }; 
        pm = new Pacman(pos);
        map[pm->GetPos().y][pm->GetPos().x] = L'ⓟ';
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

            if (enemyMoveCnt % 5 == 0) // 적 특정 프레임마다 동작  
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
                if (map[row][col] == L'0')
                {
                    coinCnt++;
                }
            } 
            row++;
        }
        file.close(); 
    }

    void Update()
    {
        if (_kbhit())
        {
            Pos newPos{};
            Pos curPos = pm->GetPos();

            wchar_t key = _getwch(); // wide character 입력 받기

            switch (key)
            {
            case L'w': newPos.y = max(0, curPos.y - 1); break;
            case L's': newPos.y = min(ROW_MAX - 1, curPos.y + 1); break;
            case L'a': newPos.x = max(0, curPos.x - 1); break;
            case L'd': newPos.x = min(COL_MAX - 1, curPos.x + 1); break;
            case 27  : gameOver = true; return; 
            }

            if (map[newPos.y][newPos.x] != L'1')
            {
                if (map[newPos.y][newPos.x] == L'0')
                {
                    score++;
                    coinCnt--;
                }

                map[curPos.y][curPos.x] = L' '; 
                pm->SetPos(newPos);
                map[newPos.y][newPos.x] = L'ⓟ';

                if (coinCnt == 0)
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

    void Render()
    {
        COORD cursorPosition = { 0, 0 };
        SetConsoleCursorPosition(hScreen[screenIndex], cursorPosition);

        for (int row = 0; row < ROW_MAX; row++)
        {
            for (int col = 0; col < COL_MAX; col++)
            {
                wch = map[row][col];

                if (wch == L'1') {
                    SetConsoleTextAttribute(hScreen[screenIndex], 8);
                    wch = L'□';
                }
                else if (wch == L'0') {
                    SetConsoleTextAttribute(hScreen[screenIndex], 14);
                    wch = L'⊙';
                }
                else if (wch == L' ') {
                    SetConsoleTextAttribute(hScreen[screenIndex], 7);
                }
                else if (wch == L'ⓟ') {
                    SetConsoleTextAttribute(hScreen[screenIndex], pm->IsInvincible() ? 12 : 11);
                }
                else if (wch == L'ⓔ') {
                    SetConsoleTextAttribute(hScreen[screenIndex], 13);
                }

                WriteConsoleW(hScreen[screenIndex], &wch, 1, nullptr, nullptr);
                SetConsoleTextAttribute(hScreen[screenIndex], 7);
            }
            WriteConsoleW(hScreen[screenIndex], L"\n", 1, nullptr, nullptr);
        }
        FlipBuffer();
    }
    
    void SpawnEnemy()
    {
        Pos ep{}; //enemy position 
        do
        {
            ep.x = rand() % COL_MAX;
            ep.y = rand() % ROW_MAX;
        } while (map[ep.y][ep.x] != L' ');
        enemies.push_back({ ep, L' ', false }); 
        map[ep.y][ep.x] = L'ⓔ';
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
            Pos newPos{}; 
            Pos curPos = enemy.pos; 
            Dir dir; 
            do
            {
                dir = static_cast<Dir>(GetRand(3)); 
                switch (dir) 
                { 
                case Dir::Up: newPos.y = max(0, curPos.y - 1); break; // 위로 이동
                case Dir::Down: newPos.y = min(ROW_MAX - 1, curPos.y + 1); break; // 아래로 이동
                case Dir::Left: newPos.x = max(0, curPos.x - 1); break; // 왼쪽 이동
                case Dir::Right: newPos.x = min(COL_MAX - 1, curPos.x + 1); break; // 오른쪽 이동
                }

            } while (map[newPos.y][newPos.x] == L'1'); // 새 위치가 벽이면 이동할 위치 다시 설정 

            enemy.prevChar = map[newPos.y][newPos.x]; // 이동할 위치의 기존 문자 저장 

            map[newPos.y][newPos.x] = L'ⓔ'; // 새로운 위치에 `⊙` 배치 

            // 이전 프레임 때 0을 ◈이 덮어 쓴 경우 
            if (enemy.wasCoin == true)
                map[curPos.y][curPos.x] = L'0';
            else
                map[curPos.y][curPos.x] = L' ';

            // 에너미 위치를 갱신한다 
            enemy.pos = newPos;             

            if (enemy.prevChar == L'0')
                enemy.wasCoin = true;
            else
                enemy.wasCoin = false;
        }
    } 

    bool HitEnemy(const Enemy& enemy)
    {
        bool hitState = false; 
        
        if (pm->GetPos().x == enemy.pos.x && pm->GetPos().y == enemy.pos.y)
            hitState = true; 

        return hitState; 
    }

    void CheckCollision()
    {
        if (invincibleCnt > 0)
            return; // 무적 상태면 충돌 체크 안 함 

        for (const auto& enemy : enemies)
        {
            if (HitEnemy(enemy))   
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
            if (coinCnt == 0)
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
    SetConsoleOutputCP(CP_UTF8);
    (void)_setmode(_fileno(stdout), _O_U16TEXT); 
    
    Game game;
    game.Run();
    system("pause");
    return 0;
}
