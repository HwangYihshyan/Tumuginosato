#include "Game.h"

// ------------------------------------------------------------------
// キャラクター・武器・敵のデータ
// ------------------------------------------------------------------
static const CharDef kChars[3] = {
    // name        spare     hp   spd   reach  arc               dmg              dur                     cReach cArc cDmg hitFrac badger ranged
    { L"ソラ",     L"縫い針", 100, 5.4f, 2.9f, { 150, 150, 70 }, { 11, 12, 20 }, { 0.36f, 0.36f, 0.56f }, 3.3f, 110, 38, 0.35f, false, false },
    { L"クロベエ", L"小槌",   130, 4.7f, 4.2f, { 120, 120, 160 }, { 10, 10, 16 }, { 0.5f, 0.5f, 0.66f },  0, 0, 0,     0.5f,  true,  false },
    { L"カイリ",   L"まち針", 90,  5.2f, 9.0f, { 14, 14, 30 },   { 8, 8, 7 },    { 0.32f, 0.32f, 0.5f },  14, 10, 32,  0.4f,  false, true },
};

const WeaponDef kWeapons[W_COUNT] = {
    { L"大糸切り鋏",     0, 1.0f,  60, 0, L"ソラの最初の武器。開いて挟み、一本の刃で振り抜く" },
    { L"緋糸切り",       0, 1.25f, 55, 1, L"火の力を帯びた鋏。根っこ人形と蜘蛛によく効く" },
    { L"水糸切り",       0, 1.2f,  70, 2, L"水の力を帯びた鋏。胞子とネズミによく効く" },
    { L"古いひも通し",   1, 1.0f,  50, 0, L"クロベエの最初の武器。投げて扇形に薙ぎ払い、引き戻す" },
    { L"炭火のひも通し", 1, 1.3f,  55, 1, L"炭火で焼き締めたひも通し" },
    { L"ミシン針",       2, 1.0f,  70, 0, L"カイリの最初の武器。指の間に挟んで投げる" },
    { L"潮目の針",       2, 1.25f, 75, 2, L"潮の流れを宿した針" },
};

const AttachDef kAttach[AT_COUNT] = {
    { L"硬い石",   L"攻撃 +15%" },
    { L"火の欠片", L"攻撃 +10%・火の力を帯びる" },
    { L"軽い羽",   L"移動 +12%" },
    { L"鋼糸",     L"耐久が減りにくい（40%）" },
};

struct EDef { const wchar_t* name; float hp, speed, notice, atkRange, windup, atkDur, recover, dmg, radius, cd; int weak; };
static const EDef kEDefs[(int)EType::COUNT] = {
    { L"霧ネズミ",   28, 3.3f, 9, 4.2f, 0.55f, 0.42f, 0.6f, 8, 0.45f, 1.4f, 2 },
    { L"根っこ人形", 46, 2.1f, 8, 1.7f, 0.6f, 0.35f, 0.7f, 12, 0.5f, 1.5f, 1 },
    { L"胞子",       24, 1.7f, 10, 9.f, 0.7f, 0.25f, 0.8f, 7, 0.45f, 2.2f, 2 },
    { L"ほつれ蜘蛛", 340, 2.5f, 30, 4.5f, 0.85f, 0.5f, 0.9f, 16, 1.7f, 1.1f, 1 },
    { L"大樹の番人", 650, 1.5f, 40, 6.0f, 1.0f, 0.55f, 1.0f, 18, 2.2f, 1.3f, 1 },
    { L"根の主の腕", 400, 0.f, 40, 0.f, 9, 9, 9, 0, 2.5f, 99, 0 },
    { L"糸喰い蛾",   34, 2.8f, 11, 8.5f, 0.6f, 0.25f, 0.7f, 8, 0.45f, 1.8f, 1 },     // 飛び回り、2発の糸玉を撃つ
    { L"影法師",     52, 4.2f, 10, 5.5f, 0.45f, 0.38f, 0.5f, 11, 0.45f, 1.2f, 2 },   // 2回続けて突進する
    { L"根の鎧武者", 130, 1.9f, 9, 2.3f, 0.8f, 0.45f, 0.8f, 16, 0.6f, 1.6f, 1 },     // 正面からの攻撃を半分受け流す
    { L"うつろ",     733, 2.4f, 40, 9.f, 0.9f, 0.6f, 0.8f, 18, 1.6f, 1.2f, 0 },     // 12階のQTE・15階のボス（体力は前の2/3）
};

const CharDef& Game::Def(const Actor& a) const { return kChars[a.def]; }
const wchar_t* EnemyName(EType t) { return kEDefs[(int)t].name; }

// 各階の糸巻きから出るジオラマパーツ（ふつうの階は10個ずつ12階ぶん＝120個、12階のQTEのあとにカイリ1個）
//   はめ込み枠の合計（村長を除いて29）と、はめ込みパーツの数（29。余っていた酒樽2つは川にした）をそろえてある。ボスの階（5・15）は宝箱なし
#define SB(s) (SUB_ID + (s))
const FloorLootDef kFloorLoot[MAX_FLOOR + 1] = {
    { { -1 } },
    { { P_Shop, SB(S_Merchant), SB(S_Sign), SB(S_Barrel), SB(S_Noren), SB(S_Pot), P_Path, P_Path, P_Tree, P_Lantern } },          // 1階：商店一式
    { { P_Kiln, SB(S_Wood), SB(S_Wood), SB(S_Lamp), P_HouseKurobee, P_Path, P_Tree, P_Tree, P_River, P_Bridge } },                 // 2階：炭焼き窯一式（→クロベエ）
    { { P_Loom, SB(S_Weaver), SB(S_Noren), SB(S_Lamp), P_Cloth, P_Tree, P_Tree, P_Lantern, P_Path, P_River } },                     // 3階：機織り小屋一式
    { { SB(S_Noren), SB(S_Wood), SB(S_Sign), P_Well, P_River, P_Tree, P_Tree, P_Path, P_Path, P_Lantern } },                  // 4階：クロベエの家の飾り・井戸
    { { -1 } },                                                                                                                      // 5階：中ボス（宝箱なし）
    { { P_HouseRabbit, SB(S_Rabbits), SB(S_Pot), SB(S_Wood), P_Field, SB(S_Pot), P_Tree, P_Tree, P_Path, P_River } },             // 6階
    { { P_Forge, SB(S_Smith), SB(S_Barrel), SB(S_Sign), P_Tree, P_Tree, P_Lantern, P_Path, P_Path, P_River } },                    // 7階：鍛冶屋一式
    { { P_Wheel, P_Well, P_River, P_Bridge, P_River, P_Tree, P_Tree, P_Lantern, P_Path, P_Path } },                           // 8階：水車
    { { P_HouseVillager, SB(S_Villager), SB(S_Child), SB(S_Lamp), P_Field, SB(S_Pot), P_Tree, P_Lantern, P_Path, P_River } },      // 9階
    { { P_HouseVillager, SB(S_Villager), SB(S_Child), SB(S_Lamp), P_Cloth, P_Tree, P_Tree, P_Lantern, P_Path, P_Bridge } },        // 10階
    { { P_Tree, P_Tree, P_Tree, P_Lantern, P_Lantern, P_Path, P_Path, P_Path, P_River, P_Bridge } },                                // 11階
    { { SB(S_Kairi), -1 } },                                                                                                           // 12階：QTE のあとに出る「カイリ」
    { { P_Tree, P_Tree, P_Tree, P_Lantern, P_Lantern, P_Lantern, P_Path, P_Path, P_Path, P_River } },                              // 13階
    { { P_Tree, P_Tree, P_Tree, P_Lantern, P_Lantern, P_Path, P_Path, P_Path, P_Path, P_River } },                                 // 14階
    { { -1 } },                                                                                                                      // 15階：うつろ（宝箱なし）
};
#undef SB

// その階の k 番目（0〜9）のパーツの番号（無ければ -1）
static int LootId(int floor, int k)
{
    if (floor < 1 || floor > MAX_FLOOR || k < 0 || k >= LOOT_N) return -1;
    for (int i = 0; i <= k; i++) if (kFloorLoot[floor].ids[i] < 0) return -1;   // -1 から先は無し
    return kFloorLoot[floor].ids[k];
}
static bool LootValid(int floor, int k) { return LootId(floor, k) >= 0; }

int Game::LootTotal(int floor) const
{
    int n = 0;
    for (int k = 0; k < LOOT_N; k++) if (LootValid(floor, k)) n++;
    return n;
}

int Game::LootLeft(int floor) const
{
    if (floor < 1 || floor > MAX_FLOOR) return 0;
    int n = 0;
    for (int k = 0; k < LOOT_N; k++) if (LootValid(floor, k) && !((floorGot[floor] >> k) & 1)) n++;
    return n;
}

// 糸巻きを1つほどく：その糸巻きのジオラマパーツを1つ手に入れる
void Game::TakeLoot(int floor, int k)
{
    if (!LootValid(floor, k) || ((floorGot[floor] >> k) & 1)) return;
    floorGot[floor] |= (unsigned short)(1u << k);
    int id = LootId(floor, k);
    std::wstring name;
    if (id >= SUB_ID) { subInv[id - SUB_ID]++; name = std::wstring(kSubs[id - SUB_ID].name) + L"（はめ込み）"; }
    else { inventory[id]++; name = kParts[id].name; }
    crystalsThisRun++;
    int left = LootLeft(floor);
    if (left == 0) floorLoot |= 1u << floor;
    ToastMsg(L"糸がほどけた！ ジオラマパーツ：" + name, RGBA(0.6f, 0.95f, 1));
    ToastMsg(left == 0 ? std::wstring(L"この階のジオラマパーツを全部集めた！") : L"この階の糸巻き　あと " + std::to_wstring(left) + L" 個", left == 0 ? RGBA(1, 0.85f, 0.4f) : RGBA(0.8f, 0.95f, 1));
}

// ------------------------------------------------------------------
// 地形
// ------------------------------------------------------------------
bool Game::Solid(float x, float z) const
{
    if (scene == SceneId::Village) return VillageSolid(x, z);
    if (scene == SceneId::Interior)
    {
        bool doorway = std::fabs(x) < 1.0f && z > -3.6f;     // 手前の出口は歩いて抜けられる
        if (x < -3.7f || x > 3.7f || (z < -2.9f && !doorway) || z > 2.6f) return true;
        for (auto& s : roomSolids) if (x > s.x && x < s.z && z > s.y && z < s.w) return true;
        return false;
    }
    return SolidCell((int)std::floor(x / CELL), (int)std::floor(z / CELL)) || (!pillars.empty() && PillarSolid(x, z));
}

