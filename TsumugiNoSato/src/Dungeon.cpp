#include "Game.h"

// ------------------------------------------------------------------
// キャラクター・敵のデータ
// ------------------------------------------------------------------
static const CharDef kChars[2] = {
    // name     weapon         spare     hp   spd  reach arc   dmg            dur                  cReach cArc cDmg maxDur badger
    { L"ソラ", L"大糸切り鋏", L"縫い針", 100, 5.4f, 2.0f, 120, { 10, 11, 17 }, { 0.30f, 0.30f, 0.46f }, 2.7f, 220, 34, 60, false },
    { L"クロベエ", L"古いひも通し", L"小槌", 130, 4.7f, 3.6f, 50, { 9, 10, 15 }, { 0.42f, 0.42f, 0.55f }, 5.5f, 40, 26, 50, true },
};

struct EDef { const wchar_t* name; float hp, speed, notice, atkRange, windup, atkDur, recover, dmg, radius, cd; };
static const EDef kEDefs[4] = {
    { L"霧ネズミ",   28, 3.3f, 9, 4.2f, 0.55f, 0.42f, 0.6f, 8, 0.45f, 1.4f },
    { L"根っこ人形", 46, 2.1f, 8, 1.7f, 0.6f, 0.35f, 0.7f, 12, 0.5f, 1.5f },
    { L"胞子",       24, 1.7f, 10, 9.f, 0.7f, 0.25f, 0.8f, 7, 0.45f, 2.2f },
    { L"ほつれ蜘蛛", 340, 2.5f, 30, 4.5f, 0.85f, 0.5f, 0.9f, 16, 1.7f, 1.1f },
};

const CharDef& Game::Def(const Actor& a) const { return kChars[a.def]; }

// ------------------------------------------------------------------
// 地形
// ------------------------------------------------------------------
bool Game::Solid(float x, float z) const
{
    return SolidCell((int)std::floor(x / CELL), (int)std::floor(z / CELL));
}

void Game::MoveCircle(XMFLOAT3& p, float r, float dx, float dz) const
{
    auto blocked = [&](float x, float z) {
        return Solid(x - r, z - r) || Solid(x + r, z - r) || Solid(x - r, z + r) || Solid(x + r, z + r);
    };
    if (!blocked(p.x + dx, p.z)) p.x += dx;
    if (!blocked(p.x, p.z + dz)) p.z += dz;
}

int Game::RoomAt(XMFLOAT3 p) const
{
    int cx = (int)std::floor(p.x / CELL), cz = (int)std::floor(p.z / CELL);
    for (int i = 0; i < (int)rooms.size(); i++)
    {
        const Room& r = rooms[i];
        if (cx >= r.x && cx < r.x + r.w && cz >= r.z && cz < r.z + r.h) return i;
    }
    return -1;
}

XMFLOAT3 Game::RandomFloorPos(const Room& r, float margin)
{
    for (int i = 0; i < 30; i++)
    {
        XMFLOAT3 p{ RandF(r.x * CELL + margin, (r.x + r.w) * CELL - margin), 0, RandF(r.z * CELL + margin, (r.z + r.h) * CELL - margin) };
        if (!Solid(p.x, p.z)) return p;
    }
    return { (r.x + r.w * 0.5f) * CELL, 0, (r.z + r.h * 0.5f) * CELL };
}

XMFLOAT3 Game::PathNext(XMFLOAT3 from, XMFLOAT3 to)
{
    int sx = (int)std::floor(from.x / CELL), sz = (int)std::floor(from.z / CELL);
    int tx = (int)std::floor(to.x / CELL), tz = (int)std::floor(to.z / CELL);
    if ((sx == tx && sz == tz) || SolidCell(tx, tz) || SolidCell(sx, sz)) return to;
    static int parent[DN * DN];
    std::fill(parent, parent + DN * DN, -2);
    std::deque<int> q;
    q.push_back(tx * DN + tz);
    parent[tx * DN + tz] = -1;
    const int dxs[4] = { 1, -1, 0, 0 }, dzs[4] = { 0, 0, 1, -1 };
    bool found = false;
    while (!q.empty())
    {
        int c = q.front(); q.pop_front();
        int cx = c / DN, cz = c % DN;
        if (cx == sx && cz == sz) { found = true; break; }
        for (int k = 0; k < 4; k++)
        {
            int nx = cx + dxs[k], nz = cz + dzs[k];
            if (SolidCell(nx, nz) || parent[nx * DN + nz] != -2) continue;
            parent[nx * DN + nz] = c;
            q.push_back(nx * DN + nz);
        }
    }
    if (!found) return to;
    int n = parent[sx * DN + sz];
    if (n < 0) return to;
    int n2 = parent[n];
    // 2マス先が直線で行けるならそちらを目指す（動きをなめらかに）
    int pick = n;
    if (n2 >= 0)
    {
        int ax = n2 / DN, az = n2 % DN;
        if ((ax == sx || az == sz)) pick = n2;
    }
    return { (pick / DN + 0.5f) * CELL, 0, (pick % DN + 0.5f) * CELL };
}

// ------------------------------------------------------------------
// フロア生成（BSP法）
// ------------------------------------------------------------------
void Game::GenerateFloor(int floor, uint32_t seed)
{
    rng.seed(seed);
    memset(cell, 0, sizeof(cell));
    memset(seen, 0, sizeof(seen));
    rooms.clear(); decos.clear(); enemies.clear(); projs.clear(); pickups.clear();
    chests.clear(); springs.clear(); fx.clear(); nums.clear();
    tokens = 0; lockIdx = -1;

    auto carve = [&](int x0, int z0, int x1, int z1) {
        for (int x = std::max(1, std::min(x0, x1)); x <= std::min(DN - 2, std::max(x0, x1)); x++)
            for (int z = std::max(1, std::min(z0, z1)); z <= std::min(DN - 2, std::max(z0, z1)); z++)
                cell[x][z] = 1;
    };
    bool fixed = floor == 5 || floor == 10;
    if (fixed)
    {
        Room r{ 6, 4, 18, 19 };
        rooms.push_back(r);
        carve(r.x, r.z, r.x + r.w - 1, r.z + r.h - 1);
        // 柱
        int pil[4][2] = { { 10, 9 }, { 19, 9 }, { 10, 17 }, { 19, 17 } };
        for (auto& p : pil) cell[p[0]][p[1]] = 0;
        exitX = 15; exitZ = r.z + r.h;
    }
    else
    {
        struct Rc { int x, z, w, h; };
        std::vector<Rc> leaves;
        std::function<void(Rc, int)> split = [&](Rc r, int depth) {
            bool canV = r.w >= 16, canH = r.h >= 16;
            if (depth == 0 || (!canV && !canH)) { leaves.push_back(r); return; }
            bool vert = canV && (!canH || r.w > r.h || (r.w == r.h && (rng() & 1)));
            if (vert)
            {
                int s = RandI(r.w * 2 / 5, r.w * 3 / 5);
                split({ r.x, r.z, s, r.h }, depth - 1);
                split({ r.x + s, r.z, r.w - s, r.h }, depth - 1);
            }
            else
            {
                int s = RandI(r.h * 2 / 5, r.h * 3 / 5);
                split({ r.x, r.z, r.w, s }, depth - 1);
                split({ r.x, r.z + s, r.w, r.h - s }, depth - 1);
            }
        };
        split({ 1, 1, DN - 2, DN - 2 }, 3);
        if (leaves.size() > 6) leaves.resize(6);
        for (auto& l : leaves)
        {
            int rw = RandI(5, std::max(5, std::min(l.w - 2, 10)));
            int rh = RandI(5, std::max(5, std::min(l.h - 2, 10)));
            rw = std::min(rw, l.w - 2); rh = std::min(rh, l.h - 2);
            int rx = l.x + 1 + RandI(0, std::max(0, l.w - 2 - rw));
            int rz = l.z + 1 + RandI(0, std::max(0, l.h - 2 - rh));
            rooms.push_back({ rx, rz, rw, rh });
            carve(rx, rz, rx + rw - 1, rz + rh - 1);
        }
        for (size_t i = 0; i + 1 < rooms.size(); i++)
        {
            const Room& a = rooms[i]; const Room& b = rooms[i + 1];
            int ax = a.x + a.w / 2, az = a.z + a.h / 2, bx = b.x + b.w / 2, bz = b.z + b.h / 2;
            if (rng() & 1) { carve(ax, az, bx, az + 1); carve(bx, az, bx + 1, bz); }
            else { carve(ax, az, ax + 1, bz); carve(ax, bz, bx, bz + 1); }
        }
        // 出口：スタートから最も遠い部屋の北側の壁
        int best = 1; float bd = -1;
        for (int i = 1; i < (int)rooms.size(); i++)
        {
            float d = (float)(std::abs(rooms[i].x - rooms[0].x) + std::abs(rooms[i].z - rooms[0].z));
            if (d > bd) { bd = d; best = i; }
        }
        const Room& er = rooms[best];
        exitZ = er.z + er.h;
        exitX = er.x + er.w / 2;
        for (int k = 0; k < er.w; k++)
        {
            int x = er.x + (er.w / 2 + k) % er.w;
            if (exitZ < DN - 1 && cell[x][exitZ] == 0 && cell[x][exitZ - 1] == 1) { exitX = x; break; }
        }
        if (exitZ >= DN - 1) exitZ = DN - 2;
        cell[exitX][exitZ] = 0;
    }
    exitPos = { (exitX + 0.5f) * CELL, 0, exitZ * CELL };
    exitFace = { 0, 0, 1 };

    // パーティ配置
    const Room& sr = rooms[0];
    XMFLOAT3 sp = fixed ? XMFLOAT3{ 15 * CELL, 0, 6 * CELL } : XMFLOAT3{ (sr.x + sr.w * 0.5f) * CELL, 0, (sr.z + sr.h * 0.5f) * CELL };
    party[active].pos = sp; party[active].yaw = 0;
    party[1 - active].pos = Add(sp, F3(1.2f, 0, -1.2f)); party[1 - active].yaw = 0;
    if (Solid(party[1 - active].pos.x, party[1 - active].pos.z)) party[1 - active].pos = sp;
    for (auto& a : party) { a.st = a.hp > 0 ? ASt::Normal : ASt::Down; a.kb = {}; }
    camYaw = 0;
    camPosNow = Add(sp, F3(0, 4.5f, -7));
    camTargetNow = Add(sp, F3(0, 1.3f, 0));

    bool deep = floor >= 6;
    if (fixed)
    {
        SpawnEnemy(EType::Spider, { 15 * CELL, 0, 15 * CELL }, deep);
        enemies.back().keyHolder = true;
        enemies.back().yaw = PI;
        SpawnEnemy(EType::Rat, { 11 * CELL, 0, 13 * CELL }, deep);
        SpawnEnemy(EType::Rat, { 19 * CELL, 0, 13 * CELL }, deep);
        for (int i = 0; i < 2; i++) pickups.push_back({ PickKind::Crystal, { (9 + i * 12) * CELL, 0, 20 * CELL }, 0, false, 0 });
    }
    else
    {
        for (int r = 1; r < (int)rooms.size(); r++)
        {
            int n = RandI(2, 3) + (deep ? 1 : 0);
            for (int i = 0; i < n; i++)
            {
                EType t;
                int roll = RandI(0, 9);
                if (floor <= 2) t = roll < 5 ? EType::Rat : EType::Doll;
                else t = roll < 4 ? EType::Rat : roll < 7 ? EType::Doll : EType::Spore;
                SpawnEnemy(t, RandomFloorPos(rooms[r], 1.5f), deep);
            }
        }
        if (!enemies.empty())
        {
            int k = RandI(0, (int)enemies.size() - 1);
            enemies[k].keyHolder = true;
            enemies[k].hp = enemies[k].maxHp = enemies[k].maxHp * 1.4f;
        }
        int nc = RandI(3, 4);
        for (int i = 0; i < nc; i++)
        {
            int r = RandI(1, (int)rooms.size() - 1);
            pickups.push_back({ PickKind::Crystal, RandomFloorPos(rooms[r], 1.4f), RandF(0, 6), false, 0 });
        }
        int nch = RandI(1, 2);
        for (int i = 0; i < nch; i++)
        {
            int r = RandI(1, (int)rooms.size() - 1);
            chests.push_back({ RandomFloorPos(rooms[r], 1.6f), RandF(-PI, PI), false, 0 });
        }
        if (RandI(0, 1) == 0 || floor == 7)
        {
            int r = RandI(0, (int)rooms.size() - 1);
            springs.push_back({ RandomFloorPos(rooms[r], 2.2f), false });
        }
        // スタート部屋に1つ結晶を置き、最初の見せ場にする
        pickups.push_back({ PickKind::Crystal, Add(sp, F3(RandF(-2, 2), 0, 3.5f)), 0, false, 0 });
        if (Solid(pickups.back().pos.x, pickups.back().pos.z)) pickups.back().pos = Add(sp, F3(0, 0, 2));
    }

    // 飾り
    for (int x = 1; x < DN - 1; x++)
        for (int z = 1; z < DN - 1; z++)
        {
            if (!cell[x][z]) continue;
            int wx = 0, wz = 0;
            if (!cell[x - 1][z]) wx = -1; else if (!cell[x + 1][z]) wx = 1;
            else if (!cell[x][z - 1]) wz = -1; else if (!cell[x][z + 1]) wz = 1;
            XMFLOAT3 c{ (x + 0.5f) * CELL, 0, (z + 0.5f) * CELL };
            if (wx || wz)
            {
                XMFLOAT3 wallPt = Add(c, F3(wx * 0.95f, 0, wz * 0.95f));
                float roll = RandF(0, 1);
                if (roll < 0.12f) decos.push_back({ 0, wallPt, RandF(0.8f, 1.3f), YawOf(F3((float)-wx, 0, (float)-wz)), RandF(-0.4f, 0.4f) });
                else if (roll < 0.19f) decos.push_back({ 1, Add(wallPt, F3(-wx * 0.4f, 0, -wz * 0.4f)), RandF(0.7f, 1.3f), RandF(0, 6), 0 });
                else if (roll < 0.24f) decos.push_back({ 3, Add(wallPt, F3(-wx * 0.3f, 0, -wz * 0.3f)), RandF(0.6f, 1.1f), RandF(0, 6), 0 });
            }
            else if (RandF(0, 1) < 0.03f) decos.push_back({ 2, c, RandF(0.5f, 0.9f), RandF(0, 6), 0 });
        }
    BuildDungeonMeshes();
}

