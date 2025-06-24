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
#include <sstream> 

constexpr int ROW_MAX = 14;
constexpr int COL_MAX = 40;
constexpr int ENEMY_CNT = 5;
constexpr int COIN_CNT = 30;

using namespace std;

enum class ObjType
{
    Way = 0,    // 길
    Wall = 1,   // 벽
    Coin = 2,   // 코인(코인)
    Enemy = 3,  // 적
    Pacman = 4  // 플레이어
};

struct Pos
{
    int x, y;
};

struct Enemy
{
    Pos pos;
};

struct Coin
{
    Pos pos;
};

class Pacman
{
private:    
    Pos pos;
    bool inv;
public:
    Pacman(const Pos& p) : pos(p), inv(false) {}
    Pos  GetPos() const { return pos; }
    void SetPos(Pos p) { pos = p; }
    bool GetInv() const { return inv; }
    void ActivateInv() { inv = true; }
    void DeactivateInv() { inv = false; }
};

class Game
{
private:
    vector<vector<ObjType>>  map;    // 맵 저장 (ObjType)
    vector<Enemy>            enemies;
    vector<Coin>             coins;
    Pacman* pm = nullptr;
    bool    gameOver = false;
    int     score = 0;
    int     enemyMoveCnt = 0; 

    HANDLE  hConsole;
    HANDLE  hScreen[2];
    int     screenIndex = 0;
    mt19937 randEngine;
public:
    static int invCnt;

    Game()
    {
        // 랜덤 엔진 초기화
        random_device rd;
        randEngine = mt19937(rd());
        // 맵 크기 설정
        map.resize(ROW_MAX, vector<ObjType>(COL_MAX, ObjType::Way));
        // 콘솔 더블버퍼링 준비
        InitDoubleBuffer();
        // 맵 파일 로드
        LoadMapFromFile(L"mapFile.txt");
        // 코인, 적, 팩맨 생성 
        SpawnCoins(COIN_CNT);        
        SpawnEnemies();
        SpawnPacman();
    }

    ~Game()
    {
        delete pm; 
        pm = nullptr;
        for (auto& screen : hScreen)
        {
            if (screen != INVALID_HANDLE_VALUE)
                CloseHandle(screen);
        }
    }

    void InitDoubleBuffer()
    {
        CONSOLE_CURSOR_INFO cci{ 1, FALSE };
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        for (int i = 0; i < 2; ++i)
        {
            hScreen[i] = CreateConsoleScreenBuffer(
                GENERIC_READ | GENERIC_WRITE,
                0, NULL,
                CONSOLE_TEXTMODE_BUFFER,
                NULL);
            SetConsoleCursorInfo(hScreen[i], &cci);
        }
        SetConsoleActiveScreenBuffer(hScreen[0]);
    }

    void FlipBuffer()
    {
        screenIndex ^= 1;
        SetConsoleActiveScreenBuffer(hScreen[screenIndex]);
    }

    int GetRand(int max)
    {
        uniform_int_distribution<> dist(0, max);
        return dist(randEngine);
    }

    // 맵 파일 로드: '1'->Wall, 0->Way
    void LoadMapFromFile(const wstring& filename)
    {
        wifstream file(filename);
        file.imbue(locale(locale(), new codecvt_utf8<wchar_t>));
        if (!file)
        {
            wcerr << L"[Error] 맵 파일을 찾을 수 없습니다: " << filename << endl;
            exit(1);
        }

        wstring line;
        for (int y = 0; y < ROW_MAX && getline(file, line); ++y)
        {
            for (int x = 0; x < COL_MAX && x < (int)line.size(); ++x)
            {
                map[y][x] = (line[x] == L'1') ? ObjType::Wall : ObjType::Way;
            }
        }
    }