void Game::MoveCircle(XMFLOAT3& p, float r, float dx, float dz) const
{
    auto blocked = [&](float x, float z) {
        return Solid(x - r, z - r) || Solid(x + r, z - r) || Solid(x - r, z + r) || Solid(x + r, z + r) || PropSolid(x, z, r);
    };
    // 万一すでに壁にめり込んでいたら、閉じ込められないように動きを許す（抜け出すまで）
    if (blocked(p.x, p.z)) { p.x += dx; p.z += dz; return; }
    if (!blocked(p.x + dx, p.z)) p.x += dx;
    if (!blocked(p.x, p.z + dz)) p.z += dz;
}

// 通り抜けられない置き物（宝箱・糸巻き、鳥居の柱や石灯籠など）。円 r の中心が (x,z) のとき重なるか
bool Game::PropSolid(float x, float z, float r) const
{
    auto hit = [&](XMFLOAT3 c, float pr) { float dx = x - c.x, dz = z - c.z; return dx * dx + dz * dz < (pr + r) * (pr + r); };
    if (scene == SceneId::Dungeon)
    {
        for (auto& ch : chests) if (hit(ch.pos, ch.kind == 1 ? 0.75f : 0.6f)) return true;
    }
    else if (scene == SceneId::Village)
    {
        const XMFLOAT3 E = kEntrancePos;
        for (int s = -1; s <= 1; s += 2)
        {
            if (hit(F3(E.x + s * 2.1f, 0, E.z - 3.6f), 0.3f)) return true;   // 鳥居の柱
            if (hit(F3(E.x + s * 3.0f, 0, E.z - 5.2f), 0.45f)) return true;  // 石灯籠
        }
        if (hit(F3(E.x - 3.2f, 0, E.z - 3.4f), 0.25f)) return true;          // 立て札
    }
    return false;
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
    if (scene != SceneId::Dungeon) return to;
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
    int pick = n;
    if (n2 >= 0)
    {
        int ax = n2 / DN, az = n2 % DN;
        if (ax == sx || az == sz) pick = n2;
    }
    return { (pick / DN + 0.5f) * CELL, 0, (pick % DN + 0.5f) * CELL };
}