void Game::BuildDungeonMeshes()
{
    MeshBuilder fb, wb;
    for (int x = 0; x < DN; x++)
        for (int z = 0; z < DN; z++)
        {
            if (cell[x][z]) { fb.FloorQuad(x * CELL, z * CELL, (x + 1) * CELL, (z + 1) * CELL, 0, 0.22f); continue; }
            bool nearW = false;
            for (int dx = -1; dx <= 1 && !nearW; dx++)
                for (int dz = -1; dz <= 1; dz++)
                {
                    int nx = x + dx, nz = z + dz;
                    if (nx >= 0 && nz >= 0 && nx < DN && nz < DN && cell[nx][nz]) { nearW = true; break; }
                }
            if (nearW) wb.Box({ x * CELL, 0, z * CELL }, { (x + 1) * CELL, WALL_H, (z + 1) * CELL }, 0.3f);
        }
    floorMesh = R->CreateMesh(fb);
    wallMesh = R->CreateMesh(wb);
}

void Game::SpawnEnemy(EType t, XMFLOAT3 pos, bool deep)
{
    Enemy e;
    e.t = t;
    const EDef& d = kEDefs[(int)t];
    e.pos = pos;
    e.hp = e.maxHp = d.hp * (deep ? 1.35f : 1.f);
    e.radius = d.radius;
    e.yaw = RandF(-PI, PI);
    e.wander = pos;
    e.wanderT = RandF(0, 2);
    e.bob = RandF(0, 6);
    e.atkCd = RandF(0.3f, 1.2f);
    enemies.push_back(e);
}

// ------------------------------------------------------------------
void Game::EnterDungeon(int floor, uint32_t seed)
{
    bool fromVillage = scene != SceneId::Dungeon;
    scene = SceneId::Dungeon;
    sceneT = 0;
    if (fromVillage)
    {
        totalDives++;
        crystalsThisRun = 0;
        lantern = 100;
        for (auto& a : party) { a.hp = a.maxHp; a.st = ASt::Normal; }
    }
    floorNo = floor;
    hasKey = false;
    exitMenu = false;
    downT = 0;
    GenerateFloor(floor, seed);
    deepest = std::max(deepest, floor);
    std::wstring name = L"根の洞　" + std::to_wstring(floor) + L" / 10";
    if (floor == 5) name += L"　中間の間";
    else if (floor >= 6) name += L"　深層";
    ToastMsg(name, RGBA(1, 0.9f, 0.7f));
    if (floor == 5) { Say(L"ツムギ", L"糸が…ほどけてる？ 気をつけて、なにか大きいのがいる！"); bossIntroT = 0; }
    else if (fromVillage && totalDives == 1) Say(L"クロベエ", L"坊、灯（ともしび）が尽きる前に結晶を集めるぞ。鍵持ちの化け物を探せ。");
}

void Game::LeaveDungeon(bool dead)
{
    if (dead)
    {
        ToastMsg(L"力尽きた…村へ戻された", RGBA(1, 0.55f, 0.5f));
        threads = threads / 2;
    }
    EnterVillage(true);
}

void Game::SpawnFx(int kind, XMFLOAT3 pos, float size, XMFLOAT4 col, float dur)
{
    fx.push_back({ pos, 0, dur, size, kind, col, RandF(0, 6.28f) });
}

void Game::AddNum(XMFLOAT3 pos, int v, D2D1_COLOR_F c, float size)
{
    nums.push_back({ Add(pos, F3(RandF(-0.3f, 0.3f), 0, RandF(-0.3f, 0.3f))), 0, v, c, size });
}

void Game::UseItem(int i)
{
    if (i < 0 || i > 3 || items[i] <= 0) return;
    Actor& a = party[active];
    switch (i)
    {
    case 0: a.hp = std::min(a.maxHp, a.hp + 40); ToastMsg(L"薬草を使った（HP回復）", RGBA(0.6f, 1, 0.6f)); break;
    case 1: lantern = std::min(100.f, lantern + 50); ToastMsg(L"灯油を注いだ（灯 回復）", RGBA(1, 0.8f, 0.4f)); break;
    case 2:
        if (a.broken) { ToastMsg(L"壊れた武器は研げない", RGBA(1, 0.5f, 0.45f)); return; }
        a.dur = std::min(a.maxDur, a.dur + a.maxDur * 0.3f); ToastMsg(L"研ぎ粉で刃を整えた（耐久回復）"); break;
    case 3: items[i]--; ToastMsg(L"帰り糸をたぐって村へ戻った"); LeaveDungeon(false); return;
    }
    items[i]--;
}

// ------------------------------------------------------------------
// 戦闘
// ------------------------------------------------------------------
void Game::WeaponWear(Actor& a, int n)
{
    if (a.broken) return;
    a.dur -= (float)n;
    if (a.dur <= 0)
    {
        a.dur = 0;
        a.broken = true;
        threads++;
        ToastMsg(std::wstring(Def(a).weapon) + L"が壊れた！「ほつれ糸」を回収した", RGBA(1, 0.55f, 0.45f));
        SpawnFx(1, Add(a.pos, F3(0, 1, 0)), 3.f, C4(1, 0.8f, 0.5f), 0.6f);
    }
}

void Game::ActorAttackHits(Actor& a, float reach, float arcDeg, float dmg, bool heavy, int wear)
{
    bool any = false;
    XMFLOAT3 f = Fwd(a.yaw);
    float cosHalf = std::cos(arcDeg * 0.5f * PI / 180.f);
    float mult = (a.broken ? 0.45f : 1.f) * (1.f + 0.06f * (a.lvl - 1));
    for (auto& e : enemies)
    {
        if (e.st == ESt::Dead) continue;
        XMFLOAT3 to = Sub(e.pos, a.pos);
        float d = LenXZ(to) - e.radius;
        if (d > reach) continue;
        XMFLOAT3 n = NormXZ(to);
        if (DotXZ(n, f) < cosHalf && d > 0.4f) continue;
        float kb = heavy ? 7.f : 3.5f;
        if (Def(a).badger && heavy) kb = -6.f;   // ひも通し：刺して引き寄せる
        HitEnemy(e, std::round(dmg * mult * RandF(0.92f, 1.08f)), a.pos, kb, heavy, &a);
        any = true;
    }
    if (any)
    {
        WeaponWear(a, wear);
        hitstop = std::max(hitstop, heavy ? 0.09f : 0.05f);
        if (&a == &party[active]) shake = std::max(shake, heavy ? 0.25f : 0.1f);
    }
}

void Game::HitEnemy(Enemy& e, float dmg, XMFLOAT3 from, float kb, bool heavy, Actor* by)
{
    if (e.st == ESt::Dead) return;
    e.hp -= dmg;
    e.flash = 1;
    float h = e.t == EType::Spider ? 3.2f : 1.3f;
    AddNum(Add(e.pos, F3(0, h, 0)), (int)dmg, heavy ? RGBA(1, 0.7f, 0.25f) : RGBA(1, 1, 1), heavy ? 36.f : 28.f);
    SpawnFx(0, Add(e.pos, F3(0, h * 0.6f, 0)), heavy ? 2.8f : 1.9f, C4(1, 0.85f, 0.6f), 0.5f);
    XMFLOAT3 dir = NormXZ(Sub(e.pos, from));
    if (e.t != EType::Spider)
    {
        e.kb = Mul(dir, kb);
        if (e.token) { e.token = false; tokens--; }
        e.st = ESt::Hit; e.stT = 0;
    }
    else if (heavy) e.kb = Mul(dir, kb * 0.25f);
    if (e.hp <= 0) KillEnemy(e, by);
}

