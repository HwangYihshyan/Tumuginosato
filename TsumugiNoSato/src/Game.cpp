#include "Game.h"

// ------------------------------------------------------------------
// 初期化
// ------------------------------------------------------------------
bool Game::Init(Renderer* r, const std::wstring& assetDir)
{
    R = r;
    assets = assetDir;
    struct TexEntry { const char* key; const wchar_t* file; };
    const TexEntry list[] = {
        { "cave_floor", L"03_Dungeon\\T_Cave_WetRock_Floor_A.png" },
        { "cave_wall", L"03_Dungeon\\T_Cave_WetRock_Wall_A.png" },
        { "soil", L"03_Dungeon\\T_Bamboo_Ground_Soil_A.png" },
        { "rope", L"03_Dungeon\\T_Shimenawa_Rope_A.png" },
        { "grass", L"05_Village\\T_Ground_Grass_A.png" },
        { "path", L"05_Village\\T_Ground_Path_A.png" },
        { "field", L"05_Village\\T_Ground_Field_A.png" },
        { "stonepave", L"05_Village\\T_Ground_StonePave_A.png" },
        { "thatch", L"05_Village\\T_House_ThatchRoof_A.png" },
        { "shingle", L"05_Village\\T_House_ShingleRoof_A.png" },
        { "mudwall", L"05_Village\\T_House_MudWall_A.png" },
        { "plank", L"05_Village\\T_House_PlankWall_A.png" },
        { "door", L"05_Village\\T_House_Door_A.png" },
        { "shoji", L"05_Village\\T_House_Shoji_A.png" },
        { "bark", L"05_Village\\T_Tree_Bark_A.png" },
        { "chochin", L"05_Village\\T_Chochin_A.png" },
        { "forge", L"05_Village\\T_Forge_A.png" },
        { "well", L"05_Village\\T_Well_A.png" },
        { "water", L"05_Village\\T_River_Water_A.png" },
        { "bank", L"05_Village\\T_River_Bank_A.png" },
        { "wood", L"05_Village\\T_Watchtower_Wood_A.png" },
        { "sky_day", L"06_Common\\T_Sky_Day_A.png" },
        { "sky_dusk", L"06_Common\\T_Sky_Dusk_A.png" },
        { "mountains", L"06_Common\\T_Far_Mountains_A.png" },
        { "mist", L"06_Common\\T_Far_Mist_A.png" },
        { "grid_ok", L"06_Common\\T_Grid_OK_A.png" },
        { "grid_ng", L"06_Common\\T_Grid_NG_A.png" },
        { "grid_cell", L"06_Common\\T_Grid_Cell_A.png" },
        { "seal_closed", L"04_Props\\T_ExitSeal_Closed_A.png" },
        { "seal_open", L"04_Props\\T_ExitSeal_Open_A.png" },
        { "spring", L"04_Props\\T_Spring_Stone_A.png" },
        { "blob", L"07_FX\\T_FX_BlobShadow_A.png" },
        { "spark", L"07_FX\\T_FX_Hit_Spark_4x4.png" },
        { "ring", L"07_FX\\T_FX_Hit_Ring_4x4.png" },
        { "lockon", L"07_FX\\T_FX_Marker_LockOn_A.png" },
        { "glint", L"07_FX\\T_FX_Renki_Glint_A.png" },
        { "exclaim", L"07_FX\\T_FX_Mark_Exclaim_A.png" },
        { "vignette", L"07_FX\\T_FX_Screen_DamageVignette_A.png" },
        { "inkborder", L"07_FX\\T_FX_Screen_InkBorder_A.png" },
    };
    for (auto& e : list) T[e.key] = R->LoadTexture(assets + L"Textures\\" + e.file);

    // 物語で戻ってくる順番（足りなくなったら汎用部品）
    storyQueue = { P_Loom, P_Tree, P_Lantern, P_HouseKurobee, P_Kiln, P_Tree, P_Field, P_HouseRabbit,
                   P_Path, P_Lantern, P_Cloth, P_Well, P_HouseVillager, P_Tree, P_Forge, P_Wheel };
    inventory[P_HouseTouji] = 1;
    inventory[P_Path] = 3;

    // 遠景の筒
    {
        MeshBuilder mb;
        const int sl = 48;
        for (int j = 0; j <= sl; j++)
        {
            float ph = 2 * PI * j / sl;
            XMFLOAT3 n{ -std::cos(ph), 0, -std::sin(ph) };
            mb.AddV({ std::cos(ph) * 0.5f, 0, std::sin(ph) * 0.5f }, n, { (float)j / sl, 1 });
            mb.AddV({ std::cos(ph) * 0.5f, 1, std::sin(ph) * 0.5f }, n, { (float)j / sl, 0 });
        }
        for (int j = 0; j < sl; j++) { uint32_t a = j * 2; mb.Tri(a, a + 1, a + 3); mb.Tri(a, a + 3, a + 2); }
        tube = R->CreateMesh(mb);
    }
    // 村のまわりの森
    {
        std::mt19937 fr(42);
        std::uniform_real_distribution<float> u(0, 1);
        for (int i = 0; i < 70; i++)
        {
            float a = u(fr) * 2 * PI, r = 27 + u(fr) * 30;
            float x = std::cos(a) * r, z = std::sin(a) * r;
            if (x > 14 && x < 22) continue;   // 川の上には置かない
            forest.push_back({ x, 0.8f + u(fr) * 0.7f, z, u(fr) });
        }
    }

    party[0].def = 0; party[1].def = 1;
    for (auto& a : party) { a.maxHp = a.hp = Def(a).maxHp; a.maxDur = a.dur = (float)Def(a).maxDur; }

    scene = SceneId::Title;
    sceneT = 0;
    if (autoshot) BuildShotScript();
    return true;
}

