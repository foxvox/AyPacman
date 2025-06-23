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
#include <memory>
#include <stdexcept> 

constexpr int ROW_MAX = 24;
constexpr int COL_MAX = 32;
constexpr int ENEMY_CNT = 7;
constexpr int COIN_CNT = 40;

using namespace std;

enum class ObjType { Way, Wall, Coin, Enemy, Pacman };

struct Pos { int x, y; };
struct Enemy { Pos pos; };
struct Coin { Pos pos; };

class Pacman {
    Pos pos;
    bool inv;
public:
    Pacman(const Pos& p) : pos(p), inv(false) {}
    Pos  GetPos() const { return pos; }
    void SetPos(const Pos& p) { pos = p; }
    bool IsInvincible() const { return inv; }
    void ActivateInv() { inv = true; }
    void DeactivateInv() { inv = false; }
};

class Game {
    vector<vector<ObjType>> map;
    vector<Enemy>           enemies;
    vector<Coin>            coins;
    unique_ptr<Pacman>      pm;               // 스마트 포인터
    bool                    gameOver = false;
    int                     score = 0;
    int                     invCnt = 0;     // 인스턴스 멤버로 이동

    HANDLE hConsole;
    HANDLE hScreen[2] = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE };
    int    screenIndex = 0;

    mt19937 randEngine;