void Game::KillEnemy(Enemy& e, Actor* by)
{
    e.hp = 0;
    e.st = ESt::Dead;
    e.deadT = 0;
    if (e.token) { e.token = false; tokens--; }
    SpawnFx(1, Add(e.pos, F3(0, 0.8f, 0)), e.t == EType::Spider ? 6.f : 2.6f, C4(0.8f, 0.85f, 1.f), 0.6f);
    if (e.keyHolder)
        pickups.push_back({ PickKind::Key, Add(e.pos, F3(0, 1.0f, 0)), 0, false, 4.f });
    else if (RandI(0, 3) == 0)
        pickups.push_back({ PickKind::Herb, Add(e.pos, F3(0, 0.8f, 0)), 0, false, 3.f });
    if (e.t == EType::Spider)
    {
        ToastMsg(L"ほつれ蜘蛛を倒した！", RGBA(1, 0.85f, 0.4f));
        if (!knot) { knot = true; ToastMsg(L"「結び目」ができた。次からは6階から潜れる", RGBA(0.7f, 0.95f, 1)); }
    }
    if (lockIdx >= 0 && &enemies[lockIdx] == &e) lockIdx = -1;
    if (by)
    {
        by->exp += e.t == EType::Spider ? 160.f : 24.f;
        while (by->exp >= 100.f * by->lvl)
        {
            by->exp -= 100.f * by->lvl;
            by->lvl++;
            ToastMsg(std::wstring(Def(*by).weapon) + L" が Lv" + std::to_wstring(by->lvl) + L" に成長した", RGBA(0.7f, 1, 0.7f));
        }
    }
}

void Game::DamageActor(Actor& a, float dmg, XMFLOAT3 from, float kb)
{
    if (a.invuln > 0 || a.st == ASt::Down || a.st == ASt::Dodge) return;
    XMFLOAT3 dir = NormXZ(Sub(a.pos, from));
    bool front = DotXZ(Fwd(a.yaw), Mul(dir, -1)) > 0.3f;
    if (a.st == ASt::Guard && front)
    {
        dmg *= 0.25f;
        kb *= 0.35f;
        WeaponWear(a, 1);
        SpawnFx(0, Add(a.pos, Add(Mul(Fwd(a.yaw), 0.5f), F3(0, 1, 0))), 1.4f, C4(0.7f, 0.85f, 1), 0.4f);
    }
    else
    {
        a.st = ASt::Hit; a.stT = 0;
    }
    a.hp -= dmg;
    a.flash = 1;
    a.kb = Mul(dir, kb);
    a.invuln = 0.5f;
    AddNum(Add(a.pos, F3(0, 1.8f, 0)), (int)std::round(dmg), RGBA(1, 0.4f, 0.35f), 26);
    if (&a == &party[active]) shake = std::max(shake, 0.3f);
    if (a.hp <= 0)
    {
        a.hp = 0;
        a.st = ASt::Down;
        ToastMsg(std::wstring(Def(a).name) + L"が倒れた…", RGBA(1, 0.5f, 0.45f));
    }
}

// ------------------------------------------------------------------
// プレイヤー／相棒
// ------------------------------------------------------------------
void Game::UpdateActor(Actor& a, const Controls* c, float dt, bool controlled)
{
    const CharDef& d = Def(a);
    a.invuln = std::max(0.f, a.invuln - dt);
    a.flash = std::max(0.f, a.flash - dt * 4);
    a.stT += dt;
    if (LenXZ(a.kb) > 0.02f)
    {
        MoveCircle(a.pos, 0.4f, a.kb.x * dt, a.kb.z * dt);
        a.kb = Mul(a.kb, std::max(0.f, 1 - dt * 8));
    }
    if (a.st == ASt::Down) return;
    if (!controlled) { UpdateFollower(a, dt); return; }

    XMFLOAT3 right{ std::cos(camYaw), 0, -std::sin(camYaw) }, fwd = Fwd(camYaw);
    XMFLOAT3 mv = Add(Mul(right, c->mx), Mul(fwd, c->mz));
    float ml = LenXZ(mv);
    if (ml > 1) { mv = Mul(mv, 1 / ml); ml = 1; }
    Enemy* lock = lockIdx >= 0 ? &enemies[lockIdx] : nullptr;
    if (c->atkDown) a.holdT += dt; else a.holdT = 0;

    auto faceTarget = [&]() {
        if (lock) a.yaw = YawOf(Sub(lock->pos, a.pos));
        else if (ml > 0.05f) a.yaw = YawOf(mv);
        else
        {
            // 近くの敵へ自動で向く（手触り重視）
            float bd = 3.2f; Enemy* best = nullptr;
            for (auto& e : enemies)
            {
                if (e.st == ESt::Dead) continue;
                float dd = DistXZ(e.pos, a.pos) - e.radius;
                if (dd < bd && DotXZ(NormXZ(Sub(e.pos, a.pos)), Fwd(a.yaw)) > -0.2f) { bd = dd; best = &e; }
            }
            if (best) a.yaw = YawOf(Sub(best->pos, a.pos));
        }
    };
    auto startAttack = [&](int k) {
        a.st = ASt::Attack; a.combo = k; a.stT = 0; a.hitDone = false; a.queued = false;
        faceTarget();
    };
    auto move = [&](float speedMul) {
        if (ml > 0.05f)
        {
            float sp = d.speed * speedMul;
            MoveCircle(a.pos, 0.4f, mv.x * sp * dt, mv.z * sp * dt);
            if (!lock || a.st == ASt::Normal) a.yaw = ApproachAngle(a.yaw, lock && a.st != ASt::Normal ? YawOf(Sub(lock->pos, a.pos)) : YawOf(mv), dt * 14);
            a.walkT += dt * ml * speedMul;
            a.speedNow = sp * ml;
        }
        else a.speedNow = 0;
    };

    switch (a.st)
    {
    case ASt::Normal:
        move(1.f);
        if (c->dodge)
        {
            a.st = ASt::Dodge; a.stT = 0;
            a.dodgeDir = ml > 0.05f ? NormXZ(mv) : Mul(Fwd(a.yaw), -1);
            a.invuln = 0.35f;
        }
        else if (c->guard) { a.st = ASt::Guard; a.stT = 0; }
        else if (c->atkPressed) startAttack(0);
        break;
    case ASt::Attack:
    {
        float dur = d.dur[a.combo];
        float act = dur * 0.35f;
        if (a.stT < act)
        {
            XMFLOAT3 f = Fwd(a.yaw);
            MoveCircle(a.pos, 0.4f, f.x * 2.4f * dt, f.z * 2.4f * dt);
        }
        if (!a.hitDone && a.stT >= act)
        {
            a.hitDone = true;
            ActorAttackHits(a, d.reach, d.arcDeg, d.dmg[a.combo], false, 1);
        }
        if (c->atkPressed && a.stT > dur * 0.2f) a.queued = true;
        if (a.combo == 0 && c->atkDown && a.holdT > 0.38f) { a.st = ASt::Charging; a.stT = 0; break; }
        if (c->dodge && a.stT > act) { a.st = ASt::Dodge; a.stT = 0; a.dodgeDir = ml > 0.05f ? NormXZ(mv) : Mul(Fwd(a.yaw), -1); a.invuln = 0.35f; break; }
        if (a.stT >= dur)
        {
            if (a.queued && a.combo < 2) startAttack(a.combo + 1);
            else a.st = ASt::Normal;
        }
        break;
    }
    case ASt::Charging:
        move(0.3f);
        if (lock) a.yaw = YawOf(Sub(lock->pos, a.pos));
        if (!c->atkDown) { a.st = ASt::ChargeAtk; a.stT = 0; a.hitDone = false; a.holdT = 0; }
        break;
    case ASt::ChargeAtk:
        if (!a.hitDone && a.stT >= 0.18f)
        {
            a.hitDone = true;
            ActorAttackHits(a, d.chargeReach, d.chargeArc, d.chargeDmg, true, 3);
            SpawnFx(1, Add(a.pos, Add(Mul(Fwd(a.yaw), 1.2f), F3(0, 0.9f, 0))), 4.f, C4(1, 0.85f, 0.5f), 0.45f);
        }
        if (a.stT >= 0.58f) a.st = ASt::Normal;
        break;
    case ASt::Dodge:
    {
        float k = 1 - a.stT / 0.4f;
        float sp = 12.f * std::max(0.f, k);
        MoveCircle(a.pos, 0.4f, a.dodgeDir.x * sp * dt, a.dodgeDir.z * sp * dt);
        if (a.stT >= 0.4f) a.st = ASt::Normal;
        break;
    }
    case ASt::Guard:
        move(0.35f);
        if (lock) a.yaw = YawOf(Sub(lock->pos, a.pos));
        if (!c->guard) a.st = ASt::Normal;
        break;
    case ASt::Hit:
        if (a.stT > 0.32f) a.st = ASt::Normal;
        break;
    default: break;
    }
}