// ------------------------------------------------------------------
// フロア生成（BSP法。5・7・10階は固定の広間）
// ------------------------------------------------------------------
void Game::GenerateFloor(int floor, uint32_t seed)
{
    rng.seed(seed);
    memset(cell, 0, sizeof(cell));
    memset(seen, 0, sizeof(seen));
    rooms.clear(); decos.clear(); enemies.clear(); projs.clear(); pickups.clear(); markers.clear();
    chests.clear(); springs.clear(); fx.clear(); nums.clear(); trail.clear();
    tokens = 0; lockIdx = -1;

    auto carve = [&](int x0, int z0, int x1, int z1) {
        for (int x = std::max(1, std::min(x0, x1)); x <= std::min(DN - 2, std::max(x0, x1)); x++)
            for (int z = std::max(1, std::min(z0, z1)); z <= std::min(DN - 2, std::max(z0, z1)); z++)
                cell[x][z] = 1;
    };
    bool fixed = floor == 5 || floor == 12 || floor == 15;     // ボスの階と QTE の階は決まった広間
    if (fixed)
    {
        Room r{ 6, 4, 18, 19 };
        rooms.push_back(r);
        carve(r.x, r.z, r.x + r.w - 1, r.z + r.h - 1);
        if (floor == 5)
        {
            int pil[4][2] = { { 10, 9 }, { 19, 9 }, { 10, 17 }, { 19, 17 } };
            for (auto& p : pil) cell[p[0]][p[1]] = 0;
        }
        exitX = 15; exitZ = r.z + r.h;
    }
    else
    {
        struct Rc { int x, z, w, h; };
        std::vector<Rc> leaves;
        std::function<void(Rc, int)> split = [&](Rc r, int depth) {
            bool canV = r.w >= 14, canH = r.h >= 14;
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
        split({ 1, 1, DN - 2, DN - 2 }, 4);      // 広いマップを細かく分けて部屋を増やす
        if (leaves.size() > 12) leaves.resize(12);
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

    // 操作キャラを配置（仲間は画面に出ない）
    const Room& sr = rooms[0];
    XMFLOAT3 sp = fixed ? XMFLOAT3{ 15 * CELL, 0, 6 * CELL } : XMFLOAT3{ (sr.x + sr.w * 0.5f) * CELL, 0, (sr.z + sr.h * 0.5f) * CELL };
    for (int i = 0; i < 3; i++) { party[i].pos = sp; party[i].yaw = 0; party[i].kb = {}; party[i].st = party[i].hp > 0 ? ASt::Normal : ASt::Down; }
    camYaw = 0;
    camPosNow = Add(sp, F3(0, 10.f, -6.f));
    camTargetNow = Add(sp, F3(0, 1.3f, 0));

    bool deep = floor >= 9;                     // 9階から少しだけ体力が増える（強さは主に敵の種類で変える）
    auto addChest = [&](int kind, XMFLOAT3 p) { chests.push_back({ p, RandF(-PI, PI), false, 0, kind }); };

    pillars.clear(); summonT = 0;
    wearMul = floor == 14 ? 3.f : 1.f;           // 14階：リミテッド層（武器の消耗が激しい）
    if (fixed)
    {
        // ボスの階は、ボスだけ（ほかの敵はボスが呼び出す）。宝箱も出ない
        // ボスを倒したあとは、ボスが現れない何もない層（出口は最初から開いている）
        if (floor == 5 && BossCleared(5)) hasKey = true;
        else if (floor == 15 && BossCleared(15)) hasKey = true;
        else if (floor == 5)
        {
            SpawnEnemy(EType::Spider, { 15 * CELL, 0, 15 * CELL }, deep);
            enemies.back().yaw = PI;                 // 鍵は倒すと出る宝箱に入っている
        }
        else if (floor == 12)
        {
            // QTE だけの階：敵も宝箱もいない。広間のまん中へ行くと QTE（クリアしたあとは起きず、出口は開いている）
            eventPos = F3(15 * CELL, 0, 14 * CELL);
            if (Done(EV_F12)) hasKey = true;
        }
        else
        {
            SpawnEnemy(EType::Utsuro, { 15 * CELL, 0, 17 * CELL }, deep);
            enemies.back().yaw = PI; enemies.back().keyHolder = true;
            // うつろを守る4本の糸の柱（壊せる地形）
            const float pc[4][2] = { { 10.5f, 10.5f }, { 19.5f, 10.5f }, { 10.5f, 19.5f }, { 19.5f, 19.5f } };
            for (auto& q : pc) pillars.push_back({ F3(q[0] * CELL, 0, q[1] * CELL), 70.f, 70.f, 0.f, true });
        }
    }
    else
    {
        // 階ごとに決まった顔ぶれ（深くなると新しい種類が加わる）
        static const EType kRoster[MAX_FLOOR + 1][3] = {
            { EType::Rat, EType::Rat, EType::Rat },
            { EType::Rat, EType::Rat, EType::Rat },         // 1階：霧ネズミ
            { EType::Rat, EType::Doll, EType::Rat },        // 2階：＋根っこ人形
            { EType::Doll, EType::Spore, EType::Doll },     // 3階：＋胞子
            { EType::Rat, EType::Spore, EType::Moth },      // 4階：＋糸喰い蛾
            { EType::Rat, EType::Rat, EType::Rat },         // 5階（ボス）
            { EType::Doll, EType::Moth, EType::Spore },     // 6階
            { EType::Moth, EType::Shade, EType::Doll },     // 7階：＋影法師
            { EType::Spore, EType::Shade, EType::Moth },    // 8階
            { EType::Shade, EType::Knight, EType::Doll },   // 9階：＋根の鎧武者
            { EType::Moth, EType::Knight, EType::Spore },   // 10階
            { EType::Shade, EType::Knight, EType::Moth },   // 11階
            { EType::Knight, EType::Shade, EType::Knight }, // 12階
            { EType::Moth, EType::Shade, EType::Knight },   // 13階
            { EType::Shade, EType::Knight, EType::Moth },   // 14階
            { EType::Rat, EType::Rat, EType::Rat },         // 15階（ボス）
        };
        for (int r = 1; r < (int)rooms.size(); r++)
        {
            // 敵の数をおさえる：1部屋に1〜2体、深い階ほど1体だけの部屋が多い。1つの階で多くても12体（深い階は10体）
            int cap = floor >= 9 ? 10 : 12;
            int n = RandI(0, 9) < (floor >= 9 ? 6 : 4) ? 1 : 2;
            for (int i = 0; i < n && (int)enemies.size() < cap; i++) SpawnEnemy(kRoster[floor][RandI(0, 2)], RandomFloorPos(rooms[r], 1.5f), deep);
        }
        if (!enemies.empty())
        {
            int k = RandI(0, (int)enemies.size() - 1);
            enemies[k].keyHolder = true;
            enemies[k].hp = enemies[k].maxHp = enemies[k].maxHp * 1.4f;
        }
        if (floor == 12)
        {
            // QTE の階：マップのまん中にいちばん近い部屋でイベント。宝箱は出ない
            XMFLOAT3 mid{ DN * CELL * 0.5f, 0, DN * CELL * 0.5f };
            float bd = 1e9f;
            for (int r = 1; r < (int)rooms.size(); r++)
            {
                XMFLOAT3 c{ (rooms[r].x + rooms[r].w * 0.5f) * CELL, 0, (rooms[r].z + rooms[r].h * 0.5f) * CELL };
                if (DistXZ(c, mid) < bd) { bd = DistXZ(c, mid); eventPos = c; }
            }
            for (auto& e : enemies) if (DistXZ(e.pos, eventPos) < 7.f) { e.pos = RandomFloorPos(rooms[0], 1.5f); e.st = ESt::Idle; }
        }
        else
        {
            int nch = RandI(2, 3);
            for (int i = 0; i < nch; i++) addChest(0, RandomFloorPos(rooms[RandI(1, (int)rooms.size() - 1)], 1.6f));
            if (floor == 2 || RandI(0, 9) < 3) addChest(1, RandomFloorPos(rooms[(int)rooms.size() - 1], 2.0f));
        }
        if (RandI(0, 1) == 0)
            springs.push_back({ RandomFloorPos(rooms[RandI(0, (int)rooms.size() - 1)], 2.2f), false });
    }

    // ジオラマパーツの糸巻き：その階に入ると、まだ開けていないパーツの糸巻きが全部出る（1つにパーツ1つ）
    // ボスの階（5・15）と QTE の階（12：QTE のあとに出る）は出ない
    if (floor != 5 && floor != 12 && floor != 15) SpawnLootSpools(floor);
    else if (floor == 12 && Done(EV_F12)) SpawnLootSpools(12);   // QTE を終えたあとに取り残した分
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
// 出入り
// ------------------------------------------------------------------
// その階のまだ開けていない糸巻きを出す（12階は QTE のあと、イベントの場所に）
void Game::SpawnLootSpools(int floor)
{
    if (floor < 1 || floor > MAX_FLOOR) return;
    XMFLOAT3 sp = party[active].pos; sp.y = 0;
    int placedN = 0;
    for (int k = 0; k < LOOT_N; k++)
    {
        if (!LootValid(floor, k) || ((floorGot[floor] >> k) & 1)) continue;
        bool exists = false;
        for (auto& ch : chests) if (ch.kind == 2 && ch.loot == k && !ch.opened) exists = true;
        if (exists) continue;
        XMFLOAT3 p;
        if (floor == 12) p = Add(eventPos, F3(std::cos(placedN * 2.1f) * 1.5f, 0, std::sin(placedN * 2.1f) * 1.5f));
        else if (floor == 1 && placedN == 0) p = Add(sp, F3(RandF(-2, 2), 0, 3.5f));   // 1階は最初の1つをスタート部屋に
        else
        {
            int nr = (int)rooms.size() - 1;                                             // 部屋を順に回して散らばらせる
            int r = nr > 0 ? 1 + (placedN + floor) % nr : 0;
            p = RandomFloorPos(rooms[r], 1.6f);
        }
        if (Solid(p.x, p.z)) p = Add(sp, F3(RandF(-2, 2), 0, 2.f));
        chests.push_back({ p, RandF(-PI, PI), false, 0, 2, k });
        placedN++;
    }
}

void Game::EnterDungeon(int floor, uint32_t seed)
{
    bool fromVillage = scene != SceneId::Dungeon;
    scene = SceneId::Dungeon;
    sceneT = 0;
    vWalkCam = false;
    if (fromVillage)
    {
        totalDives++;
        crystalsThisRun = 0;
        lantern = 100;
        for (int i = 0; i < 3; i++) { party[i].hp = party[i].maxHp; party[i].st = ASt::Normal; }
        if (!joined[active]) active = 0;
    }
    floorNo = floor;
    // 操作キャラの決まった階（3階＝ソラ、7階＝クロベエ、14階＝カイリ）
    forceChar = RequiredChar(floor);
    if (forceChar >= 0 && joined[forceChar]) active = forceChar; else forceChar = -1;
    hasKey = false;
    exitMenu = false; menuOpen = false;
    downT = 0;
    qteOn = false; eventState = 0; eventT = 0; nagiAlpha = 0; rootAnim = 0;
    GenerateFloor(floor, seed);
    deepest = std::max(deepest, floor);
    arriveT = 0;                       // 上から降り立つ演出
    party[active].pos.y = 5.f;
}

void Game::GoToFloor(int floor)
{
    std::wstring block = FloorBlock(floor);
    if (!block.empty()) { ToastMsg(block, RGBA(1, 0.6f, 0.5f)); return; }
    std::wstring sub = FloorSubtitle(floor);
    uint32_t seed = (uint32_t)(GetTickCount() + floor * 101);
    if (autoshot) seed = 1000 + floor * 37;
    StartTransition(L"根の洞", sub, [this, floor, seed] { EnterDungeon(floor, seed); });
}

void Game::LeaveDungeon(bool dead)
{
    if (dead) threads = threads / 2;
    StartTransition(L"ハタオリ村", dead ? L"力尽きて、ツムギの糸に引き戻された…" : L"村へ戻る", [this, dead] {
        EnterVillage(true);
        if (dead) ToastMsg(L"力尽きた…村へ戻された", RGBA(1, 0.55f, 0.5f));
    });
}

void Game::SpawnFx(int kind, XMFLOAT3 pos, float size, XMFLOAT4 col, float dur)
{
    fx.push_back({ pos, 0, dur, size, kind, col, RandF(0, 6.28f) });
}

void Game::AddNum(XMFLOAT3 pos, int v, D2D1_COLOR_F c, float size)
{
    if (!setDmgNums) return;   // 設定でダメージの数字を消せる
    AddNum2(pos, v, c, size);
}
void Game::AddNum2(XMFLOAT3 pos, int v, D2D1_COLOR_F c, float size)
{
    nums.push_back({ Add(pos, F3(RandF(-0.3f, 0.3f), 0, RandF(-0.3f, 0.3f))), 0, v, c, size });
}

void Game::UseItem(int i) { UseItemOn(i, party[active]); }

// アイテムを使う（武器画面では、見ているキャラの武器に使える）
void Game::UseItemOn(int i, Actor& a)
{
    if (i < 0 || i >= ITEM_N || items[i] <= 0) return;
    switch (i)
    {
    case 0: a.hp = std::min(a.maxHp, a.hp + 40); ToastMsg(L"薬草を使った（HP回復）", RGBA(0.6f, 1, 0.6f)); break;
    case 1: lantern = std::min(100.f, lantern + 50); ToastMsg(L"灯油を注いだ（灯 回復）", RGBA(1, 0.8f, 0.4f)); break;
    case 2:
        if (a.broken) { ToastMsg(L"壊れた武器は研げない（繕い糸で直そう）", RGBA(1, 0.5f, 0.45f)); return; }
        a.dur = std::min(a.maxDur, a.dur + a.maxDur * 0.3f); ToastMsg(L"研ぎ粉で刃を整えた（耐久回復）"); break;
    case 3: items[i]--; ToastMsg(L"帰り糸をたぐって村へ戻る"); LeaveDungeon(false); return;
    case 4:
        // 繕い糸：壊れた最初の武器を元どおりに直す
        if (!a.broken) { ToastMsg(L"武器は壊れていない（繕い糸は壊れた最初の武器を直す）", RGBA(1, 0.8f, 0.7f)); return; }
        a.broken = false; a.dur = a.maxDur;
        SyncWeapon(a);
        SpawnFx(2, Add(a.pos, F3(0, 1.0f, 0)), 3.f, C4(0.7f, 1.f, 0.8f), 0.6f);
        ToastMsg(std::wstring(kWeapons[weapons[a.weapon].def].name) + L" を繕い糸で直した！", RGBA(0.6f, 1, 0.75f));
        break;
    }
    items[i]--;
}

// ------------------------------------------------------------------
// 戦闘
// ------------------------------------------------------------------
void Game::WeaponWear(Actor& a, int n)
{
    if (a.broken) return;
    for (int i = 0; i < n; i++) if (RandF(0, 1) >= a.wearSave) a.dur -= 1.f * (scene == SceneId::Dungeon ? wearMul : 1.f);   // 14階は3倍減る
    if (a.dur <= 0) BreakWeapon(a);
}

static int ActorElement(const Game* g, const Actor& a, const std::vector<OwnedWeapon>& ws)
{
    if (a.weapon < 0 || a.weapon >= (int)ws.size()) return 0;
    const OwnedWeapon& w = ws[a.weapon];
    if (kWeapons[w.def].element) return kWeapons[w.def].element;
    for (int s = 0; s < 2; s++) if (w.att[s] == AT_FIRE) return 1;
    return 0;
}

void Game::ActorAttackHits(Actor& a, float reach, float arcDeg, float dmg, bool heavy, int wear)
{
    bool any = false;
    XMFLOAT3 f = Fwd(a.yaw);
    float cosHalf = std::cos(arcDeg * 0.5f * PI / 180.f);
    float mult = AttackMul(a);
    for (auto& e : enemies)
    {
        if (e.st == ESt::Dead) continue;
        XMFLOAT3 to = Sub(e.pos, a.pos);
        float d = LenXZ(to) - e.radius;
        if (d > reach) continue;
        XMFLOAT3 n = NormXZ(to);
        if (DotXZ(n, f) < cosHalf && d > 0.4f) continue;
        HitEnemy(e, std::round(dmg * mult * RandF(0.92f, 1.08f)), a.pos, heavy ? 7.f : 3.5f, heavy, &a);
        any = true;
    }
    if (!pillars.empty()) DamagePillars(a.pos, a.yaw, reach, arcDeg, std::round(dmg * mult));   // 15階：糸の柱にも当たる
    if (any)
    {
        WeaponWear(a, wear);
        hitstop = std::max(hitstop, heavy ? 0.09f : 0.05f);
        shake = std::max(shake, heavy ? 0.25f : 0.1f);
    }
}

// 着地点のまわりを巻き込む攻撃（クロベエ）。pull=true で使い手の方へ引き寄せる
void Game::AreaHit(Actor& a, XMFLOAT3 center, float radius, float dmg, bool pull, int wear)
{
    bool any = false;
    float mult = AttackMul(a);
    for (auto& e : enemies)
    {
        if (e.st == ESt::Dead) continue;
        if (DistXZ(e.pos, center) > radius + e.radius) continue;
        XMFLOAT3 from = pull ? Add(e.pos, NormXZ(Sub(e.pos, a.pos))) : center;
        HitEnemy(e, std::round(dmg * mult * RandF(0.92f, 1.08f)), from, pull ? 5.5f : 4.f, false, &a);
        any = true;
    }
    if (any) { WeaponWear(a, wear); hitstop = std::max(hitstop, 0.05f); shake = std::max(shake, 0.12f); }
}

void Game::HitEnemy(Enemy& e, float dmg, XMFLOAT3 from, float kb, bool heavy, Actor* by)
{
    if (e.st == ESt::Dead) return;
    if (e.t == EType::RootArm && !qteOn)
    {
        SpawnFx(0, Add(e.pos, F3(0, 2.f, 0)), 1.5f, C4(0.7f, 0.7f, 0.7f), 0.3f);
        return;   // 根の主の腕は QTE でしか切れない
    }
    bool weak = false;
    if (by)
    {
        int el = ActorElement(this, *by, weapons);
        if (el && el == kEDefs[(int)e.t].weak) { dmg = std::round(dmg * 1.5f); weak = true; }
        // 根の鎧武者：正面からの攻撃は半分受け流す（後ろや横から狙う）
        if (e.t == EType::Knight && e.st != ESt::Attack && DotXZ(Fwd(e.yaw), NormXZ(Sub(by->pos, e.pos))) > 0.4f)
        {
            dmg = std::round(dmg * 0.5f);
            SpawnFx(0, Add(e.pos, Add(Mul(Fwd(e.yaw), 0.7f), F3(0, 1.0f, 0))), 1.6f, C4(0.8f, 0.9f, 1.f), 0.3f);
        }
    }
    if (e.t == EType::Utsuro && floorNo == 15)
    {
        // うつろ：4本の糸の柱が1本でも残っている間は、まったく効かない（柱を全部壊すと守りが消える）
        bool shield = false;
        for (auto& p : pillars) if (p.alive) shield = true;
        if (shield)
        {
            e.flash = 0.3f;
            SpawnFx(0, Add(e.pos, F3(RandF(-0.8f, 0.8f), 2.2f, RandF(-0.8f, 0.8f))), 2.6f, C4(0.55f, 0.6f, 1.f), 0.35f);
            AddNum2(Add(e.pos, F3(0, 3.6f, 0)), 0, RGBA(0.6f, 0.7f, 1.f), 26);
            if (immuneMsgT <= 0) { ToastMsg(L"糸の柱に守られて効かない！ 4本の柱を先に壊そう", RGBA(0.7f, 0.8f, 1.f)); immuneMsgT = 4.f; }
            return;
        }
        if (e.pattern == 9) dmg = std::round(dmg * 1.6f);   // 気絶中は大きく効く
    }
    e.hp -= dmg;
    e.flash = 1;
    float h = (e.t == EType::Spider || e.t == EType::Guardian || e.t == EType::Utsuro) ? 3.4f : 1.3f;
    AddNum(Add(e.pos, F3(0, h, 0)), (int)dmg, weak ? RGBA(1, 0.5f, 0.15f) : heavy ? RGBA(1, 0.7f, 0.25f) : RGBA(1, 1, 1), weak || heavy ? 36.f : 28.f);
    SpawnFx(0, Add(e.pos, F3(0, h * 0.6f, 0)), heavy ? 2.8f : 1.9f, weak ? C4(1, 0.55f, 0.3f) : C4(1, 0.85f, 0.6f), 0.5f);
    XMFLOAT3 dir = NormXZ(Sub(e.pos, from));
    bool big = IsBossType(e.t);
    if (!big)
    {
        e.kb = Mul(dir, kb);
        if (e.token) { e.token = false; tokens--; }
        e.st = ESt::Hit; e.stT = 0;
    }
    else if (heavy && e.t != EType::RootArm) e.kb = Mul(dir, kb * 0.25f);
    if (e.hp <= 0) KillEnemy(e, by);
    else if (e.st == ESt::Idle) e.st = ESt::Notice;
}

void Game::KillEnemy(Enemy& e, Actor* by)
{
    e.hp = 0;
    e.st = ESt::Dead;
    e.deadT = 0;
    if (e.token) { e.token = false; tokens--; }
    bool big = IsBossType(e.t);
    SpawnFx(1, Add(e.pos, F3(0, 0.8f, 0)), big ? 7.f : 2.6f, C4(0.8f, 0.85f, 1.f), 0.6f);
    if (e.keyHolder)
        pickups.push_back({ PickKind::Key, Add(e.pos, F3(0, 1.0f, 0)), 0, false, 4.f });
    else if (RandI(0, 3) == 0)
        pickups.push_back({ PickKind::Herb, Add(e.pos, F3(0, 0.8f, 0)), 0, false, 3.f });
    if (e.t != EType::RootArm)
        pickups.push_back({ PickKind::Money, Add(e.pos, F3(RandF(-0.4f, 0.4f), 0.8f, RandF(-0.4f, 0.4f))), 0, false, 3.5f, big ? 120 : RandI(3, 8) });
    if (e.t == EType::Spider)
    {
        ToastMsg(L"ほつれ蜘蛛を倒した！", RGBA(1, 0.85f, 0.4f));
        if (!knot) { knot = true; ToastMsg(L"「結び目」ができた。入口で深い階を選べるようになる", RGBA(0.7f, 0.95f, 1)); }
        if (floorNo == 5)
        {
            eventsDone |= EV_BOSS5;
            SaveGame(false);
            // 中ボスを倒すと、武器と鍵が入った宝箱が出る
            Chest ch{ Add(e.pos, F3(0, 0, -1.5f)), PI, false, 0, 1 };
            ch.key = true;
            chests.push_back(ch);
            SpawnFx(2, Add(ch.pos, F3(0, 1.f, 0)), 5.f, C4(1, 0.85f, 0.5f), 0.9f);
            ToastMsg(L"武器と鍵が入った宝箱が現れた！", RGBA(1, 0.85f, 0.4f));
        }
    }
    if (e.t == EType::Guardian) ToastMsg(L"大樹の番人を鎮めた！", RGBA(1, 0.85f, 0.4f));
    if (e.t == EType::Utsuro && floorNo == 15) ToastMsg(L"うつろを鎮めた！", RGBA(1, 0.85f, 0.4f));
    if (by)
    {
        by->exp += big ? 160.f : 24.f;
        while (by->exp >= 100.f * by->lvl)
        {
            by->exp -= 100.f * by->lvl;
            by->lvl++;
            ToastMsg(std::wstring(kWeapons[weapons[by->weapon].def].name) + L" が Lv" + std::to_wstring(by->lvl) + L" に成長した", RGBA(0.7f, 1, 0.7f));
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
        // クロベエの旋回防御：正面からの攻撃は完全に防ぐ（耐久−1）
        dmg = Def(a).badger ? 0.f : dmg * 0.25f;
        kb *= 0.25f;
        WeaponWear(a, 1);
        SpawnFx(0, Add(a.pos, Add(Mul(Fwd(a.yaw), 1.0f), F3(0, 1.05f, 0))), 2.0f, C4(1.f, 0.8f, 0.6f), 0.4f);
        hitstop = std::max(hitstop, 0.04f);
    }
    else if (!(a.st == ASt::Guard && Def(a).badger))
    {
        a.st = ASt::Hit; a.stT = 0;
    }
    a.hp -= dmg;
    a.kb = Mul(dir, kb);
    a.invuln = 0.5f;
    if (dmg > 0.5f)
    {
        a.flash = 1;
        AddNum(Add(a.pos, F3(0, 1.8f, 0)), (int)std::round(dmg), RGBA(1, 0.4f, 0.35f), 26);
        shake = std::max(shake, 0.3f);
    }
    if (a.hp <= 0)
    {
        a.hp = 0;
        a.st = ASt::Down;
        ToastMsg(std::wstring(Def(a).name) + L"が倒れた…", RGBA(1, 0.5f, 0.45f));
    }
}

// ------------------------------------------------------------------
// 操作キャラ
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
    if (!controlled) return;

    XMFLOAT3 right{ std::cos(camYaw), 0, -std::sin(camYaw) }, fwd = Fwd(camYaw);
    XMFLOAT3 mv = Add(Mul(right, c->mx), Mul(fwd, c->mz));
    float ml = LenXZ(mv);
    if (ml > 1) { mv = Mul(mv, 1 / ml); ml = 1; }
    Enemy* lock = lockIdx >= 0 && lockIdx < (int)enemies.size() ? &enemies[lockIdx] : nullptr;
    if (c->atkDown) a.holdT += dt; else a.holdT = 0;
    bool inTown = scene != SceneId::Dungeon;

    auto faceTarget = [&]() {
        if (lock) { a.yaw = YawOf(Sub(lock->pos, a.pos)); return; }
        if (ml > 0.05f && !d.ranged) { a.yaw = YawOf(mv); return; }
        // 近くの敵へ自動で向く（遠距離のカイリは遠くまで狙う）
        float bd = d.ranged ? 12.f : (d.badger ? 6.f : 3.2f); Enemy* best = nullptr;
        XMFLOAT3 look = ml > 0.05f ? NormXZ(mv) : Fwd(a.yaw);
        for (auto& e : enemies)
        {
            if (e.st == ESt::Dead) continue;
            float dd = DistXZ(e.pos, a.pos) - e.radius;
            if (dd < bd && DotXZ(NormXZ(Sub(e.pos, a.pos)), look) > (d.ranged ? 0.4f : -0.2f)) { bd = dd; best = &e; }
        }
        if (best) a.yaw = YawOf(Sub(best->pos, a.pos));
        else if (ml > 0.05f) a.yaw = YawOf(mv);
    };
    auto startAttack = [&](int k) {
        a.st = ASt::Attack; a.combo = k; a.stT = 0; a.hitDone = false; a.queued = false;
        faceTarget();
    };
    auto move = [&](float speedMul) {
        a.dashing = false;
        if (ml > 0.05f)
        {
            bool dash = c->dash && a.st == ASt::Normal;
            float sp = d.speed * speedMul * a.spdMul * (dash ? 1.65f : 1.f);
            MoveCircle(a.pos, 0.4f, mv.x * sp * dt, mv.z * sp * dt);
            if (!lock || a.st == ASt::Normal) a.yaw = ApproachAngle(a.yaw, lock && a.st != ASt::Normal ? YawOf(Sub(lock->pos, a.pos)) : YawOf(mv), dt * 14);
            a.walkT += dt * ml * speedMul * (dash ? 1.35f : 1.f);
            a.speedNow = sp * ml;
            a.dashing = dash;
            if (dash)
            {
                a.dustT -= dt;
                if (a.dustT <= 0) { a.dustT = 0.12f; SpawnFx(3, Add(a.pos, F3(0, 0.15f, 0)), 0.9f, scene == SceneId::Dungeon ? C4(0.6f, 0.65f, 0.75f, 0.6f) : C4(0.75f, 0.68f, 0.55f, 0.6f), 0.45f); }
            }
        }
        else a.speedNow = 0;
    };
    auto startDodge = [&]() {
        a.st = ASt::Dodge; a.stT = 0;
        a.dodgeDir = ml > 0.05f ? NormXZ(mv) : Mul(Fwd(a.yaw), -1);
        float dur = DodgeDur(a.def);
        a.invuln = DodgeInvuln(a.def);         // 無敵はもとより1秒長く続く（動きと硬直はもとのまま）
        if (a.def == 1) SpawnFx(3, Add(a.pos, F3(0, 0.2f, 0)), 2.0f, C4(0.45f, 0.33f, 0.22f, 0.85f), 0.5f);     // 掘り始めの土煙
        if (a.def == 2) SpawnFx(3, Add(a.pos, F3(0, 0.2f, 0)), 1.6f, C4(0.6f, 0.85f, 1.f, 0.7f), 0.4f);         // 水しぶき
        a.glideT = 0;
    };
    // 回避のあとの1秒：無敵のあいだ、回避の向きへすべり続ける（硬直はないので、そのまま動ける）
    if (a.glideT > 0)
    {
        if (a.st == ASt::Hit || a.st == ASt::Down || a.st == ASt::Dodge) a.glideT = 0;
        else
        {
            a.glideT = std::max(0.f, a.glideT - dt);
            float sp = DodgeGlideV(a.def) * a.glideT;    // 1秒で0まで落ちる
            MoveCircle(a.pos, 0.4f, a.dodgeDir.x * sp * dt, a.dodgeDir.z * sp * dt);
            glideDist += sp * dt;
        }
    }

    switch (a.st)
    {
    case ASt::Normal:
        move(1.f);
        if (inTown) break;                 // 町では武器をしまっている
        if (c->dodge) startDodge();
        else if (c->guard)
        {
            // 右クリック：ソラ＝溜め→回転攻撃／クロベエ＝旋回防御／カイリ＝溜め→貫く針
            a.st = d.badger ? ASt::Guard : ASt::Charging;
            a.stT = 0;
        }
        else if (c->atkPressed) startAttack(0);
        break;
    case ASt::Attack:
    {
        float dur = d.dur[a.combo];
        float act = dur * d.hitFrac;
        if (a.def == 0 && a.stT < act)
        {
            XMFLOAT3 f = Fwd(a.yaw);
            MoveCircle(a.pos, 0.4f, f.x * 2.4f * dt, f.z * 2.4f * dt);
        }
        if (!a.hitDone && a.stT >= act)
        {
            a.hitDone = true;
            if (a.def == 0)
            {
                ActorAttackHits(a, d.reach, d.arc[a.combo], d.dmg[a.combo], a.combo == 2, a.combo == 2 ? 2 : 1);
                WeaponPose wp; XMMATRIX W = SoraWeaponWorld(a, &wp);
                XMFLOAT3 mid; XMStoreFloat3(&mid, XMVector3Transform(XMVectorSet(0, 0, 1.4f, 1), W));
                SpawnFx(2, mid, 2.2f, C4(1, 0.95f, 0.8f), 0.3f);
            }
            else if (a.def == 1)
            {
                // ひも通しを紐ごと扇形に薙ぎ払う：扇の中の敵すべてに当たる
                ActorAttackHits(a, kFanReach[a.combo], kFanArc[a.combo], d.dmg[a.combo], a.combo == 2, 1);
                float half = kFanArc[a.combo] * 0.5f * PI / 180.f;
                for (int k = 0; k <= 4; k++)
                {
                    float ang = a.yaw + (k / 4.f * 2.f - 1.f) * half;
                    XMFLOAT3 q = Add(a.pos, Mul(Fwd(ang), kFanReach[a.combo] * 0.85f));
                    SpawnFx(3, Add(q, F3(0, 0.2f, 0)), 1.8f, C4(0.5f, 0.45f, 0.4f, 0.75f), 0.45f);
                }
                SpawnFx(1, Add(a.pos, Add(Mul(Fwd(a.yaw), kFanReach[a.combo] * 0.6f), F3(0, 0.4f, 0))), a.combo == 2 ? 4.6f : 3.8f, C4(1.f, 0.55f, 0.45f), 0.35f);
            }
            else
            {
                // ミシン針を投げる（3段目は3本）
                XMFLOAT3 hand = Add(a.pos, Add(Mul(Fwd(a.yaw), 0.4f), F3(0, 1.05f, 0)));
                int n = a.combo == 2 ? 3 : 1;
                for (int i = 0; i < n; i++)
                {
                    float yy = a.yaw + (n == 3 ? (i - 1) * 0.2f : 0.f);
                    Projectile p{ hand, Mul(Fwd(yy), 19.f), 0.7f, d.dmg[a.combo] * AttackMul(a), 0.35f, 2 };
                    p.friendly = true; p.owner = active;
                    projs.push_back(p);
                }
            }
        }
        if (c->atkPressed && a.stT > dur * 0.2f) a.queued = true;
        if (c->dodge && a.stT > act) { startDodge(); break; }
        if (a.stT >= dur)
        {
            if (a.queued && a.combo < 2) startAttack(a.combo + 1);
            else a.st = ASt::Normal;
        }
        break;
    }
    case ASt::Charging:
        move(0.35f);
        if (lock) a.yaw = YawOf(Sub(lock->pos, a.pos));
        if (a.stT >= 0.8f && a.stT - dt < 0.8f)
            SpawnFx(2, Add(a.pos, F3(0, 1.0f, 0)), 3.f, C4(1, 0.75f, 0.35f), 0.4f);
        if (!c->guard)
        {
            a.chargeLv = Saturate(a.stT / 0.8f);
            a.st = ASt::ChargeAtk; a.stT = 0; a.hitDone = false; a.combo = 0;
            if (a.def == 2) faceTarget();
        }
        break;
    case ASt::ChargeAtk:
    {
        float mul = 0.6f + 0.4f * a.chargeLv;
        if (a.def == 2)
        {
            // カイリ：溜めた大きな針で敵を貫く
            if (!a.hitDone && a.stT >= 0.12f)
            {
                a.hitDone = true;
                XMFLOAT3 hand = Add(a.pos, Add(Mul(Fwd(a.yaw), 0.4f), F3(0, 1.1f, 0)));
                Projectile p{ hand, Mul(Fwd(a.yaw), 24.f), 0.75f, d.chargeDmg * mul * AttackMul(a), 0.55f, 3 };
                p.friendly = true; p.pierce = true; p.owner = active;
                projs.push_back(p);
                SpawnFx(1, hand, 2.5f, C4(0.6f, 0.9f, 1.f), 0.35f);
                shake = std::max(shake, 0.15f);
            }
            if (a.stT >= 0.45f) a.st = ASt::Normal;
            break;
        }
        // ソラ：刃を開いたまま2回転して周囲を斬り、最後に正面で挟む
        if (ml > 0.05f && a.stT < 0.8f)
            MoveCircle(a.pos, 0.4f, mv.x * d.speed * 0.45f * dt, mv.z * d.speed * 0.45f * dt);
        const float ticks[2] = { 0.3f, 0.55f };
        if (a.combo < 2 && a.stT >= ticks[a.combo])
        {
            ActorAttackHits(a, d.chargeReach, 360.f, d.chargeDmg * 0.45f * mul, false, 1);
            a.combo++;
        }
        if (!a.hitDone && a.stT >= 0.8f)
        {
            a.hitDone = true;
            ActorAttackHits(a, d.chargeReach, d.chargeArc, d.chargeDmg * mul, true, 1);
            SpawnFx(1, Add(a.pos, Add(Mul(Fwd(a.yaw), 1.5f), F3(0, 0.9f, 0))), 4.5f, C4(1, 0.85f, 0.5f), 0.45f);
        }
        if (a.stT >= 1.05f) a.st = ASt::Normal;
        break;
    }
    case ASt::Dodge:
    {
        // 回避：動きと硬直はもとのまま。無敵の持続時間だけ1秒長い（startDodge で設定）
        float dur = DodgeDur(a.def), base = DodgeBase(a.def);
        float bt = a.stT / dur * base;            // もとの動きでの経過時間
        float slow = 1.f;                          // 動く速さ・距離はもとのまま
        float sp;
        if (a.def == 1)
        {
            // クロベエ：地面に潜って進み、飛び出す
            sp = (bt > 0.15f && bt < 0.45f) ? 11.f * slow : bt >= 0.45f ? DodgeGlideV(1) : 0.f;   // 飛び出したあとも、すべりへつなぐ
            float btPrev = (a.stT - dt) / dur * base;
            if (bt >= 0.45f && btPrev < 0.45f)
                SpawnFx(3, Add(a.pos, F3(0, 0.3f, 0)), 2.4f, C4(0.45f, 0.33f, 0.22f, 0.9f), 0.55f);
            if (sp > 0) { a.dustT -= dt; if (a.dustT <= 0) { a.dustT = 0.08f; SpawnFx(3, Add(a.pos, F3(RandF(-0.3f, 0.3f), 0.1f, RandF(-0.3f, 0.3f))), 1.0f, C4(0.42f, 0.32f, 0.22f, 0.7f), 0.4f); } }
        }
        else
        {
            // ソラ：前転／カイリ：腹ばいで滑る。終わりの速さを、あとのすべりの速さにつなぐ
            float peak = a.def == 2 ? 13.f : 12.f, v0 = DodgeGlideV(a.def);
            sp = (peak - (peak - v0) * std::min(1.f, a.stT / dur)) * slow;
        }
        MoveCircle(a.pos, 0.4f, a.dodgeDir.x * sp * dt, a.dodgeDir.z * sp * dt);
        dodgeDist += sp * dt;
        if (a.stT >= dur) { a.st = ASt::Normal; a.glideT = 1.0f; }   // 回避の距離を1秒ぶんのばす（無敵の1秒のあいだすべる）
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

// 町での仲間（主人公の横をついて歩く）
void Game::UpdateFollower(Actor& a, float dt, int slot)
{
    const CharDef& d = Def(a);
    Actor& L = party[active];
    a.stT += dt;
    a.st = ASt::Normal;
    XMFLOAT3 camR{ std::cos(camYaw), 0, -std::sin(camYaw) };
    float side = slot == 0 ? 1.7f : -1.7f;
    XMFLOAT3 goal = Add(L.pos, Add(Mul(camR, side), Mul(Fwd(camYaw), -0.6f - slot * 0.4f)));
    if (Solid(goal.x, goal.z)) goal = Add(L.pos, Mul(Fwd(camYaw), -1.4f - slot));
    if (Solid(goal.x, goal.z)) goal = L.pos;
    float dist = DistXZ(a.pos, goal);
    if (DistXZ(a.pos, L.pos) > 14.f) { a.pos = goal; return; }
    if (dist > 0.6f)
    {
        XMFLOAT3 dir = NormXZ(Sub(goal, a.pos));
        float sp = d.speed * std::min(1.7f, dist / 1.5f + 0.4f);
        MoveCircle(a.pos, 0.4f, dir.x * sp * dt, dir.z * sp * dt);
        a.yaw = ApproachAngle(a.yaw, YawOf(dir), dt * 10);
        a.walkT += dt * sp / d.speed;
        a.speedNow = sp;
    }
    else
    {
        a.speedNow = 0;
        a.yaw = ApproachAngle(a.yaw, L.yaw, dt * 4);
    }
}

// ------------------------------------------------------------------
// 敵AI：待機→発見→接近→攻撃→後退→被弾→死亡
// ------------------------------------------------------------------
void Game::UpdateEnemies(float dt)
{
    auto release = [&](Enemy& e) { if (e.token) { e.token = false; tokens--; } };
    Actor& tgtA = party[active];
    for (auto& e : enemies)
    {
        const EDef& ed = kEDefs[(int)e.t];
        e.flash = std::max(0.f, e.flash - dt * 5);
        e.bob += dt;
        if (e.st == ESt::Dead) { e.deadT += dt; continue; }
        if (e.t == EType::RootArm) continue;    // 根の主の腕はイベントで動く
        if (e.t == EType::Utsuro) { if (floorNo == 15) UpdateUtsuro(e, dt); continue; }   // 12階ではイベントだけ
        if (LenXZ(e.kb) > 0.02f)
        {
            MoveCircle(e.pos, e.radius, e.kb.x * dt, e.kb.z * dt);
            e.kb = Mul(e.kb, std::max(0.f, 1 - dt * 7));
        }
        e.stT += dt;
        e.atkCd -= dt;

        Actor* tgt = &tgtA;
        if (tgt->st == ASt::Down || arriveT < 0.8f) { if (e.st != ESt::Idle) e.st = ESt::Chase; if (tgt->st == ASt::Down) { e.st = ESt::Idle; continue; } }
        XMFLOAT3 to = Sub(tgt->pos, e.pos);
        float d = LenXZ(to);
        XMFLOAT3 dir = NormXZ(to);
        bool boss = IsBossType(e.t);
        auto moveDir = [&](XMFLOAT3 mdir, float sp) {
            MoveCircle(e.pos, e.radius, mdir.x * sp * dt, mdir.z * sp * dt);
            if (LenXZ(mdir) > 0.01f) e.yaw = ApproachAngle(e.yaw, YawOf(mdir), dt * 8);
        };
        auto startWindup = [&]() {
            e.st = ESt::Windup; e.stT = 0;
            if (!boss) { e.token = true; tokens++; }
        };
        bool canTake = boss || tokens < 2;
        // ほつれ蜘蛛：ときどき子分（霧ネズミ）を呼び出す。ボスの階の敵はボスが呼んだものだけ
        if (e.t == EType::Spider && e.st != ESt::Idle && e.st != ESt::Notice)
        {
            summonT += dt;
            int alive = 0;
            for (auto& o : enemies) if (o.st != ESt::Dead && !IsBossType(o.t)) alive++;
            if (summonT > 9.f && alive < 3)
            {
                summonT = 0;
                for (int k = -1; k <= 1; k += 2) pendingSpawns.push_back({ EType::Rat, Add(e.pos, Mul(Fwd(e.yaw + k * 1.2f), 3.f)) });
                ToastMsg(L"ほつれ蜘蛛が子分を呼んだ！", RGBA(1, 0.7f, 0.6f));
            }
        }

        switch (e.st)
        {
        case ESt::Idle:
        {
            e.wanderT -= dt;
            if (e.wanderT <= 0) { e.wander = Add(e.pos, F3(RandF(-3, 3), 0, RandF(-3, 3))); e.wanderT = RandF(2, 4); }
            XMFLOAT3 wd = Sub(e.wander, e.pos);
            if (LenXZ(wd) > 0.3f && !boss) moveDir(NormXZ(wd), ed.speed * 0.3f);
            int rr = RoomAt(e.pos);
            if (d < ed.notice && ((rr >= 0 && rr == RoomAt(tgt->pos)) || d < 6.f)) { e.st = ESt::Notice; e.stT = 0; }
            break;
        }
        case ESt::Notice:
            e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 10);
            if (e.stT > 0.55f) e.st = ESt::Chase;
            break;
        case ESt::Chase:
            if (e.t == EType::Spore || e.t == EType::Moth)
            {
                if (d < 4.f) moveDir(Mul(dir, -1), ed.speed);
                else if (d > 7.5f) moveDir(dir, ed.speed);
                else
                {
                    // 蛾はジグザグに飛び回る
                    float zig = e.t == EType::Moth ? std::sin(e.bob * 2.3f) * 1.4f : 0.4f;
                    moveDir(F3(-dir.z * zig, 0, dir.x * zig), ed.speed * (e.t == EType::Moth ? 0.35f : 0.25f));   // 回りこみはゆっくり
                    e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 8);
                }
                if (d < ed.atkRange && e.atkCd <= 0 && canTake) startWindup();
            }
            else if (e.t == EType::Guardian)
            {
                if (d > 4.5f) moveDir(dir, ed.speed); else e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 3);
                if (e.atkCd <= 0) startWindup();
            }
            else
            {
                if (d > ed.atkRange * 0.85f)
                {
                    XMFLOAT3 wp = d > 6.f ? PathNext(e.pos, tgt->pos) : tgt->pos;
                    moveDir(NormXZ(Sub(wp, e.pos)), ed.speed);
                }
                else if (e.atkCd <= 0 && canTake) startWindup();
                else { moveDir(F3(-dir.z, 0, dir.x), ed.speed * 0.1f); e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 4); }   // 回りこみはゆっくり
            }
            break;
        case ESt::Windup:
            if (e.stT < ed.windup * 0.75f) e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 10);
            if (e.t == EType::Guardian && e.stT < dt * 1.5f)
            {
                e.pattern = d < 6.f ? 0 : 1;
                if (e.pattern == 1)
                {
                    // 根の突き上げ：足元に予告円を出す
                    markers.push_back({ tgt->pos, 0, 0.95f, 1.7f, 16 });
                    for (int k = 0; k < 2; k++)
                        markers.push_back({ Add(tgt->pos, F3(RandF(-3, 3), 0, RandF(-3, 3))), 0, 1.1f + k * 0.2f, 1.6f, 14 });
                }
            }
            if (e.stT >= ed.windup)
            {
                e.st = ESt::Attack; e.stT = 0; e.hitDone = false;
                e.dashDir = Fwd(e.yaw);
                if (e.t == EType::Spore)
                {
                    XMFLOAT3 o = Add(e.pos, F3(0, 1.0f, 0));
                    XMFLOAT3 v = Sub(Add(tgt->pos, F3(0, 0.9f, 0)), o);
                    projs.push_back({ o, Mul(v, 7.5f / Len(v)), 3.f, ed.dmg, 0.35f, 0 });
                }
                if (e.t == EType::Moth)
                {
                    // 糸玉を左右に2発
                    XMFLOAT3 o = Add(e.pos, F3(0, 1.4f, 0));
                    for (int k = -1; k <= 1; k += 2)
                    {
                        float yy = YawOf(dir) + k * 0.22f;
                        projs.push_back({ o, Mul(Fwd(yy), 8.5f), 2.5f, ed.dmg, 0.32f, 0 });
                    }
                }
                if (e.t == EType::Shade) e.pattern = 0;   // 1回目の突進
                if (e.t == EType::Spider)
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
            auto hitTgt = [&](float range, float minDot, float dmg, float kb, XMFLOAT3 center) {
                if (tgt->st == ASt::Down) return;
                if (LenXZ(Sub(tgt->pos, center)) > range) return;
                if (minDot > -1 && DotXZ(NormXZ(Sub(tgt->pos, e.pos)), Fwd(e.yaw)) < minDot) return;
                DamageActor(*tgt, dmg, e.pos, kb);
            };
            if (e.t == EType::Rat || e.t == EType::Shade)
            {
                moveDir(e.dashDir, e.t == EType::Shade ? 13.f : 11.f);
                if (!e.hitDone && DistXZ(tgt->pos, e.pos) < e.radius + 0.55f) { DamageActor(*tgt, ed.dmg, e.pos, 7); e.hitDone = true; }
                // 影法師：1回目の突進のあと、向き直ってもう一度突進する
                if (e.t == EType::Shade && e.pattern == 0 && e.stT >= ed.atkDur)
                {
                    e.pattern = 1; e.stT = 0; e.hitDone = false;
                    e.dashDir = dir; e.yaw = YawOf(dir);
                    SpawnFx(3, Add(e.pos, F3(0, 0.8f, 0)), 1.6f, C4(0.2f, 0.15f, 0.3f, 0.8f), 0.4f);
                }
            }
            else if (e.t == EType::Doll && !e.hitDone && e.stT > 0.1f) { e.hitDone = true; hitTgt(ed.atkRange + 0.5f, 0.2f, ed.dmg, 6, e.pos); }
            else if (e.t == EType::Knight && !e.hitDone && e.stT > 0.15f)
            {
                // 根の鎧武者：大きく横に薙ぐ
                e.hitDone = true; hitTgt(ed.atkRange + 0.8f, -0.3f, ed.dmg, 8, e.pos);
                SpawnFx(1, Add(e.pos, Add(Mul(Fwd(e.yaw), 1.4f), F3(0, 0.8f, 0))), 3.f, C4(0.9f, 0.85f, 0.7f), 0.3f);
            }
            else if (boss && e.pattern == 0 && !e.hitDone && e.stT > 0.15f)
            {
                e.hitDone = true;
                XMFLOAT3 c = Add(e.pos, Mul(Fwd(e.yaw), e.t == EType::Guardian ? 3.0f : 2.2f));
                hitTgt(e.t == EType::Guardian ? 4.2f : 3.4f, -2, ed.dmg, 9, c);
                SpawnFx(1, Add(c, F3(0, 0.3f, 0)), 7.f, C4(0.9f, 0.9f, 1.f), 0.5f);
                SpawnFx(3, Add(c, F3(0, 0.3f, 0)), 5.f, C4(0.5f, 0.45f, 0.4f, 0.8f), 0.6f);
                shake = std::max(shake, 0.35f);
            }
            if (e.stT >= ed.atkDur) { e.st = ESt::Recover; e.stT = 0; }
            break;
        }
        case ESt::Recover:
            if (e.t != EType::Spore && e.t != EType::Moth && !boss) { moveDir(Mul(dir, -1), 1.2f); e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 6); }
            if (e.stT >= ed.recover) { release(e); e.atkCd = ed.cd; e.st = ESt::Chase; }
            break;
        case ESt::Hit:
            if (e.stT > 0.32f) e.st = ESt::Chase;
            break;
        default: break;
        }
        e.pos.y = 0;
    }
    // ボスが呼び出した敵を出す
    for (auto& ps : pendingSpawns)
    {
        XMFLOAT3 p = ps.second;
        if (Solid(p.x, p.z)) p = tgtA.pos;
        SpawnEnemy(ps.first, p, floorNo >= 9);
        enemies.back().st = ESt::Chase;
        SpawnFx(3, Add(p, F3(0, 0.6f, 0)), 2.6f, C4(0.5f, 0.45f, 0.6f, 0.85f), 0.6f);
    }
    pendingSpawns.clear();
    // 予告円（根の突き上げ）
    for (auto& m : markers)
    {
        float before = m.t;
        m.t += dt;
        if (before < m.delay && m.t >= m.delay)
        {
            if (DistXZ(tgtA.pos, m.pos) < m.radius) DamageActor(tgtA, m.dmg, Sub(m.pos, F3(0, 0, 0.01f)), 8);
            SpawnFx(3, Add(m.pos, F3(0, 0.3f, 0)), 3.f, C4(0.45f, 0.35f, 0.25f, 0.85f), 0.6f);
            shake = std::max(shake, 0.2f);
        }
    }
    markers.erase(std::remove_if(markers.begin(), markers.end(), [](const Marker& m) { return m.t > m.delay + 0.5f; }), markers.end());
    // 敵同士の押し合い
    for (size_t i = 0; i < enemies.size(); i++)
        for (size_t j = i + 1; j < enemies.size(); j++)
        {
            Enemy& a = enemies[i]; Enemy& b = enemies[j];
            if (a.st == ESt::Dead || b.st == ESt::Dead || a.t == EType::RootArm || b.t == EType::RootArm) continue;
            XMFLOAT3 dd = Sub(b.pos, a.pos);
            float l = LenXZ(dd), m = a.radius + b.radius;
            if (l < m && l > 0.001f)
            {
                XMFLOAT3 n = Mul(dd, (m - l) * 0.5f / l);
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
    if (p.st == ASt::Down || exitMenu || menuOpen || qteOn || !dialog.empty()) return c;
    c.dash = autoDash;
    Enemy* tgt = nullptr; float best = 13.f; int ti = -1;
    for (int i = 0; i < (int)enemies.size(); i++)
    {
        auto& e = enemies[i];
        if (e.st == ESt::Dead || e.t == EType::RootArm) continue;
        float d = DistXZ(e.pos, p.pos);
        if (d < best && (e.st != ESt::Idle || d < 8)) { best = d; tgt = &e; ti = i; }
    }
    const CharDef& D = Def(p);
    XMFLOAT3 goal{}; bool have = false; float stop = 0.6f;
    Chest* nearChest = nullptr;
    if (tgt) { goal = tgt->pos; have = true; stop = (autoGuard ? 3.2f : D.reach * 0.65f) + tgt->radius; lockIdx = ti; }
    else
    {
        lockIdx = -1;
        float bd = 1e9f;
        for (auto& ch : chests)
            if (!ch.opened) { float d = DistXZ(ch.pos, p.pos) - (ch.kind == 2 ? 2.f : 0.f); if (d < bd) { bd = d; goal = ch.pos; have = true; nearChest = &ch; } }
        for (auto& k : pickups) if (!k.taken) { float d = DistXZ(k.pos, p.pos); if (d < bd) { bd = d; goal = k.pos; have = true; nearChest = nullptr; } }
        if (!have) { goal = Sub(exitPos, Mul(exitFace, 1.5f)); have = true; }
        stop = nearChest ? 1.3f : 0.5f;
        if (nearChest && DistXZ(nearChest->pos, p.pos) < 1.6f) { c.interact = true; return c; }
    }
    if (autoGuard || autoSpecial > 0)
    {
        c.guard = true;
        if (autoGuard) lockIdx = -1;
        if (autoSpecial > 0) autoSpecial -= dt;
        if (tgt)
        {
            XMFLOAT3 dir = NormXZ(Sub(goal, p.pos));
            XMFLOAT3 right{ std::cos(camYaw), 0, -std::sin(camYaw) }, fwd = Fwd(camYaw);
            if (DistXZ(p.pos, goal) > stop) { c.mx = DotXZ(dir, right); c.mz = DotXZ(dir, fwd); }
        }
        return c;
    }
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
        if (d > stop) { c.mx = DotXZ(dir, right); c.mz = DotXZ(dir, fwd); }
        if (tgt && d < stop + 0.8f)
        {
            autoAtkT -= dt;
            if (autoAtkT <= 0 && p.st == ASt::Normal && p.def != 1 && autoCount % 7 == 6) { autoCount++; autoSpecial = 0.7f; }
            else if (autoAtkT <= 0) { autoCount++; c.atkPressed = true; c.atkDown = true; autoAtkT = 0.2f; }
        }
    }
    return c;
}

// ------------------------------------------------------------------
// 更新
// ------------------------------------------------------------------
void Game::UpdateDungeon(float dt, const Controls& cIn)
{
    Controls c = cIn;
    float gdt = dt;
    if (hitstop > 0) { hitstop -= dt; gdt = 0; }

    // 降り立つ演出（暗転が明けてから）
    if (transOn && arriveT < 1.2f) { c = Controls{}; party[active].pos.y = 5.f; UpdateFollowCam(dt); return; }
    if (arriveT < 1.2f)
    {
        float before = arriveT;
        arriveT += dt;
        float k = Saturate(arriveT / 0.5f);
        party[active].pos.y = 5.f * (1 - k * k);
        if (before < 0.5f && arriveT >= 0.5f)
        {
            party[active].pos.y = 0;
            SpawnFx(1, Add(party[active].pos, F3(0, 0.2f, 0)), 4.f, C4(0.7f, 0.9f, 1.f), 0.5f);
            SpawnFx(3, Add(party[active].pos, F3(0, 0.2f, 0)), 3.f, C4(0.55f, 0.6f, 0.7f, 0.8f), 0.6f);
            shake = std::max(shake, 0.25f);
        }
        if (arriveT < 0.7f) c = Controls{};
    }

    if (menuOpen) { UpdateMenu(c); gdt = 0; c = Controls{}; }
    else if (c.menu && !exitMenu && !qteOn && dialog.empty()) { menuOpen = true; menuTab = 0; menuSel = 0; menuChar = active; gdt = 0; c = Controls{}; }

    UpdateFloorEvents(gdt, c);
    if (qteOn || !dialog.empty()) { Controls keepCam; keepCam.camYaw = c.camYaw; c = keepCam; }

    if (exitMenu)
    {
        gdt = 0;
        std::wstring block = floorNo < MAX_FLOOR ? FloorBlock(floorNo + 1) : L"";
        bool lockNext = !block.empty();
        if (cIn.up || cIn.down) exitSel = 1 - exitSel;
        if (cIn.confirm)
        {
            if (exitSel == 0)
            {
                if (lockNext) ToastMsg(block, RGBA(1, 0.6f, 0.5f));
                else if (floorNo >= MAX_FLOOR) { exitMenu = false; LeaveDungeon(false); }
                else { exitMenu = false; GoToFloor(floorNo + 1); }
            }
            else { exitMenu = false; LeaveDungeon(false); }
        }
        if (cIn.back) exitMenu = false;
    }
    else if (!transOn)
    {
        // 灯
        float drain = floorNo >= 6 ? 0.5f : 0.333f;
        lantern = std::max(0.f, lantern - drain * gdt);
        Actor& A0 = party[active];
        if (lantern <= 0 && A0.st != ASt::Down) { A0.hp -= 1.5f * gdt; if (A0.hp <= 0) { A0.hp = 0; A0.st = ASt::Down; } }

        // カメラ：ロックオン中はマウスでの回転を弱め、敵の方へ自動で向く。ホイールで寄る／引く、V で背後に戻す
        camYaw += lockIdx >= 0 ? c.camYaw * 0.25f : c.camYaw;
        if (lockIdx < 0 && c.wheel != 0) camDist = Clampf(camDist - c.wheel * 0.006f, 4.5f, 11.f);
        if (c.camReset) { camYaw = A0.yaw; camDist = 7.f; }
        // ロックオン：Q で画面の正面にいちばん近い敵をロック／解除。R かホイールで次の敵へ切り替え
        auto pickTarget = [&](int skip, float sideSign) {
            int best = -1; float bs = 1e9f;
            for (int i = 0; i < (int)enemies.size(); i++)
            {
                if (enemies[i].st == ESt::Dead || i == skip) continue;
                if (enemies[i].t == EType::RootArm && !qteOn) continue;
                float d = DistXZ(enemies[i].pos, A0.pos);
                if (d > 16.f) continue;
                XMFLOAT3 to = NormXZ(Sub(enemies[i].pos, A0.pos));
                float facing = DotXZ(to, Fwd(camYaw));
                if (facing < -0.3f) continue;
                float score = d * (1.6f - facing);        // 近くて画面の真ん中に近いほど優先
                if (sideSign != 0)
                {
                    // 切り替え：今の敵から見て右（左）にいる敵を優先
                    XMFLOAT3 cur = NormXZ(Sub(enemies[skip].pos, A0.pos));
                    float cross = cur.x * to.z - cur.z * to.x;
                    if (cross * sideSign > 0) score *= 0.5f;
                }
                if (score < bs) { bs = score; best = i; }
            }
            return best;
        };
        if (c.lockon)
        {
            if (lockIdx >= 0) { lockIdx = -1; ToastMsg(L"ロックオン解除", RGBA(0.8f, 0.85f, 0.9f)); }
            else
            {
                lockIdx = pickTarget(-1, 0);
                if (lockIdx < 0) ToastMsg(L"近くにロックオンできる敵がいない", RGBA(0.8f, 0.85f, 0.9f));
            }
        }
        else if (lockIdx >= 0 && (c.lockNext || c.wheel != 0))
        {
            int n = pickTarget(lockIdx, c.wheel < 0 ? -1.f : 1.f);
            if (n >= 0) lockIdx = n;
        }
        if (lockIdx >= 0 && (enemies[lockIdx].st == ESt::Dead || DistXZ(enemies[lockIdx].pos, A0.pos) > 20))
        {
            // 倒したら近くの次の敵へ自動で移る
            int prev = lockIdx; lockIdx = -1;
            int n = pickTarget(prev, 0);
            if (n >= 0 && DistXZ(enemies[n].pos, A0.pos) < 10.f) lockIdx = n;
        }
        // 交代（仲間は画面に出ず、交代すると入れ替わる）
        if (c.swap)
        {
            int n = NextMember(active);
            if (n >= 0) SwapTo(n); else ToastMsg(forceChar >= 0 ? std::wstring(L"この階は") + Def(party[forceChar]).name + L"しか戦えない" : std::wstring(L"交代できる仲間がいない"), RGBA(1, 0.8f, 0.7f));
        }
        if (c.item >= 0) { UseItem(c.item); if (scene != SceneId::Dungeon || transOn) return; }

        UpdateActor(party[active], &c, gdt, true);
        // 控えの仲間は少しずつ回復
        for (int i = 0; i < 3; i++)
            if (i != active && joined[i] && party[i].st != ASt::Down) party[i].hp = std::min(party[i].maxHp, party[i].hp + 1.0f * gdt);
        if (party[active].st == ASt::Down)
        {
            downT += dt;
            int n = NextMember(active);
            if (n >= 0 && downT > 0.8f) { downT = 0; SwapTo(n); ToastMsg(std::wstring(Def(party[n]).name) + L"が駆けつけた！", RGBA(1, 0.9f, 0.6f)); }
            else if (n < 0 && downT > 2.0f) { downT = -100; LeaveDungeon(true); return; }
        }
        // 会話している間は、敵は動かず攻撃もしない（飛んでいる弾も止まる）
        bool talkingNow = !dialog.empty();
        if (!talkingNow) UpdateEnemies(gdt);

        // 飛び道具
        for (auto& pr : projs)
        {
            if (talkingNow) continue;
            pr.pos = Add(pr.pos, Mul(pr.vel, gdt));
            pr.life -= gdt;
            // 壁で止まる。15階の柱は、敵の弾は止める（隠れられる）・味方の弾は柱に当たる（下で柱を削る）
            if (SolidCell((int)std::floor(pr.pos.x / CELL), (int)std::floor(pr.pos.z / CELL)) || (!pr.friendly && !pillars.empty() && PillarSolid(pr.pos.x, pr.pos.z))) { pr.life = 0; SpawnFx(0, pr.pos, 1.f, pr.friendly ? C4(0.8f, 0.9f, 1) : C4(0.6f, 1, 0.6f), 0.3f); continue; }
            if (pr.friendly)
            {
                // 15階：糸の柱に当たる
                for (auto& pl : pillars)
                    if (pl.alive && pl.grow >= 0.99f && pr.life > 0 && DistXZ(pl.pos, pr.pos) < 0.95f + pr.radius)
                    {
                        pl.hp -= pr.dmg; pl.flash = 1;
                        AddNum(Add(pl.pos, F3(0, 3.f, 0)), (int)pr.dmg, RGBA(0.75f, 0.85f, 1.f), 24);
                        if (pl.hp <= 0) BreakPillar(pl, false);
                        if (!pr.pierce) pr.life = 0;
                    }
                for (int i = 0; i < (int)enemies.size() && i < 64; i++)
                {
                    Enemy& e = enemies[i];
                    if (e.st == ESt::Dead || (pr.hitMask >> i) & 1ull || pr.life <= 0) continue;
                    if (DistXZ(e.pos, pr.pos) < pr.radius + e.radius && pr.pos.y < (e.t == EType::Spider || e.t == EType::Guardian || e.t == EType::Utsuro ? 5.f : 2.5f))
                    {
                        bool first = pr.hitMask == 0;
                        pr.hitMask |= 1ull << i;
                        HitEnemy(e, std::round(pr.dmg * RandF(0.92f, 1.08f)), Sub(pr.pos, pr.vel), pr.pierce ? 5.f : 2.5f, pr.pierce, &party[pr.owner]);
                        if (first) { WeaponWear(party[pr.owner], 1); hitstop = std::max(hitstop, 0.03f); }
                        if (!pr.pierce) pr.life = 0;
                    }
                }
                continue;
            }
            Actor& a = party[active];
            if (a.st == ASt::Guard && Def(a).badger)
            {
                XMFLOAT3 to = Sub(pr.pos, a.pos);
                if (LenXZ(to) < 1.7f && DotXZ(NormXZ(to), Fwd(a.yaw)) > 0.3f)
                {
                    pr.life = 0;
                    WeaponWear(a, 1);
                    SpawnFx(0, pr.pos, 2.2f, C4(1, 0.85f, 0.6f), 0.4f);
                    SpawnFx(1, pr.pos, 2.0f, C4(1, 0.7f, 0.5f), 0.35f);
                    continue;
                }
            }
            if (a.st != ASt::Down && DistXZ(a.pos, pr.pos) < pr.radius + 0.45f)
            {
                DamageActor(a, pr.dmg, Sub(pr.pos, pr.vel), 4);
                pr.life = 0;
                SpawnFx(0, pr.pos, 1.4f, pr.kind == 0 ? C4(0.6f, 1, 0.5f) : C4(1, 1, 1), 0.35f);
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
            if (k.k == PickKind::Money && DistXZ(k.pos, A.pos) < 3.f && k.t > 0.4f)
                k.pos = Add(k.pos, Mul(NormXZ(Sub(A.pos, k.pos)), 8.f * dt));   // お金は吸い寄せる
            if (DistXZ(k.pos, A.pos) < 1.1f && k.t > 0.3f)
            {
                k.taken = true;
                SpawnFx(2, Add(k.pos, F3(0, 0.6f, 0)), 1.8f, k.k == PickKind::Money ? C4(1, 0.85f, 0.4f) : C4(0.6f, 0.95f, 1), 0.5f);
                if (k.k == PickKind::Key) { hasKey = true; ToastMsg(L"鍵を手に入れた！ 出口の封印を解こう", RGBA(1, 0.85f, 0.4f)); }
                else if (k.k == PickKind::Herb) { items[0]++; ToastMsg(L"薬草を拾った", RGBA(0.6f, 1, 0.6f)); }
                else if (k.k == PickKind::Money) { money += k.value; }
            }
        }

        // 調べる
        interactHint.clear();
        Chest* nc = nullptr; Spring* ns = nullptr;
        for (auto& ch : chests) if (!ch.opened && DistXZ(ch.pos, A.pos) < (ch.kind == 1 ? 2.3f : 1.9f)) nc = &ch;
        for (auto& s : springs) if (!s.used && DistXZ(s.pos, A.pos) < 2.4f) ns = &s;
        bool nearExit = DistXZ(exitPos, A.pos) < 2.8f;
        if (nc) interactHint = nc->kind == 2 ? L"[E] 糸巻きをほどく（ジオラマパーツ）" : nc->kind == 1 ? L"[E] 武器の宝箱を開ける" : L"[E] 宝箱を開ける";
        else if (ns) interactHint = L"[E] 泉の水で 灯をともす";
        else if (nearExit) interactHint = hasKey ? L"[E] 封印を解く" : L"[E] 封印の扉を調べる";
        if (c.interact)
        {
            if (nc) OpenChest(*nc);
            else if (ns)
            {
                ns->used = true;
                lantern = 100;
                A.hp = std::min(A.maxHp, A.hp + 30);
                ToastMsg(L"泉の水で灯がともった", RGBA(0.6f, 0.9f, 1));
                SpawnFx(2, Add(ns->pos, F3(0, 1, 0)), 3.5f, C4(0.6f, 0.9f, 1), 0.8f);
            }
            else if (nearExit)
            {
                if (!hasKey) ToastMsg(floorNo == 7 ? L"根に塞がれている…根の主の腕を断ち切らないと進めない" : L"封印されている…「鍵持ち」を倒して鍵を手に入れよう", RGBA(1, 0.75f, 0.6f));
                else { exitMenu = true; exitSel = 0; }
            }
        }
        for (auto& ch : chests) if (ch.opened) ch.openT += dt;

        // ミニマップ
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
    UpdateFollowCam(dt);
}

// 宝箱を開ける
void Game::OpenChest(Chest& ch)
{
    ch.opened = true; ch.openT = 0;
    XMFLOAT3 top = Add(ch.pos, F3(0, 1.0f, 0));
    if (ch.kind == 2)
    {
        int k = ch.loot;
        if (k < 0) for (int i = 0; i < LOOT_N && k < 0; i++) if (LootValid(floorNo, i) && !((floorGot[floorNo] >> i) & 1)) k = i;
        if (k >= 0) TakeLoot(floorNo, k);
        else ToastMsg(L"糸巻きは空っぽだった（この階のパーツは集め終わっている）", RGBA(0.8f, 0.85f, 0.9f));
        SpawnFx(2, top, 3.5f, C4(1, 0.85f, 0.5f), 0.8f);
        SpawnFx(1, top, 3.0f, C4(1, 0.8f, 0.5f), 0.5f);
    }
    else if (ch.kind == 1)
    {
        if (ch.key) { hasKey = true; ToastMsg(L"鍵を手に入れた！ 出口が開いた", RGBA(1, 0.85f, 0.4f)); }
        std::vector<int> cand;
        for (int w = 0; w < W_COUNT; w++)
        {
            if (!joined[kWeapons[w].owner]) continue;
            bool owned = false;
            for (auto& o : weapons) if (o.def == w) owned = true;
            if (!owned) cand.push_back(w);
        }
        if (!cand.empty())
        {
            int w = cand[RandI(0, (int)cand.size() - 1)];
            GiveWeapon(w);
            ToastMsg(L"武器：" + std::wstring(kWeapons[w].name) + L" を手に入れた！（Esc→武器で装備）", RGBA(1, 0.8f, 0.4f));
        }
        else
        {
            int t = RandI(0, AT_COUNT - 1);
            attInv[t] += 2;
            ToastMsg(L"アタッチメント：" + std::wstring(kAttach[t].name) + L" ×2", RGBA(1, 0.8f, 0.4f));
        }
        SpawnFx(2, top, 4.f, C4(1, 0.8f, 0.4f), 0.8f);
    }
    else
    {
        int r = RandI(0, 9);
        if (r < 5)
        {
            int it = RandI(0, ITEM_N - 1);
            const wchar_t* names[ITEM_N] = { L"薬草", L"灯油", L"研ぎ粉", L"帰り糸", L"繕い糸" };
            items[it]++;
            ToastMsg(std::wstring(L"宝箱から ") + names[it] + L" を手に入れた", RGBA(1, 0.9f, 0.6f));
        }
        else if (r < 8)
        {
            int t = RandI(0, AT_COUNT - 1);
            attInv[t]++;
            ToastMsg(L"アタッチメント：" + std::wstring(kAttach[t].name) + L"（" + kAttach[t].desc + L"）", RGBA(0.8f, 0.9f, 1));
        }
        else
        {
            int m = RandI(30, 60);
            money += m;
            ToastMsg(L"宝箱から " + std::to_wstring(m) + L" 文 を手に入れた", RGBA(1, 0.9f, 0.5f));
        }
        SpawnFx(2, top, 2.5f, C4(1, 0.9f, 0.5f), 0.6f);
    }
}

// カメラ（背後追従＋壁との衝突補正）。町歩きでも使う
void Game::UpdateFollowCam(float dt)
{
    Actor& Pa = party[active];
    bool locked = scene == SceneId::Dungeon && lockIdx >= 0 && lockIdx < (int)enemies.size();
    float dist = camDist;
    if (locked)
    {
        // ロックオンカメラ：自分の肩ごしに敵を正面に見る。離れているほど少し引いて両方を画面に入れる
        const Enemy& e = enemies[lockIdx];
        float ty = YawOf(Sub(e.pos, Pa.pos));
        camYaw = ApproachAngle(camYaw, ty, dt * 5.f);
        dist = camDist + std::min(4.f, DistXZ(e.pos, Pa.pos) * 0.3f) + (e.radius > 1.5f ? 2.f : 0.f);
    }
    else if (autopilot && !autoGuard && Pa.speedNow > 0.5f) camYaw = ApproachAngle(camYaw, Pa.yaw, dt * 1.5f);
    XMFLOAT3 target = Add(Pa.pos, F3(0, 1.3f, 0));
    target.y = std::max(1.3f, target.y);
    if (locked) target = Lerp3(target, Add(enemies[lockIdx].pos, F3(0, 1.0f, 0)), 0.35f);
    XMFLOAT3 f = Fwd(camYaw);
    XMFLOAT3 desired = Add(target, F3(-f.x * dist * std::cos(camPitch), dist * std::sin(camPitch), -f.z * dist * std::cos(camPitch)));
    for (float t = 0.3f; t <= 1.0f; t += 0.04f)
    {
        XMFLOAT3 q = Lerp3(target, desired, t);
        if (Solid(q.x, q.z) && q.y < WALL_H + 0.4f)
        {
            desired = Lerp3(target, desired, std::max(0.25f, t - 0.06f));
            break;
        }
    }
    camPosNow = Lerp3(camPosNow, desired, Saturate(dt * (arriveT < 1.2f ? 4.f : 9.f)));
    camTargetNow = Lerp3(camTargetNow, target, Saturate(dt * 12));
}