PartType Game::NextCrystalPart()
{
    if (!storyQueue.empty())
    {
        PartType p = storyQueue.front();
        storyQueue.erase(storyQueue.begin());
        return p;
    }
    const PartType pool[] = { P_Tree, P_Tree, P_Lantern, P_Path, P_Path, P_Cloth, P_HouseVillager, P_Well, P_Field };
    return pool[RandI(0, (int)(sizeof(pool) / sizeof(pool[0])) - 1)];
}

// ------------------------------------------------------------------
// 入力
// ------------------------------------------------------------------
Controls Game::ReadControls(InputState& in, float dt)
{
    Controls c;
    if (in.Down('W')) c.mz += 1;
    if (in.Down('S')) c.mz -= 1;
    if (in.Down('D')) c.mx += 1;
    if (in.Down('A')) c.mx -= 1;
    c.camYaw = in.mdx * 0.0035f + ((in.Down(VK_RIGHT) ? 1.f : 0.f) - (in.Down(VK_LEFT) ? 1.f : 0.f)) * 2.2f * dt;
    c.atkDown = in.lmb || in.Down('J');
    c.atkPressed = in.LPressed() || in.Pressed('J');
    c.atkReleased = (!in.lmb && in.plmb) || (!in.Down('J') && in.prevKey['J']);
    c.guard = in.rmb || in.Down('K');
    c.dodge = in.Pressed(VK_SPACE);
    c.lockon = in.Pressed('Q');
    c.interact = in.Pressed('E');
    c.swap = in.Pressed(VK_TAB);
    for (int i = 0; i < 4; i++) if (in.Pressed('1' + i)) c.item = i;
    c.up = in.Pressed('W') || in.Pressed(VK_UP);
    c.down = in.Pressed('S') || in.Pressed(VK_DOWN);
    c.confirm = in.Pressed(VK_RETURN) || in.Pressed('E') || in.LPressed();
    return c;
}