void Game::UpdateFollower(Actor& a, float dt)
{
    const CharDef& d = Def(a);
    Actor& L = party[active];
    a.aiCd -= dt;
    a.hp = std::min(a.maxHp, a.hp + 0.8f * dt);   // 控えは少しずつ回復
    if (a.st == ASt::Attack)
    {
        float dur = d.dur[a.combo], act = dur * 0.4f;
        if (!a.hitDone && a.stT >= act) { a.hitDone = true; ActorAttackHits(a, d.reach, d.arcDeg, d.dmg[a.combo], false, 1); }
        if (a.stT >= dur) a.st = ASt::Normal;
        return;
    }
    if (a.st == ASt::Hit) { if (a.stT > 0.32f) a.st = ASt::Normal; return; }
    a.st = ASt::Normal;

    Enemy* tgt = nullptr; float bd = 9.f;
    for (auto& e : enemies)
    {
        if (e.st == ESt::Dead || e.st == ESt::Idle) continue;
        float dd = DistXZ(e.pos, L.pos);
        if (dd < bd) { bd = dd; tgt = &e; }
    }
    XMFLOAT3 goal; float stop;
    // カメラと主人公の間に入らないよう、画面の右側に立つ
    XMFLOAT3 camR{ std::cos(camYaw), 0, -std::sin(camYaw) };
    if (tgt)
    {
        XMFLOAT3 side = NormXZ(Sub(Add(L.pos, Mul(camR, 2.5f)), tgt->pos));
        goal = Add(tgt->pos, Mul(side, d.reach * 0.8f + tgt->radius));
        if (Solid(goal.x, goal.z)) goal = tgt->pos;
        stop = Solid(goal.x, goal.z) ? d.reach * 0.8f : 0.5f;
        if (DistXZ(a.pos, tgt->pos) < d.reach * 0.85f + tgt->radius && DistXZ(a.pos, goal) < 1.5f) stop = 2.f;
    }
    else
    {
        goal = Add(L.pos, Add(Mul(camR, 1.8f), Mul(Fwd(camYaw), 0.2f)));
        if (Solid(goal.x, goal.z)) goal = Add(L.pos, Mul(camR, -1.8f));
        if (Solid(goal.x, goal.z)) goal = L.pos;
        stop = 0.7f;
    }
    float dist = DistXZ(a.pos, goal);
    if (DistXZ(a.pos, L.pos) > 16.f) { a.pos = Add(L.pos, Mul(Fwd(L.yaw), -1.f)); if (Solid(a.pos.x, a.pos.z)) a.pos = L.pos; }
    if (dist > stop)
    {
        XMFLOAT3 wp = dist > 4.f ? PathNext(a.pos, goal) : goal;
        XMFLOAT3 dir = NormXZ(Sub(wp, a.pos));
        float sp = d.speed * (dist > 6 ? 1.2f : (tgt ? 1.f : std::min(1.f, dist / 2.f + 0.3f)));
        MoveCircle(a.pos, 0.4f, dir.x * sp * dt, dir.z * sp * dt);
        a.yaw = ApproachAngle(a.yaw, YawOf(dir), dt * 10);
        a.walkT += dt * sp / d.speed;
        a.speedNow = sp;
    }
    else
    {
        a.speedNow = 0;
        if (tgt)
        {
            a.yaw = ApproachAngle(a.yaw, YawOf(Sub(tgt->pos, a.pos)), dt * 10);
            if (a.aiCd <= 0)
            {
                a.st = ASt::Attack; a.combo = RandI(0, 2); a.stT = 0; a.hitDone = false;
                a.aiCd = 1.15f;
            }
        }
    }
}

// ------------------------------------------------------------------
// 敵AI：待機→発見→接近→攻撃→後退→被弾→死亡
// ------------------------------------------------------------------
void Game::UpdateEnemies(float dt)
{
    auto release = [&](Enemy& e) { if (e.token) { e.token = false; tokens--; } };
    for (auto& e : enemies)
    {
        const EDef& ed = kEDefs[(int)e.t];
        e.flash = std::max(0.f, e.flash - dt * 5);
        e.bob += dt;
        if (e.st == ESt::Dead) { e.deadT += dt; continue; }
        if (LenXZ(e.kb) > 0.02f)
        {
            MoveCircle(e.pos, e.radius, e.kb.x * dt, e.kb.z * dt);
            e.kb = Mul(e.kb, std::max(0.f, 1 - dt * 7));
        }
        e.stT += dt;
        e.atkCd -= dt;

        Actor* tgt = &party[active];
        Actor& other = party[1 - active];
        if (tgt->st == ASt::Down || (other.st != ASt::Down && DistXZ(other.pos, e.pos) + 2.f < DistXZ(tgt->pos, e.pos))) tgt = &other;
        if (tgt->st == ASt::Down) { e.st = ESt::Idle; continue; }
        XMFLOAT3 to = Sub(tgt->pos, e.pos);
        float d = LenXZ(to);
        XMFLOAT3 dir = NormXZ(to);
        bool boss = e.t == EType::Spider;
        auto moveDir = [&](XMFLOAT3 mdir, float sp) {
            MoveCircle(e.pos, e.radius, mdir.x * sp * dt, mdir.z * sp * dt);
            if (LenXZ(mdir) > 0.01f) e.yaw = ApproachAngle(e.yaw, YawOf(mdir), dt * 8);
        };
        auto startWindup = [&]() {
            e.st = ESt::Windup; e.stT = 0;
            if (!boss) { e.token = true; tokens++; }
        };
        bool canTake = boss || tokens < 2;

        switch (e.st)
        {
        case ESt::Idle:
        {
            e.wanderT -= dt;
            if (e.wanderT <= 0)
            {
                e.wander = Add(e.pos, F3(RandF(-3, 3), 0, RandF(-3, 3)));
                e.wanderT = RandF(2, 4);
            }
            XMFLOAT3 wd = Sub(e.wander, e.pos);
            if (LenXZ(wd) > 0.3f) moveDir(NormXZ(wd), ed.speed * 0.3f);
            int rr = RoomAt(e.pos);
            if (d < ed.notice && ((rr >= 0 && rr == RoomAt(tgt->pos)) || d < 6.f)) { e.st = ESt::Notice; e.stT = 0; }
            break;
        }
        case ESt::Notice:
            e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 10);
            if (e.stT > 0.55f) e.st = ESt::Chase;
            break;
        case ESt::Chase:
            if (e.t == EType::Spore)
            {
                if (d < 4.f) moveDir(Mul(dir, -1), ed.speed);
                else if (d > 7.5f) moveDir(dir, ed.speed);
                else
                {
                    XMFLOAT3 tan{ -dir.z, 0, dir.x };
                    moveDir(tan, ed.speed * 0.4f);
                    e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 8);
                }
                if (d < ed.atkRange && e.atkCd <= 0 && canTake) startWindup();
            }
            else
            {
                if (d > ed.atkRange * 0.85f)
                {
                    XMFLOAT3 wp = d > 6.f ? PathNext(e.pos, tgt->pos) : tgt->pos;
                    moveDir(NormXZ(Sub(wp, e.pos)), ed.speed);
                }
                else if (e.atkCd <= 0 && canTake) startWindup();
                else
                {
                    XMFLOAT3 tan{ -dir.z, 0, dir.x };
                    moveDir(tan, ed.speed * 0.3f);
                    e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 8);
                }
            }
            break;
        case ESt::Windup:
            if (e.stT < ed.windup * 0.75f) e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 10);
            if (e.stT >= ed.windup)
            {
                e.st = ESt::Attack; e.stT = 0; e.hitDone = false;
                e.dashDir = Fwd(e.yaw);
                if (e.t == EType::Spore)
                {
                    XMFLOAT3 o = Add(e.pos, F3(0, 1.0f, 0));
                    XMFLOAT3 tp = Add(tgt->pos, F3(0, 0.9f, 0));
                    XMFLOAT3 v = Sub(tp, o);
                    float l = Len(v);
                    projs.push_back({ o, Mul(v, 7.5f / l), 3.f, ed.dmg, 0.35f, 0 });
                }
                if (boss)
                {
                    e.pattern = d < 5.f ? 0 : 1;
                    if (e.pattern == 1)
                        for (int k = -1; k <= 1; k++)
                        {
                            float yy = YawOf(dir) + k * 0.3f;
                            XMFLOAT3 o = Add(e.pos, Add(Mul(Fwd(yy), 1.5f), F3(0, 1.2f, 0)));
                            projs.push_back({ o, Mul(Fwd(yy), 8.5f), 2.5f, 9.f, 0.45f, 1 });
                        }
                }
            }
            break;
        case ESt::Attack:
        {
            auto hitParty = [&](float range, float minDot, float dmg, float kb, XMFLOAT3 center) {
                for (auto& a : party)
                {
                    if (a.st == ASt::Down) continue;
                    XMFLOAT3 t2 = Sub(a.pos, center);
                    if (LenXZ(t2) > range) continue;
                    if (minDot > -1 && DotXZ(NormXZ(Sub(a.pos, e.pos)), Fwd(e.yaw)) < minDot) continue;
                    DamageActor(a, dmg, e.pos, kb);
                }
            };
            if (e.t == EType::Rat)
            {
                moveDir(e.dashDir, 11.f);
                if (!e.hitDone)
                    for (auto& a : party)
                        if (a.st != ASt::Down && DistXZ(a.pos, e.pos) < e.radius + 0.55f) { DamageActor(a, ed.dmg, e.pos, 7); e.hitDone = true; }
            }
            else if (e.t == EType::Doll && !e.hitDone && e.stT > 0.1f)
            {
                e.hitDone = true;
                hitParty(ed.atkRange + 0.5f, 0.2f, ed.dmg, 6, e.pos);
            }
            else if (boss && e.pattern == 0 && !e.hitDone && e.stT > 0.15f)
            {
                e.hitDone = true;
                XMFLOAT3 c = Add(e.pos, Mul(Fwd(e.yaw), 2.2f));
                hitParty(3.4f, -2, ed.dmg, 9, c);
                SpawnFx(1, Add(c, F3(0, 0.3f, 0)), 7.f, C4(0.9f, 0.9f, 1.f), 0.5f);
                shake = std::max(shake, 0.35f);
            }
            if (e.stT >= ed.atkDur) { e.st = ESt::Recover; e.stT = 0; }
            break;
        }
        case ESt::Recover:
            if (e.t != EType::Spore && !boss) moveDir(Mul(dir, -1), 1.2f), e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 6);
            if (e.stT >= ed.recover) { release(e); e.atkCd = ed.cd; e.st = ESt::Chase; }
            break;
        case ESt::Hit:
            if (e.stT > 0.32f) e.st = ESt::Chase;
            break;
        default: break;
        }
        if (e.t == EType::Spore) e.pos.y = 0;
    }
    // 敵同士の押し合い
    for (size_t i = 0; i < enemies.size(); i++)
        for (size_t j = i + 1; j < enemies.size(); j++)
        {
            Enemy& a = enemies[i]; Enemy& b = enemies[j];
            if (a.st == ESt::Dead || b.st == ESt::Dead) continue;
            XMFLOAT3 d = Sub(b.pos, a.pos);
            float l = LenXZ(d), m = a.radius + b.radius;
            if (l < m && l > 0.001f)
            {
                XMFLOAT3 n = Mul(d, (m - l) * 0.5f / l);
                MoveCircle(a.pos, a.radius, -n.x, -n.z);
                MoveCircle(b.pos, b.radius, n.x, n.z);
            }
        }
}

