#include "Game.h"

// ------------------------------------------------------------------
// データ（部品と住人の要望）
// ------------------------------------------------------------------
const PartDef kParts[P_COUNT] = {
    { L"トウジの家",     2, 2, 8,  L"村長トウジ（フクロウ）の家。最初からある" },
    { L"クロベエの家",   2, 2, 8,  L"炭焼き・アナグマのクロベエの家" },
    { L"ウサギ姉妹の家", 2, 2, 8,  L"畑仕事が好きなウサギの姉妹の家" },
    { L"里人の家",       2, 2, 6,  L"人間の家族が住む家" },
    { L"機織り小屋",     3, 2, 10, L"完成すると、ほつれ糸からアタッチメントを紡げる" },
    { L"鍛冶屋",         2, 2, 8,  L"完成すると、武器の修理ができる" },
    { L"炭焼き窯",       2, 2, 8,  L"クロベエの仕事場。完成するとイベント" },
    { L"畑",             2, 2, 4,  L"野菜が育つ" },
    { L"木",             1, 1, 3,  L"自然物" },
    { L"提灯",           1, 1, 2,  L"夜道を照らす" },
    { L"道",             1, 1, 1,  L"歩ける道" },
    { L"布干し",         2, 1, 3,  L"藍・茜・黄土の反物" },
    { L"井戸",           1, 1, 4,  L"村の水場。川から2マス以内にだけ置ける" },
    { L"水車",           1, 2, 6,  L"川の上にだけ置ける。完成するとイベント" },
    { L"商人の店",       2, 2, 6,  L"完成すると、タヌキの商人のよろず屋が開く" },
    { L"川",             1, 1, 2,  L"村に引き込む小川。橋をかけられる" },
    { L"橋",             2, 1, 3,  L"川の上にだけかけられる。渡って歩ける" },
    { L"お地蔵さん",     1, 1, 2,  L"村長が教えてくれた。道ばたで村を見守る" },
    { L"縁台",           2, 1, 2,  L"商人が教えてくれた。ひと休みする長いす" },
    { L"花畑",           1, 1, 2,  L"機織りの娘が教えてくれた。季節の花" },
    { L"石灯籠",         1, 1, 2,  L"鍛冶屋の親方が教えてくれた。石の灯り" },
    { L"かかし",         1, 1, 2,  L"ウサギの姉妹が教えてくれた。畑の番人" },
};

// はめ込みパーツ（主要なジオラマの枠にはめる小さなジオラマ）。人物は、はめ込むと住人として町に現れる
const SubDef kSubs[S_COUNT] = {
    { L"暖簾",     3, L"店や家の入口を飾る" },
    { L"軒提灯",   3, L"軒先を照らす小さな提灯" },
    { L"植木鉢",   2, L"花の咲いた鉢" },
    { L"酒樽",     3, L"こもかぶりの樽" },
    { L"薪の束",   2, L"積み上げた薪" },
    { L"看板",     3, L"屋号を書いた立て看板" },
    { L"村長トウジ",   5, L"人物：フクロウの村長" },
    { L"タヌキの商人", 5, L"人物：よろず屋の主人" },
    { L"機織りの娘",   4, L"人物：機織り小屋で働く" },
    { L"鍛冶屋の親方", 4, L"人物：鍛冶屋の主人" },
    { L"ウサギの姉妹", 4, L"人物：畑仕事が好き" },
    { L"里人",         3, L"人物：村の人" },
    { L"里の子ども",   3, L"人物：元気な子ども" },
    { L"カイリ",       5, L"人物：カワウソの船大工。水車にはめて完成させると仲間になる" },
};

const ReqDef kReqs[] = {
    { P_HouseTouji,    RK_NearCenter,   P_COUNT, 3, 0, 10, L"トウジ",     L"御神木の近く（3マス以内）" },
    { P_HouseTouji,    RK_NearPart,     P_Loom,  1, 0, 10, L"トウジ",     L"機織り小屋のとなり" },
    { P_HouseKurobee,  RK_NearPart,     P_Kiln,  1, 0, 10, L"クロベエ",   L"炭焼き窯のそばの土の上" },
    { P_HouseRabbit,   RK_NearPart,     P_Field, 1, 0, 10, L"ウサギ姉妹", L"畑のとなり" },
    { P_Loom,          RK_AdjRiver,     P_COUNT, 1, 0, 10, L"機織り小屋", L"川に面している" },
    { P_HouseVillager, RK_TreesAround,  P_Tree,  2, 3, 8,  L"里人",       L"木が3本以上まわりにある" },
};
const int kReqCount = (int)(sizeof(kReqs) / sizeof(kReqs[0]));
// 復興率の満点：100個のパーツ全部＋村長の家と村長＋住人の要望
static float MaxPoints()
{
    static float m = 0;
    if (m > 0) return m;
    m = (float)kParts[P_HouseTouji].pts + (float)kSubs[S_Chief].pts;
    for (int f = 1; f <= MAX_FLOOR; f++)
        for (int k = 0; k < LOOT_N; k++)
        {
            int id = kFloorLoot[f].ids[k];
            if (id < 0) break;
            if (id >= SUB_ID) m += (float)kSubs[id - SUB_ID].pts; else if (id >= 0) m += (float)kParts[id].pts;
        }
    for (int r = 0; r < kReqCount; r++) m += (float)kReqs[r].pts;
    m += 9 * 3.f;   // 住人9人の思い出話（各3）
    return m;
}

// ------------------------------------------------------------------
XMFLOAT3 Game::CellCenter(int x, int z, int w, int h) const
{
    return { (x + w * 0.5f - VG * 0.5f) * 2.f, 0, (z + h * 0.5f - VG * 0.5f) * 2.f };
}

void Game::PartSize(PartType t, int rot, int& w, int& h) const
{
    w = kParts[t].w; h = kParts[t].h;
    if (rot & 1) std::swap(w, h);
}

int Game::Gap(int ax, int az, int aw, int ah, int bx, int bz, int bw, int bh) const
{
    int gx = std::max(0, std::max(ax - (bx + bw), bx - (ax + aw)));
    int gz = std::max(0, std::max(az - (bz + bh), bz - (az + ah)));
    return std::max(gx, gz);
}

static bool Overlap(int ax, int az, int aw, int ah, int bx, int bz, int bw, int bh)
{
    return ax < bx + bw && bx < ax + aw && az < bz + bh && bz < az + ah;
}

// 川のマス：もとの川（18・19列）と、置いた「川」
bool Game::RiverCell(int cx, int cz) const
{
    if (cz < 0 || cz >= VG) return false;
    if (IsRiverCol(cx)) return true;
    for (auto& p : placed) if (p.t == P_River && p.x == cx && p.z == cz) return true;
    return false;
}

bool Game::BridgeCell(int cx, int cz) const
{
    for (auto& p : placed)
    {
        if (p.t != P_Bridge) continue;
        int w, h; PartSize(p.t, p.rot, w, h);
        if (Overlap(cx, cz, 1, 1, p.x, p.z, w, h)) return true;
    }
    return false;
}

bool Game::CanPlace(PartType t, int x, int z, int rot, int ignore) const
{
    int w, h; PartSize(t, rot, w, h);
    // 橋の端（渡った先の通り道）には、道と川のほかは置けない。橋を置くときも、端に何かあると置けない
    if (t != P_Path && t != P_River && t != P_Bridge)
        for (int i = 0; i < w; i++) for (int j = 0; j < h; j++) if (BridgeEndCell(x + i, z + j)) return false;
    if (t == P_Bridge)
    {
        auto blockedEnd = [&](int cx, int cz) {
            for (int k = 0; k < (int)placed.size(); k++)
            {
                if (k == ignore || placed[k].t == P_Path || placed[k].t == P_River || placed[k].t == P_Bridge) continue;
                int pw, ph; PartSize(placed[k].t, placed[k].rot, pw, ph);
                if (Overlap(cx, cz, 1, 1, placed[k].x, placed[k].z, pw, ph)) return true;
            }
            return false;
        };
        if (rot & 1) { for (int i = 0; i < w; i++) if (blockedEnd(x + i, z - 1) || blockedEnd(x + i, z + h)) return false; }
        else { for (int j = 0; j < h; j++) if (blockedEnd(x - 1, z + j) || blockedEnd(x + w, z + j)) return false; }
    }
    // 橋・井戸：となり（ななめも）に建物があると置けない
    if (t == P_Bridge || t == P_Well)
        for (int i = 0; i < (int)placed.size(); i++)
        {
            if (i == ignore || SlotCount(placed[i].t) == 0 || placed[i].t == P_Well || placed[i].t == P_Wheel || placed[i].t == P_Field) continue;   // 家・工房・店など
            int pw, ph; PartSize(placed[i].t, placed[i].rot, pw, ph);
            if (Gap(x, z, w, h, placed[i].x, placed[i].z, pw, ph) < 1) return false;
        }
    if (t == P_Bridge || t == P_Wheel)
    {
        // 橋・水車：すべてのマスが川の上で、ほかの部品と重ならない
        if (x < 0 || z < 0 || x + w > VG || z + h > VG) return false;
        for (int i = 0; i < w; i++) for (int j = 0; j < h; j++) if (!RiverCell(x + i, z + j)) return false;
        for (int i = 0; i < (int)placed.size(); i++)
        {
            if (i == ignore || placed[i].t == P_River) continue;
            int pw, ph; PartSize(placed[i].t, placed[i].rot, pw, ph);
            if (Overlap(x, z, w, h, placed[i].x, placed[i].z, pw, ph)) return false;
        }
        return true;
    }
    if (x < 0 || z < 0 || x + w > VG || z + h > VG) return false;
    for (int i = 0; i < w; i++) if (IsRiverCol(x + i)) return false;          // もとの川の上には置けない（川の向こう岸には置ける）
    if (t == P_Well)
    {
        // 井戸：川（もとの川・置いた川）から2マス以内の地面にだけ置ける
        bool nearRiver = false;
        for (int dx = -2; dx <= 2 && !nearRiver; dx++) for (int dz = -2; dz <= 2 && !nearRiver; dz++) if (RiverCell(x + dx, z + dz)) nearRiver = true;
        if (!nearRiver) return false;
    }
    if (Overlap(x, z, w, h, 9, 9, 2, 2)) return false;
    if (Overlap(x, z, w, h, 8, 18, 4, 2)) return false;    // 根の洞への参道
    for (int i = 0; i < (int)placed.size(); i++)
    {
        if (i == ignore) continue;
        int pw, ph; PartSize(placed[i].t, placed[i].rot, pw, ph);
        if (Overlap(x, z, w, h, placed[i].x, placed[i].z, pw, ph)) return false;
    }
    return true;
}

bool Game::ReqSatisfied(int r) const
{
    const ReqDef& q = kReqs[r];
    for (auto& p : placed)
    {
        if (p.t != q.owner) continue;
        int w, h; PartSize(p.t, p.rot, w, h);
        switch (q.kind)
        {
        case RK_NearCenter:
            if (Gap(p.x, p.z, w, h, 9, 9, 2, 2) < q.dist) return true;
            break;
        case RK_AdjRiver:
            if (RIVER_X - (p.x + w) < q.dist && p.x + w <= RIVER_X) return true;     // こちら岸から川に面している
            if (p.x - (RIVER_X + 2) < q.dist && p.x >= RIVER_X + 2) return true;     // 向こう岸から川に面している
            break;
        case RK_NearPart:
        case RK_TreesAround:
        {
            int n = 0;
            for (auto& o : placed)
            {
                if (o.t != q.other) continue;
                int ow, oh; PartSize(o.t, o.rot, ow, oh);
                if (Gap(p.x, p.z, w, h, o.x, o.z, ow, oh) < q.dist) n++;
            }
            if (n >= std::max(1, q.count)) return true;
            break;
        }
        }
    }
    return false;
}

// はめ込み枠の数：家・工房・店は3つ、井戸・水車・畑は1つ、ほかは0
// はめ込み枠ごとに決まっているパーツ（レシピ）。-1 は枠なし
//   全部の枠の合計 = 糸巻きから出るはめ込みパーツ29個 ＋ 最初からいる村長
static const int kRecipe[P_COUNT][MAX_SLOT] = {
    { S_Chief, S_Noren, S_Pot },        // トウジの家
    { S_Noren, S_Wood, S_Sign },        // クロベエの家
    { S_Rabbits, S_Pot, S_Wood },       // ウサギ姉妹の家
    { S_Villager, S_Child, S_Lamp },    // 里人の家（2軒）
    { S_Weaver, S_Noren, S_Lamp },      // 機織り小屋
    { S_Smith, S_Barrel, S_Sign },      // 鍛冶屋
    { S_Wood, S_Wood, S_Lamp },         // 炭焼き窯
    { S_Pot, -1, -1 },                  // 畑（2つ）
    { -1, -1, -1 },                     // 木
    { -1, -1, -1 },                     // 提灯
    { -1, -1, -1 },                     // 道
    { -1, -1, -1 },                     // 布干し
    { -1, -1, -1 },                     // 井戸（はめ込みなし。酒樽が余らないように）
    { S_Kairi, -1, -1 },                // 水車（カイリ。12階の QTE のあとに手に入る）
    { S_Merchant, S_Sign, S_Barrel },   // 商人の店
    { -1, -1, -1 },                     // 川
    { -1, -1, -1 },                     // 橋
    { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 },   // 会話で教わるジオラマ（はめ込み枠なし）
};
int Game::SlotNeed(PartType t, int s) const { return (t >= 0 && t < P_COUNT && s >= 0 && s < MAX_SLOT) ? kRecipe[t][s] : -1; }

int Game::SlotCount(PartType t) const
{
    int n = 0;
    while (n < MAX_SLOT && kRecipe[t][n] >= 0) n++;
    return n;
}

// はめ込み枠の位置（建物の手前の左右の角・奥の右の角）
XMFLOAT3 Game::SlotPos(const Placed& p, int s) const
{
    float hx = kParts[p.t].w * 1.f, hz = kParts[p.t].h * 1.f;
    XMFLOAT3 local = s == 0 ? F3(-hx + 0.45f, 0, -hz + 0.3f) : s == 1 ? F3(hx - 0.45f, 0, -hz + 0.3f) : F3(hx - 0.45f, 0, hz - 0.45f);
    int w, h; PartSize(p.t, p.rot, w, h);
    XMFLOAT3 c = CellCenter(p.x, p.z, w, h);
    XMFLOAT3 o; XMStoreFloat3(&o, XMVector3Transform(XMLoadFloat3(&local), XMMatrixRotationY(p.rot * PI * 0.5f)));
    return Add(c, o);
}