// ------------------------------------------------------------------
// 更新
// ------------------------------------------------------------------
void Game::Update(float dt, InputState& realIn)
{
    static InputState blank;
    InputState& in = autoshot ? blank : realIn;
    time += dt;
    sceneT += dt;
    if (autoshot) UpdateDirector(dt);

    for (auto& t : toasts) t.t += dt;
    toasts.erase(std::remove_if(toasts.begin(), toasts.end(), [](const Toast& t) { return t.t > 3.4f; }), toasts.end());

    if (!autoshot && in.Pressed(VK_ESCAPE))
    {
        if (scene == SceneId::Dungeon && exitMenu) exitMenu = false;
        else quit = true;
    }

    Controls c = autoshot ? Controls{} : ReadControls(in, dt);
    bool consumed = false;
    if (!dialog.empty() && !autoshot)
    {
        if (in.Pressed(VK_RETURN) || in.Pressed('E') || in.LPressed() || in.Pressed(VK_SPACE))
        {
            dialog.pop_front();
            consumed = true;
        }
        if (scene != SceneId::Dungeon || consumed) c = Controls{};
    }

    switch (scene)
    {
    case SceneId::Title: UpdateTitle(dt, in); break;
    case SceneId::Village: if (!consumed) UpdateVillage(dt, in); break;
    case SceneId::Dungeon:
        if (autopilot) c = AutoControls(dt);
        UpdateDungeon(dt, c);
        break;
    }
    wantMouseLock = scene == SceneId::Dungeon && !autoshot && !exitMenu;
}

void Game::UpdateTitle(float dt, InputState& in)
{
    if (in.Pressed(VK_RETURN) || in.LPressed() || in.Pressed(VK_SPACE))
        EnterVillage(false);
}

// ------------------------------------------------------------------
// 描画
// ------------------------------------------------------------------
void Game::Render()
{
    switch (scene)
    {
    case SceneId::Title:
        RenderVillage(true);
        R->Begin2D();
        DrawTitleUI();
        R->End2D();
        break;
    case SceneId::Village:
        RenderVillage(false);
        R->Begin2D();
        DrawVillageUI();
        DrawDialog();
        DrawToasts();
        R->End2D();
        break;
    case SceneId::Dungeon:
        RenderDungeon();
        R->Begin2D();
        DrawDungeonUI();
        DrawDialog();
        DrawToasts();
        R->End2D();
        break;
    }
}

void Game::DrawTitleUI()
{
    float W = (float)R->W, H = (float)R->H;
    R->FillRect(0, 0, W, H, RGBA(0.05f, 0.07f, 0.12f, 0.18f));
    R->Text(L"GAME DEVELOPMENT PLAN  ―  VERTICAL SLICE", 0, 150, W, 30, 16, RGBA(0.85f, 0.55f, 0.2f, 0.95f), 1, false, false);
    R->Text(L"ツムギノサト", 0, 180, W, 120, 92, RGBA(0.12f, 0.2f, 0.36f), 1, true, false, 1);
    R->Text(L"ツムギノサト", 0, 177, W, 120, 92, RGBA(1, 0.98f, 0.94f), 1, true, true, 1);
    R->Text(L"― 霧海の記憶 ―", 0, 300, W, 50, 32, RGBA(1, 1, 1, 0.95f), 1, false, true, 1);
    R->FillRect(W / 2 - 110, 360, 220, 3, RGBA(0.85f, 0.55f, 0.2f));
    R->Text(L"ダンジョン探索 × 町の復興　アクションRPG", 0, 380, W, 30, 20, RGBA(1, 1, 1, 0.9f), 1);
    float a = 0.55f + 0.45f * std::sin(time * 3.f);
    R->Text(L"Enter / クリック で はじめる", 0, 520, W, 40, 24, RGBA(1, 0.95f, 0.8f, a), 1, true);
    R->Text(L"C++17 / DirectX 11 / HLSL", 0, H - 46, W - 24, 30, 15, RGBA(1, 1, 1, 0.7f), 2);
}

void Game::Bar(float x, float y, float w, float h, float v, D2D1_COLOR_F fill, D2D1_COLOR_F back)
{
    R->FillRound(x, y, w, h, h * 0.5f, back);
    v = Saturate(v);
    if (v > 0.001f) R->FillRound(x, y, std::max(h, w * v), h, h * 0.5f, fill);
    R->StrokeRound(x, y, w, h, h * 0.5f, RGBA(1, 1, 1, 0.35f), 1.f);
}