// ------------------------------------------------------------------
// 自動操作（撮影・デモ用）
// ------------------------------------------------------------------
Controls Game::AutoControls(float dt)
{
    Controls c;
    Actor& p = party[active];
    if (p.st == ASt::Down || exitMenu) return c;
    Enemy* tgt = nullptr; float best = 13.f; int ti = -1;
    for (int i = 0; i < (int)enemies.size(); i++)
    {
        auto& e = enemies[i];
        if (e.st == ESt::Dead) continue;
        float d = DistXZ(e.pos, p.pos);
        if (d < best && (e.st != ESt::Idle || d < 8)) { best = d; tgt = &e; ti = i; }
    }
    XMFLOAT3 goal{}; bool have = false; float stop = 0.6f;
    if (tgt) { goal = tgt->pos; have = true; stop = Def(p).reach * 0.65f + tgt->radius; lockIdx = ti; }
    else
    {
        lockIdx = -1;
        float bd = 1e9f;
        for (auto& k : pickups) if (!k.taken) { float d = DistXZ(k.pos, p.pos); if (d < bd) { bd = d; goal = k.pos; have = true; } }
        if (!have) { goal = Sub(exitPos, Mul(exitFace, 1.5f)); have = true; }
    }
    // 自動回避：突進の予備動作を見たら横へ
    for (auto& e : enemies)
        if (e.st == ESt::Windup && e.t == EType::Rat && e.stT > 0.35f && DistXZ(e.pos, p.pos) < 4.f && p.st == ASt::Normal && RandI(0, 2) == 0)
        {
            XMFLOAT3 side = NormXZ(F3(-(p.pos.z - e.pos.z), 0, p.pos.x - e.pos.x));
            XMFLOAT3 right{ std::cos(camYaw), 0, -std::sin(camYaw) }, fwd = Fwd(camYaw);
            c.mx = DotXZ(side, right); c.mz = DotXZ(side, fwd);
            c.dodge = true;
            return c;
        }
    if (have)
    {
        float d = DistXZ(p.pos, goal);
        XMFLOAT3 wp = (!tgt || d > 4.f) ? PathNext(p.pos, goal) : goal;
        XMFLOAT3 dir = NormXZ(Sub(wp, p.pos));
        XMFLOAT3 right{ std::cos(camYaw), 0, -std::sin(camYaw) }, fwd = Fwd(camYaw);
        if (d > stop)
        {
            c.mx = DotXZ(dir, right);
            c.mz = DotXZ(dir, fwd);
        }
        if (tgt && d < stop + 0.8f)
        {
            autoAtkT -= dt;
            if (autoHold > 0)
            {
                autoHold -= dt;
                c.atkDown = autoHold > 0;
            }
            else if (autoAtkT <= 0)
            {
                autoCount++;
                c.atkPressed = true; c.atkDown = true;
                autoAtkT = 0.2f;
                if (autoCount % 6 == 5) autoHold = 0.75f;
            }
        }
    }
    return c;
}

// ------------------------------------------------------------------
// 更新
// ------------------------------------------------------------------
void Game::UpdateDungeon(float dt, const Controls& c)
{
    float gdt = dt;
    if (hitstop > 0) { hitstop -= dt; gdt = 0; }
    Actor& P0 = party[active];

    if (exitMenu)
    {
        gdt = 0;
        bool lockNext = (floorNo == 5 && RestoreRate() < 0.3f) || (floorNo == 9 && RestoreRate() < 0.6f);
        if (c.up || c.down) exitSel = 1 - exitSel;
        if (c.confirm)
        {
            if (exitSel == 0)
            {
                if (lockNext) ToastMsg(floorNo == 5 ? L"6階への扉は 復興率30% で開く" : L"10階への扉は 復興率60% で開く", RGBA(1, 0.6f, 0.5f));
                else if (floorNo >= 10) { ToastMsg(L"第1章 クリア！", RGBA(1, 0.85f, 0.4f)); LeaveDungeon(false); return; }
                else { exitMenu = false; EnterDungeon(floorNo + 1, (uint32_t)(GetTickCount() + floorNo * 101)); return; }
            }
            else { exitMenu = false; LeaveDungeon(false); return; }
        }
    }
    else
    {
        // 灯
        float drain = floorNo >= 6 ? 0.5f : 0.333f;
        lantern = std::max(0.f, lantern - drain * gdt);
        if (lantern <= 0)
            for (auto& a : party) if (a.st != ASt::Down) { a.hp -= 1.5f * gdt; if (a.hp <= 0) { a.hp = 0; a.st = ASt::Down; } }

        camYaw += c.camYaw;
        if (c.lockon)
        {
            if (lockIdx >= 0) lockIdx = -1;
            else
            {
                float bd = 14.f;
                for (int i = 0; i < (int)enemies.size(); i++)
                {
                    if (enemies[i].st == ESt::Dead) continue;
                    float d = DistXZ(enemies[i].pos, P0.pos);
                    float facing = DotXZ(NormXZ(Sub(enemies[i].pos, P0.pos)), Fwd(camYaw));
                    if (d < bd && facing > -0.2f) { bd = d; lockIdx = i; }
                }
            }
        }
        if (lockIdx >= 0 && (enemies[lockIdx].st == ESt::Dead || DistXZ(enemies[lockIdx].pos, P0.pos) > 18)) lockIdx = -1;
        if (c.swap && party[1 - active].st != ASt::Down)
        {
            active = 1 - active;
            ToastMsg(std::wstring(Def(party[active]).name) + L" に交代");
        }
        if (c.item >= 0) { UseItem(c.item); if (scene != SceneId::Dungeon) return; }

        UpdateActor(party[active], &c, gdt, true);
        UpdateActor(party[1 - active], nullptr, gdt, false);
        if (party[active].st == ASt::Down && party[1 - active].st != ASt::Down) active = 1 - active;
        if (party[0].st == ASt::Down && party[1].st == ASt::Down)
        {
            downT += dt;
            if (downT > 2.2f) { LeaveDungeon(true); return; }
        }
        UpdateEnemies(gdt);

        // 飛び道具
        for (auto& pr : projs)
        {
            pr.pos = Add(pr.pos, Mul(pr.vel, gdt));
            pr.life -= gdt;
            if (Solid(pr.pos.x, pr.pos.z)) { pr.life = 0; SpawnFx(0, pr.pos, 1.f, C4(0.6f, 1, 0.6f), 0.3f); continue; }
            for (auto& a : party)
            {
                if (a.st == ASt::Down || pr.life <= 0) continue;
                if (DistXZ(a.pos, pr.pos) < pr.radius + 0.45f)
                {
                    DamageActor(a, pr.dmg, Sub(pr.pos, pr.vel), 4);
                    pr.life = 0;
                    SpawnFx(0, pr.pos, 1.4f, pr.kind == 0 ? C4(0.6f, 1, 0.5f) : C4(1, 1, 1), 0.35f);
                }
            }
        }
        projs.erase(std::remove_if(projs.begin(), projs.end(), [](const Projectile& p) { return p.life <= 0; }), projs.end());

        // 拾い物
        Actor& A = party[active];
        for (auto& k : pickups)
        {
            if (k.taken) continue;
            k.t += dt;
            if (k.vy != 0 || k.pos.y > 0.6f)
            {
                k.vy -= 12 * gdt;
                k.pos.y += k.vy * gdt;
                if (k.pos.y < 0.6f) { k.pos.y = 0.6f; k.vy = 0; }
            }
            float reach = k.k == PickKind::Crystal ? 1.3f : 1.1f;
            if (DistXZ(k.pos, A.pos) < reach || DistXZ(k.pos, party[1 - active].pos) < reach)
            {
                k.taken = true;
                SpawnFx(2, Add(k.pos, F3(0, 0.6f, 0)), 2.5f, C4(0.6f, 0.95f, 1), 0.6f);
                if (k.k == PickKind::Crystal)
                {
                    PartType pt = NextCrystalPart();
                    inventory[pt]++;
                    crystalsThisRun++;
                    ToastMsg(L"記憶の結晶：" + std::wstring(kParts[pt].name), RGBA(0.6f, 0.95f, 1));
                }
                else if (k.k == PickKind::Key) { hasKey = true; ToastMsg(L"鍵を手に入れた！ 出口の封印を解こう", RGBA(1, 0.85f, 0.4f)); }
                else { items[0]++; ToastMsg(L"薬草を拾った", RGBA(0.6f, 1, 0.6f)); }
            }
        }

        // 調べる
        interactHint.clear();
        Chest* nc = nullptr; Spring* ns = nullptr;
        for (auto& ch : chests) if (!ch.opened && DistXZ(ch.pos, A.pos) < 1.9f) nc = &ch;
        for (auto& s : springs) if (!s.used && DistXZ(s.pos, A.pos) < 2.4f) ns = &s;
        bool nearExit = DistXZ(exitPos, A.pos) < 2.8f;
        if (nc) interactHint = L"[E] 宝箱を開ける";
        else if (ns) interactHint = L"[E] 泉の水で 灯をともす";
        else if (nearExit) interactHint = hasKey ? L"[E] 封印を解く" : L"[E] 封印の扉を調べる";
        if (c.interact)
        {
            if (nc)
            {
                nc->opened = true;
                int it = RandI(0, 3);
                const wchar_t* names[4] = { L"薬草", L"灯油", L"研ぎ粉", L"帰り糸" };
                items[it]++;
                ToastMsg(std::wstring(L"宝箱から ") + names[it] + L" を手に入れた", RGBA(1, 0.9f, 0.6f));
                SpawnFx(2, Add(nc->pos, F3(0, 1, 0)), 2.5f, C4(1, 0.9f, 0.5f), 0.6f);
            }
            else if (ns)
            {
                ns->used = true;
                lantern = 100;
                for (auto& a : party) if (a.st != ASt::Down) a.hp = std::min(a.maxHp, a.hp + 30);
                ToastMsg(L"泉の水で灯がともった", RGBA(0.6f, 0.9f, 1));
                SpawnFx(2, Add(ns->pos, F3(0, 1, 0)), 3.5f, C4(0.6f, 0.9f, 1), 0.8f);
            }
            else if (nearExit)
            {
                if (!hasKey) ToastMsg(L"封印されている…「鍵持ち」を倒して鍵を手に入れよう", RGBA(1, 0.75f, 0.6f));
                else { exitMenu = true; exitSel = 0; }
            }
        }

        // 見た場所をミニマップに記録
        int cx = (int)(A.pos.x / CELL), cz = (int)(A.pos.z / CELL);
        int rr = RoomAt(A.pos);
        if (rr >= 0)
        {
            const Room& r = rooms[rr];
            for (int x = r.x - 1; x <= r.x + r.w; x++)
                for (int z = r.z - 1; z <= r.z + r.h; z++)
                    if (x >= 0 && z >= 0 && x < DN && z < DN) seen[x][z] = true;
        }
        for (int x = cx - 3; x <= cx + 3; x++)
            for (int z = cz - 3; z <= cz + 3; z++)
                if (x >= 0 && z >= 0 && x < DN && z < DN) seen[x][z] = true;
    }

    // 演出
    for (auto& f : fx) f.t += dt;
    fx.erase(std::remove_if(fx.begin(), fx.end(), [](const FxInst& f) { return f.t >= f.dur; }), fx.end());
    for (auto& n : nums) { n.t += dt; n.pos.y += dt * 0.9f; }
    nums.erase(std::remove_if(nums.begin(), nums.end(), [](const DmgNum& n) { return n.t > 0.9f; }), nums.end());
    shake = std::max(0.f, shake - dt * 1.5f);

    // カメラ（背後追従＋壁との衝突補正）
    Actor& Pa = party[active];
    if (lockIdx >= 0)
    {
        float ty = YawOf(Sub(enemies[lockIdx].pos, Pa.pos));
        camYaw = ApproachAngle(camYaw, ty, dt * 3.f);
    }
    else if (autopilot && Pa.speedNow > 0.5f) camYaw = ApproachAngle(camYaw, Pa.yaw, dt * 1.5f);
    XMFLOAT3 target = Add(Pa.pos, F3(0, 1.3f, 0));
    if (lockIdx >= 0) target = Lerp3(target, Add(enemies[lockIdx].pos, F3(0, 1.0f, 0)), 0.25f);
    XMFLOAT3 f = Fwd(camYaw);
    XMFLOAT3 desired = Add(target, F3(-f.x * camDist * std::cos(camPitch), camDist * std::sin(camPitch), -f.z * camDist * std::cos(camPitch)));
    for (float t = 0.3f; t <= 1.0f; t += 0.04f)
    {
        XMFLOAT3 q = Lerp3(target, desired, t);
        if (Solid(q.x, q.z) && q.y < WALL_H + 0.4f)
        {
            desired = Lerp3(target, desired, std::max(0.25f, t - 0.06f));
            break;
        }
    }
    camPosNow = Lerp3(camPosNow, desired, Saturate(dt * 9));
    camTargetNow = Lerp3(camTargetNow, target, Saturate(dt * 12));
}