// カーソルの下にある配置済みの部品
int Game::PartAtCursor() const
{
    for (int i = 0; i < (int)placed.size(); i++)
    {
        int pw, ph; PartSize(placed[i].t, placed[i].rot, pw, ph);
        if (Overlap(cursorX, cursorZ, 1, 1, placed[i].x, placed[i].z, pw, ph)) return i;
    }
    return -1;
}

bool Game::AnyParts() const
{
    for (int i = 0; i < P_COUNT; i++) if (inventory[i] > 0) return true;
    for (int i = 0; i < S_COUNT; i++) if (subInv[i] > 0) return true;
    return false;
}

// はめ込みパーツの模型（正面は +z）
static Walker PersonWalker(int s);
void Game::DrawSub(int s, XMFLOAT3 pos, float yaw, float appear)
{
    float k = appear >= 1.f ? 1.f : std::max(0.01f, 1.f + std::sin(appear * PI) * 0.25f - (1 - appear) * (1 - appear));
    XMMATRIX P = XMMatrixScaling(k, k, k) * XMMatrixRotationY(yaw) * XMMatrixTranslation(pos.x, pos.y, pos.z);
    XMFLOAT3 tv = tint;
    auto D = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, float ol = 0.012f, XMFLOAT3 em = { 0, 0, 0 }) {
        DrawParams p; p.color = c; p.outline = ol; p.emissive = Add(em, tv);
        if (silhouette) { p.color = C4(0.66f, 0.72f, 0.7f); p.emissive = { 0, 0, 0 }; p.unlit = true; p.outline = 0.03f; }
        R->Draw(m, L * P, p);
    };
    auto M = [](float sx, float sy, float sz, float x, float y, float z) { return XMMatrixScaling(sx, sy, sz) * XMMatrixTranslation(x, y, z); };
    if (IsPerson(s))
    {
        // 人物のジオラマ：みんな「だるま型」の置き物。体の色と、耳・頭の飾りで誰かがわかる（正面は +z）
        XMFLOAT4 body = C4(0.8f, 0.18f, 0.14f), trim = C4(0.95f, 0.8f, 0.3f);
        switch (s)
        {
        case S_Chief: body = C4(0.55f, 0.38f, 0.22f); break;      // 村長（フクロウ）
        case S_Merchant: body = C4(0.5f, 0.36f, 0.24f); break;    // タヌキの商人
        case S_Weaver: body = C4(0.75f, 0.2f, 0.28f); break;      // 機織りの娘
        case S_Smith: body = C4(0.32f, 0.34f, 0.38f); break;      // 鍛冶屋の親方
        case S_Rabbits: body = C4(0.96f, 0.94f, 0.92f); trim = C4(0.95f, 0.55f, 0.6f); break;
        case S_Villager: body = C4(0.18f, 0.28f, 0.58f); break;   // 里人（藍）
        case S_Child: body = C4(0.9f, 0.62f, 0.18f); break;       // 子ども（山吹）
        case S_Kairi: body = C4(0.55f, 0.4f, 0.28f); trim = C4(0.3f, 0.75f, 0.8f); break;   // カイリ（カワウソ）
        }
        float sc = s == S_Child ? 0.8f : 1.f;
        XMMATRIX Q = XMMatrixScaling(sc, sc, sc) * P;
        auto DQ = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, float ol = 0.014f) {
            DrawParams p; p.color = c; p.outline = ol; p.emissive = tv;
            if (silhouette) { p.color = C4(0.66f, 0.72f, 0.7f); p.emissive = { 0, 0, 0 }; p.unlit = true; p.outline = 0.03f; }
            R->Draw(m, L * Q, p);
        };
        DQ(R->sphere, M(0.72f, 0.62f, 0.66f, 0, 0.32f, 0), body);                       // 胴（下ぶくれ）
        DQ(R->sphere, M(0.56f, 0.52f, 0.54f, 0, 0.78f, 0), body);                       // 頭
        DQ(R->sphere, M(0.4f, 0.32f, 0.16f, 0, 0.78f, 0.22f), C4(0.98f, 0.9f, 0.8f), 0.008f);   // 顔
        DQ(R->cylinder, M(0.74f, 0.06f, 0.68f, 0, 0.5f, 0), trim, 0);                   // 帯
        DQ(R->sphere, M(0.22f, 0.12f, 0.06f, 0, 0.33f, 0.31f), trim, 0);                // 胸の金の模様
        bool owl = s == S_Chief;
        for (int k = -1; k <= 1; k += 2)
        {
            DQ(R->sphere, M(owl ? 0.12f : 0.07f, owl ? 0.12f : 0.08f, 0.04f, k * 0.09f, 0.81f, 0.3f), owl ? C4(1, 0.8f, 0.2f) : C4(0.1f, 0.08f, 0.08f), 0);   // 目
            DQ(R->sphere, M(0.11f, 0.05f, 0.04f, k * 0.12f, 0.92f, 0.28f), C4(0.1f, 0.08f, 0.08f), 0);   // 眉
        }
        if (owl) { DQ(R->cone, M(0.06f, 0.08f, 0.06f, 0, 0.74f, 0.32f), C4(0.95f, 0.6f, 0.15f), 0); for (int k = -1; k <= 1; k += 2) DQ(R->cone, M(0.1f, 0.16f, 0.1f, k * 0.17f, 1.06f, 0), body); }
        if (s == S_Merchant || s == S_Kairi) for (int k = -1; k <= 1; k += 2) DQ(R->sphere, M(0.11f, 0.11f, 0.07f, k * 0.22f, 1.0f, 0), body);   // タヌキ・カワウソの耳
        if (s == S_Kairi) DQ(R->cylinder, M(0.58f, 0.07f, 0.56f, 0, 0.95f, 0), C4(0.3f, 0.75f, 0.8f), 0);   // 水色のはちまき
        if (s == S_Merchant) DQ(R->cone, M(0.5f, 0.12f, 0.5f, 0, 1.06f, 0), C4(0.85f, 0.7f, 0.4f));   // 笠
        if (s == S_Rabbits) for (int k = -1; k <= 1; k += 2) DQ(R->sphere, M(0.08f, 0.34f, 0.06f, k * 0.1f, 1.16f, 0), body);   // ウサギの耳
        if (s == S_Weaver) DQ(R->cube, M(0.6f, 0.08f, 0.56f, 0, 1.0f, -0.02f), C4(0.95f, 0.85f, 0.7f));   // 手ぬぐい
        if (s == S_Smith) DQ(R->cylinder, M(0.58f, 0.07f, 0.56f, 0, 0.92f, 0), C4(0.9f, 0.9f, 0.88f), 0);   // はちまき
        if (s == S_Villager || s == S_Child) DQ(R->sphere, M(0.5f, 0.22f, 0.48f, 0, 0.98f, -0.04f), C4(0.15f, 0.12f, 0.12f));   // 髪
        return;
    }
    XMFLOAT4 wood = C4(0.45f, 0.3f, 0.18f), dark = C4(0.25f, 0.17f, 0.1f);
    switch (s)
    {
    case S_Noren:      // 暖簾：鳥居のような小さな枠に藍の布
        for (int i = -1; i <= 1; i += 2) D(R->cylinder, M(0.07f, 1.5f, 0.07f, i * 0.42f, 0.75f, 0), dark);
        D(R->cube, M(1.0f, 0.07f, 0.09f, 0, 1.5f, 0), dark);
        for (int i = 0; i < 3; i++) D(R->cube, M(0.26f, 0.62f, 0.03f, -0.28f + i * 0.28f, 1.15f, 0.02f), C4(0.16f, 0.25f, 0.55f), 0.008f);
        D(R->cube, M(0.12f, 0.12f, 0.035f, 0, 1.2f, 0.04f), C4(0.95f, 0.92f, 0.85f), 0);
        break;
    case S_Lamp:       // 軒提灯：柱に吊るした赤い提灯
        D(R->cylinder, M(0.07f, 1.6f, 0.07f, 0, 0.8f, 0), dark);
        D(R->cube, M(0.5f, 0.05f, 0.06f, 0.2f, 1.58f, 0), dark);
        D(R->sphere, M(0.3f, 0.38f, 0.3f, 0.36f, 1.25f, 0), C4(0.85f, 0.2f, 0.15f), 0.01f, F3(0.9f, 0.35f, 0.15f));
        D(R->cylinder, M(0.2f, 0.04f, 0.2f, 0.36f, 1.45f, 0), C4(0.15f, 0.12f, 0.1f), 0);
        break;
    case S_Pot:        // 植木鉢
        D(R->cylinder, M(0.42f, 0.35f, 0.42f, 0, 0.17f, 0), C4(0.62f, 0.33f, 0.2f));
        D(R->sphere, M(0.55f, 0.45f, 0.55f, 0, 0.55f, 0), C4(0.35f, 0.58f, 0.3f), 0.015f);
        for (int i = 0; i < 4; i++) D(R->sphere, M(0.13f, 0.13f, 0.13f, std::cos(i * 1.6f) * 0.2f, 0.75f, std::sin(i * 1.6f) * 0.2f), i % 2 ? C4(1, 0.75f, 0.85f) : C4(1, 0.95f, 0.5f), 0);
        break;
    case S_Barrel:     // 酒樽（こもかぶり）
        D(R->cylinder, M(0.6f, 0.7f, 0.6f, 0, 0.35f, 0), C4(0.9f, 0.82f, 0.62f));
        for (int i = 0; i < 2; i++) D(R->cylinder, M(0.63f, 0.06f, 0.63f, 0, 0.18f + i * 0.36f, 0), C4(0.15f, 0.2f, 0.4f), 0);
        D(R->cube, M(0.3f, 0.25f, 0.02f, 0, 0.36f, 0.3f), C4(0.8f, 0.2f, 0.15f), 0);
        break;
    case S_Wood:       // 薪の束
        for (int r = 0; r < 3; r++)
            for (int i = 0; i < 3 - r; i++)
                D(R->cylinder, XMMatrixScaling(0.16f, 0.8f, 0.16f) * XMMatrixRotationX(PI / 2) * XMMatrixTranslation(-0.17f * (2 - r) + i * 0.34f, 0.09f + r * 0.15f, 0), wood);
        break;
    case S_Sign:       // 立て看板
        for (int i = -1; i <= 1; i += 2) D(R->cylinder, M(0.06f, 1.2f, 0.06f, i * 0.3f, 0.6f, 0), dark);
        D(R->cube, M(0.75f, 0.5f, 0.06f, 0, 1.0f, 0.03f), C4(0.88f, 0.78f, 0.58f));
        for (int i = 0; i < 3; i++) D(R->cube, M(0.08f, 0.08f, 0.01f, 0, 1.14f - i * 0.13f, 0.065f), C4(0.15f, 0.12f, 0.1f), 0);
        break;
    }
}

float Game::RestoreRate() const
{
    float pts = 0;
    for (auto& p : placed)
    {
        pts += (float)kParts[p.t].pts;
        for (int s = 0; s < MAX_SLOT; s++) if (p.sub[s] >= 0 && p.sub[s] < S_COUNT) pts += (float)kSubs[p.sub[s]].pts;
    }
    for (int r = 0; r < kReqCount; r++) if (ReqSatisfied(r)) pts += (float)kReqs[r].pts;
    for (unsigned b = talkBoost; b; b &= b - 1) pts += 3.f;   // 村の思い出話（はじめて聞くたびに +3）
    return std::min(1.f, pts / MaxPoints());
}

std::vector<int> Game::PaletteList() const
{
    std::vector<int> v;
    // 置くジオラマ（左の一覧）→ はめ込みパーツ（右の欄）。
    // 建物も道・川・橋・木も、置いて手持ちが0になっても一覧から消えない（×0で残る）
    for (int i = 0; i < P_COUNT; i++)
    {
        bool keep = inventory[i] > 0;
        // 置いたあとも ×0 で残る（片付けると ×1 に戻る）。はめ込みのある建物は、置いた建物のカード（はめ込みのようす）が代わりに残る
        if (!keep && i != P_HouseTouji && SlotCount((PartType)i) == 0) for (auto& p : placed) if (p.t == i) keep = true;
        if (keep) v.push_back(i);
    }
    for (int i = 0; i < S_COUNT; i++) if (subInv[i] > 0) v.push_back(SUB_ID + i);   // はめ込みパーツ
    return v;
}

// 人物のはめ込みパーツ → 町を歩く住人の見た目
static Walker PersonWalker(int s)
{
    Walker w{};
    w.person = s;
    switch (s)
    {
    case S_Chief: w.kind = 1; w.color = C4(0.55f, 0.38f, 0.22f); break;
    case S_Merchant: w.kind = 4; w.color = C4(1, 1, 1); break;
    case S_Weaver: w.kind = 0; w.color = C4(0.72f, 0.22f, 0.2f); break;
    case S_Smith: w.kind = 0; w.color = C4(0.3f, 0.32f, 0.35f); break;
    case S_Rabbits: w.kind = 3; w.color = C4(1, 1, 1); break;
    case S_Child: w.kind = 0; w.color = C4(0.85f, 0.62f, 0.22f); break;
    default: w.kind = 0; w.color = C4(0.2f, 0.3f, 0.6f); break;
    }
    return w;
}