public:
    Game()
        try : randEngine(random_device{}())
    {
        map.resize(ROW_MAX, vector<ObjType>(COL_MAX, ObjType::Way));
        InitDoubleBuffer();
        LoadMapFromFile(L"mapFile.txt");
        SpawnCoins(COIN_CNT);
        SpawnPacman();
        SpawnEnemies();
    }
    catch (...) {
        // 생성 도중 실패 시 열린 핸들 정리
        for (auto& h : hScreen)
            if (h != INVALID_HANDLE_VALUE)
                CloseHandle(h);
        throw;
    }

    ~Game() {
        for (auto& h : hScreen)
            if (h != INVALID_HANDLE_VALUE)
                CloseHandle(h);
    }

    void InitDoubleBuffer()
    {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole == INVALID_HANDLE_VALUE)
            throw runtime_error("GetStdHandle failed");

        CONSOLE_CURSOR_INFO cci{ 1, FALSE };
        for (int i = 0; i < 2; ++i) {
            hScreen[i] = CreateConsoleScreenBuffer(
                GENERIC_READ | GENERIC_WRITE,
                0, NULL,
                CONSOLE_TEXTMODE_BUFFER,
                NULL);
            if (hScreen[i] == INVALID_HANDLE_VALUE)
                throw runtime_error("CreateConsoleScreenBuffer failed");
            if (!SetConsoleCursorInfo(hScreen[i], &cci))
                throw runtime_error("SetConsoleCursorInfo failed");
        }
        if (!SetConsoleActiveScreenBuffer(hScreen[0]))
            throw runtime_error("SetConsoleActiveScreenBuffer failed");
    }

    void FlipBuffer()
    {
        screenIndex ^= 1;
        if (!SetConsoleActiveScreenBuffer(hScreen[screenIndex]))
            throw runtime_error("SetConsoleActiveScreenBuffer failed");
    }


    int GetRand(int max)
    {
        return uniform_int_distribution<>{ 0, max }(randEngine);
    }

    void LoadMapFromFile(const wstring& filename)
    {
        wifstream file(filename);
        file.imbue(locale(locale(), new codecvt_utf8<wchar_t>));
        if (!file)
            throw runtime_error("Cannot open map file");

        wstring line;
        for (int y = 0; y < ROW_MAX; ++y) {
            if (!getline(file, line))
                line.assign(COL_MAX, L'0');  // 남은 줄은 전부 '0'
            if ((int)line.size() < COL_MAX)
                line += wstring(COL_MAX - line.size(), L'0');

            for (int x = 0; x < COL_MAX; ++x) {
                switch (line[x]) {
                case L'1': map[y][x] = ObjType::Wall; break;
                case L'0': /*fall through*/
                default:   map[y][x] = ObjType::Way;  break;
                }
            }
        }
    }

    void SpawnCoins(int count)
    {
        coins.clear();
        int placed = 0;
        int attempts = 0;
        const int MAX_ATTEMPTS = 10000;

        while (placed < count && attempts < MAX_ATTEMPTS) {
            ++attempts;
            int x = GetRand(COL_MAX - 1);
            int y = GetRand(ROW_MAX - 1);

            if (map[y][x] != ObjType::Way) continue;
            bool occupied = any_of(coins.begin(), coins.end(),
                [&](auto& c) { return c.pos.x == x && c.pos.y == y; });
            if (occupied) continue;

            coins.push_back({ {x, y} });
            map[y][x] = ObjType::Coin;
            ++placed;
        }
        if (placed < count)
            wcerr << L"[Warning] Coins placed: " << placed << L"/" << count << L"\n";
    }

    void SpawnPacman()
    {
        Pos start{ 16, 12 };
        pm = make_unique<Pacman>(start);
        map[start.y][start.x] = ObjType::Pacman;
    }

    void SpawnEnemies()
    {
        enemies.clear();
        int placed = 0, attempts = 0;
        const int MAX_ATTEMPTS = 10000;

        while (placed < ENEMY_CNT && attempts < MAX_ATTEMPTS) {
            ++attempts;
            int x = GetRand(COL_MAX - 1);
            int y = GetRand(ROW_MAX - 1);
            if (map[y][x] != ObjType::Way) continue;

            enemies.push_back({ {x, y} });
            map[y][x] = ObjType::Enemy;
            ++placed;
        }
        if (placed < ENEMY_CNT)
            wcerr << L"[Warning] Enemies placed: " << placed << L"/" << ENEMY_CNT << L"\n";
    }

    void Run()
    {
        while (!gameOver) {
            Update();        // 사용자 입력 & 충돌
            MoveEnemies();   // 적 이동 & 충돌
            Render();        // 화면 그리기 (버퍼는 뒤에서 한 번만 플립)
            FlipBuffer();
            Sleep(100);
        }

        // 최종 메시지 출력
        wcout << L"\n=== "
            << (coins.empty() ? L"Game Clear!" : L"Game Over!")
            << L" ===\n";
        FlipBuffer();

        // 종료 대기 (system() 대신 키 입력)
        wcout << L"Press any key to exit...";
        (void)_getwch();
    }

    void Update()
    {
        if (!_kbhit()) return;
        wchar_t key = _getwch();
        if (key == 27) { // ESC
            gameOver = true;
            return;
        }

        Pos old = pm->GetPos();
        Pos nw = old;
        switch (key) {
        case L'w': nw.y = max(0, old.y - 1); break;
        case L's': nw.y = min(ROW_MAX - 1, old.y + 1); break;
        case L'a': nw.x = max(0, old.x - 1); break;
        case L'd': nw.x = min(COL_MAX - 1, old.x + 1); break;
        default:   return;
        }

        auto target = map[nw.y][nw.x];
        if (target == ObjType::Wall) return;

        // 적과 충돌
        if (target == ObjType::Enemy) {
            if (invCnt <= 0) {
                gameOver = true;
                return;
            }
            else {
                // 제거 전 안전하게 iterator 검사
                auto it = find_if(enemies.begin(), enemies.end(),
                    [&](auto& e) { return e.pos.x == nw.x && e.pos.y == nw.y; });
                if (it != enemies.end()) {
                    enemies.erase(it);
                    map[nw.y][nw.x] = ObjType::Way;
                    if (enemies.empty())
                        gameOver = true;
                }
                return;
            }
        }

        // 코인 획득
        if (target == ObjType::Coin) {
            ++score;
            auto it = find_if(coins.begin(), coins.end(),
                [&](auto& c) { return c.pos.x == nw.x && c.pos.y == nw.y; });
            if (it != coins.end()) {
                coins.erase(it);
                map[nw.y][nw.x] = ObjType::Way;
                if (coins.empty())
                    gameOver = true;
            }
            // 20점에 무적
            if (score == 20 && invCnt == 0) {
                pm->ActivateInv();
                invCnt = 200; // 20초
            }
        }

        // 맵 갱신
        map[old.y][old.x] = ObjType::Way;
        pm->SetPos(nw);
        map[nw.y][nw.x] = ObjType::Pacman;
    }

    void MoveEnemies()
    {
        for (auto& e : enemies) {
            Pos old = e.pos, nw = old;
            // 플레이어와 충돌 체크
            if (invCnt <= 0 &&
                old.x == pm->GetPos().x && old.y == pm->GetPos().y) {
                gameOver = true;
                return;
            }

            switch (GetRand(3)) {
            case 0: nw.y = max(0, old.y - 1); break;
            case 1: nw.y = min(ROW_MAX - 1, old.y + 1); break;
            case 2: nw.x = max(0, old.x - 1); break;
            case 3: nw.x = min(COL_MAX - 1, old.x + 1); break;
            }

            if (map[nw.y][nw.x] == ObjType::Wall ||
                map[nw.y][nw.x] == ObjType::Enemy)
                continue;

            map[old.y][old.x] = ObjType::Way;
            e.pos = nw;
            map[nw.y][nw.x] = ObjType::Enemy;
        }
    }

    void Render()
    {
        // 무적 타이머 감소
        if (invCnt > 0) {
            if (--invCnt == 0)
                pm->DeactivateInv();
        }

        COORD cursor{ 0, 0 };
        SetConsoleCursorPosition(hScreen[screenIndex], cursor);

        for (int y = 0; y < ROW_MAX; ++y) {
            for (int x = 0; x < COL_MAX; ++x) {
                wchar_t ch;
                WORD    color;
                switch (map[y][x]) {
                case ObjType::Wall:   ch = L'■'; color = 8;  break;
                case ObjType::Way:    ch = L' ';  color = 7;  break;
                case ObjType::Coin:   ch = L'*';  color = 14; break;
                case ObjType::Enemy:  ch = L'ⓔ'; color = 13; break;
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

        // 스코어 & 타이머
        SetConsoleCursorPosition(hConsole, { 0, (SHORT)ROW_MAX });
        wcout << L"Score: " << score;
        if (invCnt > 0)
            wcout << L"  Invincible: " << invCnt / 10 << L"s";
    }    
};

int main()
{
    // UTF-8 콘솔 세팅
    SetConsoleOutputCP(CP_UTF8);
    (void)_setmode(_fileno(stdout), _O_U8TEXT); 
    try {
        Game g;
        g.Run();
    }
    catch (const exception& ex) {
        wcerr << L"[Fatal] " << ex.what() << L"\n";
        wcout << L"Press any key to exit...";
        (void)_getwch();
        return 1;
    }
    return 0;
}