// ------------------------------------------------------------------
// 描画
// ------------------------------------------------------------------
void Game::RenderDungeon()
{
    FrameSettings fs;
    bool deep = floorNo >= 6;
    float lk = Saturate(lantern / 100.f);
    XMFLOAT3 sh{ 0, 0, 0 };
    if (shake > 0) sh = F3(std::sin(time * 71) * shake * 0.25f, std::sin(time * 53) * shake * 0.2f, std::cos(time * 67) * shake * 0.25f);
    fs.camPos = Add(camPosNow, sh);
    fs.camTarget = Add(camTargetNow, Mul(sh, 0.5f));
    fs.time = time;
    fs.fogColor = deep ? F3(0.07f, 0.06f, 0.11f) : F3(0.06f, 0.09f, 0.13f);
    fs.clear = fs.fogColor;
    fs.fogStart = Lerpf(2.f, 8.f, lk);
    fs.fogEnd = Lerpf(12.f, deep ? 26.f : 32.f, SmoothStep(0.f, 0.5f, lk));
    fs.fogMax = 1.f;
    fs.ambient = deep ? F3(0.27f, 0.24f, 0.38f) : F3(0.3f, 0.34f, 0.44f);
    fs.lightDir = F3(0.35f, -1.f, 0.5f);
    fs.lightColor = deep ? F3(0.42f, 0.4f, 0.62f) : F3(0.45f, 0.52f, 0.66f);
    Actor& A = party[active];
    fs.lights.push_back({ Add(A.pos, F3(0, 2.4f, 0)), 6.f + 8.f * lk, Mul(F3(1.25f, 0.85f, 0.5f), 0.45f + 0.75f * lk) });
    for (auto& k : pickups)
        if (!k.taken) fs.lights.push_back({ Add(k.pos, F3(0, 1.0f, 0)), k.k == PickKind::Crystal ? 4.5f : 3.f,
                                            k.k == PickKind::Crystal ? F3(0.3f, 0.8f, 1.0f) : F3(1.0f, 0.8f, 0.3f) });
    for (auto& d : decos) if (d.kind == 1) fs.lights.push_back({ Add(d.pos, F3(0, 0.6f, 0)), 3.2f, F3(0.15f, 0.6f, 0.55f) });
    fs.lights.push_back({ Add(exitPos, F3(0, 1.6f, -1.0f)), 6.f, hasKey ? F3(1.0f, 0.8f, 0.35f) : F3(0.6f, 0.35f, 0.9f) });
    for (auto& s : springs) fs.lights.push_back({ Add(s.pos, F3(0, 0.8f, 0)), 5.f, s.used ? F3(0.2f, 0.3f, 0.4f) : F3(0.35f, 0.7f, 1.0f) });
    for (auto& p : projs) fs.lights.push_back({ p.pos, 3.f, p.kind == 0 ? F3(0.5f, 1.0f, 0.4f) : F3(0.8f, 0.8f, 1.0f) });
    for (auto& f : fx) if (f.kind == 0) fs.lights.push_back({ f.pos, 3.5f * (1 - f.t / f.dur), F3(1.2f, 0.8f, 0.4f) });
    R->BeginFrame(fs);

    // 地形
    DrawParams fl; fl.tex = T["cave_floor"]; fl.color = deep ? C4(0.8f, 0.75f, 0.95f) : C4(0.85f, 0.9f, 1.0f); fl.rim = 0;
    R->Draw(floorMesh, XMMatrixIdentity(), fl);
    DrawParams wl; wl.tex = T["cave_wall"]; wl.color = deep ? C4(0.75f, 0.68f, 0.9f) : C4(0.82f, 0.88f, 1.0f); wl.rim = 0.1f;
    R->Draw(wallMesh, XMMatrixIdentity(), wl);

    // 飾り
    for (auto& d : decos)
    {
        DrawParams p;
        switch (d.kind)
        {
        case 0:
        {
            p.tex = T["bark"]; p.color = C4(0.55f, 0.42f, 0.32f); p.outline = 0.02f;
            XMFLOAT3 in = Fwd(d.rot);
            XMFLOAT3 top = Add(d.pos, F3(0, WALL_H * 0.95f, 0));
            XMFLOAT3 mid = Add(d.pos, Add(Mul(in, 0.6f * d.scale), F3(std::sin(d.tilt) * 0.4f, 1.2f, std::cos(d.tilt) * 0.2f)));
            XMFLOAT3 bot = Add(d.pos, Add(Mul(in, 1.1f * d.scale), F3(d.tilt, 0, 0)));
            Seg(top, mid, 0.28f * d.scale, p);
            Seg(mid, bot, 0.2f * d.scale, p);
            break;
        }
        case 1:
        {
            float s = d.scale;
            p.color = C4(0.85f, 0.88f, 0.85f);
            R->Draw(R->cylinder, XMMatrixScaling(0.12f * s, 0.35f * s, 0.12f * s) * XMMatrixTranslation(d.pos.x, 0, d.pos.z), p);
            DrawParams cp; cp.color = C4(0.2f, 0.7f, 0.65f); cp.emissive = { 0.1f, 0.45f, 0.42f }; cp.outline = 0.015f;
            R->Draw(R->sphere, XMMatrixScaling(0.4f * s, 0.2f * s, 0.4f * s) * XMMatrixTranslation(d.pos.x, 0.36f * s, d.pos.z), cp);
            R->Draw(R->cylinder, XMMatrixScaling(0.08f * s, 0.22f * s, 0.08f * s) * XMMatrixTranslation(d.pos.x + 0.25f, 0, d.pos.z + 0.1f), p);
            R->Draw(R->sphere, XMMatrixScaling(0.24f * s, 0.12f * s, 0.24f * s) * XMMatrixTranslation(d.pos.x + 0.25f, 0.22f * s, d.pos.z + 0.1f), cp);
            break;
        }
        case 2:
            p.tex = T["cave_wall"]; p.color = C4(0.7f, 0.75f, 0.85f); p.outline = 0.015f;
            R->Draw(R->sphere, XMMatrixScaling(1.2f * d.scale, 0.7f * d.scale, 1.0f * d.scale) * XMMatrixRotationY(d.rot) * XMMatrixTranslation(d.pos.x, 0.1f, d.pos.z), p);
            break;
        case 3:
        {
            DrawParams cp; cp.color = deep ? C4(0.6f, 0.4f, 0.9f) : C4(0.45f, 0.55f, 0.95f);
            cp.emissive = deep ? F3(0.2f, 0.1f, 0.35f) : F3(0.08f, 0.15f, 0.35f); cp.rim = 0.5f;
            for (int i = 0; i < 3; i++)
            {
                float a = d.rot + i * 2.1f;
                R->Draw(R->octa, XMMatrixScaling(0.25f * d.scale, (0.9f - i * 0.2f) * d.scale, 0.25f * d.scale) * XMMatrixRotationZ(0.3f * std::sin(a)) * XMMatrixRotationY(a) *
                                 XMMatrixTranslation(d.pos.x + std::cos(a) * 0.25f, 0.3f * d.scale, d.pos.z + std::sin(a) * 0.25f), cp);
            }
            break;
        }
        }
    }

    // 出口（封印の扉）
    {
        float yaw = YawOf(exitFace);
        XMMATRIX base = XMMatrixRotationY(yaw) * XMMatrixTranslation(exitPos.x, 0, exitPos.z);
        DrawParams seal; seal.tex = hasKey ? T["seal_open"] : T["seal_closed"]; seal.unlit = true; seal.fog = 0.3f;
        seal.color = C4(1, 1, 1);
        R->Draw(R->quad, XMMatrixScaling(1.9f, 2.6f, 1) * XMMatrixTranslation(0, 1.35f, -0.03f) * base, seal);
        DrawParams wd; wd.color = C4(0.45f, 0.25f, 0.2f); wd.outline = 0.02f;
        R->Draw(R->cube, XMMatrixScaling(0.28f, 3.0f, 0.28f) * XMMatrixTranslation(-1.05f, 1.5f, -0.2f) * base, wd);
        R->Draw(R->cube, XMMatrixScaling(0.28f, 3.0f, 0.28f) * XMMatrixTranslation(1.05f, 1.5f, -0.2f) * base, wd);
        R->Draw(R->cube, XMMatrixScaling(2.8f, 0.25f, 0.4f) * XMMatrixTranslation(0, 3.0f, -0.2f) * base, wd);
        DrawParams rp; rp.tex = T["rope"]; rp.color = C4(1, 0.92f, 0.7f); rp.uv = { 4, 1, 0, 0.35f };
        R->Draw(R->cylinder, XMMatrixScaling(0.2f, 2.3f, 0.2f) * XMMatrixRotationZ(PI / 2) * XMMatrixTranslation(1.15f, 2.7f, -0.3f) * base, rp);
    }

    // 宝箱
    for (auto& ch : chests)
    {
        XMMATRIX b = XMMatrixRotationY(ch.yaw) * XMMatrixTranslation(ch.pos.x, 0, ch.pos.z);
        DrawShadow(ch.pos, 1.6f);
        DrawParams p; p.color = C4(0.6f, 0.15f, 0.12f); p.outline = 0.02f;
        R->Draw(R->cube, XMMatrixScaling(1.0f, 0.55f, 0.65f) * XMMatrixTranslation(0, 0.3f, 0) * b, p);
        DrawParams g; g.color = C4(0.95f, 0.75f, 0.3f); g.emissive = { 0.1f, 0.07f, 0.02f };
        R->Draw(R->cube, XMMatrixScaling(1.04f, 0.1f, 0.69f) * XMMatrixTranslation(0, 0.5f, 0) * b, g);
        float open = ch.opened ? -1.4f : 0;
        R->Draw(R->cube, XMMatrixScaling(1.0f, 0.22f, 0.65f) * XMMatrixTranslation(0, 0.11f, 0.32f) * XMMatrixRotationX(open) * XMMatrixTranslation(0, 0.58f, -0.32f) * b, p);
        R->Draw(R->cube, XMMatrixScaling(0.18f, 0.2f, 0.05f) * XMMatrixTranslation(0, 0.5f, -0.34f) * b, g);
    }
    // 泉
    for (auto& s : springs)
    {
        DrawParams st; st.tex = T["spring"]; st.outline = 0.02f;
        R->Draw(R->cylinder, XMMatrixScaling(2.4f, 0.45f, 2.4f) * XMMatrixTranslation(s.pos.x, 0, s.pos.z), st);
        DrawParams w; w.tex = T["water"]; w.uv = { 1, 1, time * 0.05f, 0 }; w.color = s.used ? C4(0.4f, 0.5f, 0.6f) : C4(0.6f, 0.9f, 1);
        w.emissive = s.used ? F3(0, 0, 0) : F3(0.1f, 0.3f, 0.45f);
        R->Draw(R->cylinder, XMMatrixScaling(2.0f, 0.02f, 2.0f) * XMMatrixTranslation(s.pos.x, 0.42f, s.pos.z), w);
    }

    // 拾い物
    for (auto& k : pickups)
    {
        if (k.taken) continue;
        float b = std::sin(k.t * 2.5f) * 0.15f;
        XMFLOAT3 p = Add(k.pos, F3(0, k.k == PickKind::Crystal ? 0.9f + b : b * 0.5f, 0));
        if (k.k == PickKind::Crystal)
        {
            DrawShadow(k.pos, 1.0f, 0.35f);
            DrawParams cp; cp.color = C4(0.55f, 0.9f, 1.0f); cp.emissive = { 0.25f, 0.55f, 0.7f }; cp.rim = 0.8f; cp.outline = 0.015f;
            R->Draw(R->octa, XMMatrixScaling(0.45f, 0.8f, 0.45f) * XMMatrixRotationY(k.t * 1.5f) * XMMatrixTranslation(p.x, p.y, p.z), cp);
            R->DrawBillboard(T["glint"], p, 2.2f, 2.2f, C4(0.5f, 0.9f, 1.f, 0.7f), Blend::Add, { 1, 1, 0, 0 }, k.t * 0.5f);
        }
        else if (k.k == PickKind::Key)
        {
            DrawParams g; g.color = C4(1.f, 0.8f, 0.3f); g.emissive = { 0.4f, 0.3f, 0.05f }; g.outline = 0.015f;
            XMMATRIX b2 = XMMatrixRotationY(k.t * 2) * XMMatrixTranslation(p.x, p.y + 0.3f, p.z);
            R->Draw(R->cylinder, XMMatrixScaling(0.4f, 0.08f, 0.4f) * XMMatrixRotationX(PI / 2) * XMMatrixTranslation(0, 0.3f, 0) * b2, g);
            R->Draw(R->cube, XMMatrixScaling(0.08f, 0.5f, 0.08f) * XMMatrixTranslation(0, -0.05f, 0) * b2, g);
            R->Draw(R->cube, XMMatrixScaling(0.16f, 0.08f, 0.08f) * XMMatrixTranslation(0.08f, -0.25f, 0) * b2, g);
            R->DrawBillboard(T["glint"], Add(p, F3(0, 0.3f, 0)), 2.f, 2.f, C4(1, 0.85f, 0.4f, 0.7f), Blend::Add);
        }
        else
        {
            DrawParams g; g.color = C4(0.4f, 0.8f, 0.35f); g.outline = 0.015f;
            R->Draw(R->sphere, XMMatrixScaling(0.35f, 0.2f, 0.35f) * XMMatrixTranslation(p.x, p.y + 0.1f, p.z), g);
            R->Draw(R->sphere, XMMatrixScaling(0.2f, 0.35f, 0.2f) * XMMatrixTranslation(p.x + 0.1f, p.y + 0.25f, p.z), g);
        }
    }

    // 敵
    for (auto& e : enemies) DrawEnemy(e);

    // パーティ
    for (int i = 0; i < 2; i++)
    {
        const Actor& a = party[i];
        if (a.def == 0) DrawSora(a, 1); else DrawKurobee(a, 1);
    }
    DrawTsumugi(Add(A.pos, Add(Mul(Fwd(A.yaw + 2.4f), 0.9f), F3(0, 2.0f + std::sin(time * 2.2f) * 0.15f, 0))));

    // 飛び道具
    for (auto& p : projs)
    {
        DrawParams g; g.unlit = true;
        g.color = p.kind == 0 ? C4(0.6f, 1.f, 0.45f) : C4(0.95f, 0.95f, 1.f);
        R->Draw(R->sphere, XMMatrixScaling(0.35f, 0.35f, 0.35f) * XMMatrixTranslation(p.pos.x, p.pos.y, p.pos.z), g);
        R->DrawBillboard(T["glint"], p.pos, 1.6f, 1.6f, p.kind == 0 ? C4(0.5f, 1, 0.4f, 0.8f) : C4(1, 1, 1, 0.7f), Blend::Add, { 1, 1, 0, 0 }, time * 4);
    }

    // マーカー
    for (auto& e : enemies)
    {
        if (e.st == ESt::Dead) continue;
        float h = e.t == EType::Spider ? 4.2f : 1.9f;
        if (e.st == ESt::Notice)
            R->DrawBillboard(T["exclaim"], Add(e.pos, F3(0, h + 0.3f, 0)), 0.9f, 0.9f, C4(1, 1, 1, 1), Blend::Alpha, { 1, 1, 0, 0 }, 0, false, 0);
        if (e.keyHolder)
            R->DrawBillboard(T["glint"], Add(e.pos, F3(0, h, 0)), 1.2f, 1.2f, C4(1, 0.85f, 0.3f, 0.9f), Blend::Add, { 1, 1, 0, 0 }, time * 2);
    }
    if (lockIdx >= 0)
    {
        const Enemy& e = enemies[lockIdx];
        float h = e.t == EType::Spider ? 1.6f : 0.8f;
        R->DrawBillboard(T["lockon"], Add(e.pos, F3(0, h, 0)), 1.4f + (e.t == EType::Spider ? 2.f : 0.f), 1.4f + (e.t == EType::Spider ? 2.f : 0.f),
                         C4(1, 0.85f, 0.4f, 0.95f), Blend::Alpha, { 1, 1, 0, 0 }, time * 1.5f, false, 0);
    }

    // エフェクト（連番画像）
    for (auto& f : fx)
    {
        float k = f.t / f.dur;
        if (f.kind == 0 || f.kind == 1)
        {
            int frame = std::min(15, (int)(k * 16));
            XMFLOAT4 uv{ 0.25f, 0.25f, (frame % 4) * 0.25f, (frame / 4) * 0.25f };
            XMFLOAT4 col = f.color; col.w = 1.f;
            R->DrawBillboard(f.kind == 0 ? T["spark"] : T["ring"], f.pos, f.size, f.size, col, Blend::Add, uv, f.rot, true, 0.3f);
        }
        else
        {
            XMFLOAT4 col = f.color; col.w = 1 - k;
            R->DrawBillboard(T["glint"], f.pos, f.size * (0.5f + k), f.size * (0.5f + k), col, Blend::Add, { 1, 1, 0, 0 }, f.rot + k * 2);
        }
    }

    // 画面効果
    float hpK = A.hp / A.maxHp;
    if (lantern < 30)
        R->DrawScreenOverlay(T["inkborder"], C4(0, 0, 0, Saturate((30 - lantern) / 22.f) * 0.95f), Blend::Alpha);
    if (hpK < 0.4f || A.invuln > 0.2f)
    {
        float a = std::max(Saturate((0.4f - hpK) / 0.3f) * (0.6f + 0.25f * std::sin(time * 6)), A.invuln > 0.2f ? 0.6f : 0.f);
        R->DrawScreenOverlay(T["vignette"], C4(1, 1, 1, a), Blend::Alpha);
    }
}