// 住人：はめ込んだ人物だけが町に現れる（はめ込む前は出てこない）
// 仲間になったクロベエ・カイリは、町では仕事場のそばにいる（町を歩くのは主人公だけ）
void Game::RebuildWalkers()
{
    walkers.clear();
    for (int i = 0; i < (int)placed.size(); i++)
    {
        const Placed& p = placed[i];
        int w, h; PartSize(p.t, p.rot, w, h);
        XMFLOAT3 c = CellCenter(p.x, p.z, w, h);
        XMFLOAT3 door; int pat;
        bool hasDoor = DoorOf(p, door, pat);
        for (int s = 0; s < MAX_SLOT; s++)
        {
            if (!IsPerson(p.sub[s]) || p.sub[s] == S_Kairi) continue;   // カイリは仲間として水車のそばに出る
            Walker wk = PersonWalker(p.sub[s]);
            wk.home = i;
            wk.pos = hasDoor ? Add(door, F3(RandF(-1.2f, 1.2f), 0, RandF(-0.3f, 0.3f))) : Add(c, F3(RandF(-1, 1), 0, -h - 0.6f));
            if (VillageSolid(wk.pos.x, wk.pos.z)) wk.pos = NearWalkable(Add(c, F3(0, 0, -h - 1.0f)));
            wk.phase = RandF(0, 6); wk.waitT = RandF(0, 2);
            if (p.sub[s] == S_Merchant && hasDoor) { wk.pos = Add(door, F3(1.3f, 0, 0)); wk.waitT = 1e9f; }   // 商人は店の前
            wk.target = wk.pos;
            walkers.push_back(wk);
            if (p.sub[s] == S_Rabbits)
            {
                Walker w2 = wk; w2.color = C4(1, 0.9f, 0.9f); w2.pos = Add(wk.pos, F3(0.8f, 0, 0)); w2.target = w2.pos;
                walkers.push_back(w2);
            }
        }
    }
    auto nearPart = [&](PartType t, int kind) {
        for (int i = 0; i < (int)placed.size(); i++)
            if (placed[i].t == t)
            {
                int w, h; PartSize(placed[i].t, placed[i].rot, w, h);
                Walker wk{};
                wk.kind = kind; wk.color = C4(1, 1, 1); wk.home = i;
                wk.pos = Add(CellCenter(placed[i].x, placed[i].z, w, h), F3(-1.0f, 0, -h - 0.8f));
                if (VillageSolid(wk.pos.x, wk.pos.z)) wk.pos = NearWalkable(wk.pos);
                wk.target = wk.pos; wk.phase = RandF(0, 6); wk.waitT = RandF(0, 2);
                walkers.push_back(wk);
                return true;
            }
        return false;
    };
    if (joined[1] && !nearPart(P_Kiln, 2)) nearPart(P_HouseKurobee, 2);
    if (joined[2]) nearPart(P_Wheel, 5);
}

// 最初の村：村長トウジと村長の家だけ（ほかは全部、根の洞の糸巻きから手に入れる）
void Game::EnsureStart()
{
    for (auto& p : placed) if (p.t == P_HouseTouji) return;
    Placed p{ P_HouseTouji, 12, 10, 0, 10.f };
    p.sub[0] = S_Chief;
    placed.insert(placed.begin(), p);
}

// ------------------------------------------------------------------
void Game::EnterVillage(bool fromDungeon)
{
    scene = SceneId::Village;
    sceneT = 0;
    autopilot = false; autoGuard = false; autoSpecial = 0; autoDash = false;
    enemies.clear(); projs.clear(); fx.clear(); nums.clear(); trail.clear(); pickups.clear(); markers.clear();
    lockIdx = -1; tokens = 0; exitMenu = false; menuOpen = false; qteOn = false; hitstop = 0; shake = 0; arriveT = 9;
    EnsureStart();
    for (auto& a : party) { a.hp = a.maxHp; a.st = ASt::Normal; a.pos.y = 0; }
    active = 0;            // 町を歩くのは主人公（ソラ）だけ
    CancelMove();
    townMenu = false; geoFocus = 0; carrySub = -1; talkMenu = false; talkWalker = -1;
    lantern = 100;
    rateShown = RestoreRate();
    RebuildWalkers();
    // 村では基本は町歩き。洞から戻ったら入口の前に立つ
    EnterTownWalk(fromDungeon);
    if (fromDungeon)
    {
        if (crystalsThisRun > 0)
        {
            ToastMsg(L"ジオラマパーツを " + std::to_wstring(crystalsThisRun) + L" 個 持ち帰った", RGBA(0.6f, 0.95f, 1));
            if (!Done(EV_FIRSTRETURN))
            {
                eventsDone |= EV_FIRSTRETURN;
                Say(L"ツムギ", L"おかえり！ 持ち帰ったパーツは、わたしが紡ぎなおして村に戻せるよ。");
                Say(L"ツムギ", L"Tab で配置モードにして置いたら、住人と話して「はめ込み」を解放してね。はめ込みは建物の前で F を押して自分でやるよ。");
            }
            else Say(L"ツムギ", L"おかえり！ パーツを村に置いてあげよう（Tab で配置モード）。");
        }
        crystalsThisRun = 0;
        SaveGame(false);   // 村に戻ったら自動セーブ
    }
    else if (!Done(EV_OPENING))
    {
        // パーツ0から始まるオープニング → チュートリアル
        eventsDone |= EV_OPENING;
        Say(L"ツムギ", L"ソラ、気がついた？ ここはハタオリ村…霧に呑まれて、何もかも消えちゃった。");
        Say(L"ソラ", L"……村が、ない。家も、みんなも。");
        Say(L"ツムギ", L"消えたものは「ジオラマパーツ」になって、地の底の「根の洞」に散らばってるの。持ち帰れば、わたしが紡ぎなおしてあげる！");
        Say(L"トウジ", L"ほう…目が覚めたか、ソラ。残ったのは、わしとこの家だけじゃ。");
        Say(L"ツムギ", L"村の北に洞の入口があるよ。各階には糸巻きが10個ずつ散らばってるはず。持ち帰ったら Tab で置いて、建物の前で F を押して自分ではめ込もう！");
        Say(L"ツムギ", L"はめ込みは、住人と話すと手伝ってもらえるようになるよ。まずは村長さんに話しかけてみて。Esc で村のメニューも開けるよ。");
    }
}

void Game::UpdateVillage(float dt, InputState& in)
{
    rateShown = Approach(rateShown, RestoreRate(), dt * 0.25f);
    for (auto& p : placed) p.appearT += dt;

    // 住人の散歩・仲間の追従・演出
    UpdateWalkersAndFollowers(dt);
    TickFx(dt);

    if (vWalkCam) { UpdateTown(dt, in); return; }
    if (!dialog.empty() || transOn) return;

    // カメラ
    if (in.Down('Q')) vYaw += dt * 1.5f;
    if (in.Down('E')) vYaw -= dt * 1.5f;
    if (in.mouseX >= 360 && in.mouseX <= R->W - 410) vDist = Clampf(vDist - in.wheel * 0.02f, 18, 52);
    if (in.Pressed(VK_TAB)) { carrySub = -1; CancelMove(); WalkFromCursor(); return; }

    std::vector<int> pal = PaletteList();
    if (!pal.empty()) palSel = std::clamp(palSel, 0, (int)pal.size() - 1);
    std::vector<std::pair<int, int>> left; std::vector<int> right;
    GeoLists(pal, left, right);
    auto cursorToPart = [&](int idx) {
        const Placed& p = placed[idx];
        int w, h; PartSize(p.t, p.rot, w, h);
        cursorX = p.x + w / 2; cursorZ = p.z + h / 2;
    };
    // 置いた建物のカード（はめ込み先）の一覧
    std::vector<int> cards;
    for (auto& e : left) if (e.first == 1) cards.push_back(e.second);
    // メニューの項目を選んで決める（キーでもクリックでも同じ）
    auto choose = [&](int col, int row) {
        if (col == 0 && row < (int)left.size())
        {
            if (left[row].first == 0) { palSel = left[row].second; geoFocus = 1; }
            else ToastMsg(L"はめ込むには、右のパーツを選んで、このカードの同じ形の枠へ運ぶ", RGBA(1, 0.92f, 0.75f));
        }
        else if (col == 1 && row < (int)right.size())
        {
            palSel = right[row];
            if (pal[palSel] >= SUB_ID)
            {
                // はめ込みパーツを持ち上げて運ぶ（運ぶ先は左のカードの枠）
                carrySub = pal[palSel] - SUB_ID; carryCard = 0; carrySlot = 0;
                if (cards.empty()) { ToastMsg(L"はめ込み先の建物がまだない", RGBA(1, 0.7f, 0.6f)); carrySub = -1; }
            }
            else geoFocus = 1;
        }
    };

    float W = (float)R->W, H = (float)R->H;
    uiMouseX = in.mouseX; uiMouseY = in.mouseY;
    if (flyT >= 0) { flyT += dt; if (flyT > 0.6f) flyT = -1; }
    bool overPalette = (in.mouseX < 360 && in.mouseY > 96 && in.mouseY < H - 40) || (in.mouseX > W - 410 && in.mouseY > 60 && in.mouseY < H - 40);
    bool backKey = in.Pressed(VK_BACK) || in.Pressed(VK_ESCAPE);
    if (moving && backKey) { CancelMove(); ToastMsg(L"移動をやめて、元の場所へ戻した", RGBA(1, 0.92f, 0.8f)); backKey = false; }
    bool okKey = in.Pressed(VK_RETURN) || in.Pressed(VK_SPACE);
    int dx = 0, dz = 0;
    if (in.Pressed('W') || in.Pressed(VK_UP)) dz = 1;
    if (in.Pressed('S') || in.Pressed(VK_DOWN)) dz = -1;
    if (in.Pressed('D') || in.Pressed(VK_RIGHT)) dx = 1;
    if (in.Pressed('A') || in.Pressed(VK_LEFT)) dx = -1;
    bool placeKey = false;
    // 左のカードの枠をマウスで指しているか
    auto slotAtMouse = [&](int& pi, int& si) {
        std::vector<GeoRect> geo = GeoLayout(pal);
        for (auto& g : geo)
        {
            if (g.kind != 1) continue;
            for (int s = 0; s < SlotCount(placed[g.ref].t); s++)
            {
                float cx = g.x + 146 + s * 52.f, cy = g.y + 66;
                if ((in.mouseX - cx) * (in.mouseX - cx) + (in.mouseY - cy) * (in.mouseY - cy) < 26 * 26) { pi = g.ref; si = s; return true; }
            }
        }
        return false;
    };
    auto afterFit = [&]() {
        if (carrySub >= 0 && subInv[carrySub] <= 0) { carrySub = -1; carryByMouse = false; geoCol = 1; geoRow = 0; }
    };

    if (carrySub >= 0)
    {
        // ---- はめ込みパーツを運んでいる：W/S で建物のカード、A/D で枠、Enter/Space ではめる、Esc でやめる
        if (cards.empty()) carrySub = -1;
        else
        {
            if (dz) { carryCard = std::clamp(carryCard - dz, 0, (int)cards.size() - 1); carrySlot = 0; carryByMouse = false; }
            int ns = SlotCount(placed[cards[carryCard]].t);
            if (dx) { carrySlot = std::clamp(carrySlot + dx, 0, ns - 1); carryByMouse = false; }
            carrySlot = std::clamp(carrySlot, 0, ns - 1);
            // 運ぶ先のカードが見えるようにスクロール
            for (int r = 0, k = 0; r < (int)left.size(); r++) if (left[r].first == 1) { if (k == carryCard) { geoCol = 0; geoRow = r; } k++; }
            if (okKey && TryFit(cards[carryCard], carrySlot, carrySub)) afterFit();
            if (backKey || in.RPressed()) { carrySub = -1; carryByMouse = false; geoCol = 1; geoRow = 0; }
            // マウス：枠をクリック（またはドラッグして離す）ではめる
            if (in.mouseMoved) carryByMouse = true;
            int pi, si;
            bool released = !in.lmb && in.plmb;
            if (carrySub >= 0 && (in.LPressed() || released) && slotAtMouse(pi, si))
            {
                if (TryFit(pi, si, carrySub)) afterFit();
            }
        }
    }
    else if (geoFocus == 0)
    {
        // ---- メニュー操作（WASD）：W/S で選ぶ、A/D で左の一覧と右のパーツ欄（5列）を行き来、Enter/Space で決定、Esc で町へ
        if (geoCol == 0)
        {
            if (dz) geoRow -= dz;
            if (dx > 0 && !right.empty()) { geoCol = 1; geoRow = 0; }
        }
        else
        {
            if (dz) geoRow -= dz * 5;
            if (dx > 0 && geoRow % 5 < 4) geoRow++;
            if (dx < 0) { if (geoRow % 5 == 0) { geoCol = 0; geoRow = 0; } else geoRow--; }
        }
        int n = geoCol == 0 ? (int)left.size() : (int)right.size();
        if (n == 0) { geoCol = (geoCol == 0 && !right.empty()) ? 1 : 0; n = geoCol == 0 ? (int)left.size() : (int)right.size(); }
        geoRow = n > 0 ? std::clamp(geoRow, 0, n - 1) : 0;
        // 選んでいるものの説明を出す（マップのカーソルは動かさない）
        if (geoCol == 0 && geoRow < (int)left.size()) { if (left[geoRow].first == 0) palSel = left[geoRow].second; }
        else if (geoCol == 1 && geoRow < (int)right.size()) palSel = right[geoRow];
        if (okKey) choose(geoCol, geoRow);
        if (backKey) { vWalkCam = true; camYaw = party[active].yaw; return; }
    }
    else
    {
        // ---- マップ操作（WASD でカーソル、Enter/Space で置く、Esc でメニューへ戻る）
        if (dx || dz)
        {
            int q = ((int)std::lround(vYaw / (PI / 2)) % 4 + 4) % 4;
            for (int i = 0; i < q; i++) { int t = dx; dx = dz; dz = -t; }
            cursorX = std::clamp(cursorX + dx, 0, VG - 1);
            cursorZ = std::clamp(cursorZ + dz, 0, VG - 1);
        }
        if (okKey) placeKey = true;
        if (backKey) geoFocus = 0;
    }
    if (in.mouseMoved && !overPalette && carrySub < 0)
    {
        XMFLOAT3 o, d;
        R->ScreenRay(in.mouseX, in.mouseY, o, d);
        if (d.y < -0.01f)
        {
            float t = -o.y / d.y;
            float wx = o.x + d.x * t, wz = o.z + d.z * t;
            int cx = (int)std::floor(wx / 2.f + VG * 0.5f), cz = (int)std::floor(wz / 2.f + VG * 0.5f);
            if (cx >= 0 && cz >= 0 && cx < VG && cz < VG) { cursorX = cx; cursorZ = cz; }
        }
    }
    if (in.wheel != 0 && in.mouseX < 360) { geoScroll -= in.wheel > 0 ? 1 : -1; in.wheel = 0; }
    // Z/C・数字キーでも選べる
    if (!pal.empty() && carrySub < 0)
    {
        if (in.Pressed('Z')) palSel = (palSel + (int)pal.size() - 1) % (int)pal.size();
        if (in.Pressed('C')) palSel = (palSel + 1) % (int)pal.size();
        for (int i = 0; i < 9 && i < (int)pal.size(); i++) if (in.Pressed('1' + i)) palSel = i;
    }
    // クリック：メニューの項目は選ぶ（はめ込みパーツは持ち上げる）、マップは置く
    if (in.LPressed() && carrySub < 0)
    {
        if (overPalette)
        {
            std::vector<GeoRect> geo = GeoLayout(pal);
            for (auto& g : geo)
            {
                if (in.mouseX < g.x || in.mouseX > g.x + g.w || in.mouseY < g.y || in.mouseY > g.y + g.h) continue;
                if (g.kind == 1)
                {
                    for (int r = 0; r < (int)left.size(); r++) if (left[r].first == 1 && left[r].second == g.ref) { geoCol = 0; geoRow = r; choose(0, r); }
                }
                else
                {
                    for (int r = 0; r < (int)left.size(); r++) if (left[r].first == 0 && left[r].second == g.ref) { geoCol = 0; geoRow = r; }
                    for (int r = 0; r < (int)right.size(); r++) if (right[r] == g.ref) { geoCol = 1; geoRow = r; }
                    choose(geoCol, geoRow);
                    if (carrySub >= 0) carryByMouse = true;
                }
            }
        }
        else { placeKey = true; geoFocus = 1; }
    }
    // F：置いた建物（はめ込み枠のあるもの）へ順にカーソルを合わせる
    if (in.Pressed('F'))
    {
        int cur = PartAtCursor(), n = (int)placed.size();
        for (int k = 1; k <= n; k++)
        {
            int i = ((cur < 0 ? -1 : cur) + k + n) % n;
            if (SlotCount(placed[i].t) == 0) continue;
            cursorToPart(i);
            break;
        }
    }
    if (in.Pressed('R')) cursorRot = (cursorRot + 1) % 4;

    bool wasMoving = moving;   // 同じフレームで「片付け」と「持ち上げ」が重ならないように
    // ---- 置いたジオラマを動かしている（X／右クリックで持ち上げた）
    //   クリック／Enter：カーソルの場所へ移動　もう一度 X／右クリック：片付ける　Esc：やめて元の場所へ
    if (moving)
    {
        PartType t = movingPart.t;
        int w, h; PartSize(t, cursorRot, w, h);
        int x = std::clamp(cursorX, 0, VG - w), z = std::clamp(cursorZ, 0, VG - h);
        if (placeKey)
        {
            if (CanPlace(t, x, z, cursorRot))
            {
                Placed p = movingPart;
                p.x = x; p.z = z; p.rot = cursorRot; p.appearT = 0.35f;
                placed.push_back(p);
                moving = false;
                ToastMsg(std::wstring(kParts[t].name) + L" を移動した");
                RebuildWalkers();
                SaveGame(false);
            }
            else ToastMsg(L"そこには置けない", RGBA(1, 0.5f, 0.45f));
        }
        else if (in.RPressed() || in.Pressed('X'))
        {
            bool fitted = false;
            for (int k = 0; k < MAX_SLOT; k++) if (movingPart.sub[k] >= 0) fitted = true;
            if (t == P_HouseTouji) ToastMsg(L"村長の家は片付けられない（移動だけできる）", RGBA(1, 0.7f, 0.6f));
            else
            {
                // 建物は片付けられる。はめ込んだパーツは外れず、建物についたまま手持ちへ（もう一度置くとそのまま戻る）
                if (fitted) stored.push_back(movingPart);
                inventory[t]++;
                moving = false;
                ToastMsg(std::wstring(kParts[t].name) + (fitted ? L" を片付けた（はめ込んだパーツはついたまま）" : L" を片付けた"));
                RebuildWalkers();
                SaveGame(false);
            }
        }
        placeKey = false;
    }
    // 配置（はめ込みパーツは左のカードへ運ぶ）
    if (placeKey && !pal.empty() && pal[palSel] >= SUB_ID)
    {
        ToastMsg(L"はめ込みパーツは、左のカードの同じ形の枠へ運んではめる", RGBA(1, 0.7f, 0.6f));
        placeKey = false;
    }
    if (placeKey && !pal.empty() && pal[palSel] < SUB_ID && inventory[pal[palSel]] <= 0)
    {
        ToastMsg(std::wstring(kParts[pal[palSel]].name) + L"の手持ちはない（もう全部置いた）", RGBA(1, 0.7f, 0.6f));
        placeKey = false;
    }
    if (placeKey && !pal.empty())
    {
        PartType t = (PartType)pal[palSel];
        int w, h; PartSize(t, cursorRot, w, h);
        int x = std::clamp(cursorX, 0, VG - w), z = std::clamp(cursorZ, 0, VG - h);
        if (CanPlace(t, x, z, cursorRot))
        {
            bool before[16]{};
            for (int r = 0; r < kReqCount; r++) before[r] = ReqSatisfied(r);
            int si = StoredIndex(t);
            if (si >= 0)
            {
                // はめ込んだまま片付けた建物は、そのパーツごと戻る
                Placed p = stored[si];
                p.x = x; p.z = z; p.rot = cursorRot; p.appearT = 0.f;
                placed.push_back(p);
                stored.erase(stored.begin() + si);
            }
            else placed.push_back({ t, x, z, cursorRot, 0.f });
            inventory[t]--;
            ToastMsg(std::wstring(kParts[t].name) + L" を紡ぎなおした");
            if (SlotCount(t) > 0 && !placed.back().open) ToastMsg(L"はめ込みは、住人と話すと解放される", RGBA(0.8f, 0.92f, 1));
            for (int r = 0; r < kReqCount; r++)
                if (!before[r] && ReqSatisfied(r))
                    ToastMsg(L"要望達成！ " + std::wstring(kReqs[r].who) + L"「" + kReqs[r].text + L"」", RGBA(1, 0.85f, 0.4f));
            RebuildWalkers();
            OnPartPlaced(t, CellCenter(x, z, w, h));
            SaveGame(false);
            if (inventory[t] <= 0) geoFocus = 0;     // 使い切ったらメニューへ戻る
        }
        else ToastMsg((t == P_Bridge || t == P_Wheel) ? std::wstring(kParts[t].name) + (t == P_Bridge ? L"は川の上の、建物のとなりでない所にだけ置ける" : L"は川の上にだけ置ける") : t == P_Well ? std::wstring(L"井戸は川から2マス以内の、建物のとなりでも橋の端でもない所にだけ置ける") : std::wstring(L"そこには置けない（橋の端は通り道なので置けない）"), RGBA(1, 0.5f, 0.45f));
    }
    // X／右クリック：置いたジオラマを持ち上げて移動する（はめ込んだパーツは外れない）
    else if (!wasMoving && !(carrySub >= 0) && (in.RPressed() || in.Pressed('X')))
    {
        int i = -1;
        // 橋が上にあれば橋を先に
        for (int k = 0; k < (int)placed.size() && i < 0; k++)
        {
            if (placed[k].t != P_Bridge) continue;
            int pw, ph; PartSize(placed[k].t, placed[k].rot, pw, ph);
            if (Overlap(cursorX, cursorZ, 1, 1, placed[k].x, placed[k].z, pw, ph)) i = k;
        }
        if (i < 0) i = PartAtCursor();
        if (i >= 0)
        {
            movingPart = placed[i];
            moveFromX = movingPart.x; moveFromZ = movingPart.z;
            cursorRot = movingPart.rot;
            placed.erase(placed.begin() + i);
            moving = true; geoFocus = 1;
            RebuildWalkers();
            ToastMsg(std::wstring(kParts[movingPart.t].name) + L" を移動：クリック／Enter で置く・もう一度 X で片付け", RGBA(1, 0.92f, 0.75f));
        }
    }
    XMFLOAT3 target = CellCenter(cursorX, cursorZ);
    vFocus = Lerp3(vFocus, Mul(target, 0.45f), Saturate(dt * 3));
}

