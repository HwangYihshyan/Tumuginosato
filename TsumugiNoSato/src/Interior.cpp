// ジオラマの建物の中（3パターン：0=民家 1=工房 2=商店）と商人の店
#include "Game.h"

namespace
{
    XMMATRIX S(float x, float y, float z) { return XMMatrixScaling(x, y, z); }
    XMMATRIX Tr(float x, float y, float z) { return XMMatrixTranslation(x, y, z); }
    const wchar_t* kPatternName[3] = { L"民家", L"工房", L"商店" };
    struct ShopItem { const wchar_t* name; const wchar_t* desc; int price; int item; int att; };
    constexpr int SHOP_N = 7;
    const ShopItem kShop[SHOP_N] = {
        { L"薬草",   L"HPを40回復",           30, 0, -1 },
        { L"灯油",   L"灯を50回復",           40, 1, -1 },
        { L"研ぎ粉", L"武器の耐久を30%回復",  60, 2, -1 },
        { L"帰り糸", L"ダンジョンからすぐ戻る", 80, 3, -1 },
        { L"繕い糸", L"壊れた最初の武器を直す", 90, 4, -1 },
        { L"硬い石", L"アタッチメント：攻撃+15%", 120, -1, AT_STONE },
        { L"軽い羽", L"アタッチメント：移動+12%", 150, -1, AT_FEATHER },
    };
}

// 建物の入口（外側の立ち位置）と、中の部屋のパターン
bool Game::DoorOf(const Placed& p, XMFLOAT3& outside, int& pattern) const
{
    XMFLOAT3 local;
    switch (p.t)
    {
    // 建物の壁は中心から2m。キャラの半径（0.4m）より十分外に立ち位置を置く
    case P_HouseTouji: case P_HouseKurobee: case P_HouseRabbit: case P_HouseVillager: local = F3(0.6f, 0, -2.9f); pattern = 0; break;
    case P_Shop: local = F3(0.6f, 0, -2.9f); pattern = 2; break;
    case P_Loom: local = F3(-1.6f, 0, -2.9f); pattern = 1; break;
    case P_Forge: local = F3(0.f, 0, -2.9f); pattern = 1; break;
    default: return false;
    }
    int w, h; PartSize(p.t, p.rot, w, h);
    XMFLOAT3 c = CellCenter(p.x, p.z, w, h);
    XMFLOAT3 o; XMStoreFloat3(&o, XMVector3Transform(XMLoadFloat3(&local), XMMatrixRotationY(p.rot * PI * 0.5f)));
    outside = Add(c, o);
    return true;
}

void Game::EnterInterior(int partIdx)
{
    const Placed& p = placed[partIdx];
    XMFLOAT3 door; int pat;
    if (!DoorOf(p, door, pat)) return;
    std::wstring name = kParts[p.t].name;
    StartTransition(name, kPatternName[pat], [this, partIdx, door, pat] {
        const Placed& pp = placed[partIdx];
        roomPattern = pat; roomPart = partIdx;
        roomReturn = door;
        roomReturnYaw = WrapAngle(pp.rot * PI * 0.5f + PI);
        scene = SceneId::Interior;
        shopOpen = false;
        Actor& A = party[active];
        A.pos = F3(0, 0, -2.2f); A.yaw = 0; A.st = ASt::Normal; A.speedNow = 0; A.kb = {};
        camYaw = 0;
        roomSolids.clear();
        roomNpc = Walker{};
        roomNpc.phase = 0;
        if (pat == 0)
        {
            roomSolids.push_back({ -0.95f, -0.45f, 0.95f, 1.45f });   // 囲炉裏
            roomSolids.push_back({ -3.8f, 1.9f, -2.2f, 3.f });        // たんす
            roomNpc.pos = F3(1.7f, 0, 1.1f);
            roomNpc.kind = pp.t == P_HouseTouji ? 1 : pp.t == P_HouseKurobee ? 2 : pp.t == P_HouseRabbit ? 3 : 0;
            roomNpc.color = C4(0.72f, 0.22f, 0.2f);
        }
        else if (pat == 1)
        {
            roomSolids.push_back({ -1.8f, 1.2f, 1.8f, 3.f });         // 機（または炉）
            roomSolids.push_back({ -3.8f, 0.6f, -2.7f, 3.f });        // 糸巻きの棚
            roomSolids.push_back({ 2.1f, -0.2f, 3.6f, 1.0f });        // 作業台
            roomNpc.pos = F3(1.0f, 0, 0.6f);
            roomNpc.kind = 0; roomNpc.color = C4(0.2f, 0.3f, 0.6f);
        }
        else
        {
            roomSolids.push_back({ -2.7f, 0.55f, 2.7f, 1.25f });      // 帳場（カウンター）
            roomSolids.push_back({ -3.8f, 2.1f, 3.8f, 3.f });         // 棚
            roomNpc.pos = F3(0, 0, 1.75f);
            roomNpc.kind = 4;
        }
        roomNpc.yaw = PI;
        camPosNow = F3(0, 6.2f, -9.0f); camTargetNow = F3(0, 0.8f, 0.6f);
    }, true);
}