    // 코인 랜덤 배치 (빈 칸에만)
    void SpawnCoins(int count) 
    {
        coins.clear();
        int placed = 0;
        while (placed < count)
        {
            int x = GetRand(COL_MAX - 1);
            int y = GetRand(ROW_MAX - 1);
            if (map[y][x] == ObjType::Way &&
                !any_of(coins.begin(), coins.end(),
                    [&](auto& c) { return c.pos.x == x && c.pos.y == y; }))
            {
                coins.push_back({ {x,y} });
                map[y][x] = ObjType::Coin;
                ++placed;
            }
        }
    }

    // 팩맨 생성
    void SpawnPacman()
    {
        Pos start{ 20, 7 };
        pm = new Pacman(start);
        map[start.y][start.x] = ObjType::Pacman;
    }

    // 적 여러 마리 생성
    void SpawnEnemies()
    {
        enemies.clear();
        for (int i = 0; i < ENEMY_CNT; ++i)
            SpawnEnemy();
    }

    // 적 1마리 생성
    void SpawnEnemy()
    {
        Pos p;
        do {
            p.x = GetRand(COL_MAX - 1);
            p.y = GetRand(ROW_MAX - 1);
        } while (map[p.y][p.x] != ObjType::Way);

        enemies.push_back({ p });
        map[p.y][p.x] = ObjType::Enemy;
    }

    void CheckCollision()
    {
        if (invCnt > 0)
            return;

        for (const auto& enemy : enemies)
        {
            if (enemy.pos.x == pm->GetPos().x && enemy.pos.y == pm->GetPos().y) 
            {
                gameOver = true;
            }
        }
    } 

    void UpdateCoin() 
    {
        for (const auto& coin : coins)
        {
            map[coin.pos.y][coin.pos.x] = ObjType::Coin; 
        }
    } 

    void UpdateEnemy() 
    {
        for (const auto& enemy : enemies) 
        {
            map[enemy.pos.y][enemy.pos.x] = ObjType::Enemy; 
        }
    }

    void Run()
    {
        while (!gameOver)
        {
            Update(); 

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

            UpdateCoin(); 
            UpdateEnemy(); 
            
            Render(); 
            FlipBuffer();
            Sleep(100);
        } 

        SetConsoleCursorPosition(hScreen[screenIndex], { 0, (SHORT)(ROW_MAX + 4) }); 
        
        if (coins.empty())
        {
            WriteConsoleW(hScreen[screenIndex], L"Game Clear!", 11, nullptr, nullptr);
        }
        else
        {
            WriteConsoleW(hScreen[screenIndex], L"Game Over!", 10, nullptr, nullptr);
        }        
    }
   
    void Update()
    {
        if (!_kbhit()) return;

        wchar_t key = _getwch();
        if (key == VK_ESCAPE) { gameOver = true; return; }

        Pos old = pm->GetPos();
        Pos nw = old;

        switch (key)
        {
        case L'w': nw.y = max(0, old.y - 1); break;
        case L's': nw.y = min(ROW_MAX - 1, old.y + 1); break;
        case L'a': nw.x = max(0, old.x - 1); break;
        case L'd': nw.x = min(COL_MAX - 1, old.x + 1); break;
        default:   return;
        }

        // 벽 충돌 검사
        if (map[nw.y][nw.x] == ObjType::Wall)
        {
            return;
        }
        else if (map[nw.y][nw.x] == ObjType::Enemy)
        {            
            if (invCnt <= 0)
            {
                gameOver = true;
                return;
            }
            else
            {
                // 무적 상태면 적 제거
                map[nw.y][nw.x] = ObjType::Way;
                enemies.erase(find_if(enemies.begin(), enemies.end(),
                    [&](auto& e) { return e.pos.x == nw.x && e.pos.y == nw.y; }));
                if (enemies.empty())
                    gameOver = true;
                return;
			}
        }            

        // 코인 획득
        if (map[nw.y][nw.x] == ObjType::Coin)
        {
            score++;
            coins.erase(find_if(coins.begin(), coins.end(),
                [&](auto& c) { return c.pos.x == nw.x && c.pos.y == nw.y; }));
            if (coins.empty())
                gameOver = true;

            if (score == 20)
            {
                // 20점 획득 시 무적 상태로 전환
                if (!pm->GetInv())
                {
                    pm->ActivateInv();
                    invCnt = 200; // 20초
                }
            }
        }
        // 맵 갱신
        map[old.y][old.x] = ObjType::Way;
        pm->SetPos(nw);
        map[nw.y][nw.x] = ObjType::Pacman;
    }
    // 적 랜덤 이동
    void MoveEnemies()
    {
        for (auto& e : enemies)
        {
            Pos old = e.pos, nw = old;
            // 무적 아닐 때만 플레이어 충돌 체크
            if (invCnt <= 0 && old.x == pm->GetPos().x && old.y == pm->GetPos().y)
            {
                gameOver = true;
                return;
            }

            // 4방 중 랜덤
            int d = GetRand(3);
            if (d == 0)      nw.y = max(0, old.y - 1);
            else if (d == 1) nw.y = min(ROW_MAX - 1, old.y + 1);
            else if (d == 2) nw.x = max(0, old.x - 1);
            else             nw.x = min(COL_MAX - 1, old.x + 1);

            if (map[nw.y][nw.x] == ObjType::Wall ||
                map[nw.y][nw.x] == ObjType::Enemy)
                continue;

            // 이동 처리
            map[old.y][old.x] = ObjType::Way;
            e.pos = nw;
            map[nw.y][nw.x] = ObjType::Enemy;
        }
    }

