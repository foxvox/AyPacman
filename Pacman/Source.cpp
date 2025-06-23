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
#include <io.h>
#include <fcntl.h> 
#include <codecvt> 
#include <random>
#include <algorithm>  

constexpr int ROW_MAX = 24;
constexpr int COL_MAX = 32;
constexpr int ENEMY_CNT = 7;
constexpr int COIN_CNT = 40;

using namespace std;

enum class ObjType
{
    Way,    // 길
    Wall,   // 벽
    Coin,   // 코인(코인)
    Enemy,  // 적
    Pacman  // 플레이어
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
    Pos pos;
    bool invincible;
public:
    Pacman(const Pos& p) : pos(p), invincible(false) {}
    Pos  GetPos() const { return pos; }
    void SetPos(Pos p) { pos = p; }
    bool IsInvincible() const { return invincible; }
    void ActivateInv() { invincible = true; }
    void DeactivateInv() { invincible = false; }
};

class Game
{
    vector<vector<ObjType>>  map;    // 맵 저장 (ObjType)
    vector<Enemy>            enemies;
    vector<Coin>             coins;
    Pacman* pm = nullptr;

    bool  gameOver = false;
    int   score = 0;

    HANDLE hConsole;
    HANDLE hScreen[2];
    int    screenIndex = 0;

    mt19937 randEngine;

public:
    static int invincibleCnt;

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

        // 코인, 팩맨, 적 초기 배치
        SpawnCoins(COIN_CNT);
        SpawnPacman();
        SpawnEnemies();
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

    // 콘솔 더블버퍼 초기화
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
        Pos start{ 16, 12 };
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

    // 게임 루프
    void Run()
    {
        while (!gameOver)
        {
            Update(); 
            Render(); 
            FlipBuffer();
            Sleep(100);
        }

        wcout << L"\n=== " << (coins.empty() ? L"Game Clear!" : L"Game Over!") << L" ===\n"; 
    }

    // 키 입력, 팩맨 이동, 충돌/코인 처리
    void Update()
    {
        if (!_kbhit()) return;

        wchar_t key = _getwch();
        if (key == 27) { gameOver = true; return; }

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
            if (invincibleCnt <= 0)
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
                if (!pm->IsInvincible())
                {
                    pm->ActivateInv();
                    invincibleCnt = 200; // 20초
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
            if (invincibleCnt <= 0 && old.x == pm->GetPos().x && old.y == pm->GetPos().y)
            {
                gameOver = true;
                return;
            }

            // 4방 중 랜덤
            int d = GetRand(3);
            if (d == 0) nw.y = max(0, old.y - 1);
            else if (d == 1) nw.y = min(ROW_MAX - 1, old.y + 1);
            else if (d == 2) nw.x = max(0, old.x - 1);
            else           nw.x = min(COL_MAX - 1, old.x + 1);

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

        if (invincibleCnt > 0)
            invincibleCnt--;

        for (int y = 0; y < ROW_MAX; ++y)
        {
            for (int x = 0; x < COL_MAX; ++x)
            {
                ObjType o = map[y][x];
                wchar_t ch{};
                WORD    color{};

                switch (o)
                {
                case ObjType::Wall:
                    ch = L'■'; color = 8; break;
                case ObjType::Way:
                    ch = L' ';  color = 7; break;
                case ObjType::Coin:
                    ch = L'*';  color = 14; break;
                case ObjType::Enemy:
                    ch = L'ⓔ';  color = 13; break;
                case ObjType::Pacman:
                    ch = L'ⓟ';
                    color = pm->IsInvincible() ? 12 : 11;
                    break;
                }

                SetConsoleTextAttribute(hScreen[screenIndex], color);
                WriteConsoleW(hScreen[screenIndex], &ch, 1, nullptr, nullptr);
            }
            WriteConsoleW(hScreen[screenIndex], L"\n", 1, nullptr, nullptr);
        }

        // 스코어 & 무적UI        
        SetConsoleCursorPosition(hConsole, { 0, (SHORT)ROW_MAX });
        wcout << L"Score: " << score;
        if (invincibleCnt > 0)
            wcout << L"  Invincible: " << invincibleCnt / 10 << L"s"; 
    }
}; 

int Game::invincibleCnt = 0;

int main()
{
    // UTF-8 콘솔
    SetConsoleOutputCP(CP_UTF8);
    (void)_setmode(_fileno(stdout), _O_U8TEXT);

    Game g;
    g.Run();

    wcout << L"\nThank you for playing!" << endl; 
    system("pause");
    return 0;
}  
*/