void Game::LeaveInterior()
{
    StartTransition(L"ハタオリ村", L"", [this] {
        scene = SceneId::Village;
        vWalkCam = true;
        shopOpen = false;
        Actor& A = party[active];
        A.pos = roomReturn; A.yaw = roomReturnYaw; A.st = ASt::Normal; A.speedNow = 0;
        int slot = 0;
        for (int i = 0; i < 3; i++)
            if (i != active && joined[i]) { party[i].pos = Add(roomReturn, F3(slot == 0 ? 1.4f : -1.4f, 0, 0)); if (Solid(party[i].pos.x, party[i].pos.z)) party[i].pos = roomReturn; slot++; }
        camYaw = A.yaw;
        camPosNow = Add(A.pos, F3(-std::sin(camYaw) * 6.f, 4.5f, -std::cos(camYaw) * 6.f));
        camTargetNow = Add(A.pos, F3(0, 1.3f, 0));
    }, true);
}

void Game::UpdateInterior(float dt, InputState& in)
{
    TickFx(dt);
    roomNpc.phase += dt;
    Controls c = autoshot ? Controls{} : ReadControls(in, dt);
    if (shopOpen)
    {
        if (c.up) shopSel = (shopSel + SHOP_N - 1) % SHOP_N;
        if (c.down) shopSel = (shopSel + 1) % SHOP_N;
        if (c.confirm)
        {
            const ShopItem& s = kShop[shopSel];
            if (money < s.price) ToastMsg(L"お金が足りない…", RGBA(1, 0.6f, 0.5f));
            else
            {
                money -= s.price;
                if (s.item >= 0) items[s.item]++; else attInv[s.att]++;
                ToastMsg(std::wstring(s.name) + L" を買った", RGBA(1, 0.92f, 0.6f));
            }
        }
        if (c.back || c.menu || in.Pressed(VK_TAB)) shopOpen = false;
        return;
    }
    bool talking = !dialog.empty();
    c.atkPressed = c.atkDown = false; c.guard = false; c.dodge = false; c.swap = false;
    if (talking || transOn) { c.mx = c.mz = 0; c.interact = false; }
    UpdateActor(party[active], &c, dt, true);
    Actor& A = party[active];
    talkHint.clear();
    if (!talking && !transOn)
    {
        if (A.pos.z < -2.0f && std::fabs(A.pos.x) < 1.3f)
        {
            talkHint = L"[E] 外に出る";
            if (c.interact || A.pos.z < -2.9f) { LeaveInterior(); return; }
        }
        else if (DistXZ(A.pos, roomNpc.pos) < (roomNpc.kind == 4 ? 2.2f : 1.8f))
        {
            // 機能のあるジオラマ（商店・鍛冶屋・機織り小屋）は、はめ込みを全部終えて完成すると使える
            PartType pt = roomPart >= 0 && roomPart < (int)placed.size() ? placed[roomPart].t : P_COUNT;
            bool done = pt != P_COUNT && IsComplete(placed[roomPart]);
            int filled = 0, nSlot = pt != P_COUNT ? SlotCount(pt) : 0;
            for (int s = 0; s < nSlot; s++) if (placed[roomPart].sub[s] >= 0) filled++;
            wchar_t prog[48]; swprintf_s(prog, L"（はめ込み %d/%d）", filled, nSlot);
            bool func = pt == P_Shop || pt == P_Forge || pt == P_Loom;
            talkHint = !func ? L"[E] 話す：" + WalkerName(roomNpc)
                     : !done ? L"[E] 話す（まだ準備中" + std::wstring(prog) + L"）"
                     : pt == P_Shop ? L"[E] 買い物をする" : pt == P_Forge ? L"[E] 武器を直してもらう（40文）" : L"[E] ほつれ糸3本でアタッチメントを紡ぐ";
            if (c.interact)
            {
                if (func && !done)
                {
                    std::wstring who = pt == P_Shop ? L"タヌキの商人" : pt == P_Forge ? L"鍛冶屋の親方" : L"機織りの娘";
                    Say(who, L"すまんね、まだ支度ができてないんだ。この建物のはめ込みを全部そろえておくれ" + std::wstring(prog) + L"。");
                }
                else if (pt == P_Shop) { shopOpen = true; shopSel = 0; }
                else if (pt == P_Forge)
                {
                    // 鍛冶屋：全員の武器をまとめて直す（壊れた最初の武器も）
                    if (money < 40) Say(L"鍛冶屋の親方", L"修理は40文だ。お金をためてきな。");
                    else
                    {
                        money -= 40;
                        for (auto& w : weapons) { w.dur = (float)kWeapons[w.def].maxDur; w.broken = false; }
                        for (int i = 0; i < 3; i++) { party[i].dur = party[i].maxDur; party[i].broken = false; }
                        Say(L"鍛冶屋の親方", L"よし、全部叩き直した。刃こぼれひとつないぜ。");
                        ToastMsg(L"武器をすべて直した（-40文）", RGBA(0.7f, 1, 0.75f));
                    }
                }
                else if (pt == P_Loom)
                {
                    // 機織り小屋：ほつれ糸3本 → アタッチメント1つ
                    if (threads < 3) Say(L"機織りの娘", L"ほつれ糸が3本あれば紡げるわ。武器が壊れると手に入るの。");
                    else
                    {
                        threads -= 3;
                        int t = RandI(0, AT_COUNT - 1);
                        attInv[t]++;
                        Say(L"機織りの娘", std::wstring(L"できたわ！「") + kAttach[t].name + L"」よ。");
                        ToastMsg(std::wstring(L"アタッチメント：") + kAttach[t].name + L"（" + kAttach[t].desc + L"）", RGBA(0.8f, 0.9f, 1));
                    }
                }
                else Say(WalkerName(roomNpc), WalkerLine(roomNpc, roomPart));
            }
        }
    }
    // 部屋はジオラマのように正面上から見下ろす
    camPosNow = Lerp3(camPosNow, F3(A.pos.x * 0.3f, 6.2f, -9.0f), Saturate(dt * 3));
    camTargetNow = Lerp3(camTargetNow, F3(A.pos.x * 0.3f, 0.8f, 0.6f), Saturate(dt * 3));
}