// ------------------------------------------------------------------
// 描画
// ------------------------------------------------------------------
void Game::DrawShadow(XMFLOAT3 pos, float size, float alpha)
{
    DrawParams p;
    p.tex = T["blob"]; p.color = C4(0, 0, 0, alpha); p.blend = Blend::Alpha; p.unlit = true; p.depthWrite = false;
    R->Draw(R->plane, XMMatrixScaling(size, 1, size) * XMMatrixTranslation(pos.x, (scene == SceneId::Interior ? 0.07f : 0.03f), pos.z), p);
}

// 根の洞の入口（村の北）：岩山の洞穴・鳥居・しめ縄・石灯籠
void Game::DrawEntrance()
{
    XMFLOAT3 E = kEntrancePos;
    DrawParams rock; rock.tex = T["cave_wall"]; rock.color = C4(0.55f, 0.58f, 0.6f); rock.outline = 0.03f; rock.uv = { 3, 3, 0, 0 };
    R->Draw(R->sphere, XMMatrixScaling(14.f, 9.f, 9.f) * XMMatrixTranslation(E.x, 1.0f, E.z + 4.0f), rock);
    R->Draw(R->sphere, XMMatrixScaling(8.f, 6.f, 6.f) * XMMatrixTranslation(E.x - 6.f, 0.5f, E.z + 2.5f), rock);
    R->Draw(R->sphere, XMMatrixScaling(8.f, 6.5f, 6.f) * XMMatrixTranslation(E.x + 6.5f, 0.5f, E.z + 2.5f), rock);
    DrawParams moss; moss.color = C4(0.35f, 0.5f, 0.3f); moss.outline = 0.02f;
    for (int i = 0; i < 6; i++) R->Draw(R->sphere, XMMatrixScaling(2.2f, 1.2f, 2.0f) * XMMatrixTranslation(E.x - 6.f + i * 2.4f, 4.6f - std::abs(i - 2.5f) * 0.5f, E.z + 2.f), moss);
    DrawParams hole; hole.color = C4(0.02f, 0.02f, 0.03f); hole.unlit = true; hole.fog = 0.2f;
    R->Draw(R->sphere, XMMatrixScaling(3.4f, 4.2f, 1.2f) * XMMatrixTranslation(E.x, 1.2f, E.z - 0.35f), hole);
    // 洞の奥の光
    R->DrawBillboard(T["glint"], Add(E, F3(0, 1.4f, 0.2f)), 2.4f, 2.4f, C4(0.5f, 0.6f, 1.f, 0.5f + 0.2f * std::sin(time * 2)), Blend::Add);
    // 鳥居としめ縄
    DrawParams red; red.color = C4(0.78f, 0.22f, 0.16f); red.outline = 0.02f;
    float tz = E.z - 3.6f;
    for (int s = -1; s <= 1; s += 2) R->Draw(R->cylinder, XMMatrixScaling(0.4f, 4.0f, 0.4f) * XMMatrixTranslation(E.x + s * 2.1f, 0, tz), red);
    R->Draw(R->cube, XMMatrixScaling(5.6f, 0.35f, 0.5f) * XMMatrixTranslation(E.x, 4.1f, tz), red);
    DrawParams blk; blk.color = C4(0.12f, 0.1f, 0.1f); blk.outline = 0.02f;
    R->Draw(R->cube, XMMatrixScaling(6.2f, 0.25f, 0.6f) * XMMatrixTranslation(E.x, 4.4f, tz), blk);
    R->Draw(R->cube, XMMatrixScaling(4.6f, 0.2f, 0.3f) * XMMatrixTranslation(E.x, 3.4f, tz), red);
    DrawParams rp; rp.tex = T["rope"]; rp.color = C4(1, 0.92f, 0.7f); rp.uv = { 8, 1, 0, 0.35f };
    R->Draw(R->cylinder, XMMatrixScaling(0.25f, 4.2f, 0.25f) * XMMatrixRotationZ(PI / 2) * XMMatrixTranslation(E.x + 2.1f, 3.1f, tz), rp);
    DrawParams shide; shide.color = C4(1, 1, 1); shide.cullNone = true;
    for (int i = 0; i < 4; i++) R->Draw(R->cube, XMMatrixScaling(0.18f, 0.5f, 0.02f) * XMMatrixTranslation(E.x - 1.3f + i * 0.85f, 2.75f, tz - 0.1f), shide);
    // 看板
    DrawParams sign; sign.tex = T["plank"]; sign.color = C4(0.8f, 0.7f, 0.55f); sign.outline = 0.02f;
    R->Draw(R->cube, XMMatrixScaling(0.15f, 1.6f, 0.15f) * XMMatrixTranslation(E.x - 3.2f, 0.8f, tz + 0.2f), sign);
    R->Draw(R->cube, XMMatrixScaling(1.2f, 0.6f, 0.1f) * XMMatrixTranslation(E.x - 3.2f, 1.6f, tz + 0.1f), sign);
    // 石灯籠
    DrawParams st; st.tex = T["stonelantern"]; st.color = C4(0.8f, 0.8f, 0.78f); st.outline = 0.02f;
    for (int s = -1; s <= 1; s += 2)
    {
        float x = E.x + s * 3.0f, z = E.z - 5.2f;
        R->Draw(R->cylinder, XMMatrixScaling(0.35f, 1.0f, 0.35f) * XMMatrixTranslation(x, 0, z), st);
        DrawParams lt = st; lt.emissive = { 0.6f, 0.4f, 0.15f };
        R->Draw(R->cube, XMMatrixScaling(0.6f, 0.5f, 0.6f) * XMMatrixTranslation(x, 1.25f, z), lt);
        R->Draw(R->prism, XMMatrixScaling(0.9f, 0.35f, 0.9f) * XMMatrixTranslation(x, 1.5f, z), st);
    }
    // 石畳の参道
    DrawParams pv; pv.tex = T["stonepave"]; pv.color = C4(0.85f, 0.85f, 0.82f); pv.worldUV = true; pv.uv = { 0.35f, 0.35f, 0, 0 };
    R->Draw(R->plane, XMMatrixScaling(3.6f, 1, 9.f) * XMMatrixTranslation(E.x, 0.03f, E.z - 6.0f), pv);
}

