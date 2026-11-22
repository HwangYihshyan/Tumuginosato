#include "Game.h"

// ------------------------------------------------------------------
// データ（部品と住人の要望）
// ------------------------------------------------------------------
const PartDef kParts[P_COUNT] = {
    { L"トウジの家",     2, 2, 8,  L"村長・フクロウ" },
    { L"クロベエの家",   2, 2, 8,  L"炭焼き・アナグマ" },
    { L"ウサギ姉妹の家", 2, 2, 8,  L"畑仕事・ウサギ" },
    { L"里人の家",       2, 2, 6,  L"人間の家族" },
    { L"機織り小屋",     3, 2, 10, L"ツムギが武器を紡ぐ" },
    { L"鍛冶屋",         2, 2, 8,  L"武器の修理" },
    { L"炭焼き窯",       2, 2, 8,  L"クロベエの仕事場" },
    { L"畑",             2, 2, 4,  L"野菜が育つ" },
    { L"木",             1, 1, 3,  L"自然物" },
    { L"提灯",           1, 1, 2,  L"夜道を照らす" },
    { L"道",             1, 1, 1,  L"自然物" },
    { L"布干し",         2, 1, 3,  L"藍・茜・黄土の反物" },
    { L"井戸",           1, 1, 4,  L"施設" },
    { L"水車",           1, 2, 6,  L"川辺に置く" },
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
static const float MAX_POINTS = 220.f;

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

bool Game::CanPlace(PartType t, int x, int z, int rot, int ignore) const
{
    int w, h; PartSize(t, rot, w, h);
    if (x < 0 || z < 0 || x + w > RIVER_X || z + h > VG) return false;
    if (Overlap(x, z, w, h, 9, 9, 2, 2)) return false;
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
            if (RIVER_X - (p.x + w) < q.dist) return true;
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

float Game::RestoreRate() const
{
    float pts = 0;
    for (auto& p : placed) pts += (float)kParts[p.t].pts;
    for (int r = 0; r < kReqCount; r++) if (ReqSatisfied(r)) pts += (float)kReqs[r].pts;
    return std::min(1.f, pts / MAX_POINTS);
}

std::vector<int> Game::PaletteList() const
{
    std::vector<int> v;
    for (int i = 0; i < P_COUNT; i++) if (inventory[i] > 0) v.push_back(i);
    return v;
}

void Game::RebuildWalkers()
{
    walkers.clear();
    const XMFLOAT4 kimono[] = { C4(0.2f, 0.3f, 0.6f), C4(0.72f, 0.22f, 0.2f), C4(0.85f, 0.62f, 0.22f), C4(0.3f, 0.5f, 0.35f) };
    auto add = [&](int kind, const Placed& p, XMFLOAT4 col) {
        int w, h; PartSize(p.t, p.rot, w, h);
        XMFLOAT3 c = CellCenter(p.x, p.z, w, h);
        Walker wk{ kind, Add(c, F3(RandF(-1, 1), 0, -h - 0.6f)), {}, 0, RandF(0, 2), RandF(0, 6), col };
        wk.target = wk.pos;
        walkers.push_back(wk);
    };
    for (auto& p : placed)
    {
        switch (p.t)
        {
        case P_HouseTouji: add(1, p, C4(0.55f, 0.38f, 0.22f)); break;
        case P_HouseKurobee: add(2, p, C4(1, 1, 1)); break;
        case P_HouseRabbit: add(3, p, C4(1, 1, 1)); add(3, p, C4(1, 0.9f, 0.9f)); break;
        case P_HouseVillager: add(0, p, kimono[RandI(0, 3)]); add(0, p, kimono[RandI(0, 3)]); break;
        case P_Loom: add(0, p, kimono[0]); break;
        case P_Forge: add(0, p, kimono[3]); break;
        default: break;
        }
    }
}

// ------------------------------------------------------------------
void Game::EnterVillage(bool fromDungeon)
{
    scene = SceneId::Village;
    sceneT = 0;
    autopilot = false;
    vWalkCam = false;
    if (fromDungeon)
    {
        if (crystalsThisRun > 0)
        {
            ToastMsg(L"記憶の結晶を " + std::to_wstring(crystalsThisRun) + L" 個 持ち帰った", RGBA(0.6f, 0.95f, 1));
            Say(L"ツムギ", L"おかえり！ 持ち帰った結晶を紡いで、村に戻してあげよう。");
        }
        crystalsThisRun = 0;
    }
    else if (placed.empty() && totalDives == 0)
    {
        Say(L"ツムギ", L"ソラ、気がついた？ ここはハタオリ村…霧に呑まれて、何もかも消えちゃった。");
        Say(L"クロベエ", L"坊、しっかりしな。村のみんなは「記憶の結晶」になって、地の底の洞に散らばってるらしい。");
        Say(L"ツムギ", L"結晶を持ち帰れば、わたしが紡ぎなおしてあげる！ まずはトウジさんの家を置いてみて。");
    }
    for (auto& a : party) { a.hp = a.maxHp; a.st = ASt::Normal; }
    lantern = 100;
    rateShown = RestoreRate();
    RebuildWalkers();
}

void Game::UpdateVillage(float dt, InputState& in)
{
    rateShown = Approach(rateShown, RestoreRate(), dt * 0.25f);
    for (auto& p : placed) p.appearT += dt;

    // 住人の散歩
    for (auto& w : walkers)
    {
        w.phase += dt;
        if (w.waitT > 0) { w.waitT -= dt; continue; }
        XMFLOAT3 d = Sub(w.target, w.pos);
        float l = LenXZ(d);
        if (l < 0.2f)
        {
            w.waitT = RandF(1.0f, 3.5f);
            for (int tries = 0; tries < 10; tries++)
            {
                int cx = RandI(0, RIVER_X - 1), cz = RandI(0, VG - 1);
                bool free = true;
                for (auto& p : placed)
                {
                    int pw, ph; PartSize(p.t, p.rot, pw, ph);
                    if (p.t != P_Path && Overlap(cx, cz, 1, 1, p.x, p.z, pw, ph)) { free = false; break; }
                }
                if (free && !Overlap(cx, cz, 1, 1, 9, 9, 2, 2)) { w.target = CellCenter(cx, cz); break; }
            }
        }
        else
        {
            XMFLOAT3 n = Mul(d, 1.f / l);
            w.pos = Add(w.pos, Mul(n, std::min(l, 1.3f * dt)));
            w.yaw = ApproachAngle(w.yaw, YawOf(n), dt * 6);
        }
    }

    if (!dialog.empty()) return;

    // カメラ
    if (in.Down('Q')) vYaw += dt * 1.5f;
    if (in.Down('E')) vYaw -= dt * 1.5f;
    vDist = Clampf(vDist - in.wheel * 0.02f, 18, 52);
    if (in.Pressed(VK_TAB)) vWalkCam = !vWalkCam;

    // カーソル移動（カメラの向きに合わせる）
    int dx = 0, dz = 0;
    if (in.Pressed('W') || in.Pressed(VK_UP)) dz = 1;
    if (in.Pressed('S') || in.Pressed(VK_DOWN)) dz = -1;
    if (in.Pressed('D') || in.Pressed(VK_RIGHT)) dx = 1;
    if (in.Pressed('A') || in.Pressed(VK_LEFT)) dx = -1;
    if (dx || dz)
    {
        int q = ((int)std::lround(vYaw / (PI / 2)) % 4 + 4) % 4;
        for (int i = 0; i < q; i++) { int t = dx; dx = dz; dz = -t; }
        cursorX = std::clamp(cursorX + dx, 0, VG - 1);
        cursorZ = std::clamp(cursorZ + dz, 0, VG - 1);
    }
    float W = (float)R->W, H = (float)R->H;
    std::vector<int> pal = PaletteList();
    float cardW = 118, cardH = 84, gap = 10;
    float totalW = pal.size() * (cardW + gap) - gap;
    float px0 = (W - totalW) / 2, py0 = H - cardH - 46;
    bool overPalette = in.mouseY > py0 - 10 && in.mouseY < py0 + cardH + 10;
    if (in.mouseMoved && !overPalette && !vWalkCam)
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

    // パレット
    if (!pal.empty())
    {
        palSel = std::clamp(palSel, 0, (int)pal.size() - 1);
        if (in.Pressed('Z')) palSel = (palSel + (int)pal.size() - 1) % (int)pal.size();
        if (in.Pressed('C')) palSel = (palSel + 1) % (int)pal.size();
        for (int i = 0; i < 9 && i < (int)pal.size(); i++) if (in.Pressed('1' + i)) palSel = i;
        if (in.LPressed() && overPalette)
        {
            for (int i = 0; i < (int)pal.size(); i++)
            {
                float x = px0 + i * (cardW + gap);
                if (in.mouseX >= x && in.mouseX <= x + cardW) palSel = i;
            }
        }
    }
    if (in.Pressed('R')) cursorRot = (cursorRot + 1) % 4;

    // 配置
    if (!pal.empty() && ((in.LPressed() && !overPalette) || in.Pressed(VK_RETURN)))
    {
        PartType t = (PartType)pal[palSel];
        int w, h; PartSize(t, cursorRot, w, h);
        int x = std::clamp(cursorX, 0, VG - w), z = std::clamp(cursorZ, 0, VG - h);
        if (CanPlace(t, x, z, cursorRot))
        {
            bool before[16]{};
            for (int r = 0; r < kReqCount; r++) before[r] = ReqSatisfied(r);
            placed.push_back({ t, x, z, cursorRot, 0.f });
            inventory[t]--;
            ToastMsg(std::wstring(kParts[t].name) + L" を紡ぎなおした");
            for (int r = 0; r < kReqCount; r++)
                if (!before[r] && ReqSatisfied(r))
                    ToastMsg(L"要望達成！ " + std::wstring(kReqs[r].who) + L"「" + kReqs[r].text + L"」", RGBA(1, 0.85f, 0.4f));
            if (t == P_HouseTouji && totalDives == 0)
            {
                Say(L"ツムギ", L"トウジさんが戻ってきた！ 要望をかなえると「復興率」が上がって、村の霧が晴れていくよ。");
                Say(L"ツムギ", L"次は根の洞へ潜って、記憶の結晶を探そう。準備ができたら F キー！");
            }
            RebuildWalkers();
        }
        else ToastMsg(L"そこには置けない", RGBA(1, 0.5f, 0.45f));
    }
    // 撤去
    if (in.RPressed() || in.Pressed('X'))
    {
        for (int i = 0; i < (int)placed.size(); i++)
        {
            int pw, ph; PartSize(placed[i].t, placed[i].rot, pw, ph);
            if (Overlap(cursorX, cursorZ, 1, 1, placed[i].x, placed[i].z, pw, ph))
            {
                inventory[placed[i].t]++;
                ToastMsg(std::wstring(kParts[placed[i].t].name) + L" を片付けた");
                placed.erase(placed.begin() + i);
                RebuildWalkers();
                break;
            }
        }
    }
    // 根の洞へ
    if (in.Pressed('F'))
    {
        int start = (knot && RestoreRate() >= 0.3f) ? 6 : 1;
        EnterDungeon(start, (uint32_t)(GetTickCount() ^ (totalDives * 7919)));
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
    R->Draw(R->plane, XMMatrixScaling(size, 1, size) * XMMatrixTranslation(pos.x, pos.y + 0.03f, pos.z), p);
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
    R->Draw(R->cube, XMMatrixScaling(0.5f, 0.3f, 400) * XMMatrixTranslation(rx0, 0.1f, 0), b);
    R->Draw(R->cube, XMMatrixScaling(0.5f, 0.3f, 400) * XMMatrixTranslation(rx1, 0.1f, 0), b);
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
        XMFLOAT4 leaf = maple ? C4(0.92f, 0.45f, 0.18f) : C4(0.36f, 0.6f, 0.3f);
        XMFLOAT4 leaf2 = maple ? C4(0.98f, 0.62f, 0.2f) : C4(0.45f, 0.7f, 0.36f);
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
    default: break;
    }
}

void Game::DrawWalker(const Walker& w)
{
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
        fs.camPos = F3(-3.5f, 2.3f, -19.f);
        fs.camTarget = F3(1.5f, 1.8f, 0.f);
        fs.fovY = 0.9f;
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
    }
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
        if (p.t != P_Path && p.t != P_Field) DrawShadow(c, (float)std::max(w, h) * 2.4f, 0.3f);
        DrawPart(p.t, c, p.rot, Saturate(p.appearT / 0.6f), (float)i * 0.37f + p.x * 0.11f);
    }
    for (auto& w : walkers) DrawWalker(w);

    if (!title && !vWalkCam)
    {
        // グリッド
        DrawParams g; g.tex = T["grid_cell"]; g.blend = Blend::Alpha; g.unlit = true; g.depthWrite = false;
        g.color = C4(1, 1, 1, 0.22f); g.uv = { (float)RIVER_X, (float)VG, 0, 0 };
        R->Draw(R->plane, XMMatrixScaling(RIVER_X * 2.f, 1, VG * 2.f) * XMMatrixTranslation((RIVER_X * 0.5f - VG * 0.5f) * 2.f, 0.06f, 0), g);

        // カーソルと配置予定
        std::vector<int> pal = PaletteList();
        if (!pal.empty())
        {
            PartType t = (PartType)pal[std::clamp(palSel, 0, (int)pal.size() - 1)];
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
            DrawTsumugi(Add(CellCenter(x, z, w, h), F3(-1.6f, 4.2f + std::sin(time * 2) * 0.2f, -1.2f)));
        }
        else
        {
            XMFLOAT3 cc = CellCenter(cursorX, cursorZ);
            DrawParams o; o.tex = T["grid_cell"]; o.blend = Blend::Alpha; o.unlit = true; o.depthWrite = false; o.color = C4(1, 0.9f, 0.5f, 0.9f);
            R->Draw(R->plane, XMMatrixScaling(2, 1, 2) * XMMatrixTranslation(cc.x, 0.09f, cc.z), o);
            DrawTsumugi(Add(cc, F3(-1.4f, 3.4f + std::sin(time * 2) * 0.2f, -1.0f)));
        }
    }
}