void Game::RenderInterior()
{
    FrameSettings fs;
    fs.camPos = camPosNow; fs.camTarget = camTargetNow; fs.fovY = 0.75f;
    fs.clear = F3(0.11f, 0.08f, 0.07f);
    fs.fogColor = fs.clear; fs.fogStart = 100; fs.fogEnd = 200; fs.fogMax = 0;
    fs.ambient = F3(0.55f, 0.5f, 0.48f);
    fs.lightDir = F3(0.3f, -1.f, 0.6f);
    fs.lightColor = F3(0.6f, 0.55f, 0.5f);
    fs.time = time;
    if (roomPattern == 0)
    {
        fs.lights.push_back({ F3(0, 0.6f, 0.5f), 5.5f, F3(1.4f, 0.7f, 0.3f) });
        fs.lights.push_back({ F3(2.9f, 1.2f, 2.2f), 3.5f, F3(1.0f, 0.75f, 0.45f) });
    }
    else if (roomPattern == 1)
    {
        bool forge = roomPart >= 0 && placed[roomPart].t == P_Forge;
        fs.lights.push_back({ F3(0, 2.0f, 1.8f), 5.f, forge ? F3(1.6f, 0.6f, 0.2f) : F3(0.9f, 0.8f, 0.6f) });
    }
    else
    {
        fs.lights.push_back({ F3(-2.0f, 2.2f, 0.2f), 4.f, F3(1.3f, 0.7f, 0.3f) });
        fs.lights.push_back({ F3(2.0f, 2.2f, 0.2f), 4.f, F3(1.3f, 0.7f, 0.3f) });
    }
    R->BeginFrame(fs);

    auto D = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, Texture* tx = nullptr, float ol = 0.015f, XMFLOAT4 uv = { 1, 1, 0, 0 }, XMFLOAT3 em = { 0, 0, 0 }) {
        DrawParams p; p.color = c; p.tex = tx; p.outline = ol; p.uv = uv; p.emissive = em; p.rim = 0.15f; R->Draw(m, L, p);
    };
    XMFLOAT4 darkWood = C4(0.35f, 0.24f, 0.16f), white = C4(1, 1, 1);
    // ジオラマの台座
    D(R->cube, S(9.0f, 0.6f, 7.0f) * Tr(0, -0.3f, 0), C4(0.3f, 0.2f, 0.14f), T["wood"], 0.02f, { 3, 0.4f, 0, 0 });
    // 床
    if (roomPattern == 0)
    {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 3; j++)
            {
                bool rotq = (i + j) % 2 == 0;
                D(R->cube, S(rotq ? 1.95f : 1.95f, 0.06f, 1.95f) * Tr(-3.f + i * 2.f, 0.03f, -2.f + j * 2.f), white, T["tatami"], 0.f, { rotq ? 1.f : 2.f, rotq ? 2.f : 1.f, 0, 0 });
            }
    }
    else D(R->cube, S(8.0f, 0.06f, 6.0f) * Tr(0, 0.03f, 0), white, T["woodfloor"], 0.f, { 3, 2, 0, 0 });
    // 壁（手前は切り取ったジオラマの断面）
    D(R->cube, S(8.2f, 3.0f, 0.2f) * Tr(0, 1.5f, 3.1f), white, T["plaster"], 0.f, { 3, 1, 0, 0 });
    D(R->cube, S(0.2f, 3.0f, 6.2f) * Tr(-4.1f, 1.5f, 0), white, T["plaster"], 0.f, { 2, 1, 0, 0 });
    D(R->cube, S(0.2f, 3.0f, 6.2f) * Tr(4.1f, 1.5f, 0), white, T["plaster"], 0.f, { 2, 1, 0, 0 });
    for (int i = 0; i < 5; i++) D(R->cube, S(0.18f, 3.1f, 0.22f) * Tr(-4.f + i * 2.f, 1.55f, 3.f), darkWood);
    D(R->cube, S(8.4f, 0.22f, 0.3f) * Tr(0, 3.0f, 3.0f), darkWood, T["beam"]);
    D(R->cube, S(0.3f, 0.22f, 6.4f) * Tr(-4.0f, 3.0f, 0), darkWood, T["beam"]);
    D(R->cube, S(0.3f, 0.22f, 6.4f) * Tr(4.0f, 3.0f, 0), darkWood, T["beam"]);
    // 入口（手前中央の敷居と暖簾）
    D(R->cube, S(2.2f, 0.12f, 0.3f) * Tr(0, 0.06f, -3.0f), darkWood);
    D(R->cube, S(0.16f, 2.6f, 0.16f) * Tr(-1.1f, 1.3f, -3.0f), darkWood);
    D(R->cube, S(0.16f, 2.6f, 0.16f) * Tr(1.1f, 1.3f, -3.0f), darkWood);

    if (roomPattern == 0)
    {
        // 民家：襖・障子窓・囲炉裏・座布団・たんす・掛け軸・行灯
        for (int i = 0; i < 4; i++) D(R->cube, S(1.9f, 2.3f, 0.05f) * Tr(-3.f + i * 2.f, 1.2f, 2.97f), white, i % 2 ? T["fusuma2"] : T["fusuma1"], 0.f);
        D(R->cube, S(0.05f, 1.2f, 2.4f) * Tr(3.97f, 1.6f, 0.3f), white, T["shoji"], 0.f, { 2, 1, 0, 0 });
        D(R->cube, S(1.8f, 0.2f, 1.8f) * Tr(0, 0.1f, 0.5f), darkWood);
        D(R->cube, S(1.4f, 0.05f, 1.4f) * Tr(0, 0.18f, 0.5f), C4(0.55f, 0.55f, 0.55f), nullptr, 0.f);
        for (int i = 0; i < 6; i++)
            D(R->sphere, S(0.25f, 0.15f, 0.2f) * Tr(std::cos(i * 1.1f) * 0.3f, 0.25f, 0.5f + std::sin(i * 1.1f) * 0.3f), C4(0.2f, 0.1f, 0.05f), nullptr, 0.f, { 1, 1, 0, 0 }, F3(0.9f, 0.35f, 0.05f));
        R->DrawBillboard(T["glint"], F3(0, 0.55f, 0.5f), 1.4f, 1.4f, C4(1, 0.6f, 0.25f, 0.7f), Blend::Add, { 1, 1, 0, 0 }, time * 2);
        DrawParams hk; hk.color = C4(0.2f, 0.18f, 0.16f);
        Seg(F3(0, 3.0f, 0.5f), F3(0, 1.3f, 0.5f), 0.04f, hk);
        D(R->sphere, S(0.55f, 0.45f, 0.55f) * Tr(0, 1.05f, 0.5f), C4(0.15f, 0.14f, 0.14f));
        D(R->cylinder, S(0.08f, 0.25f, 0.08f) * XMMatrixRotationZ(-0.9f) * Tr(0.25f, 1.1f, 0.5f), C4(0.15f, 0.14f, 0.14f));
        XMFLOAT4 zc[3] = { C4(0.72f, 0.22f, 0.2f), C4(0.2f, 0.3f, 0.6f), C4(0.85f, 0.6f, 0.22f) };
        float zx[3] = { -1.5f, 1.5f, 0 }, zz[3] = { 0.5f, 0.5f, -0.8f };
        for (int i = 0; i < 3; i++) D(R->cube, S(0.8f, 0.12f, 0.8f) * Tr(zx[i], 0.12f, zz[i]), zc[i]);
        D(R->cube, S(1.4f, 1.3f, 0.7f) * Tr(-3.0f, 0.68f, 2.5f), C4(0.55f, 0.36f, 0.22f), T["plank"]);
        for (int i = 0; i < 3; i++) D(R->cube, S(1.3f, 0.03f, 0.02f) * Tr(-3.0f, 0.35f + i * 0.4f, 2.14f), darkWood, nullptr, 0.f);
        D(R->cube, S(0.7f, 1.4f, 0.03f) * Tr(1.8f, 1.7f, 2.93f), white, T["fusuma3"], 0.f);
        D(R->cube, S(0.45f, 0.7f, 0.45f) * Tr(2.9f, 0.45f, 2.2f), C4(1, 0.95f, 0.8f), nullptr, 0.012f, { 1, 1, 0, 0 }, F3(0.6f, 0.45f, 0.25f));
    }
    else if (roomPattern == 1)
    {
        bool forge = roomPart >= 0 && placed[roomPart].t == P_Forge;
        if (forge)
        {
            // 鍛冶場：炉・金床・ふいご
            D(R->cube, S(2.4f, 2.2f, 1.4f) * Tr(0, 1.1f, 2.2f), white, T["forge"], 0.02f, { 1, 1, 0, 0 }, F3(0.12f, 0.05f, 0.f));
            D(R->cube, S(0.9f, 0.6f, 0.5f) * Tr(0, 0.3f, 1.2f), C4(0.25f, 0.25f, 0.28f));
            D(R->cube, S(1.1f, 0.18f, 0.45f) * Tr(0, 0.69f, 1.2f), C4(0.35f, 0.35f, 0.4f));
            R->DrawBillboard(T["glint"], F3(0, 0.9f, 1.6f), 2.0f, 2.0f, C4(1, 0.5f, 0.15f, 0.7f), Blend::Add, { 1, 1, 0, 0 }, time);
            for (int i = 0; i < 4; i++) D(R->cube, S(0.08f, 1.0f, 0.15f) * XMMatrixRotationZ(0.2f) * Tr(-3.6f + i * 0.25f, 1.6f, 2.9f), C4(0.7f, 0.72f, 0.78f));
        }
        else
        {
            // 機織り：大きな機・経糸・反物
            for (int s = -1; s <= 1; s += 2)
            {
                D(R->cube, S(0.18f, 2.4f, 0.18f) * Tr(s * 1.6f, 1.2f, 2.6f), darkWood);
                D(R->cube, S(0.18f, 1.2f, 0.18f) * Tr(s * 1.6f, 0.6f, 1.4f), darkWood);
            }
            D(R->cube, S(3.4f, 0.16f, 0.16f) * Tr(0, 2.4f, 2.6f), darkWood);
            D(R->cube, S(3.4f, 0.16f, 1.4f) * Tr(0, 1.15f, 2.0f), C4(0.5f, 0.36f, 0.22f), T["plank"]);
            D(R->cylinder, S(0.3f, 3.2f, 0.3f) * XMMatrixRotationZ(PI / 2) * Tr(1.6f, 1.3f, 1.4f), C4(0.2f, 0.3f, 0.6f));
            XMFLOAT4 tc[3] = { C4(0.2f, 0.3f, 0.6f), C4(0.75f, 0.22f, 0.2f), C4(0.9f, 0.65f, 0.25f) };
            for (int i = 0; i < 15; i++)
            {
                DrawParams th; th.color = tc[i % 3]; th.emissive = Mul(F3(tc[i % 3].x, tc[i % 3].y, tc[i % 3].z), 0.15f);
                float x = -1.4f + i * 0.2f;
                Seg(F3(x, 2.3f, 2.6f), F3(x, 1.3f, 1.4f), 0.012f, th);
            }
            D(R->cube, S(0.6f, 0.3f, 0.15f) * Tr(0.4f + std::sin(time * 3) * 0.8f, 1.35f, 1.6f), C4(0.85f, 0.7f, 0.45f));
        }
        // 糸巻きの棚
        D(R->cube, S(1.0f, 2.2f, 2.0f) * Tr(-3.3f, 1.1f, 1.8f), C4(0.55f, 0.38f, 0.24f), T["plank"]);
        XMFLOAT4 sc[4] = { C4(0.2f, 0.3f, 0.6f), C4(0.75f, 0.22f, 0.2f), C4(0.9f, 0.65f, 0.25f), C4(0.35f, 0.6f, 0.35f) };
        for (int r = 0; r < 3; r++)
            for (int i = 0; i < 4; i++)
                D(R->cylinder, S(0.3f, 0.35f, 0.3f) * Tr(-2.75f, 0.35f + r * 0.7f, 1.1f + i * 0.45f), sc[(r + i) % 4], T["rope"], 0.01f, { 3, 2, 0, 0 });
        // 作業台と道具
        D(R->cube, S(1.4f, 0.8f, 1.1f) * Tr(2.9f, 0.4f, 0.4f), C4(0.55f, 0.38f, 0.24f), T["plank"]);
        D(R->cylinder, S(0.5f, 1.2f, 0.5f) * XMMatrixRotationX(PI / 2) * Tr(2.7f, 0.95f, -0.1f), sc[1], nullptr, 0.012f);
        D(R->cylinder, S(0.5f, 1.2f, 0.5f) * XMMatrixRotationX(PI / 2) * Tr(3.2f, 0.95f, -0.1f), sc[0], nullptr, 0.012f);
        // 干した布
        for (int i = 0; i < 3; i++)
        {
            float sw = std::sin(time * 1.5f + i) * 0.06f;
            D(R->cube, S(0.03f, 1.4f, 0.8f) * Tr(0, -0.7f, 0) * XMMatrixRotationZ(sw) * Tr(-3.9f, 2.9f, -1.6f + i * 0.9f), sc[i], nullptr, 0.f);
        }
    }
    else
    {
        // 商店：帳場・品物の棚・提灯・暖簾
        D(R->cube, S(5.4f, 0.9f, 0.7f) * Tr(0, 0.45f, 0.9f), C4(0.5f, 0.3f, 0.18f), T["plank"], 0.02f, { 3, 1, 0, 0 });
        D(R->cube, S(5.5f, 0.08f, 0.8f) * Tr(0, 0.93f, 0.9f), darkWood);
        for (int r = 0; r < 3; r++)
        {
            D(R->cube, S(7.4f, 0.08f, 0.7f) * Tr(0, 0.5f + r * 0.8f, 2.6f), darkWood);
            for (int i = 0; i < 9; i++)
            {
                float x = -3.3f + i * 0.82f;
                int k = (i + r * 2) % 4;
                if (k == 0) D(R->cylinder, S(0.4f, 0.5f, 0.4f) * Tr(x, 0.55f + r * 0.8f, 2.6f), C4(0.6f, 0.45f, 0.3f));
                else if (k == 1) D(R->sphere, S(0.45f, 0.5f, 0.45f) * Tr(x, 0.78f + r * 0.8f, 2.6f), C4(0.3f, 0.4f, 0.55f));
                else if (k == 2) D(R->cube, S(0.5f, 0.35f, 0.4f) * Tr(x, 0.72f + r * 0.8f, 2.6f), C4(0.75f, 0.25f, 0.2f));
                else D(R->cylinder, S(0.3f, 0.4f, 0.3f) * Tr(x, 0.55f + r * 0.8f, 2.6f), C4(0.85f, 0.7f, 0.35f), T["rope"], 0.01f);
            }
        }
        // 帳場の上の品物（団子・薬草・灯油の瓶）
        D(R->cylinder, S(0.08f, 0.6f, 0.08f) * Tr(-1.6f, 0.95f, 0.9f), darkWood, nullptr, 0.f);
        for (int i = 0; i < 3; i++) D(R->sphere, S(0.18f, 0.18f, 0.18f) * Tr(-1.6f, 1.15f + i * 0.17f, 0.9f), i == 0 ? C4(0.95f, 0.6f, 0.65f) : i == 1 ? C4(1, 1, 0.95f) : C4(0.55f, 0.75f, 0.4f));
        D(R->sphere, S(0.4f, 0.25f, 0.4f) * Tr(1.4f, 1.05f, 0.9f), C4(0.4f, 0.7f, 0.35f));
        D(R->cylinder, S(0.3f, 0.5f, 0.3f) * Tr(0.6f, 0.97f, 0.9f), C4(0.75f, 0.55f, 0.25f), nullptr, 0.012f, { 1, 1, 0, 0 }, F3(0.1f, 0.06f, 0.02f));
        for (int s = -1; s <= 1; s += 2)
        {
            DrawParams ch; ch.tex = T["chochin"]; ch.emissive = { 0.55f, 0.3f, 0.12f };
            R->Draw(R->cylinder, S(0.5f, 0.7f, 0.5f) * Tr(s * 2.0f, 1.9f, 0.2f), ch);
        }
        D(R->cube, S(2.0f, 0.7f, 0.05f) * Tr(0, 2.5f, -3.0f), white, T["noren"], 0.f);
        D(R->cube, S(2.0f, 0.5f, 0.08f) * Tr(0, 2.7f, 2.9f), C4(0.25f, 0.18f, 0.12f));
    }

    // 住人
    if (roomNpc.kind == 4) DrawTanuki(roomNpc.pos, roomNpc.yaw, roomNpc.phase);
    else if (!(roomNpc.kind == 2 && joined[1])) DrawWalker(roomNpc);
    DrawActor(party[active]);
    DrawFx();
}