void Game::DrawDialog()
{
    if (dialog.empty()) return;
    float W = (float)R->W, H = (float)R->H;
    const DialogLine& d = dialog.front();
    float bw = 860, bh = 128, bx = (W - bw) / 2, by = H - bh - (scene == SceneId::Village ? 150.f : 34.f);
    R->FillRound(bx, by, bw, bh, 14, RGBA(0.06f, 0.09f, 0.17f, 0.88f));
    R->StrokeRound(bx + 4, by + 4, bw - 8, bh - 8, 11, RGBA(0.85f, 0.6f, 0.25f, 0.85f), 2);
    // 話者アイコン
    D2D1_COLOR_F ic = d.who == L"ツムギ" ? RGBA(0.98f, 0.95f, 0.85f) : d.who == L"クロベエ" ? RGBA(0.45f, 0.45f, 0.5f) :
                      d.who == L"ナギ" ? RGBA(0.75f, 0.75f, 0.9f) : RGBA(0.2f, 0.3f, 0.6f);
    R->FillCircle(bx + 62, by + 64, 40, ic);
    R->StrokeCircle(bx + 62, by + 64, 40, RGBA(0.85f, 0.6f, 0.25f), 3);
    if (d.who == L"ツムギ")
    {
        R->FillCircle(bx + 50, by + 62, 5, RGBA(0.1f, 0.1f, 0.15f));
        R->FillCircle(bx + 74, by + 62, 5, RGBA(0.1f, 0.1f, 0.15f));
        R->FillCircle(bx + 44, by + 74, 5, RGBA(1, 0.6f, 0.65f, 0.7f));
        R->FillCircle(bx + 80, by + 74, 5, RGBA(1, 0.6f, 0.65f, 0.7f));
        R->Line(bx + 52, by + 30, bx + 40, by + 12, RGBA(0.8f, 0.6f, 0.3f), 2.5f);
        R->Line(bx + 72, by + 30, bx + 84, by + 12, RGBA(0.8f, 0.6f, 0.3f), 2.5f);
    }
    else if (d.who == L"クロベエ")
    {
        R->FillCircle(bx + 62, by + 60, 30, RGBA(0.96f, 0.95f, 0.92f));
        R->FillRound(bx + 42, by + 44, 12, 26, 6, RGBA(0.12f, 0.12f, 0.14f));
        R->FillRound(bx + 70, by + 44, 12, 26, 6, RGBA(0.12f, 0.12f, 0.14f));
        R->FillCircle(bx + 62, by + 76, 6, RGBA(0.12f, 0.12f, 0.14f));
    }
    R->FillRound(bx + 120, by - 18, 150, 34, 8, RGBA(0.85f, 0.55f, 0.2f));
    R->Text(d.who, bx + 120, by - 14, 150, 30, 19, RGBA(1, 1, 1), 1, true, false);
    R->Text(d.text, bx + 125, by + 28, bw - 160, bh - 36, 21, RGBA(1, 1, 1), 0, false, true);
    float a = 0.5f + 0.5f * std::sin(time * 5);
    R->Text(L"▼", bx + bw - 44, by + bh - 36, 30, 30, 16, RGBA(1, 0.85f, 0.5f, a), 1);
}

void Game::DrawToasts()
{
    float W = (float)R->W;
    float y = 150;
    for (auto& t : toasts)
    {
        float a = t.t < 0.2f ? t.t / 0.2f : (t.t > 2.8f ? Saturate((3.4f - t.t) / 0.6f) : 1.f);
        float w = 560;
        R->FillRound((W - w) / 2, y, w, 40, 20, RGBA(0.06f, 0.08f, 0.15f, 0.75f * a));
        D2D1_COLOR_F c = t.col; c.a = a;
        R->Text(t.text, (W - w) / 2, y + 7, w, 32, 20, c, 1, true);
        y += 48;
    }
}

// ------------------------------------------------------------------
// 自動撮影スクリプト（--autoshot）
// ------------------------------------------------------------------
void Game::UpdateDirector(float dt)
{
    if (stepIdx >= steps.size()) { quit = true; return; }
    stepT += dt;
    if (stepT >= steps[stepIdx].wait)
    {
        ShotStep& s = steps[stepIdx];
        if (s.act) s.act();
        if (!s.shot.empty()) pendingShot = shotDir + L"\\" + s.shot + L".png";
        stepIdx++;
        stepT = 0;
    }
}

