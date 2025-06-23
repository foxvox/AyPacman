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
    Way,    // ��
    Wall,   // ��
    Coin,   // ����(����)
    Enemy,  // ��
    Pacman  // �÷��̾�
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
    vector<vector<ObjType>>  map;    // �� ���� (ObjType)
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
        // ���� ���� �ʱ�ȭ
        random_device rd;
        randEngine = mt19937(rd());

        // �� ũ�� ����
        map.resize(ROW_MAX, vector<ObjType>(COL_MAX, ObjType::Way));

        // �ܼ� ������۸� �غ�
        InitDoubleBuffer();

        // �� ���� �ε�
        LoadMapFromFile(L"mapFile.txt");

        // ����, �Ѹ�, �� �ʱ� ��ġ
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

    // �ܼ� ������� �ʱ�ȭ
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

    // �� ���� �ε�: '1'->Wall, 0->Way
    void LoadMapFromFile(const wstring& filename)
    {
        wifstream file(filename);
        file.imbue(locale(locale(), new codecvt_utf8<wchar_t>));
        if (!file)
        {
            wcerr << L"[Error] �� ������ ã�� �� �����ϴ�: " << filename << endl;
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

    // ���� ���� ��ġ (�� ĭ����)
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

    // �Ѹ� ����
    void SpawnPacman()
    {
        Pos start{ 16, 12 };
        pm = new Pacman(start);
        map[start.y][start.x] = ObjType::Pacman;
    }

    // �� ���� ���� ����
    void SpawnEnemies()
    {
        enemies.clear();
        for (int i = 0; i < ENEMY_CNT; ++i)
            SpawnEnemy();
    }

    // �� 1���� ����
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

    // ���� ����
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

    // Ű �Է�, �Ѹ� �̵�, �浹/���� ó��
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

        // �� �浹 �˻�
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
                // ���� ���¸� �� ����
                map[nw.y][nw.x] = ObjType::Way;
                enemies.erase(find_if(enemies.begin(), enemies.end(),
                    [&](auto& e) { return e.pos.x == nw.x && e.pos.y == nw.y; }));
                if (enemies.empty())
                    gameOver = true;
                return;
			}
        }            

        // ���� ȹ��
        if (map[nw.y][nw.x] == ObjType::Coin)
        {
            score++;
            coins.erase(find_if(coins.begin(), coins.end(),
                [&](auto& c) { return c.pos.x == nw.x && c.pos.y == nw.y; }));
            if (coins.empty())
                gameOver = true;

            if (score == 20)
            {
                // 20�� ȹ�� �� ���� ���·� ��ȯ
                if (!pm->IsInvincible())
                {
                    pm->ActivateInv();
                    invincibleCnt = 200; // 20��
                }
            }
        }
        // �� ����
        map[old.y][old.x] = ObjType::Way;
        pm->SetPos(nw);
        map[nw.y][nw.x] = ObjType::Pacman;
    }
    // �� ���� �̵�
    void MoveEnemies()
    {
        for (auto& e : enemies)
        {
            Pos old = e.pos, nw = old;
            // ���� �ƴ� ���� �÷��̾� �浹 üũ
            if (invincibleCnt <= 0 && old.x == pm->GetPos().x && old.y == pm->GetPos().y)
            {
                gameOver = true;
                return;
            }

            // 4�� �� ����
            int d = GetRand(3);
            if (d == 0) nw.y = max(0, old.y - 1);
            else if (d == 1) nw.y = min(ROW_MAX - 1, old.y + 1);
            else if (d == 2) nw.x = max(0, old.x - 1);
            else           nw.x = min(COL_MAX - 1, old.x + 1);

            if (map[nw.y][nw.x] == ObjType::Wall ||
                map[nw.y][nw.x] == ObjType::Enemy)
                continue;

            // �̵� ó��
            map[old.y][old.x] = ObjType::Way;
            e.pos = nw;
            map[nw.y][nw.x] = ObjType::Enemy;
        }
    }

    // ȭ�� �׸���
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
                    ch = L'��'; color = 8; break;
                case ObjType::Way:
                    ch = L' ';  color = 7; break;
                case ObjType::Coin:
                    ch = L'*';  color = 14; break;
                case ObjType::Enemy:
                    ch = L'��';  color = 13; break;
                case ObjType::Pacman:
                    ch = L'��';
                    color = pm->IsInvincible() ? 12 : 11;
                    break;
                }

                SetConsoleTextAttribute(hScreen[screenIndex], color);
                WriteConsoleW(hScreen[screenIndex], &ch, 1, nullptr, nullptr);
            }
            WriteConsoleW(hScreen[screenIndex], L"\n", 1, nullptr, nullptr);
        }

        // ���ھ� & ����UI        
        SetConsoleCursorPosition(hConsole, { 0, (SHORT)ROW_MAX });
        wcout << L"Score: " << score;
        if (invincibleCnt > 0)
            wcout << L"  Invincible: " << invincibleCnt / 10 << L"s"; 
    }
}; 

int Game::invincibleCnt = 0;

int main()
{
    // UTF-8 �ܼ�
    SetConsoleOutputCP(CP_UTF8);
    (void)_setmode(_fileno(stdout), _O_U8TEXT);

    Game g;
    g.Run();

    wcout << L"\nThank you for playing!" << endl; 
    system("pause");
    return 0;
}  
*/