void Game::DrawShopUI()
{
    float W = (float)R->W, H = (float)R->H;
    float bw = 620, bh = 470, bx = W - bw - 40, by = (H - bh) / 2;
    R->FillRound(bx, by, bw, bh, 16, RGBA(0.06f, 0.09f, 0.17f, 0.95f));
    R->StrokeRound(bx + 5, by + 5, bw - 10, bh - 10, 12, RGBA(0.85f, 0.6f, 0.25f), 2);
    R->Text(L"よろず屋（タヌキの商人）", bx + 24, by + 18, 400, 30, 22, RGBA(1, 0.92f, 0.75f), 0, true);
    wchar_t buf[96];
    swprintf_s(buf, L"所持金 %d 文", money);
    R->Text(buf, bx + bw - 224, by + 22, 200, 26, 18, RGBA(1, 0.85f, 0.4f), 2, true);
    for (int i = 0; i < SHOP_N; i++)
    {
        float y = by + 66 + i * 50.f;
        bool sel = shopSel == i;
        R->FillRound(bx + 20, y, bw - 40, 42, 10, sel ? RGBA(0.98f, 0.94f, 0.85f) : RGBA(1, 1, 1, 0.06f));
        D2D1_COLOR_F tc = sel ? RGBA(0.12f, 0.15f, 0.25f) : RGBA(1, 1, 1);
        R->Text(kShop[i].name, bx + 36, y + 9, 140, 28, 18, tc, 0, true, !sel);
        R->Text(kShop[i].desc, bx + 160, y + 11, 300, 26, 15, sel ? RGBA(0.3f, 0.35f, 0.45f) : RGBA(1, 1, 1, 0.7f), 0, false, !sel);
        int have = kShop[i].item >= 0 ? items[kShop[i].item] : attInv[kShop[i].att];
        swprintf_s(buf, L"%d 文（所持 %d）", kShop[i].price, have);
        R->Text(buf, bx + bw - 220, y + 10, 180, 26, 15, sel ? RGBA(0.75f, 0.4f, 0.1f) : RGBA(1, 0.85f, 0.4f), 2, true, !sel);
    }
    R->Text(L"W/S：選ぶ　Enter：買う　Esc：閉じる", bx, by + bh - 36, bw, 24, 14, RGBA(1, 1, 1, 0.6f), 1);
}