void Game::DrawDungeonUI()
{
    float W = (float)R->W, H = (float)R->H;
    wchar_t buf[128];
    Actor& A = party[active];
    Actor& B = party[1 - active];
    const CharDef& d = Def(A);

    // 敵のHP・ダメージ数字
    for (auto& e : enemies)
    {
        if (e.st == ESt::Dead || e.t == EType::Spider) continue;
        if (e.hp >= e.maxHp && !e.keyHolder) continue;
        float sx, sy;
        if (!R->Project(Add(e.pos, F3(0, 1.75f, 0)), sx, sy)) continue;
        Bar(sx - 30, sy, 60, 7, e.hp / e.maxHp, RGBA(0.9f, 0.3f, 0.3f), RGBA(0, 0, 0, 0.6f));
        if (e.keyHolder)
        {
            R->FillCircle(sx - 44, sy + 3, 11, RGBA(0.9f, 0.7f, 0.25f));
            R->Text(L"鍵", sx - 55, sy - 7, 22, 20, 13, RGBA(0.2f, 0.12f, 0.05f), 1, true, false);
        }
    }
    for (auto& n : nums)
    {
        float sx, sy;
        if (!R->Project(n.pos, sx, sy)) continue;
        float pop = n.t < 0.1f ? 1.f + (0.1f - n.t) * 4 : 1.f;
        D2D1_COLOR_F c = n.col; c.a = Saturate((0.9f - n.t) / 0.3f);
        float s = n.size * pop;
        R->Text(std::to_wstring(n.val), sx - 60, sy - s * 0.6f, 120, s * 1.3f, s, c, 1, true, true);
    }

    // 左上：ステータス
    R->FillRound(16, 14, 410, 150, 12, RGBA(0.05f, 0.07f, 0.13f, 0.78f));
    R->FillCircle(52, 52, 26, d.badger ? RGBA(0.45f, 0.45f, 0.5f) : RGBA(0.2f, 0.3f, 0.6f));
    R->StrokeCircle(52, 52, 26, RGBA(0.9f, 0.65f, 0.3f), 2.5f);
    R->Text(d.badger ? L"ク" : L"ソ", 30, 34, 44, 40, 24, RGBA(1, 1, 1), 1, true, false);
    R->Text(d.name, 88, 20, 150, 30, 21, RGBA(1, 0.95f, 0.85f), 0, true);
    swprintf_s(buf, L"HP %d / %d", (int)std::ceil(A.hp), (int)A.maxHp);
    R->Text(buf, 260, 23, 150, 24, 14, RGBA(1, 1, 1, 0.85f), 2);
    float hk = A.hp / A.maxHp;
    Bar(88, 50, 320, 13, hk, hk > 0.35f ? RGBA(0.45f, 0.85f, 0.45f) : RGBA(0.95f, 0.35f, 0.3f), RGBA(0.15f, 0.15f, 0.2f, 0.9f));
    // 灯
    R->Text(L"灯", 88, 72, 24, 24, 15, RGBA(1, 0.8f, 0.4f), 0, true);
    bool lanternLow = lantern < 25;
    float blink = lanternLow ? 0.6f + 0.4f * std::sin(time * 8) : 1.f;
    Bar(110, 78, 298, 11, lantern / 100.f, RGBA(1.f, 0.7f, 0.25f, blink), RGBA(0.15f, 0.15f, 0.2f, 0.9f));
    // 武器
    float dk = A.maxDur > 0 ? A.dur / A.maxDur : 0;
    D2D1_COLOR_F wc = RGBA(0.85f, 0.9f, 1);
    if (!A.broken && dk <= 0.1f) wc = RGBA(1, 0.3f, 0.25f, 0.55f + 0.45f * std::sin(time * 12));
    else if (!A.broken && dk <= 0.25f) wc = RGBA(1, 0.85f, 0.25f);
    swprintf_s(buf, L"%s  Lv%d", A.broken ? d.spare : d.weapon, A.lvl);
    R->Text(buf, 24, 100, 260, 24, 16, A.broken ? RGBA(0.7f, 0.7f, 0.75f) : RGBA(1, 1, 1), 0, true);
    if (A.broken) R->Text(L"（予備・壊れない）", 220, 101, 190, 24, 14, RGBA(1, 0.6f, 0.5f), 2);
    else
    {
        swprintf_s(buf, L"耐久 %d / %d", (int)A.dur, (int)A.maxDur);
        R->Text(buf, 220, 101, 190, 24, 14, wc, 2, true);
    }
    Bar(24, 126, 384, 9, A.broken ? 0.f : dk, wc, RGBA(0.15f, 0.15f, 0.2f, 0.9f));
    Bar(24, 142, 384, 5, A.exp / (100.f * A.lvl), RGBA(0.55f, 0.75f, 1.f), RGBA(0.15f, 0.15f, 0.2f, 0.9f));
    if (!A.broken && dk <= 0.1f)
        R->Text(L"⚠ 刃が欠けそう！", 270, 150, 160, 24, 14, wc, 2, true);

    // 控え
    const CharDef& bd = Def(B);
    R->FillRound(16, 172, 250, 52, 10, RGBA(0.05f, 0.07f, 0.13f, 0.7f));
    R->FillCircle(42, 198, 17, bd.badger ? RGBA(0.45f, 0.45f, 0.5f) : RGBA(0.2f, 0.3f, 0.6f));
    R->Text(bd.badger ? L"ク" : L"ソ", 25, 186, 34, 26, 16, RGBA(1, 1, 1), 1, true, false);
    R->Text(std::wstring(bd.name) + L"（控え）", 66, 178, 190, 22, 14, RGBA(1, 1, 1, 0.9f), 0, true);
    Bar(66, 203, 186, 8, B.hp / B.maxHp, B.st == ASt::Down ? RGBA(0.5f, 0.5f, 0.5f) : RGBA(0.45f, 0.85f, 0.45f), RGBA(0.15f, 0.15f, 0.2f, 0.9f));

    // 右上：ミニマップ
    float ms = 6.6f, mx0 = W - DN * ms - 18, my0 = 14;
    R->FillRound(mx0 - 8, my0, DN * ms + 16, DN * ms + 56, 10, RGBA(0.05f, 0.07f, 0.13f, 0.78f));
    float myb = my0 + 8 + DN * ms;
    for (int x = 0; x < DN; x++)
        for (int z = 0; z < DN; z++)
            if (seen[x][z] && cell[x][z]) R->FillRect(mx0 + x * ms, myb - (z + 1) * ms, ms, ms, RGBA(0.55f, 0.62f, 0.75f, 0.85f));
    if (seen[exitX][exitZ - 1] || seen[exitX][exitZ])
        R->FillRect(mx0 + exitX * ms - 1, myb - (exitZ + 1) * ms - 1, ms + 2, ms + 2, hasKey ? RGBA(1, 0.8f, 0.3f) : RGBA(0.7f, 0.4f, 1));
    for (auto& k : pickups)
    {
        if (k.taken || k.k != PickKind::Crystal) continue;
        int cx = (int)(k.pos.x / CELL), cz = (int)(k.pos.z / CELL);
        if (seen[cx][cz]) R->FillCircle(mx0 + (k.pos.x / CELL) * ms, myb - (k.pos.z / CELL) * ms, 3, RGBA(0.5f, 0.95f, 1));
    }
    for (auto& ch : chests)
    {
        int cx = (int)(ch.pos.x / CELL), cz = (int)(ch.pos.z / CELL);
        if (seen[cx][cz] && !ch.opened) R->FillRect(mx0 + (ch.pos.x / CELL) * ms - 2.5f, myb - (ch.pos.z / CELL) * ms - 2.5f, 5, 5, RGBA(0.95f, 0.5f, 0.3f));
    }
    R->FillCircle(mx0 + (B.pos.x / CELL) * ms, myb - (B.pos.z / CELL) * ms, 3, RGBA(0.8f, 0.8f, 0.85f));
    float px = mx0 + (A.pos.x / CELL) * ms, py = myb - (A.pos.z / CELL) * ms;
    R->FillCircle(px, py, 4.5f, RGBA(1, 0.85f, 0.3f));
    XMFLOAT3 fd = Fwd(A.yaw);
    R->Line(px, py, px + fd.x * 9, py - fd.z * 9, RGBA(1, 0.85f, 0.3f), 2);
    swprintf_s(buf, L"根の洞　%d / 10", floorNo);
    R->Text(buf, mx0, myb + 8, DN * ms, 26, 17, RGBA(1, 0.92f, 0.75f), 0, true);
    R->Text(hasKey ? L"鍵 ✓" : L"鍵 －", mx0, myb + 10, DN * ms, 24, 15, hasKey ? RGBA(1, 0.85f, 0.4f) : RGBA(1, 1, 1, 0.6f), 2, true);
    swprintf_s(buf, L"結晶 ×%d", crystalsThisRun);
    R->Text(buf, mx0 - 130, my0 + 6, 120, 24, 15, RGBA(0.6f, 0.95f, 1), 2, true);

    // ボスのHP
    for (auto& e : enemies)
    {
        if (e.t != EType::Spider || e.st == ESt::Dead) continue;
        float bw = 560, bx = (W - bw) / 2;
        R->Text(L"中ボス　ほつれ蜘蛛", bx, 16, bw, 28, 20, RGBA(1, 0.9f, 0.85f), 1, true);
        Bar(bx, 46, bw, 14, e.hp / e.maxHp, RGBA(0.85f, 0.25f, 0.3f), RGBA(0.1f, 0.08f, 0.12f, 0.85f));
    }

    // 右下：アイテム
    const wchar_t* inames[4] = { L"薬草", L"灯油", L"研ぎ粉", L"帰り糸" };
    D2D1_COLOR_F icol[4] = { RGBA(0.4f, 0.8f, 0.4f), RGBA(1, 0.7f, 0.3f), RGBA(0.75f, 0.8f, 0.9f), RGBA(0.9f, 0.75f, 0.4f) };
    for (int i = 0; i < 4; i++)
    {
        float x = W - 4 * 78 - 14 + i * 78, y = H - 112;
        R->FillRound(x, y, 70, 70, 10, RGBA(0.05f, 0.07f, 0.13f, 0.78f));
        R->StrokeRound(x, y, 70, 70, 10, items[i] > 0 ? icol[i] : RGBA(1, 1, 1, 0.2f), 2);
        R->FillCircle(x + 35, y + 30, 13, items[i] > 0 ? icol[i] : RGBA(0.3f, 0.3f, 0.3f));
        R->Text(inames[i], x, y + 46, 70, 20, 13, RGBA(1, 1, 1, 0.9f), 1, true);
        swprintf_s(buf, L"%d", i + 1);
        R->Text(buf, x + 5, y + 2, 20, 20, 13, RGBA(1, 1, 1, 0.6f));
        swprintf_s(buf, L"×%d", items[i]);
        R->Text(buf, x + 30, y + 2, 36, 20, 14, RGBA(1, 0.95f, 0.8f), 2, true);
    }

    if (lantern < 25)
        R->Text(lantern <= 0 ? L"灯が消えた…体力が削られていく" : L"灯が消えかけている…", 0, H - 200, W, 30, 20, RGBA(1, 0.7f, 0.4f, 0.6f + 0.4f * std::sin(time * 4)), 1, true);
    if (!interactHint.empty() && !exitMenu)
    {
        R->FillRound(W / 2 - 160, H - 170, 320, 40, 20, RGBA(0.05f, 0.07f, 0.13f, 0.8f));
        R->Text(interactHint, W / 2 - 160, H - 162, 320, 30, 18, RGBA(1, 0.92f, 0.7f), 1, true);
    }

    // 出口メニュー
    if (exitMenu)
    {
        R->FillRect(0, 0, W, H, RGBA(0, 0, 0, 0.35f));
        float bw = 520, bh = 250, bx = (W - bw) / 2, by = (H - bh) / 2 - 20;
        R->FillRound(bx, by, bw, bh, 16, RGBA(0.06f, 0.09f, 0.17f, 0.95f));
        R->StrokeRound(bx + 5, by + 5, bw - 10, bh - 10, 12, RGBA(0.85f, 0.6f, 0.25f), 2);
        R->Text(L"封印が解けた", bx, by + 20, bw, 34, 26, RGBA(1, 0.9f, 0.7f), 1, true);
        bool lockNext = (floorNo == 5 && RestoreRate() < 0.3f) || (floorNo == 9 && RestoreRate() < 0.6f);
        std::wstring o1 = floorNo >= 10 ? L"章を終える" : L"次の階へ進む（" + std::to_wstring(floorNo + 1) + L"階）";
        if (lockNext) o1 += floorNo == 5 ? L"　※復興率30%で解放" : L"　※復興率60%で解放";
        swprintf_s(buf, L"村へ戻る（結晶 %d 個を持ち帰る）", crystalsThisRun);
        std::wstring opts[2] = { o1, buf };
        for (int i = 0; i < 2; i++)
        {
            float y = by + 82 + i * 64;
            bool sel = exitSel == i;
            R->FillRound(bx + 40, y, bw - 80, 50, 10, sel ? RGBA(0.98f, 0.94f, 0.85f) : RGBA(1, 1, 1, 0.08f));
            R->Text(opts[i], bx + 40, y + 11, bw - 80, 30, 20, sel ? RGBA(0.12f, 0.15f, 0.25f) : RGBA(1, 1, 1, 0.9f), 1, true, !sel);
        }
        R->Text(L"W/S：選択　Enter：決定", bx, by + bh - 34, bw, 24, 14, RGBA(1, 1, 1, 0.6f), 1);
    }

    R->FillRect(0, H - 30, W, 30, RGBA(0.04f, 0.05f, 0.1f, 0.65f));
    R->Text(L"WASD 移動　マウス カメラ　左クリック 攻撃（長押しで溜め）　右クリック ガード　Space 回避　Q ロックオン　E 調べる　Tab 交代　1-4 アイテム",
            0, H - 26, W, 24, 14, RGBA(1, 1, 1, 0.8f), 1, false, false);
}