void Game::DrawGround(bool village, float rate)
{
    // 地面
    DrawParams g;
    g.tex = T["grass"]; g.worldUV = true; g.uv = { 0.09f, 0.09f, 0, 0 };
    g.color = Lerp4(C4(0.72f, 0.74f, 0.66f), C4(1, 1, 1), rate);
    g.rim = 0;
    R->Draw(R->plane, XMMatrixScaling(400, 1, 400), g);
    // 川
    float rx0 = (RIVER_X - VG * 0.5f) * 2.f, rx1 = rx0 + 4.f;
    DrawParams w;
    w.tex = T["water"]; w.worldUV = true; w.uv = { 0.12f, 0.12f, 0, -time * 0.05f };
    w.color = C4(0.75f, 0.9f, 1.f); w.emissive = { 0.05f, 0.1f, 0.14f }; w.rim = 0;
    R->Draw(R->plane, XMMatrixScaling(rx1 - rx0, 1, 400) * XMMatrixTranslation((rx0 + rx1) * 0.5f, 0.04f, 0), w);
    DrawParams b;
    b.tex = T["bank"]; b.uv = { 60, 0.3f, 0, 0 }; b.rim = 0;
    // 岸：村の外は長い石垣。村の中は1マスずつ並べ、置いた川がとなりにつながるところは岸をあける（水がつながる）
    for (int s = -1; s <= 1; s += 2)
    {
        R->Draw(R->cube, XMMatrixScaling(0.5f, 0.3f, 180) * XMMatrixTranslation(rx0, 0.1f, s * 110.f), b);
        R->Draw(R->cube, XMMatrixScaling(0.5f, 0.3f, 180) * XMMatrixTranslation(rx1, 0.1f, s * 110.f), b);
    }
    DrawParams bc = b; bc.uv = { 0.6f, 0.3f, 0, 0 };
    for (int cz = 0; cz < VG; cz++)
    {
        float zc = (cz + 0.5f - VG * 0.5f) * 2.f;
        bool joinL = false, joinR = false;
        for (auto& p : placed) if (p.t == P_River && p.z == cz) { if (p.x == RIVER_X - 1) joinL = true; if (p.x == RIVER_X + 2) joinR = true; }
        if (!joinL) R->Draw(R->cube, XMMatrixScaling(0.5f, 0.3f, 2.02f) * XMMatrixTranslation(rx0, 0.1f, zc), bc);
        if (!joinR) R->Draw(R->cube, XMMatrixScaling(0.5f, 0.3f, 2.02f) * XMMatrixTranslation(rx1, 0.1f, zc), bc);
    }
}