void Game::DrawVillageUI()
{
    float W = (float)R->W, H = (float)R->H;
    float rate = rateShown;
    if (vWalkCam)
    {
        wchar_t b0[64]; swprintf_s(b0, L"ハタオリ村　復興率 %d%%", (int)std::round(rate * 100));
        R->FillRound(18, 16, 300, 44, 12, RGBA(0.06f, 0.08f, 0.15f, 0.7f));
        R->Text(b0, 18, 25, 300, 30, 19, RGBA(1, 0.92f, 0.75f), 1, true);
        R->FillRect(0, H - 32, W, 32, RGBA(0.04f, 0.05f, 0.1f, 0.6f));
        R->Text(L"町歩き視点　Tab：見下ろし視点に戻る", 0, H - 28, W, 24, 14, RGBA(1, 1, 1, 0.85f), 1, false, false);
        return;
    }
    // 左上：復興率
    R->FillRound(18, 16, 380, 128, 12, RGBA(0.06f, 0.08f, 0.15f, 0.78f));
    R->Text(L"第1章　ハタオリ村", 34, 24, 300, 30, 22, RGBA(1, 0.92f, 0.75f), 0, true);
    R->Text(L"復興率", 34, 60, 100, 30, 17, RGBA(1, 1, 1, 0.85f));
    wchar_t buf[64];
    swprintf_s(buf, L"%d%%", (int)std::round(rate * 100));
    R->Text(buf, 280, 50, 100, 40, 32, RGBA(1, 0.8f, 0.4f), 2, true);
    float bx = 34, by = 96, bw = 348;
    Bar(bx, by, bw, 14, rate, RGBA(0.9f, 0.6f, 0.22f), RGBA(0.2f, 0.2f, 0.25f, 0.9f));
    for (float m : { 0.3f, 0.6f })
    {
        float x = bx + bw * m;
        R->Line(x, by - 4, x, by + 18, RGBA(1, 1, 1, 0.9f), 2);
    }
    R->Text(L"6階", bx + bw * 0.3f - 20, by + 18, 40, 20, 12, RGBA(1, 1, 1, 0.8f), 1);
    R->Text(L"10階", bx + bw * 0.6f - 20, by + 18, 40, 20, 12, RGBA(1, 1, 1, 0.8f), 1);

    // 右：住人の要望
    float rx = W - 372, ry = 16, rw = 354;
    float rh = 54 + kReqCount * 46.f;
    R->FillRound(rx, ry, rw, rh, 12, RGBA(0.06f, 0.08f, 0.15f, 0.78f));
    R->Text(L"住人の要望", rx + 16, ry + 10, 200, 30, 19, RGBA(1, 0.92f, 0.75f), 0, true);
    for (int r = 0; r < kReqCount; r++)
    {
        float y = ry + 46 + r * 46.f;
        bool present = false;
        for (auto& p : placed) if (p.t == kReqs[r].owner) present = true;
        bool ok = ReqSatisfied(r);
        D2D1_COLOR_F c = present ? RGBA(1, 1, 1) : RGBA(1, 1, 1, 0.35f);
        if (ok)
        {
            R->FillCircle(rx + 28, y + 16, 11, RGBA(0.35f, 0.7f, 0.35f));
            R->Text(L"✓", rx + 18, y + 3, 20, 24, 16, RGBA(1, 1, 1), 1, true, false);
        }
        else R->StrokeCircle(rx + 28, y + 16, 10, c, 2);
        R->Text(present ? std::wstring(kReqs[r].who) : std::wstring(L"？？？"), rx + 48, y, 300, 22, 14, present ? RGBA(1, 0.8f, 0.45f) : c, 0, true);
        R->Text(present ? std::wstring(kReqs[r].text) : std::wstring(L"（まだ村に戻っていない）"), rx + 48, y + 19, 300, 22, 15, c);
    }

    // 下：部品パレット
    std::vector<int> pal = PaletteList();
    float cardW = 118, cardH = 84, gap = 10;
    float totalW = pal.size() * (cardW + gap) - gap;
    float px0 = (W - totalW) / 2, py0 = H - cardH - 46;
    if (pal.empty())
    {
        R->FillRound(W / 2 - 300, py0 + 10, 600, 56, 12, RGBA(0.06f, 0.08f, 0.15f, 0.8f));
        R->Text(L"手持ちの結晶がない。F キーで「根の洞」へ潜ろう", W / 2 - 300, py0 + 24, 600, 30, 19, RGBA(1, 0.9f, 0.7f), 1, true);
    }
    for (int i = 0; i < (int)pal.size(); i++)
    {
        float x = px0 + i * (cardW + gap);
        bool sel = i == palSel;
        R->FillRound(x, py0 - (sel ? 8.f : 0.f), cardW, cardH, 10, sel ? RGBA(0.98f, 0.94f, 0.85f, 0.95f) : RGBA(0.06f, 0.08f, 0.15f, 0.8f));
        if (sel) R->StrokeRound(x, py0 - 8, cardW, cardH, 10, RGBA(0.9f, 0.6f, 0.22f), 3);
        const PartDef& d = kParts[pal[i]];
        D2D1_COLOR_F tc = sel ? RGBA(0.12f, 0.15f, 0.25f) : RGBA(1, 1, 1);
        float yo = sel ? -8.f : 0.f;
        R->Text(d.name, x + 4, py0 + 8 + yo, cardW - 8, 44, 15, tc, 1, true, !sel);
        wchar_t b2[32]; swprintf_s(b2, L"%d×%d", d.w, d.h);
        R->Text(b2, x + 8, py0 + 54 + yo, 60, 22, 13, sel ? RGBA(0.3f, 0.35f, 0.45f) : RGBA(1, 1, 1, 0.7f), 0, false, !sel);
        swprintf_s(b2, L"×%d", inventory[pal[i]]);
        R->Text(b2, x + cardW - 58, py0 + 50 + yo, 50, 26, 18, sel ? RGBA(0.85f, 0.45f, 0.1f) : RGBA(1, 0.8f, 0.4f), 2, true, !sel);
    }
    R->FillRect(0, H - 32, W, 32, RGBA(0.04f, 0.05f, 0.1f, 0.7f));
    R->Text(L"マウス/WASD：カーソル　左クリック/Enter：紡ぐ（配置）　R：回転　右クリック/X：片付け　Z/C：部品選択　Q/E：視点　Tab：町歩き視点　F：根の洞へ潜る",
            0, H - 28, W, 24, 14, RGBA(1, 1, 1, 0.85f), 1, false, false);
}