void Game::DrawInteriorUI()
{
    float W = (float)R->W, H = (float)R->H;
    std::wstring name = roomPart >= 0 && roomPart < (int)placed.size() ? kParts[placed[roomPart].t].name : L"";
    R->FillRound(18, 16, 340, 64, 12, RGBA(0.06f, 0.08f, 0.15f, 0.78f));
    R->Text(name, 34, 22, 300, 30, 22, RGBA(1, 0.92f, 0.75f), 0, true);
    R->Text(std::wstring(L"部屋の様子：") + kPatternName[roomPattern], 34, 52, 300, 24, 14, RGBA(1, 1, 1, 0.75f));
    if (!talkHint.empty() && dialog.empty() && !shopOpen)
    {
        R->FillRound(W / 2 - 170, H - 120, 340, 42, 21, RGBA(0.05f, 0.07f, 0.13f, 0.8f));
        R->Text(talkHint, W / 2 - 170, H - 112, 340, 30, 18, RGBA(1, 0.92f, 0.7f), 1, true);
    }
    if (shopOpen) DrawShopUI();
    R->FillRect(0, H - 32, W, 32, RGBA(0.04f, 0.05f, 0.1f, 0.6f));
    R->Text(L"WASD 移動　Shift ダッシュ　E 話す／調べる　手前の出口から外へ", 0, H - 28, W, 24, 14, RGBA(1, 1, 1, 0.85f), 1, false, false);
}