void Game::DrawPart(PartType t, XMFLOAT3 c, int rot, float appear, float seed)
{
    float s = appear >= 1.f ? 1.f : (appear <= 0 ? 0.01f : 1.f + std::sin(appear * PI) * 0.15f - (1 - appear) * (1 - appear) * 0.9f);
    s = std::max(0.01f, s);
    XMMATRIX P = XMMatrixScaling(s, s, s) * XMMatrixRotationY(rot * PI * 0.5f) * XMMatrixTranslation(c.x, c.y, c.z);
    XMFLOAT3 tintv = tint;
    auto D = [&](const Mesh& m, XMMATRIX L, DrawParams p) { p.emissive = Add(p.emissive, tintv); R->Draw(m, L * P, p); };
    auto M = [](float sx, float sy, float sz, float x, float y, float z, float ry = 0) {
        return XMMatrixScaling(sx, sy, sz) * XMMatrixRotationY(ry) * XMMatrixTranslation(x, y, z);
    };
    auto Col = [](XMFLOAT4 c, Texture* tx = nullptr, float ol = 0.f, XMFLOAT4 uv = { 1, 1, 0, 0 }) {
        DrawParams p; p.color = c; p.tex = tx; p.outline = ol; p.uv = uv; return p;
    };
    XMFLOAT4 darkWood = C4(0.32f, 0.22f, 0.15f);
    XMFLOAT4 indigo = C4(0.16f, 0.25f, 0.55f), madder = C4(0.72f, 0.22f, 0.18f), ochre = C4(0.86f, 0.6f, 0.22f);

    auto House = [&](int variant) {
        D(R->cube, M(3.7f, 0.25f, 3.7f, 0, 0.12f, 0), Col(C4(0.75f, 0.75f, 0.72f), T["stonepave"], 0, { 2, 2, 0, 0 }));
        D(R->cube, M(3.2f, 2.0f, 3.0f, 0, 1.25f, 0), Col(C4(1, 0.97f, 0.92f), T["mudwall"], 0.02f, { 1.6f, 1, 0, 0 }));
        for (int i = 0; i < 4; i++)
        {
            float sx = (i & 1) ? 1.f : -1.f, sz = (i & 2) ? 1.f : -1.f;
            D(R->cube, M(0.22f, 2.1f, 0.22f, sx * 1.6f, 1.3f, sz * 1.5f), Col(darkWood));
        }
        D(R->cube, M(3.4f, 0.18f, 3.2f, 0, 2.25f, 0), Col(darkWood));
        D(R->cube, M(1.0f, 1.55f, 0.06f, 0.6f, 1.02f, -1.52f), Col(C4(1, 1, 1), T["door"]));
        D(R->cube, M(0.9f, 0.7f, 0.06f, -0.8f, 1.45f, -1.52f), Col(C4(1, 1, 1), T["shoji"], 0, { 2, 1, 0, 0 }));
        D(R->cube, M(0.06f, 0.7f, 0.9f, 1.62f, 1.45f, 0), Col(C4(1, 1, 1), T["shoji"]));
        D(R->cube, M(0.06f, 0.7f, 0.9f, -1.62f, 1.45f, 0), Col(C4(1, 1, 1), T["shoji"]));
        bool shingle = variant == 2;
        D(R->prism, M(4.3f, 1.9f, 4.1f, 0, 2.3f, 0), Col(C4(1, 1, 1), shingle ? T["shingle"] : T["thatch"], 0.02f, { 2.5f, 1.5f, 0, 0 }));
        D(R->cube, M(4.5f, 0.28f, 0.4f, 0, 4.2f, 0), Col(shingle ? C4(0.3f, 0.3f, 0.33f) : C4(0.45f, 0.35f, 0.22f)));
        XMFLOAT4 noren = variant == 0 ? indigo : variant == 1 ? C4(0.25f, 0.25f, 0.28f) : variant == 2 ? C4(0.9f, 0.55f, 0.6f) : madder;
        D(R->cube, M(1.05f, 0.42f, 0.03f, 0.6f, 1.62f, -1.57f), Col(noren));
        if (variant == 0)   // トウジ：屋根の上のフクロウ
        {
            DrawParams o = Col(C4(0.5f, 0.35f, 0.2f)); o.outline = 0.025f;
            D(R->sphere, M(0.6f, 0.75f, 0.55f, 1.4f, 4.7f, 0), o);
            D(R->sphere, M(0.2f, 0.2f, 0.08f, 1.3f, 4.85f, -0.25f), Col(C4(1, 0.95f, 0.85f)));
            D(R->sphere, M(0.2f, 0.2f, 0.08f, 1.52f, 4.85f, -0.25f), Col(C4(1, 0.95f, 0.85f)));
            D(R->sphere, M(0.08f, 0.08f, 0.05f, 1.3f, 4.85f, -0.3f), Col(C4(0.95f, 0.55f, 0.1f)));
            D(R->sphere, M(0.08f, 0.08f, 0.05f, 1.52f, 4.85f, -0.3f), Col(C4(0.95f, 0.55f, 0.1f)));
        }
        else if (variant == 1)   // クロベエ：炭俵
        {
            for (int i = 0; i < 3; i++)
                D(R->cylinder, XMMatrixScaling(0.55f, 0.8f, 0.55f) * XMMatrixRotationZ(PI / 2) * XMMatrixTranslation(-1.0f + i * 0.0f, 0.3f + i * 0.0f, -2.1f - i * 0.0f + (i - 1) * 0.6f) , Col(C4(0.55f, 0.45f, 0.3f), nullptr, 0.02f));
        }
        else if (variant == 2)   // ウサギ：花の鉢
        {
            for (int i = 0; i < 3; i++)
            {
                D(R->cylinder, M(0.3f, 0.3f, 0.3f, -1.4f + i * 0.45f, 0.25f, -2.0f), Col(C4(0.6f, 0.35f, 0.25f)));
                D(R->sphere, M(0.32f, 0.3f, 0.32f, -1.4f + i * 0.45f, 0.65f, -2.0f), Col(i == 1 ? C4(1, 0.85f, 0.3f) : C4(0.95f, 0.45f, 0.55f)));
            }
        }
        else   // 里人：樽
            D(R->cylinder, M(0.6f, 0.8f, 0.6f, -1.3f, 0.2f, -2.05f), Col(C4(0.55f, 0.38f, 0.22f), T["plank"], 0.02f));
    };

    switch (t)
    {
    case P_HouseTouji: House(0); break;
    case P_HouseKurobee: House(1); break;
    case P_HouseRabbit: House(2); break;
    case P_HouseVillager: House(3); break;
    case P_Shop:
    {
        // 商人の店：民家に紅白の日よけと看板、品物の樽
        House(3);
        D(R->cube, M(3.6f, 0.08f, 1.3f, 0, 2.05f, -2.1f) * XMMatrixRotationX(0.0f), Col(C4(1, 1, 1), T["stall"], 0.015f));
        D(R->cube, M(0.1f, 2.0f, 0.1f, -1.7f, 1.0f, -2.7f), Col(darkWood));
        D(R->cube, M(0.1f, 2.0f, 0.1f, 1.7f, 1.0f, -2.7f), Col(darkWood));
        D(R->cube, M(1.4f, 0.55f, 0.08f, -0.6f, 2.55f, -1.6f), Col(C4(0.25f, 0.18f, 0.12f), nullptr, 0.015f));
        D(R->cube, M(1.2f, 0.4f, 0.03f, -0.6f, 2.55f, -1.65f), Col(C4(0.95f, 0.85f, 0.6f)));
        for (int i = 0; i < 3; i++)
            D(R->cylinder, M(0.55f, 0.7f, 0.55f, -1.2f + i * 0.6f, 0.0f, -2.4f), Col(C4(0.6f, 0.42f, 0.25f), T["plank"], 0.015f));
        DrawParams ch = Col(C4(1, 1, 1), T["chochin"]); ch.emissive = { 0.5f, 0.28f, 0.1f };
        D(R->cylinder, M(0.4f, 0.55f, 0.4f, 1.4f, 1.35f, -2.6f), ch);
        break;
    }
    case P_Loom:
    {
        D(R->cube, M(5.8f, 0.25f, 3.8f, 0, 0.12f, 0), Col(C4(0.75f, 0.75f, 0.72f), T["stonepave"], 0, { 3, 2, 0, 0 }));
        D(R->cube, M(5.2f, 2.2f, 3.1f, 0, 1.35f, 0), Col(C4(1, 1, 1), T["plank"], 0.02f, { 2.5f, 1, 0, 0 }));
        D(R->cube, M(1.1f, 1.6f, 0.06f, -1.6f, 1.05f, -1.57f), Col(C4(1, 1, 1), T["door"]));
        D(R->cube, M(1.6f, 0.8f, 0.06f, 0.3f, 1.55f, -1.57f), Col(C4(1, 1, 1), T["shoji"], 0, { 2, 1, 0, 0 }));
        D(R->prism, M(6.2f, 1.6f, 4.2f, 0, 2.45f, 0), Col(C4(1, 1, 1), T["shingle"], 0.02f, { 3, 1.5f, 0, 0 }));
        D(R->cube, M(6.4f, 0.25f, 0.35f, 0, 4.05f, 0), Col(C4(0.3f, 0.3f, 0.33f)));
        // 反物を干す竿
        D(R->cube, M(0.12f, 2.6f, 0.12f, 2.9f, 1.3f, -2.2f), Col(darkWood));
        D(R->cube, M(0.12f, 2.6f, 0.12f, 1.0f, 1.3f, -2.2f), Col(darkWood));
        D(R->cube, M(2.1f, 0.1f, 0.1f, 1.95f, 2.55f, -2.2f), Col(darkWood));
        XMFLOAT4 cl[3] = { indigo, madder, ochre };
        for (int i = 0; i < 3; i++)
        {
            float sw = std::sin(time * 1.7f + i + seed) * 0.08f;
            D(R->cube, XMMatrixScaling(0.55f, 1.7f, 0.03f) * XMMatrixTranslation(0, -0.85f, 0) * XMMatrixRotationX(sw) * XMMatrixTranslation(1.35f + i * 0.62f, 2.5f, -2.2f), Col(cl[i]));
        }
        break;
    }
    case P_Forge:
    {
        D(R->cube, M(3.6f, 0.3f, 3.6f, 0, 0.15f, 0), Col(C4(0.7f, 0.7f, 0.7f), T["stonepave"], 0, { 2, 2, 0, 0 }));
        DrawParams f = Col(C4(1, 1, 1), T["forge"], 0.02f); f.emissive = { 0.08f, 0.03f, 0 };
        D(R->cube, M(3.0f, 2.2f, 2.8f, 0, 1.4f, 0), f);
        D(R->prism, M(3.7f, 1.2f, 3.5f, 0, 2.5f, 0), Col(C4(1, 1, 1), T["shingle"], 0.02f, { 2, 1, 0, 0 }));
        D(R->cylinder, M(0.6f, 2.0f, 0.6f, 0.9f, 2.4f, 0.7f), Col(C4(0.45f, 0.42f, 0.4f), nullptr, 0.02f));
        D(R->cube, M(0.7f, 0.5f, 0.4f, -0.9f, 0.55f, -2.0f), Col(C4(0.25f, 0.25f, 0.28f), nullptr, 0.02f));
        break;
    }
    case P_Kiln:
    {
        D(R->sphere, M(3.3f, 2.5f, 3.3f, 0, 0.3f, 0.1f), Col(C4(0.78f, 0.55f, 0.4f), T["mudwall"], 0.025f, { 2, 2, 0, 0 }));
        DrawParams m = Col(C4(0.1f, 0.05f, 0.03f)); m.emissive = { 0.9f, 0.4f, 0.08f };
        D(R->cube, M(1.0f, 0.75f, 0.4f, 0, 0.55f, -1.45f), m);
        D(R->cylinder, M(0.5f, 1.3f, 0.5f, 0.6f, 1.2f, 0.9f), Col(C4(0.3f, 0.25f, 0.22f), nullptr, 0.02f));
        for (int i = 0; i < 4; i++)
            D(R->cylinder, XMMatrixScaling(0.28f, 1.3f, 0.28f) * XMMatrixRotationZ(PI / 2) * XMMatrixTranslation(-1.6f + 0.0f, 0.15f + (i / 2) * 0.27f, -1.7f + (i % 2) * 0.3f + (i / 2) * 0.15f), Col(C4(0.5f, 0.35f, 0.2f), T["bark"]));
        break;
    }
    case P_Field:
    {
        D(R->plane, M(3.8f, 1, 3.8f, 0, 0.05f, 0), Col(C4(1, 1, 1), T["field"]));
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 5; j++)
                D(R->sphere, M(0.32f, 0.28f, 0.32f, -1.3f + i * 0.87f, 0.15f, -1.4f + j * 0.7f), Col(C4(0.35f, 0.65f, 0.25f)));
        break;
    }
    case P_Tree:
    {
        bool maple = std::fmod(seed * 7.3f, 1.f) > 0.65f;
        XMFLOAT4 leaf = maple ? C4(0.26f, 0.52f, 0.24f) : C4(0.36f, 0.62f, 0.3f);      // 木はみんな緑（濃い緑と若葉色）
        XMFLOAT4 leaf2 = maple ? C4(0.34f, 0.6f, 0.28f) : C4(0.46f, 0.72f, 0.36f);
        D(R->cylinder, M(0.4f, 2.2f, 0.4f, 0, 0, 0), Col(C4(1, 1, 1), T["bark"], 0.02f));
        D(R->sphere, M(2.0f, 1.8f, 2.0f, 0, 2.8f, 0), Col(leaf, nullptr, 0.03f));
        D(R->sphere, M(1.3f, 1.2f, 1.3f, 0.55f, 2.25f, -0.35f), Col(leaf2, nullptr, 0.03f));
        D(R->sphere, M(1.4f, 1.3f, 1.4f, -0.5f, 2.35f, 0.3f), Col(leaf2, nullptr, 0.03f));
        break;
    }
    case P_Lantern:
    {
        D(R->cube, M(0.16f, 2.5f, 0.16f, 0, 1.25f, 0), Col(darkWood));
        D(R->cube, M(0.6f, 0.1f, 0.1f, 0.25f, 2.42f, 0), Col(darkWood));
        DrawParams ch = Col(C4(1, 1, 1), T["chochin"]);
        ch.emissive = { 0.55f, 0.3f, 0.12f };
        D(R->cylinder, M(0.45f, 0.62f, 0.45f, 0.45f, 1.55f, 0), ch);
        D(R->cylinder, M(0.3f, 0.06f, 0.3f, 0.45f, 2.17f, 0), Col(C4(0.1f, 0.1f, 0.1f)));
        D(R->cylinder, M(0.3f, 0.06f, 0.3f, 0.45f, 1.5f, 0), Col(C4(0.1f, 0.1f, 0.1f)));
        break;
    }
    case P_Path:
        D(R->plane, M(2.f, 1, 2.f, 0, 0.035f, 0), Col(C4(1, 1, 1), T["path"]));
        break;
    case P_River:
    {
        // 川：1マスの小川。となりも川（置いた川・もとの川）なら岸を作らず、水がつながる
        //   partMask：1=+x 2=-x 4=+z 8=-z に川がある
        DrawParams wp = Col(C4(0.75f, 0.9f, 1.f), T["water"], 0, { 0.5f, 0.5f, 0, -time * 0.2f });
        wp.emissive = { 0.05f, 0.1f, 0.14f }; wp.rim = 0; wp.worldUV = true; wp.uv = { 0.12f, 0.12f, 0, -time * 0.05f };
        float x0 = (partMask & 2) ? -1.02f : -0.86f, x1 = (partMask & 1) ? 1.02f : 0.86f;
        float z0 = (partMask & 8) ? -1.02f : -0.86f, z1 = (partMask & 4) ? 1.02f : 0.86f;
        D(R->plane, M(x1 - x0, 1, z1 - z0, (x0 + x1) * 0.5f, 0.045f, (z0 + z1) * 0.5f), wp);
        DrawParams bank = Col(C4(0.7f, 0.68f, 0.62f), T["bank"], 0.01f, { 1, 0.3f, 0, 0 });
        if (!(partMask & 1)) D(R->cube, M(0.18f, 0.22f, 2.0f, 0.95f, 0.08f, 0), bank);
        if (!(partMask & 2)) D(R->cube, M(0.18f, 0.22f, 2.0f, -0.95f, 0.08f, 0), bank);
        if (!(partMask & 4)) D(R->cube, M(2.0f, 0.22f, 0.18f, 0, 0.08f, 0.95f), bank);
        if (!(partMask & 8)) D(R->cube, M(2.0f, 0.22f, 0.18f, 0, 0.08f, -0.95f), bank);
        break;
    }
    case P_Bridge:
    {
        // 橋：川をまたぐ朱色の太鼓橋（長い方向が x）。となりに橋があるとつながる
        //   partMask（橋の向きでの左右前後）：1=+x 2=-x の端に橋 → 平らな橋で続く　4=+z 8=-z の横に橋 → その側の欄干をなくして幅広に
        XMFLOAT4 red = C4(0.75f, 0.2f, 0.14f), plank = C4(0.55f, 0.4f, 0.26f);
        bool flat = (partMask & 3) != 0;
        auto deckY = [&](float x) { return flat ? 0.6f : 0.25f + std::cos((x / 2.0f) * PI * 0.5f) * 0.55f; };
        float zN = (partMask & 8) ? -1.0f : -0.75f, zP = (partMask & 4) ? 1.0f : 0.75f;
        for (int i = 0; i < 9; i++)
        {
            float x = -1.8f + i * 0.45f;
            float ang = flat ? 0.f : -std::sin((x / 2.0f) * PI * 0.5f) * 0.45f;
            float xw = (i == 0 && (partMask & 2)) || (i == 8 && (partMask & 1)) ? 0.7f : 0.46f;   // 端も隙間なく
            float xc = i == 0 && (partMask & 2) ? x - 0.12f : i == 8 && (partMask & 1) ? x + 0.12f : x;
            D(R->cube, XMMatrixScaling(xw, 0.1f, zP - zN) * XMMatrixRotationZ(ang) * XMMatrixTranslation(xc, deckY(x), (zN + zP) * 0.5f), Col(plank, nullptr, 0.012f));
        }
        if (flat)
            for (int s = -1; s <= 1; s += 2)   // 平らな橋は下に橋脚
                D(R->cylinder, M(0.18f, 0.6f, 0.18f, s * 1.2f, 0, 0), Col(C4(0.4f, 0.3f, 0.2f), nullptr, 0.012f));
        for (int s = -1; s <= 1; s += 2)
        {
            if ((s > 0 && (partMask & 4)) || (s < 0 && (partMask & 8))) continue;   // 横につながる側は欄干なし
            for (int i = 0; i < 5; i++)
            {
                float x = -1.8f + i * 0.9f;
                float y = deckY(x);
                D(R->cylinder, M(0.1f, 0.6f, 0.1f, x, y + 0.05f, s * 0.7f), Col(red, nullptr, 0.012f));
                if (i < 4)
                {
                    float x2 = x + 0.9f, y2 = deckY(x2);
                    float ang = std::atan2(y2 - y, 0.9f);
                    D(R->cube, XMMatrixScaling(0.95f, 0.08f, 0.08f) * XMMatrixRotationZ(ang) * XMMatrixTranslation(x + 0.45f, (y + y2) * 0.5f + 0.6f, s * 0.7f), Col(red, nullptr, 0.012f));
                }
            }
        }
        break;
    }
    case P_Cloth:
    {
        D(R->cylinder, M(0.14f, 2.3f, 0.14f, -1.8f, 0, 0), Col(darkWood));
        D(R->cylinder, M(0.14f, 2.3f, 0.14f, 1.8f, 0, 0), Col(darkWood));
        D(R->cube, M(3.8f, 0.08f, 0.08f, 0, 2.2f, 0), Col(darkWood));
        XMFLOAT4 cl[3] = { indigo, madder, ochre };
        for (int i = 0; i < 3; i++)
        {
            float sw = std::sin(time * 1.9f + i * 1.3f + seed) * 0.1f;
            D(R->cube, XMMatrixScaling(1.0f, 1.5f, 0.03f) * XMMatrixTranslation(0, -0.75f, 0) * XMMatrixRotationX(sw) * XMMatrixTranslation(-1.15f + i * 1.15f, 2.15f, 0), Col(cl[i]));
        }
        break;
    }
    case P_Well:
    {
        D(R->cylinder, M(1.5f, 0.8f, 1.5f, 0, 0, 0), Col(C4(1, 1, 1), T["well"], 0.02f));
        D(R->cylinder, M(1.2f, 0.02f, 1.2f, 0, 0.7f, 0), Col(C4(0.1f, 0.2f, 0.3f)));
        D(R->cube, M(0.12f, 2.0f, 0.12f, -0.7f, 1.0f, 0), Col(darkWood));
        D(R->cube, M(0.12f, 2.0f, 0.12f, 0.7f, 1.0f, 0), Col(darkWood));
        D(R->prism, M(1.9f, 0.6f, 1.3f, 0, 1.95f, 0), Col(C4(1, 1, 1), T["shingle"], 0.02f));
        D(R->cylinder, M(0.3f, 0.3f, 0.3f, 0.45f, 0.8f, -0.2f), Col(C4(0.55f, 0.38f, 0.22f)));
        break;
    }
    case P_Wheel:
    {
        D(R->cube, M(1.2f, 1.6f, 3.4f, -0.4f, 0.8f, 0), Col(C4(1, 1, 1), T["plank"], 0.02f));
        float spin = time * 0.8f;
        XMMATRIX wheelBase = XMMatrixRotationX(spin) * XMMatrixTranslation(0.65f, 1.4f, 0);
        D(R->cylinder, XMMatrixScaling(3.2f, 0.4f, 3.2f) * XMMatrixTranslation(0, -0.2f, 0) * XMMatrixRotationZ(PI / 2) * wheelBase,
          Col(C4(0.6f, 0.45f, 0.3f), T["wood"], 0.02f));
        for (int i = 0; i < 8; i++)
            D(R->cube, XMMatrixScaling(0.5f, 0.15f, 0.5f) * XMMatrixTranslation(0, 1.6f, 0) * XMMatrixRotationX(i * PI / 4) * wheelBase, Col(darkWood));
        D(R->cylinder, XMMatrixScaling(0.3f, 1.2f, 0.3f) * XMMatrixRotationZ(PI / 2) * XMMatrixTranslation(0.4f, 1.4f, 0), Col(darkWood));
        break;
    }
    case P_Jizo:
    {
        // お地蔵さん：赤いよだれかけと笠
        DrawParams stn = Col(C4(0.72f, 0.72f, 0.7f), T["stonepave"], 0.02f);
        D(R->cube, M(0.9f, 0.25f, 0.9f, 0, 0.12f, 0), stn);
        D(R->cylinder, M(0.55f, 0.7f, 0.5f, 0, 0.25f, 0), stn);
        D(R->sphere, M(0.48f, 0.48f, 0.48f, 0, 1.15f, 0), stn);
        D(R->cube, M(0.5f, 0.3f, 0.05f, 0, 0.75f, -0.25f), Col(C4(0.85f, 0.18f, 0.15f), nullptr, 0.01f));
        D(R->cone, M(0.9f, 0.3f, 0.9f, 0, 1.4f, 0), Col(C4(0.75f, 0.62f, 0.38f), nullptr, 0.02f));
        break;
    }
    case P_Bench:
    {
        // 縁台：赤い毛氈をかけた長いすと野点傘
        D(R->cube, M(3.2f, 0.12f, 1.0f, 0, 0.55f, 0), Col(C4(0.8f, 0.15f, 0.12f), nullptr, 0.02f));
        for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2)
            D(R->cube, M(0.12f, 0.5f, 0.12f, sx * 1.4f, 0.25f, sz * 0.35f), Col(darkWood));
        D(R->cylinder, M(0.06f, 2.2f, 0.06f, 1.0f, 0, 0.3f), Col(darkWood));
        D(R->cone, M(2.0f, 0.5f, 2.0f, 1.0f, 2.05f, 0.3f), Col(C4(0.82f, 0.2f, 0.16f), nullptr, 0.02f));
        break;
    }
    case P_Flower:
    {
        // 花畑：色とりどりの花
        D(R->cube, M(1.8f, 0.12f, 1.8f, 0, 0.06f, 0), Col(C4(0.35f, 0.55f, 0.28f), nullptr, 0.01f));
        const XMFLOAT4 fc[4] = { C4(1, 0.55f, 0.7f), C4(1, 0.95f, 0.4f), C4(0.6f, 0.65f, 1), C4(1, 1, 1) };
        for (int i = 0; i < 12; i++)
        {
            float a = i * 2.39f, r = 0.25f + (i % 4) * 0.18f;
            D(R->sphere, M(0.2f, 0.16f, 0.2f, std::cos(a) * r, 0.25f, std::sin(a) * r), Col(fc[i % 4], nullptr, 0.008f));
        }
        break;
    }
    case P_StoneLantern:
    {
        // 石灯籠：火袋に灯がともる
        DrawParams stn = Col(C4(0.78f, 0.78f, 0.75f), T["stonelantern"], 0.02f);
        D(R->cube, M(0.8f, 0.2f, 0.8f, 0, 0.1f, 0), stn);
        D(R->cylinder, M(0.3f, 0.9f, 0.3f, 0, 0.2f, 0), stn);
        DrawParams lt = stn; lt.emissive = { 0.7f, 0.45f, 0.15f };
        D(R->cube, M(0.6f, 0.5f, 0.6f, 0, 1.35f, 0), lt);
        D(R->prism, M(1.0f, 0.4f, 1.0f, 0, 1.75f, 0), stn);
        break;
    }
    case P_Scarecrow:
    {
        // かかし：へのへのもへじの顔と笠
        D(R->cylinder, M(0.1f, 1.9f, 0.1f, 0, 0, 0), Col(darkWood));
        D(R->cube, M(1.4f, 0.1f, 0.1f, 0, 1.3f, 0), Col(darkWood));
        D(R->cube, M(0.8f, 0.7f, 0.35f, 0, 1.15f, 0), Col(C4(0.3f, 0.4f, 0.65f), nullptr, 0.02f));
        D(R->sphere, M(0.45f, 0.45f, 0.45f, 0, 1.75f, 0), Col(C4(0.95f, 0.92f, 0.82f), nullptr, 0.015f));
        D(R->cone, M(0.9f, 0.3f, 0.9f, 0, 2.0f, 0), Col(C4(0.8f, 0.68f, 0.4f), nullptr, 0.02f));
        break;
    }
    default: break;
    }
}