void Game::TeleportPartyNear(XMFLOAT3 p, XMFLOAT3 lookAt)
{
    XMFLOAT3 dir = NormXZ(Sub(lookAt, p));
    float yaw = YawOf(dir);
    Actor& a = party[active];
    Actor& b = party[1 - active];
    a.pos = p; a.yaw = yaw; a.st = ASt::Normal;
    XMFLOAT3 right{ std::cos(yaw), 0, -std::sin(yaw) };
    XMFLOAT3 bp = Add(Sub(p, Mul(dir, 1.6f)), Mul(right, 1.3f));
    b.pos = Solid(bp.x, bp.z) ? p : bp;
    b.yaw = yaw; b.st = ASt::Normal;
    camYaw = yaw;
    camPosNow = Add(p, F3(-dir.x * 7, 4.5f, -dir.z * 7));
    camTargetNow = Add(p, F3(0, 1.2f, 0));
}

void Game::BuildShotScript()
{
    steps.clear();
    // 1. タイトル
    steps.push_back({ 2.0f, nullptr, L"01_title" });
    // 2. 霧に包まれた更地の村（復興0%）
    steps.push_back({ 0.1f, [this] {
        EnterVillage(false);
        dialog.clear();
        Say(L"ツムギ", L"ここがハタオリ村…だった場所。霧に呑まれて、御神木の切り株しか残ってないの。\nまずはトウジさんの家を置いてみよう！");
        cursorX = 11; cursorZ = 10; cursorRot = 0; palSel = 0;
    }, L"" });
    steps.push_back({ 1.8f, nullptr, L"02_village_start" });
    // 3. 根の洞 1階を探索
    steps.push_back({ 0.1f, [this] {
        dialog.clear();
        EnterDungeon(1, 777);
        dialog.clear();
        Say(L"クロベエ", L"坊、灯（ともしび）が尽きる前に結晶を集めるぞ。鍵持ちの化け物を探せ。");
        autopilot = true;
    }, L"" });
    steps.push_back({ 2.6f, nullptr, L"03_dungeon_explore" });
    // 4. 戦闘
    steps.push_back({ 0.1f, [this] {
        dialog.clear();
        // 敵が一番多い部屋へ移動
        int best = -1, bestN = 0;
        for (int r = 0; r < (int)rooms.size(); r++)
        {
            int n = 0;
            for (auto& e : enemies) if (e.st != ESt::Dead && RoomAt(e.pos) == r) n++;
            if (n > bestN && r != 0) { bestN = n; best = r; }
        }
        if (best >= 0)
        {
            XMFLOAT3 cen{}; int n = 0;
            for (auto& e : enemies) if (e.st != ESt::Dead && RoomAt(e.pos) == best) { cen = Add(cen, e.pos); n++; }
            cen = Mul(cen, 1.f / n);
            const Room& rm = rooms[best];
            XMFLOAT3 rc{ (rm.x + rm.w * 0.5f) * CELL, 0, (rm.z + rm.h * 0.5f) * CELL };
            XMFLOAT3 d = NormXZ(Sub(rc, cen));
            if (LenXZ(d) < 0.1f) d = F3(0, 0, -1);
            XMFLOAT3 p = Add(cen, Mul(d, 5.f));
            p.x = Clampf(p.x, (rm.x + 0.8f) * CELL, (rm.x + rm.w - 0.8f) * CELL);
            p.z = Clampf(p.z, (rm.z + 0.8f) * CELL, (rm.z + rm.h - 0.8f) * CELL);
            TeleportPartyNear(p, cen);
            for (auto& e : enemies) if (e.st != ESt::Dead && RoomAt(e.pos) == best) { e.st = ESt::Notice; e.stT = 0; }
        }
        autopilot = true;
    }, L"" });
    steps.push_back({ 1.55f, nullptr, L"04_combat" });
    steps.push_back({ 1.1f, nullptr, L"05_combat_combo" });
    // 5. 深層：灯が尽きかけ・武器が壊れかけ
    steps.push_back({ 0.1f, [this] {
        knot = true;
        EnterDungeon(7, 4242);
        dialog.clear();
        lantern = 14;
        party[0].dur = 5; party[0].hp = 38;
        party[1].hp = 70;
        // 胞子を近くに追加
        Actor& a = party[active];
        int rm = RoomAt(a.pos);
        if (rm >= 0)
            for (int i = 0; i < 2; i++)
            {
                XMFLOAT3 p = RandomFloorPos(rooms[rm], 1.5f);
                if (DistXZ(p, a.pos) < 3.5f) p = Add(a.pos, F3(RandF(-1, 1) * 4, 0, 5));
                if (!Solid(p.x, p.z)) SpawnEnemy(EType::Spore, p, true);
            }
        for (auto& e : enemies) if (DistXZ(e.pos, a.pos) < 14) { e.st = ESt::Chase; }
        autopilot = true;
    }, L"" });
    steps.push_back({ 2.2f, nullptr, L"06_deep_danger" });
    // 6. 5階 中間の間：ほつれ蜘蛛
    steps.push_back({ 0.1f, [this] {
        EnterDungeon(5, 5);
        dialog.clear();
        autopilot = true;
    }, L"" });
    steps.push_back({ 3.6f, nullptr, L"07_midboss" });
    // 7. 出口：次の階へ／町へ戻る
    steps.push_back({ 0.1f, [this] {
        autopilot = false;
        EnterDungeon(3, 99);
        dialog.clear();
        toasts.clear();
        hasKey = true;
        crystalsThisRun = 3;
        XMFLOAT3 p = Sub(exitPos, Mul(exitFace, 2.6f));
        TeleportPartyNear(p, exitPos);
        exitMenu = true; exitSel = 0;
    }, L"" });
    steps.push_back({ 1.0f, nullptr, L"08_exit_choice" });
    // 8. 復興が進んだ村
    steps.push_back({ 0.1f, [this] {
        exitMenu = false;
        placed.clear();
        auto P = [this](PartType t, int x, int z, int rot = 0) { placed.push_back({ t, x, z, rot, 10.f }); };
        P(P_HouseTouji, 13, 11); P(P_Loom, 15, 11); P(P_HouseKurobee, 3, 13); P(P_Kiln, 5, 13);
        P(P_HouseRabbit, 3, 5); P(P_Field, 5, 5); P(P_HouseVillager, 12, 4);
        P(P_Tree, 11, 4); P(P_Tree, 14, 4); P(P_Tree, 12, 6); P(P_Tree, 2, 9); P(P_Tree, 1, 16); P(P_Tree, 16, 17); P(P_Tree, 7, 17); P(P_Tree, 10, 16);
        for (int z = 0; z <= 8; z++) P(P_Path, 9, z);
        P(P_Lantern, 8, 2); P(P_Lantern, 10, 2); P(P_Lantern, 8, 6); P(P_Lantern, 10, 6);
        P(P_Cloth, 6, 8); P(P_Well, 7, 3); P(P_Forge, 13, 15); P(P_Wheel, 17, 6);
        for (auto& p : placed) p.appearT = 10.f;
        for (int i = 0; i < P_COUNT; i++) inventory[i] = 0;
        inventory[P_Tree] = 2; inventory[P_Lantern] = 1; inventory[P_Path] = 4;
        EnterVillage(true);
        dialog.clear();
        rateShown = RestoreRate();
        cursorX = 6; cursorZ = 1; palSel = 1;
        toasts.clear();
        ToastMsg(L"要望達成！ トウジ「御神木の近く」", RGBA(1, 0.85f, 0.4f));
    }, L"" });
    steps.push_back({ 3.0f, nullptr, L"09_village_restored" });
    steps.push_back({ 0.1f, [this] { vWalkCam = true; toasts.clear(); }, L"" });
    steps.push_back({ 2.0f, nullptr, L"10_village_street" });
    steps.push_back({ 0.3f, nullptr, L"" });
}