    // 화면 그리기
    void Render()
    {
        COORD pos{ 0,0 };
        SetConsoleCursorPosition(hScreen[screenIndex], pos);

        if (invCnt > 0)
            invCnt--;

        for (int y = 0; y < ROW_MAX; ++y)
        {
            for (int x = 0; x < COL_MAX; ++x)
            {
                ObjType ot = map[y][x];
                wchar_t wch{};
                WORD    color{};

                switch (ot)
                {
                case ObjType::Wall:
                    wch = L'□'; color = 8; break;
                case ObjType::Way:
                    wch = L' ';  color = 7; break;
                case ObjType::Coin:
                    wch = L'o';  color = 14; break;
                case ObjType::Enemy:
                    wch = L'ⓔ';  color = 13; break;
                case ObjType::Pacman:
                    wch = L'ⓟ';
                    color = pm->GetInv() ? 12 : 11;
                    break;
                }
                SetConsoleTextAttribute(hScreen[screenIndex], color);
                WriteConsoleW(hScreen[screenIndex], &wch, 1, nullptr, nullptr);
            }
            WriteConsoleW(hScreen[screenIndex], L"\n", 1, nullptr, nullptr);
        }

        SetConsoleCursorPosition(hScreen[screenIndex], { 0, (SHORT)(ROW_MAX + 1) });
        WriteConsoleW(hScreen[screenIndex], L"Score: ", 7, nullptr, nullptr);
        wstringstream ss1;
        ss1 << score;
        std::wstring scoreStr = ss1.str();
        WriteConsoleW(hScreen[screenIndex], scoreStr.c_str(), (DWORD)scoreStr.size(), nullptr, nullptr); 

        wstringstream ss2;
        if (invCnt > 0)
        {
            WriteConsoleW(hScreen[screenIndex], L"\n", 1, nullptr, nullptr);
            WriteConsoleW(hScreen[screenIndex], L"InvCnt: ", 8, nullptr, nullptr);
            ss2 << static_cast<int>(invCnt / 10);  
            std::wstring invCntStr = ss2.str();
            WriteConsoleW(hScreen[screenIndex], invCntStr.c_str(), (DWORD)invCntStr.size(), nullptr, nullptr);
        }
    }
}; 

int Game::invCnt = 0;

int main()
{
    // UTF-8 콘솔
    SetConsoleOutputCP(CP_UTF8);
    (void)_setmode(_fileno(stdout), _O_U8TEXT);

    Game g;
    g.Run();

    wcout << L"\nThank you for playing!" << endl; 
    (void)_getwch();
    return 0;
}  