void Game::DrawWalker(const Walker& w)
{
    if (w.kind == 4) { DrawTanuki(w.pos, w.yaw, w.phase); return; }
    float bob = std::abs(std::sin(w.phase * 7)) * (w.waitT > 0 ? 0 : 0.06f);
    XMMATRIX P = XMMatrixRotationY(w.yaw) * XMMatrixTranslation(w.pos.x, w.pos.y + bob, w.pos.z);
    auto D = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, float ol = 0.02f) { DrawParams p; p.color = c; p.outline = ol; R->Draw(m, L * P, p); };
    auto M = [](float sx, float sy, float sz, float x, float y, float z) { return XMMatrixScaling(sx, sy, sz) * XMMatrixTranslation(x, y, z); };
    DrawShadow(w.pos, 1.0f);
    switch (w.kind)
    {
    case 0: // 人間
        D(R->cylinder, M(0.5f, 0.75f, 0.45f, 0, 0.05f, 0), w.color);
        D(R->cylinder, M(0.52f, 0.1f, 0.47f, 0, 0.5f, 0), C4(0.85f, 0.75f, 0.5f), 0);
        D(R->sphere, M(0.5f, 0.5f, 0.5f, 0, 1.08f, 0), C4(0.98f, 0.83f, 0.7f));
        D(R->sphere, M(0.54f, 0.36f, 0.54f, 0, 1.2f, 0.05f), C4(0.15f, 0.12f, 0.12f));
        break;
    case 1: // フクロウ
        D(R->sphere, M(0.7f, 0.9f, 0.65f, 0, 0.5f, 0), w.color);
        D(R->sphere, M(0.5f, 0.6f, 0.2f, 0, 0.45f, -0.22f), C4(0.9f, 0.82f, 0.62f), 0);
        D(R->sphere, M(0.2f, 0.2f, 0.08f, -0.13f, 0.75f, -0.3f), C4(1, 0.95f, 0.85f), 0.01f);
        D(R->sphere, M(0.2f, 0.2f, 0.08f, 0.13f, 0.75f, -0.3f), C4(1, 0.95f, 0.85f), 0.01f);
        D(R->sphere, M(0.08f, 0.08f, 0.05f, -0.13f, 0.75f, -0.34f), C4(0.95f, 0.55f, 0.1f), 0);
        D(R->sphere, M(0.08f, 0.08f, 0.05f, 0.13f, 0.75f, -0.34f), C4(0.95f, 0.55f, 0.1f), 0);
        D(R->cone, M(0.15f, 0.25f, 0.15f, -0.2f, 0.9f, 0), w.color);
        D(R->cone, M(0.15f, 0.25f, 0.15f, 0.2f, 0.9f, 0), w.color);
        break;
    case 2: // アナグマ
    {
        Actor a; a.def = 1; a.pos = w.pos; a.yaw = w.yaw; a.walkT = w.phase; a.speedNow = w.waitT > 0 ? 0 : 1.3f;
        DrawKurobee(a, 1);
        break;
    }
    case 5: // カワウソ（カイリ）
    {
        Actor a; a.def = 2; a.pos = w.pos; a.yaw = w.yaw; a.walkT = w.phase; a.speedNow = w.waitT > 0 ? 0 : 1.3f;
        DrawKairi(a, 1);
        break;
    }
    case 3: // ウサギ
        D(R->sphere, M(0.55f, 0.6f, 0.55f, 0, 0.35f, 0), w.color);
        D(R->sphere, M(0.45f, 0.42f, 0.45f, 0, 0.8f, 0), w.color);
        D(R->sphere, M(0.12f, 0.55f, 0.1f, -0.1f, 1.2f, 0.05f), w.color);
        D(R->sphere, M(0.12f, 0.55f, 0.1f, 0.1f, 1.2f, 0.05f), w.color);
        D(R->sphere, M(0.06f, 0.06f, 0.04f, -0.09f, 0.85f, -0.2f), C4(0.15f, 0.1f, 0.1f), 0);
        D(R->sphere, M(0.06f, 0.06f, 0.04f, 0.09f, 0.85f, -0.2f), C4(0.15f, 0.1f, 0.1f), 0);
        D(R->cylinder, M(0.58f, 0.12f, 0.58f, 0, 0.35f, 0), C4(0.85f, 0.6f, 0.22f), 0);
        break;
    }
}

void Game::RenderVillage(bool title)
{
    float rate = title ? 0.08f : rateShown;
    FrameSettings fs;
    if (title)
    {
        float y = time * 0.07f;
        fs.camTarget = F3(0, 2.0f, 0);
        fs.camPos = F3(-std::sin(y) * 26, 9.f, -std::cos(y) * 26);
    }
    else if (vWalkCam)
    {
        fs.camPos = camPosNow;
        fs.camTarget = camTargetNow;
        fs.fovY = 0.95f;
    }
    else
    {
        fs.camTarget = vFocus;
        fs.camPos = Add(vFocus, F3(-std::sin(vYaw) * std::cos(vPitch) * vDist, std::sin(vPitch) * vDist, -std::cos(vYaw) * std::cos(vPitch) * vDist));
    }
    fs.time = time;
    fs.lightDir = F3(-0.45f, -1.f, 0.4f);
    fs.lightColor = Lerp3(F3(0.7f, 0.72f, 0.78f), F3(1.0f, 0.88f, 0.72f), rate);
    fs.ambient = Lerp3(F3(0.58f, 0.6f, 0.66f), F3(0.52f, 0.5f, 0.55f), rate);
    fs.shadowColor = Lerp3(F3(0.82f, 0.86f, 1.0f), F3(0.88f, 0.8f, 0.95f), rate);   // 霧の村は青い影、復興すると夕焼けの紫の影
    fs.fogColor = Lerp3(F3(0.80f, 0.83f, 0.87f), F3(0.95f, 0.82f, 0.68f), rate);
    fs.fogStart = Lerpf(title ? 4.f : 14.f, 45, rate);
    fs.fogEnd = Lerpf(title ? 48.f : 62.f, 180, rate);
    fs.fogMax = Lerpf(title ? 0.97f : 0.9f, 0.6f, rate);
    fs.heightFog = Lerpf(title ? 0.75f : 0.45f, 0.08f, rate);
    fs.clear = fs.fogColor;

    // ライト
    for (auto& p : placed)
    {
        int w, h; PartSize(p.t, p.rot, w, h);
        XMFLOAT3 c = CellCenter(p.x, p.z, w, h);
        XMMATRIX rm = XMMatrixRotationY(p.rot * PI * 0.5f);
        auto lp = [&](float x, float y, float z) {
            XMFLOAT3 o; XMStoreFloat3(&o, XMVector3Transform(XMVectorSet(x, y, z, 1), rm)); return Add(c, o);
        };
        if (p.t == P_Lantern) fs.lights.push_back({ lp(0.45f, 1.8f, 0), 7.f, F3(1.3f, 0.7f, 0.3f) });
        if (p.t == P_Kiln) fs.lights.push_back({ lp(0, 0.8f, -2.2f), 6.f, F3(1.4f, 0.6f, 0.2f) });
        if (p.t == P_Forge) fs.lights.push_back({ lp(0, 1.0f, -2.2f), 6.f, F3(1.3f, 0.6f, 0.2f) });
        if (p.t == P_Shop) fs.lights.push_back({ lp(1.4f, 1.5f, -2.6f), 5.f, F3(1.2f, 0.65f, 0.3f) });
    }
    for (int s = -1; s <= 1; s += 2) fs.lights.push_back({ Add(kEntrancePos, F3(s * 3.f, 1.3f, -5.2f)), 5.f, F3(1.2f, 0.75f, 0.35f) });
    R->BeginFrame(fs);

    // 空と遠景
    {
        DrawParams sky;
        sky.tex = rate >= 0.6f ? T["sky_dusk"] : T["sky_day"];
        sky.unlit = true; sky.cullNone = true; sky.fog = Lerpf(0.85f, 0.25f, rate);
        sky.color = C4(1, 1, 1);
        R->Draw(R->sphere, XMMatrixScaling(420, 420, 420) * XMMatrixTranslation(fs.camPos.x, 0, fs.camPos.z), sky);
        DrawParams mt;
        mt.tex = T["mountains"]; mt.unlit = true; mt.cullNone = true; mt.blend = Blend::Alpha; mt.depthWrite = false;
        mt.uv = { 3, 1, 0, 0 }; mt.fog = Lerpf(0.9f, 0.45f, rate);
        R->Draw(tube, XMMatrixScaling(170, 55, 170) * XMMatrixTranslation(0, -6, 0), mt);
        DrawParams ms = mt; ms.tex = T["mist"]; ms.uv = { 4, 1, time * 0.004f, 0 }; ms.color = C4(1, 1, 1, Lerpf(0.9f, 0.4f, rate));
        R->Draw(tube, XMMatrixScaling(120, 18, 120) * XMMatrixTranslation(0, -2, 0), ms);
    }
    DrawGround(true, rate);

    // 村の外の森
    for (auto& f : forest)
    {
        bool pine = f.w > 0.5f;
        XMMATRIX P = XMMatrixScaling(f.y, f.y, f.y) * XMMatrixTranslation(f.x, 0, f.z);
        DrawParams tr; tr.tex = T["bark"]; tr.outline = 0.02f;
        R->Draw(R->cylinder, XMMatrixScaling(0.45f, 2.6f, 0.45f) * P, tr);
        DrawParams lf; lf.color = pine ? C4(0.25f, 0.45f, 0.3f) : C4(0.4f, 0.58f, 0.3f); lf.outline = 0.03f;
        if (pine)
        {
            R->Draw(R->cone, XMMatrixScaling(2.6f, 2.6f, 2.6f) * XMMatrixTranslation(0, 1.6f, 0) * P, lf);
            R->Draw(R->cone, XMMatrixScaling(2.0f, 2.2f, 2.0f) * XMMatrixTranslation(0, 3.0f, 0) * P, lf);
        }
        else R->Draw(R->sphere, XMMatrixScaling(2.4f, 2.2f, 2.4f) * XMMatrixTranslation(0, 3.2f, 0) * P, lf);
    }

    // 御神木
    {
        DrawParams b; b.tex = T["bark"]; b.outline = 0.025f; b.uv = { 2, 1, 0, 0 };
        if (rate < 0.6f)
        {
            R->Draw(R->cylinder, XMMatrixScaling(2.6f, 1.0f, 2.6f), b);
            DrawParams top; top.color = C4(0.75f, 0.6f, 0.42f);
            R->Draw(R->cylinder, XMMatrixScaling(2.3f, 0.04f, 2.3f) * XMMatrixTranslation(0, 1.0f, 0), top);
        }
        else
        {
            float g = SmoothStep(0.6f, 1.0f, rate) * 0.7f + 0.3f;
            R->Draw(R->cylinder, XMMatrixScaling(2.0f, 7.0f * g, 2.0f), b);
            DrawParams lf; lf.color = C4(0.38f, 0.6f, 0.32f); lf.outline = 0.035f;
            R->Draw(R->sphere, XMMatrixScaling(7 * g, 5.5f * g, 7 * g) * XMMatrixTranslation(0, 7.5f * g, 0), lf);
            lf.color = C4(0.45f, 0.68f, 0.36f);
            R->Draw(R->sphere, XMMatrixScaling(4.5f * g, 3.8f * g, 4.5f * g) * XMMatrixTranslation(2.3f * g, 6.0f * g, -1.4f * g), lf);
            R->Draw(R->sphere, XMMatrixScaling(4.8f * g, 4.0f * g, 4.8f * g) * XMMatrixTranslation(-2.4f * g, 6.4f * g, 1.0f * g), lf);
        }
        DrawParams rp; rp.tex = T["rope"]; rp.color = C4(1, 0.92f, 0.7f); rp.uv = { 6, 1, 0, 0.35f };
        R->Draw(R->cylinder, XMMatrixScaling(rate < 0.6f ? 2.7f : 2.1f, 0.22f, rate < 0.6f ? 2.7f : 2.1f) * XMMatrixTranslation(0, 0.55f, 0), rp);
        DrawShadow(F3(0, 0, 0), 4.5f, 0.35f);
    }

    // 配置した部品
    for (size_t i = 0; i < placed.size(); i++)
    {
        auto& p = placed[i];
        int w, h; PartSize(p.t, p.rot, w, h);
        XMFLOAT3 c = CellCenter(p.x, p.z, w, h);
        if (p.t != P_Path && p.t != P_Field && p.t != P_River && p.t != P_Bridge) DrawShadow(c, (float)std::max(w, h) * 2.4f, 0.3f);
        partMask = (p.t == P_River || p.t == P_Bridge) ? NeighborMask(p) : 0;   // となりの川・橋とつなげる
        DrawPart(p.t, c, p.t == P_River ? 0 : p.rot, Saturate(p.appearT / 0.6f), (float)i * 0.37f + p.x * 0.11f);
        partMask = 0;
        for (int s = 0; s < MAX_SLOT; s++)
            if (p.sub[s] >= 0 && p.sub[s] < S_COUNT && !IsPerson(p.sub[s]))   // 人物は住人として歩いている
                DrawSub(p.sub[s], SlotPos(p, s), p.rot * PI * 0.5f + PI, Saturate(p.appearT / 0.6f));
    }
    for (auto& w : walkers) DrawWalker(w);
    DrawVillageBorder();
    DrawEntrance();

    if (!title && vWalkCam)
    {
        // 町を歩くのは主人公だけ（武器はしまっている）
        DrawActor(party[active]);
        // 初めての探索前は入口に目印
        if (totalDives == 0)
            R->DrawBillboard(T["objective"], Add(kEntrancePos, F3(0, 5.5f + std::sin(time * 3) * 0.2f, -3.6f)), 1.4f, 1.4f, C4(1, 0.9f, 0.5f, 1), Blend::Alpha, { 1, 1, 0, 0 }, 0, false, 0);
    }

    if (!title && !vWalkCam)
    {
        // グリッド
        DrawParams g; g.tex = T["grid_cell"]; g.blend = Blend::Alpha; g.unlit = true; g.depthWrite = false;
        g.color = C4(1, 1, 1, 0.22f); g.uv = { (float)VG, (float)VG, 0, 0 };
        R->Draw(R->plane, XMMatrixScaling(VG * 2.f, 1, VG * 2.f) * XMMatrixTranslation(0, 0.06f, 0), g);   // 川の向こう岸まで

        // カーソルと配置予定
        std::vector<int> pal = PaletteList();
        int selId = pal.empty() ? -1 : pal[std::clamp(palSel, 0, (int)pal.size() - 1)];
        if (moving) selId = movingPart.t;   // 動かしているジオラマ
        if (selId >= SUB_ID)
        {
            // はめ込みパーツ：カーソルの下のジオラマを光らせ、次の空き枠に半透明で見せる
            int idx = PartAtCursor();
            int nSlot = idx >= 0 ? SlotCount(placed[idx].t) : 0;
            int free = -1;
            for (int k = 0; k < nSlot && free < 0; k++) if (placed[idx].sub[k] < 0) free = k;
            if (idx >= 0)
            {
                int w, h; PartSize(placed[idx].t, placed[idx].rot, w, h);
                for (int i = 0; i < w; i++)
                    for (int j = 0; j < h; j++)
                    {
                        XMFLOAT3 cc = CellCenter(placed[idx].x + i, placed[idx].z + j);
                        DrawParams o; o.tex = free >= 0 ? T["grid_ok"] : T["grid_ng"]; o.blend = Blend::Alpha; o.unlit = true; o.depthWrite = false;
                        o.color = C4(1, 1, 1, 0.8f);
                        R->Draw(R->plane, XMMatrixScaling(1.9f, 1, 1.9f) * XMMatrixTranslation(cc.x, 0.09f, cc.z), o);
                    }
                // 枠の位置に目印（空き=白い輪、使用中=なし）
                for (int k = 0; k < nSlot; k++)
                {
                    XMFLOAT3 sp = SlotPos(placed[idx], k);
                    if (placed[idx].sub[k] < 0)
                        R->DrawBillboard(T["glint"], Add(sp, F3(0, 0.4f, 0)), 0.9f, 0.9f, C4(1, 1, 1, k == free ? 0.95f : 0.45f), Blend::Add, { 1, 1, 0, 0 }, time * 2);
                }
                if (free >= 0)
                {
                    tint = F3(0.05f, 0.22f, 0.12f);
                    DrawSub(selId - SUB_ID, Add(SlotPos(placed[idx], free), F3(0, 0.15f + std::sin(time * 3) * 0.1f, 0)), placed[idx].rot * PI * 0.5f + PI, 1.f);
                    tint = F3(0, 0, 0);
                }
            }
            else
            {
                XMFLOAT3 cc = CellCenter(cursorX, cursorZ);
                DrawParams o; o.tex = T["grid_ng"]; o.blend = Blend::Alpha; o.unlit = true; o.depthWrite = false; o.color = C4(1, 1, 1, 0.8f);
                R->Draw(R->plane, XMMatrixScaling(1.9f, 1, 1.9f) * XMMatrixTranslation(cc.x, 0.09f, cc.z), o);
            }
            XMFLOAT3 cc = CellCenter(cursorX, cursorZ);
        }
        else if (moving || (!pal.empty() && selId >= 0 && selId < SUB_ID && inventory[selId] > 0))   // 手持ちが0のときは置く見本を出さない（動かしているときは、そのジオラマを見せる）
        {
            PartType t = (PartType)selId;
            int w, h; PartSize(t, cursorRot, w, h);
            int x = std::clamp(cursorX, 0, VG - w), z = std::clamp(cursorZ, 0, VG - h);
            bool ok = CanPlace(t, x, z, cursorRot);
            for (int i = 0; i < w; i++)
                for (int j = 0; j < h; j++)
                {
                    XMFLOAT3 cc = CellCenter(x + i, z + j);
                    DrawParams o; o.tex = ok ? T["grid_ok"] : T["grid_ng"]; o.blend = Blend::Alpha; o.unlit = true; o.depthWrite = false;
                    o.color = C4(1, 1, 1, 0.8f);
                    R->Draw(R->plane, XMMatrixScaling(1.9f, 1, 1.9f) * XMMatrixTranslation(cc.x, 0.09f, cc.z), o);
                }
            float bobv = 0.25f + std::sin(time * 3) * 0.12f;
            tint = ok ? F3(0.05f, 0.22f, 0.12f) : F3(0.3f, 0.02f, 0.02f);
            DrawPart(t, Add(CellCenter(x, z, w, h), F3(0, bobv, 0)), cursorRot, 1.f, 0.5f);
            tint = F3(0, 0, 0);
        }
        else
        {
            XMFLOAT3 cc = CellCenter(cursorX, cursorZ);
            DrawParams o; o.tex = T["grid_cell"]; o.blend = Blend::Alpha; o.unlit = true; o.depthWrite = false; o.color = C4(1, 0.9f, 0.5f, 0.9f);
            R->Draw(R->plane, XMMatrixScaling(2, 1, 2) * XMMatrixTranslation(cc.x, 0.09f, cc.z), o);
        }
    }
    if (!title) DrawFx();
}

void Game::DrawVillageUI()
{
    float W = (float)R->W, H = (float)R->H;
    float rate = rateShown;
    if (vWalkCam)
    {
        if (!townMenu)   // 村のメニューを開いているあいだは、題字と重ならないように出さない
        {
        // 左上：機織りの村らしく、復興率は「織りかけの反物」で見せる
        //   織れたところは色のついた布、まだのところは縦糸だけ。織っている端に杼（ひ）がいる
        wchar_t b0[64];
        R->FillGradientRect(0, 0, 520, 96, RGBA(0.04f, 0.03f, 0.03f, 0.6f), RGBA(0.04f, 0.03f, 0.03f, 0.f));
        // 朱の判と村の名前
        R->FillRect(20, 16, 44, 44, RGBA(0.75f, 0.2f, 0.16f));
        R->StrokeRect(23, 19, 38, 38, RGBA(1, 0.9f, 0.8f, 0.8f), 1.5f);
        R->Text(L"紡", 20, 20, 44, 40, 27, RGBA(1, 0.96f, 0.9f), 1, true, false, 1);
        R->Text(L"ハタオリ村", 74, 10, 220, 34, 24, RGBA(1, 0.97f, 0.9f), 0, true, true, 1);
        // 織りかけの反物
        {
            float cx0 = 76, cy0 = 46, cw = 200, ch = 14;
            float wv = cw * Saturate(rate);
            R->FillRect(cx0 - 3, cy0 - 3, 3, ch + 6, RGBA(0.42f, 0.27f, 0.16f));            // 織り始めの木の棒
            R->FillRect(cx0, cy0, cw, ch, RGBA(0.05f, 0.04f, 0.04f, 0.45f));
            for (float x = cx0 + 2; x < cx0 + cw; x += 4)                                    // 縦糸
                R->Line(x, cy0, x, cy0 + ch, RGBA(0.95f, 0.9f, 0.8f, 0.45f), 1.f);
            if (wv > 0.5f)
            {
                R->FillRect(cx0, cy0, wv, ch, RGBA(0.22f, 0.32f, 0.62f));                      // 藍の布
                R->FillRect(cx0, cy0 + 4, wv, 2, RGBA(0.75f, 0.22f, 0.2f));                     // 朱と山吹の縞
                R->FillRect(cx0, cy0 + 9, wv, 1.5f, RGBA(0.88f, 0.64f, 0.22f));
                for (float x = cx0 + 3; x < cx0 + wv; x += 6)                                  // 織り目
                    R->Line(x, cy0, x, cy0 + ch, RGBA(1, 1, 1, 0.08f), 1.f);
                R->FillRect(cx0, cy0, wv, 2, RGBA(1, 1, 1, 0.18f));
            }
            // 杼：織っている端で小さくゆれる
            float sx = cx0 + wv, sy = cy0 + ch * 0.5f + std::sin(time * 3.f) * 1.5f;
            D2D1_POINT_2F sh[4] = { { sx - 9, sy }, { sx, sy - 5 }, { sx + 9, sy }, { sx, sy + 5 } };
            R->FillPoly(sh, 4, RGBA(0.62f, 0.42f, 0.24f));
            R->StrokePoly(sh, 4, RGBA(0.25f, 0.15f, 0.09f), 1.f);
            swprintf_s(b0, L"復興 %d%%", (int)std::round(rate * 100));
            R->Text(b0, cx0 + cw + 12, cy0 - 7, 100, 26, 16, RGBA(1, 0.92f, 0.78f), 0, true, true, 1);
        }
        // お金：穴あき銭
        {
            float mx = 88, my = 76;
            R->FillCircle(mx, my, 9, RGBA(0.85f, 0.65f, 0.25f));
            R->StrokeCircle(mx, my, 9, RGBA(0.45f, 0.3f, 0.1f), 1.2f);
            R->FillRect(mx - 3, my - 3, 6, 6, RGBA(0.2f, 0.14f, 0.08f));
            swprintf_s(b0, L"%d 文", money);
            R->Text(b0, mx + 16, my - 13, 140, 26, 17, RGBA(1, 0.92f, 0.78f), 0, true, true, 1);
        }
        // 仲間の顔（操作しているキャラは朱の輪）
        float fx0 = 400;
        for (int i = 0; i < 3; i++)
        {
            if (!joined[i]) continue;
            R->FillCircle(fx0, 38, 20, RGBA(0.97f, 0.94f, 0.87f));
            DrawFaceIcon(i, fx0, 38, 17);
            R->StrokeCircle(fx0, 38, 20, i == active ? RGBA(0.78f, 0.22f, 0.17f) : RGBA(1, 1, 1, 0.75f), i == active ? 3.f : 2.f);
            fx0 += 46;
        }
        }
        if (!talkHint.empty() && dialog.empty() && !transOn && !floorMenu)
        {
            DrawInkHint(talkHint, W / 2, H - 120, 380);
        }
        if (floorMenu && dialog.empty() && !transOn) DrawFloorMenu();
        return;
    }

    // カーソルが指しているジオラマの名前（と、はめ込み枠の中身）
    {
        int idx = PartAtCursor();
        XMFLOAT3 anchor;
        std::wstring name, slots;
        if (idx >= 0)
        {
            const Placed& p = placed[idx];
            int w, h; PartSize(p.t, p.rot, w, h);
            anchor = Add(CellCenter(p.x, p.z, w, h), F3(0, p.t == P_Path || p.t == P_Field ? 1.0f : 4.4f, 0));
            name = kParts[p.t].name;
            int n = SlotCount(p.t);
            for (int k = 0; k < n; k++)
            {
                if (!slots.empty()) slots += L"　";
                slots += L"[" + std::wstring(p.sub[k] >= 0 ? kSubs[p.sub[k]].name : L"空き") + L"]";
            }
        }
        else if (cursorX >= 9 && cursorX <= 10 && cursorZ >= 9 && cursorZ <= 10) { anchor = F3(0, 3.5f, 0); name = L"御神木"; }
        else if (IsRiverCol(cursorX)) { anchor = Add(CellCenter(cursorX, cursorZ), F3(0, 1.0f, 0)); name = L"川"; }
        float sx, sy;
        if (!name.empty() && R->Project(anchor, sx, sy))
        {
            float tw = std::max(150.f, 28.f + name.size() * 20.f), th = 36;
            float sw = slots.empty() ? 0.f : std::max(tw, 30.f + slots.size() * 15.f);
            float bwid = std::max(tw, sw);
            InkPanel(sx - bwid / 2, sy - th - 6, bwid, slots.empty() ? th : th + 26, 0.85f);
            R->Text(name, sx - bwid / 2, sy - th, bwid, 28, 18, RGBA(1, 0.92f, 0.75f), 1, true);
            if (!slots.empty()) R->Text(L"はめ込み " + slots, sx - bwid / 2, sy - 10, bwid, 22, 13, RGBA(0.8f, 0.95f, 1), 1);
        }
    }
    DrawGeoMenu();   // 配置／組み立てメニュー（GeoMenu.cpp）
}

// 配置モード → 町歩き：カーソルで指していた場所へ移動する（建物の上なら、いちばん近い空き地へ）
void Game::WalkFromCursor()
{
    // 町歩きに戻る：カーソルで指していた場所へ移動する（建物の上なら、いちばん近い空き地へ）
    XMFLOAT3 goal = CellCenter(cursorX, cursorZ);
    bool found = !VillageSolid(goal.x, goal.z);
    for (int r = 1; r <= 4 && !found; r++)
        for (int dz = -r; dz <= r && !found; dz++)
            for (int dx = -r; dx <= r && !found; dx++)
            {
                if (std::max(std::abs(dx), std::abs(dz)) != r) continue;
                XMFLOAT3 c = CellCenter(cursorX + dx, cursorZ + dz);
                if (!VillageSolid(c.x, c.z)) { goal = c; found = true; }
            }
    vWalkCam = true;
    if (found)
    {
        Actor& A = party[active];
        A.pos = goal; A.st = ASt::Normal; A.speedNow = 0; A.kb = {};
        A.yaw = vYaw;
        int slot = 0;
        for (int i = 0; i < 3; i++)
        {
            if (i == active || !joined[i]) continue;
            Actor& B = party[i];
            B.pos = Add(goal, Mul(Fwd(A.yaw + (slot == 0 ? 2.4f : -2.4f)), 1.4f));
            if (VillageSolid(B.pos.x, B.pos.z)) B.pos = goal;
            B.yaw = A.yaw; B.st = ASt::Normal; B.speedNow = 0;
            slot++;
        }
        camYaw = A.yaw;
        camPosNow = Add(goal, F3(-std::sin(camYaw) * 6.f, 4.5f, -std::cos(camYaw) * 6.f));
        camTargetNow = Add(goal, F3(0, 1.3f, 0));
        SpawnFx(3, Add(goal, F3(0, 0.3f, 0)), 2.2f, C4(1.f, 0.95f, 0.85f, 0.8f), 0.5f);
        ToastMsg(L"カーソルの場所へ移動した", RGBA(1, 0.92f, 0.75f));
    }
    else camYaw = party[active].yaw;
}
