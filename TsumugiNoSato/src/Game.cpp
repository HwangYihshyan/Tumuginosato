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
        { "tatami", L"03_Dungeon\\T_Tenshu_Tatami_A.png" },
        { "woodfloor", L"03_Dungeon\\T_Tenshu_WoodFloor_A.png" },
        { "plaster", L"03_Dungeon\\T_Tenshu_Plaster_A.png" },
        { "fusuma1", L"03_Dungeon\\T_Tenshu_Fusuma01_A.png" },
        { "fusuma2", L"03_Dungeon\\T_Tenshu_Fusuma02_A.png" },
        { "fusuma3", L"03_Dungeon\\T_Tenshu_Fusuma03_A.png" },
        { "beam", L"03_Dungeon\\T_Tenshu_Beam_A.png" },
        { "lattice", L"03_Dungeon\\T_Tenshu_Lattice_A.png" },
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
        { "stall", L"05_Village\\T_Stall_A.png" },
        { "noren", L"05_Village\\T_Dango_Noren_A.png" },
        { "stonelantern", L"05_Village\\T_Lantern_Stone_A.png" },
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
        { "smoke", L"07_FX\\T_FX_Smoke_Puff_8x8.png" },
        { "objective", L"07_FX\\T_FX_Marker_Objective_A.png" },
        { "pillar", L"07_FX\\T_FX_Unseal_LightPillar_A.png" },
    };
    for (auto& e : list) T[e.key] = R->LoadTexture(assets + L"Textures\\" + e.file);

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
    // 村のまわりの森（入口の岩山のあたりは空ける）
    {
        std::mt19937 fr(42);
        std::uniform_real_distribution<float> u(0, 1);
        for (int i = 0; i < 70; i++)
        {
            float a = u(fr) * 2 * PI, r = 27 + u(fr) * 30;
            float x = std::cos(a) * r, z = std::sin(a) * r;
            if (x > 14 && x < 22) continue;
            if (std::fabs(x) < 14 && z > 18 && z < 38) continue;
            forest.push_back({ x, 0.8f + u(fr) * 0.7f, z, u(fr) });
        }
    }

    BuildWeaponMeshes();
    NewGame();
    if (!autoshot) LoadSettings();   // タイトルの「設定」で変えた内容
    scene = SceneId::Title;
    sceneT = 0;
    if (movie) BuildMovieScript();
    else if (autoshot) BuildShotScript();
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
    c.camYaw = in.mdx * 0.0035f * setCamSens + ((in.Down(VK_RIGHT) ? 1.f : 0.f) - (in.Down(VK_LEFT) ? 1.f : 0.f)) * 2.2f * dt;
    c.atkDown = in.lmb || in.Down('J');
    c.atkPressed = in.LPressed() || in.Pressed('J');
    c.atkReleased = (!in.lmb && in.plmb) || (!in.Down('J') && in.prevKey['J']);
    c.guard = in.rmb || in.Down('K');
    c.guardPressed = in.RPressed() || in.Pressed('K');
    c.dodge = in.Pressed(VK_SPACE);
    c.dash = in.Down(VK_SHIFT);
    c.lockon = in.Pressed('Q');
    c.interact = in.Pressed('E');
    c.swap = in.Pressed(VK_TAB);
    for (int i = 0; i < ITEM_N; i++) if (in.Pressed('1' + i)) c.item = i;
    c.up = in.Pressed('W') || in.Pressed(VK_UP);
    c.down = in.Pressed('S') || in.Pressed(VK_DOWN);
    c.left = in.Pressed('A') || in.Pressed(VK_LEFT);
    c.right = in.Pressed('D') || in.Pressed(VK_RIGHT);
    c.confirm = in.Pressed(VK_RETURN) || in.Pressed('E') || in.LPressed();
    c.menu = in.Pressed(VK_ESCAPE) || in.Pressed('M');
    c.back = in.Pressed(VK_BACK);
    c.prevChar = in.Pressed('Z'); c.nextChar = in.Pressed('C');
    c.useA = in.Pressed('R'); c.useB = in.Pressed('F');
    c.lockNext = in.Pressed('R'); c.camReset = in.Pressed('V'); c.wheel = in.wheel;
    return c;
}

// ------------------------------------------------------------------
// 更新
// ------------------------------------------------------------------
void Game::Update(float dt, InputState& realIn)
{
    InputState& in = autoshot ? fakeIn : realIn;   // 撮影デモでは、台本が押すキー（fakeIn）だけを使う
    // メニューやカーソルで上下左右を押しっぱなしにすると、くり返し動く（0.35秒たってから 0.07秒ごと）
    {
        static float holdT[256] = {}, repT[256] = {};
        const int keys[] = { 'W', 'A', 'S', 'D', VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT };
        for (int k : keys)
        {
            if (realIn.key[k] && realIn.prevKey[k])
            {
                holdT[k] += dt;
                if (holdT[k] > 0.35f && (repT[k] += dt) >= 0.07f) { repT[k] = 0; realIn.prevKey[k] = false; }   // 押した瞬間として扱う
            }
            else if (!realIn.key[k]) { holdT[k] = 0; repT[k] = 0; }
        }
    }
    time += dt;
    sceneT += dt;
    if (autoshot) UpdateDirector(dt);
    if (movie)
    {
        movieCardT += dt; movieCapT += dt;
        if (movieTick) movieTick(dt);
    }

    for (auto& t : toasts) t.t += dt;
    toasts.erase(std::remove_if(toasts.begin(), toasts.end(), [](const Toast& t) { return t.t > 3.4f; }), toasts.end());

    if (!autoshot && in.Pressed(VK_ESCAPE) && scene == SceneId::Title && !settingsOpen) quit = true;
    UpdateTransition(dt);

    Controls c = autoshot ? Controls{} : ReadControls(in, dt);
    if (autoshot) c.dodge = fakeIn.Pressed(VK_SPACE);   // 撮影デモ：台本の Space で回避
    bool consumed = false;
    if (!dialog.empty() && !autoshot && !transOn)
    {
        if (in.Pressed(VK_RETURN) || in.Pressed('E') || in.LPressed() || in.Pressed(VK_SPACE))
        {
            dialog.pop_front();
            consumed = true;
        }
        if (scene != SceneId::Dungeon || consumed) { float cy = c.camYaw; c = Controls{}; c.camYaw = cy; }
    }

    switch (scene)
    {
    case SceneId::Title: if (!transOn) UpdateTitle(dt, in); break;
    case SceneId::Village: if (!consumed) UpdateVillage(dt, in); break;
    case SceneId::Interior: if (!consumed) UpdateInterior(dt, in); break;
    case SceneId::Dungeon:
        if (autopilot) c = AutoControls(dt);
        if (transOn) c = Controls{};
        UpdateDungeon(dt, c);
        break;
    case SceneId::Sheet:
        if (in.Pressed(VK_RIGHT)) sheetPage = (sheetPage + 1) % 9;
        if (in.Pressed(VK_LEFT)) sheetPage = (sheetPage + 8) % 9;
        break;
    }
    wantMouseLock = !autoshot && !transOn &&
        ((scene == SceneId::Dungeon && !exitMenu && !menuOpen && !qteOn) || (scene == SceneId::Village && vWalkCam && dialog.empty()));
    if (autoshot) fakeIn.EndFrame();
}

// タイトル：はじめから／つづきから
void Game::UpdateTitle(float dt, InputState& in)
{
    bool canLoad = HasSave();
    float W = (float)R->W;
    if (settingsOpen)
    {
        // 設定：W/S で選ぶ、A/D（←→・Enter）で変える、Esc で戻る
        const int n = 5;
        if (in.Pressed('W') || in.Pressed(VK_UP)) setSel = (setSel + n - 1) % n;
        if (in.Pressed('S') || in.Pressed(VK_DOWN)) setSel = (setSel + 1) % n;
        int d = (in.Pressed('D') || in.Pressed(VK_RIGHT) || in.Pressed(VK_RETURN) || in.Pressed(VK_SPACE)) ? 1 : (in.Pressed('A') || in.Pressed(VK_LEFT)) ? -1 : 0;
        bool back = in.Pressed(VK_ESCAPE) || in.Pressed(VK_BACK) || (setSel == 4 && (in.Pressed(VK_RETURN) || in.Pressed(VK_SPACE)));
        if (d && setSel != 4)
        {
            if (setSel == 0) setCamSens = Clampf(setCamSens + d * 0.25f, 0.5f, 2.0f);
            if (setSel == 1) setShake = !setShake;
            if (setSel == 2) setDmgNums = !setDmgNums;
            if (setSel == 3) setBigMap = !setBigMap;
            SaveSettings();
        }
        if (back) { settingsOpen = false; SaveSettings(); }
        return;
    }
    // はじめから／つづきから／設定／ゲームを終了する
    const int n = 4;
    int step = (in.Pressed('W') || in.Pressed(VK_UP)) ? -1 : (in.Pressed('S') || in.Pressed(VK_DOWN)) ? 1 : 0;
    if (step)
    {
        titleSel = (titleSel + step + n) % n;
        if (titleSel == 1 && !canLoad) titleSel = (titleSel + step + n) % n;   // セーブがなければ「つづきから」をとばす
    }
    if (titleSel == 1 && !canLoad) titleSel = 0;
    for (int i = 0; i < n; i++)
    {
        float y = 460.f + i * 52.f;
        if (in.mouseMoved && in.mouseX > W / 2 - 170 && in.mouseX < W / 2 + 170 && in.mouseY > y && in.mouseY < y + 44 && (i != 1 || canLoad)) titleSel = i;
    }
    if (in.Pressed(VK_RETURN) || in.LPressed() || in.Pressed(VK_SPACE))
    {
        if (titleSel == 2) { settingsOpen = true; setSel = 0; return; }
        if (titleSel == 3) { quit = true; return; }
        bool load = titleSel == 1 && canLoad;
        StartTransition(L"ハタオリ村", load ? L"― つづきから ―" : L"― 霧に呑まれた村 ―", [this, load] {
            if (load && LoadGame()) { EnterVillage(false); ToastMsg(L"セーブデータを読み込みました", RGBA(0.7f, 1, 0.75f)); }
            else { NewGame(); EnterVillage(false); }
        });
    }
}

// 設定（settings.txt に保存。セーブデータとは別）
std::wstring Game::SettingsPath() const
{
    if (autoshot) return shotDir + L"\\demo_settings.txt";
    return assets + L"..\\settings.txt";
}

void Game::SaveSettings()
{
    FILE* f = nullptr;
    if (_wfopen_s(&f, SettingsPath().c_str(), L"w") != 0 || !f) return;
    fprintf(f, "camsens %.2f\nshake %d\ndmgnums %d\nbigmap %d\n", setCamSens, setShake ? 1 : 0, setDmgNums ? 1 : 0, setBigMap ? 1 : 0);
    fclose(f);
}

void Game::LoadSettings()
{
    FILE* f = nullptr;
    if (_wfopen_s(&f, SettingsPath().c_str(), L"r") != 0 || !f) return;
    char tag[32]; int a = 1, b = 1, c = 0;
    if (fscanf_s(f, "%31s %f", tag, (unsigned)sizeof(tag), &setCamSens) == 2) setCamSens = Clampf(setCamSens, 0.5f, 2.f);
    if (fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &a) == 2) setShake = a != 0;
    if (fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &b) == 2) setDmgNums = b != 0;
    if (fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &c) == 2) setBigMap = c != 0;
    fclose(f);
}

void Game::DrawSettings()
{
    float W = (float)R->W, H = (float)R->H;
    R->FillRect(0, 0, W, H, RGBA(0.02f, 0.03f, 0.06f, 0.6f));
    float bw = 620, bh = 400, bx = (W - bw) / 2, by = (H - bh) / 2;
    InkPanel(bx, by, bw, bh, 0.97f);
    R->Text(L"設定", bx, by + 18, bw, 34, 26, RGBA(1, 0.92f, 0.75f), 1, true);
    wchar_t buf[64];
    swprintf_s(buf, L"%.2f 倍", setCamSens);
    const wchar_t* names[5] = { L"カメラの感度", L"画面の揺れ", L"ダメージの数字", L"ダンジョンの地図", L"もどる" };
    std::wstring vals[5] = { buf, setShake ? L"あり" : L"なし", setDmgNums ? L"表示する" : L"表示しない", setBigMap ? L"大きい" : L"ふつう", L"" };
    for (int i = 0; i < 5; i++)
    {
        float y = by + 72 + i * 56.f;
        bool sel = setSel == i;
        InkRow(bx + 40, y, bw - 80, 46, sel);
        R->Text(names[i], bx + 64, y + 10, 260, 28, 19, sel ? RGBA(0.16f, 0.12f, 0.1f) : RGBA(1, 1, 1), 0, true, !sel);
        if (i < 4)
        {
            R->Text(L"◀", bx + bw - 290, y + 10, 30, 28, 18, sel ? RGBA(0.85f, 0.5f, 0.15f) : RGBA(1, 1, 1, 0.4f), 1, true, false);
            R->Text(vals[i], bx + bw - 260, y + 10, 180, 28, 19, sel ? RGBA(0.75f, 0.35f, 0.08f) : RGBA(1, 0.85f, 0.4f), 1, true, !sel);
            R->Text(L"▶", bx + bw - 80, y + 10, 30, 28, 18, sel ? RGBA(0.85f, 0.5f, 0.15f) : RGBA(1, 1, 1, 0.4f), 1, true, false);
        }
    }
    R->Text(L"W/S：選ぶ　A/D・Enter：変える　Esc：もどる（変えた設定はすぐ保存）", bx, by + bh - 40, bw, 24, 14, RGBA(1, 1, 1, 0.65f), 1);
}

// ------------------------------------------------------------------
// 描画
// ------------------------------------------------------------------
void Game::Render()
{
    if (!thumbsReady) BuildThumbs();   // 配置メニュー用のサムネイル（最初の1回）
    switch (scene)
    {
    case SceneId::Title:
        RenderVillage(true);
        R->Begin2D();
        DrawTitleUI();
        DrawTransition();
        if (movie) DrawMovieOverlay();
        R->End2D();
        break;
    case SceneId::Village:
        if (townMenu && townWeapon)
        {
            // 村のメニューから開いた武器の画面
            RenderWeaponShowcase();
            R->Begin2D();
            DrawWeaponScreen();
            DrawToasts();
            if (movie) DrawMovieOverlay();
            R->End2D();
            break;
        }
        RenderVillage(false);
        R->Begin2D();
        DrawVillageUI();
        DrawTalkMenu();
        if (townMenu) { DrawTownMenu(); DrawTownAsk(); }
        DrawDialog();
        DrawToasts();
        DrawTransition();
        if (movie) DrawMovieOverlay();
        R->End2D();
        break;
    case SceneId::Interior:
        RenderInterior();
        R->Begin2D();
        DrawInteriorUI();
        DrawDialog();
        DrawToasts();
        DrawTransition();
        if (movie) DrawMovieOverlay();
        R->End2D();
        break;
    case SceneId::Dungeon:
        RenderDungeon();
        R->Begin2D();
        DrawDungeonUI();
        DrawDialog();
        DrawToasts();
        DrawTransition();
        if (movie) DrawMovieOverlay();
        R->End2D();
        break;
    case SceneId::Sheet:
        RenderSheet();
        R->Begin2D();
        DrawSheetUI();
        if (movie) DrawMovieOverlay();
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
    R->FillRect(W / 2 - 110, 360, 220, 3, RGBA(0.72f, 0.2f, 0.16f));
    R->Text(L"ダンジョン探索 × 町の復興　アクションRPG", 0, 380, W, 30, 20, RGBA(1, 1, 1, 0.9f), 1);
    bool canLoad = HasSave();
    const wchar_t* opts[4] = { L"はじめから", L"つづきから", L"設定", L"ゲームを終了する" };
    for (int i = 0; i < 4; i++)
    {
        float y = 460.f + i * 52.f;
        bool sel = titleSel == i;
        bool enabled = i != 1 || canLoad;
        InkRow(W / 2 - 170, y, 340, 44, sel);
        D2D1_COLOR_F c = sel ? RGBA(0.16f, 0.12f, 0.1f) : enabled ? RGBA(1, 1, 1) : RGBA(1, 1, 1, 0.35f);
        R->Text(opts[i], W / 2 - 170, y + 7, 340, 34, 21, c, 1, true, !sel);
    }
    if (!canLoad) R->Text(L"（セーブデータなし）", W / 2 + 90, 524, 200, 24, 13, RGBA(1, 1, 1, 0.6f), 0, false, true);
    R->Text(L"C++17 / DirectX 11 / HLSL", 0, H - 46, W - 24, 30, 15, RGBA(1, 1, 1, 0.7f), 2);
    if (settingsOpen) DrawSettings();
}

// ==================================================================
// UI の共通の部品（和紙と墨）
//   ゲーム全体の UI はこの部品の組み合わせで描く。色や形を変えたいときは、ここを変えれば全画面に効く
//     Bar       ：まっすぐなゲージ（敵の頭上の体力、ボスの体力、武器画面など）
//     SlantBar  ：斜めのゲージ（ダンジョン左上のアニメ風の HUD）
//     InkPanel  ：墨の板（メニューの枠、HUD の欄など）
//     InkTab    ：タブ（メニューの上の切り替え）
//     InkRow    ：選べる行（メニューの項目）
//   色の決まり：墨＝(0.09, 0.08, 0.09)　和紙＝(0.96, 0.93, 0.86)　朱＝(0.75, 0.2, 0.16)　金の縁＝(0.8, 0.64, 0.4)
// ==================================================================

// ゲージ：角ばった墨の溝に色を流し込む。上にうすいつや、長いゲージは4分の1ごとに目盛り
//   v：0〜1 の割合　fill：中身の色　back：溝の色
void Game::Bar(float x, float y, float w, float h, float v, D2D1_COLOR_F fill, D2D1_COLOR_F back)
{
    // 1px 外側に墨のふち（溝が薄いときでもふちは見えるよう、+0.15 しておく）
    R->FillRect(x - 1, y - 1, w + 2, h + 2, RGBA(0.05f, 0.04f, 0.05f, 0.75f * back.a + 0.15f));
    R->FillRect(x, y, w, h, back);
    v = Saturate(v);
    if (v > 0.001f)
    {
        R->FillRect(x, y, w * v, h, fill);
        R->FillRect(x, y, w * v, std::max(1.f, h * 0.3f), RGBA(1, 1, 1, 0.22f * fill.a));   // 上 3 割に白をうすく重ねて「つや」
    }
    // 目盛り：長くて太いゲージだけ。下半分に細い黒い線を3本（25%・50%・75%）
    if (w > 150 && h >= 6)
        for (int i = 1; i < 4; i++) R->FillRect(x + w * i / 4.f - 0.5f, y + h * 0.55f, 1.f, h * 0.45f, RGBA(0, 0, 0, 0.35f));
}

// 斜めのゲージ（平行四辺形）。アニメのゲームの HUD のように、右に傾いた形で勢いを出す
//   trail：受けたダメージを「白い帯」で遅れて見せる位置（0〜1）。v より大きいと、v〜trail のあいだが白く光る。
//          ダンジョンの HP では hpTrail を渡し、DrawDungeonUI で少しずつ v に近づけている
void Game::SlantBar(float x, float y, float w, float h, float v, D2D1_COLOR_F fill, D2D1_COLOR_F back, float trail)
{
    float sk = h * 0.6f;   // 傾き：上の辺が下の辺より sk だけ右にずれる（高さに合わせて、細いゲージは少しだけ傾く）
    // x0〜x1、y0〜y1 の範囲を、同じ傾きの平行四辺形で塗る。
    // y が上にあるほど右へずらすので、高さの一部（上のつやなど）だけ塗っても、傾きがゲージ全体とそろう
    auto par = [&](float x0, float x1, float y0, float y1, D2D1_COLOR_F c) {
        if (x1 <= x0) return;
        float s0 = sk * (y1 - y) / h, s1 = sk * (y0 - y) / h;   // その高さでのずれ（下ほど大きく引く＝左へ）
        D2D1_POINT_2F p[4] = { { x0 + sk - s1, y0 }, { x1 + sk - s1, y0 }, { x1 + sk - s0, y1 }, { x0 + sk - s0, y1 } };
        R->FillPoly(p, 4, c);
    };
    v = Saturate(v);
    // 墨のふち：本体より上下 2px・左右 2〜2.5px 大きい平行四辺形を先に塗る
    D2D1_POINT_2F o[4] = { { x + sk - 1.5f, y - 2 }, { x + w + sk + 2.5f, y - 2 }, { x + w + 1.5f, y + h + 2 }, { x - 2.5f, y + h + 2 } };
    R->FillPoly(o, 4, RGBA(0.04f, 0.03f, 0.04f, 0.85f));
    par(x, x + w, y, y + h, back);                                                                     // 溝
    if (trail > v) par(x + w * v, x + w * Saturate(trail), y, y + h, RGBA(1, 0.95f, 0.85f, 0.85f));   // 減ったぶんの白い帯
    par(x, x + w * v, y, y + h, fill);                                                                 // 中身
    par(x, x + w * v, y, y + h * 0.35f, RGBA(1, 1, 1, 0.28f * fill.a));                               // 上のつや
}

// 墨の板：左上と右下の角を斜めに切った六角形。落ち影 → 墨 → 金の細い縁 →（大きい板だけ）赤い糸の縫い目
//   a：不透明さ。0.5 より薄い板には落ち影をつけない（うすい板に影があると濁って見えるため）
void Game::InkPanel(float x, float y, float w, float h, float a)
{
    float k = std::min(12.f, std::min(w, h) * 0.22f);   // 角を切る大きさ（小さい板では小さく）
    // ox, oy：ずらす量（落ち影用）　in：内側へ縮める量（縫い目用）。縮めたぶん、角の切り方も少し小さくする
    auto poly = [&](float ox, float oy, float in, D2D1_POINT_2F* p) {
        float x0 = x + in + ox, y0 = y + in + oy, x1 = x + w - in + ox, y1 = y + h - in + oy, kk = std::max(2.f, k - in * 0.5f);
        // 左上の切り口 → 右上 → 右下の切り口 → 左下 → 左上の切り口、の順
        p[0] = { x0 + kk, y0 }; p[1] = { x1, y0 }; p[2] = { x1, y1 - kk }; p[3] = { x1 - kk, y1 }; p[4] = { x0, y1 }; p[5] = { x0, y0 + kk };
    };
    D2D1_POINT_2F p[6];
    if (a > 0.5f) { poly(4, 4, 0, p); R->FillPoly(p, 6, RGBA(0, 0, 0, 0.25f * a)); }   // 右下へ 4px の落ち影
    poly(0, 0, 0, p);
    R->FillPoly(p, 6, RGBA(0.09f, 0.08f, 0.09f, a));                                    // 墨
    R->StrokePoly(p, 6, RGBA(0.8f, 0.64f, 0.4f, 0.5f * a), 1.f);                        // 金の縁
    // 赤い糸の縫い目：小さい板（HUD の細い欄など）に入れるとうるさいので、大きい板だけ
    if (h >= 90 && w >= 120) { poly(0, 0, 6, p); R->StrokePoly(p, 6, RGBA(0.78f, 0.26f, 0.2f, 0.55f * a), 1.3f, true); }
}

// タブ：右に傾いた平行四辺形。選んでいるタブは朱、ほかは墨。文字は呼ぶ側で上に書く
void Game::InkTab(float x, float y, float w, float h, bool on)
{
    float sk = h * 0.3f;   // 傾き
    D2D1_POINT_2F p[4] = { { x + sk, y }, { x + w, y }, { x + w - sk, y + h }, { x, y + h } };
    R->FillPoly(p, 4, on ? RGBA(0.72f, 0.2f, 0.16f, 0.97f) : RGBA(0.09f, 0.08f, 0.09f, 0.7f));
    R->StrokePoly(p, 4, on ? RGBA(0.98f, 0.85f, 0.65f, 0.8f) : RGBA(0.8f, 0.64f, 0.4f, 0.35f), 1.f);
}

// 選べる行（メニューの項目の背景）
//   選んでいる行：和紙の札（角を切った形・墨の細い縁・左に朱の印）。文字は呼ぶ側で墨色にする
//   ほかの行　　：ほぼ透明な地と、下に点線の区切りだけ（墨の板の上に並べる前提）
void Game::InkRow(float x, float y, float w, float h, bool sel)
{
    if (sel)
    {
        float k = std::min(8.f, h * 0.25f);
        D2D1_POINT_2F p[6] = { { x + k, y }, { x + w, y }, { x + w, y + h - k }, { x + w - k, y + h }, { x, y + h }, { x, y + k } };
        R->FillPoly(p, 6, RGBA(0.96f, 0.93f, 0.86f, 0.97f));
        R->StrokePoly(p, 6, RGBA(0.3f, 0.22f, 0.17f, 0.8f), 1.f);
        R->FillRect(x + 4, y + h * 0.2f, 5, h * 0.6f, RGBA(0.75f, 0.2f, 0.16f));   // 朱の印（行の高さの 6 割）
    }
    else
    {
        R->FillRect(x, y, w, h, RGBA(1, 1, 1, 0.045f));
        R->DashedLine(x + 10, y + h - 1, x + w - 10, y + h - 1, RGBA(0.85f, 0.75f, 0.6f, 0.18f), 1.f);
    }
}

void Game::DrawDialog()
{
    if (dialog.empty() || transOn) return;
    float W = (float)R->W, H = (float)R->H;
    const DialogLine& d = dialog.front();
    // 会話の囲い：和紙の札。角を斜めに切った紙に、赤い糸の縫い目。文字は墨色の明朝体
    //   左から「顔（家紋のような二重の輪）｜区切りの線｜セリフ」、上に名前の札、右下に「次へ」の針
    //   配置モードの村では下に操作の帯が出るので、そのぶん上に置く（150）
    float bw = 880, bh = 132, bx = (W - bw) / 2, by = H - bh - (scene == SceneId::Village && !vWalkCam ? 150.f : 38.f);
    const float cut = 16;   // 角を切る大きさ
    // 枠の形（InkPanel と同じ六角形）。ox, oy：ずらす（落ち影）　in：内側へ縮める（縫い目）
    auto frame = [&](float ox, float oy, float in, D2D1_POINT_2F* p) {
        float x0 = bx + in + ox, y0 = by + in + oy, x1 = bx + bw - in + ox, y1 = by + bh - in + oy, k = std::max(2.f, cut - in * 0.6f);
        p[0] = { x0 + k, y0 }; p[1] = { x1, y0 }; p[2] = { x1, y1 - k }; p[3] = { x1 - k, y1 }; p[4] = { x0, y1 }; p[5] = { x0, y0 + k };
    };
    D2D1_POINT_2F pp[6];
    frame(7, 8, 0, pp);  R->FillPoly(pp, 6, RGBA(0.02f, 0.02f, 0.04f, 0.38f));          // 落ち影
    frame(0, 0, 0, pp);  R->FillPoly(pp, 6, RGBA(0.96f, 0.93f, 0.86f, 0.97f));          // 和紙
    R->FillGradientRect(bx + 110, by + 2, bw - 112, 22, RGBA(1, 1, 1, 0.0f), RGBA(1, 1, 1, 0.35f));   // 紙のつや
    // 和紙の繊維：うすい茶色の短い線を26本。位置は i から計算する（乱数を使わない）ので、
    // 毎フレーム同じ場所に描かれてちらつかない。fmod で枠の中に収める
    for (int i = 0; i < 26; i++)
    {
        float fx = bx + 20 + std::fmod(i * 137.3f, bw - 60), fy = by + 14 + std::fmod(i * 53.7f, bh - 28);
        float len = 10 + std::fmod(i * 7.1f, 18.f), ang = std::fmod(i * 0.83f, 3.14f);
        R->Line(fx, fy, fx + std::cos(ang) * len, fy + std::sin(ang) * len * 0.4f, RGBA(0.55f, 0.42f, 0.3f, 0.10f), 1.f);
    }
    frame(0, 0, 0, pp);  R->StrokePoly(pp, 6, RGBA(0.25f, 0.18f, 0.14f, 0.9f), 1.5f);     // 外の墨線
    frame(0, 0, 9, pp);  R->StrokePoly(pp, 6, RGBA(0.72f, 0.18f, 0.16f, 0.85f), 1.8f, true);   // 赤い糸の縫い目

    // 話者の顔：細い二重の輪（家紋のような枠）
    //   ソラ・クロベエ・カイリは DrawFaceIcon の顔。ほかの人は色の丸に、目などをここで描き足す
    float cx = bx + 72, cy = by + bh * 0.5f + 2;
    int face = d.who == L"ソラ" ? 0 : d.who == L"クロベエ" ? 1 : d.who == L"カイリ" ? 2 : -1;
    D2D1_COLOR_F ic = d.who == L"ツムギ" ? RGBA(0.98f, 0.95f, 0.85f) : d.who == L"ナギ" ? RGBA(0.78f, 0.76f, 0.9f) :
                      d.who == L"トウジ" ? RGBA(0.55f, 0.38f, 0.22f) : d.who == L"タヌキの商人" ? RGBA(0.55f, 0.42f, 0.3f) : RGBA(0.2f, 0.3f, 0.6f);
    R->FillCircle(cx, cy, 45, RGBA(0.88f, 0.83f, 0.72f));
    if (face >= 0) DrawFaceIcon(face, cx, cy, 40);
    else R->FillCircle(cx, cy, 40, ic);
    if (d.who == L"ツムギ")
    {
        R->FillCircle(cx - 12, cy - 2, 5, RGBA(0.1f, 0.1f, 0.15f));
        R->FillCircle(cx + 12, cy - 2, 5, RGBA(0.1f, 0.1f, 0.15f));
        R->FillCircle(cx - 18, cy + 10, 5, RGBA(1, 0.6f, 0.65f, 0.7f));
        R->FillCircle(cx + 18, cy + 10, 5, RGBA(1, 0.6f, 0.65f, 0.7f));
        R->Line(cx - 10, cy - 34, cx - 22, cy - 52, RGBA(0.8f, 0.6f, 0.3f), 2.5f);
        R->Line(cx + 10, cy - 34, cx + 22, cy - 52, RGBA(0.8f, 0.6f, 0.3f), 2.5f);
    }
    else if (d.who == L"トウジ")
    {
        R->FillCircle(cx - 12, cy - 4, 11, RGBA(1, 0.95f, 0.85f)); R->FillCircle(cx + 12, cy - 4, 11, RGBA(1, 0.95f, 0.85f));
        R->FillCircle(cx - 12, cy - 4, 5, RGBA(0.95f, 0.55f, 0.1f)); R->FillCircle(cx + 12, cy - 4, 5, RGBA(0.95f, 0.55f, 0.1f));
    }
    else if (d.who == L"ナギ")
    {
        R->FillCircle(cx, cy + 6, 24, RGBA(0.98f, 0.88f, 0.8f));
        R->Line(cx - 12, cy + 6, cx - 4, cy + 6, RGBA(0.3f, 0.25f, 0.35f), 2);
        R->Line(cx + 4, cy + 6, cx + 12, cy + 6, RGBA(0.3f, 0.25f, 0.35f), 2);
    }
    R->StrokeCircle(cx, cy, 40, RGBA(0.22f, 0.16f, 0.13f), 1.8f);
    R->StrokeCircle(cx, cy, 45, RGBA(0.72f, 0.18f, 0.16f, 0.8f), 1.2f);
    // 区切りの縦線（墨）
    R->Line(bx + 132, by + 22, bx + 132, by + bh - 22, RGBA(0.3f, 0.22f, 0.18f, 0.35f), 1.f);

    // 名前の札：墨の帯（右へ傾いた平行四辺形）と、赤い判
    //   幅は名前の文字数から決める（全角1文字 ≒ 21px ＋ 余白 26px）。枠の上辺にまたがるよう、by より 20px 上に置く
    float nx = bx + 148, ny = by - 20, nw = 26.f + 21.f * (float)d.who.size(), nh = 36, sk = 12;
    D2D1_POINT_2F tag[4] = { { nx + sk, ny }, { nx + nw + sk, ny }, { nx + nw, ny + nh }, { nx, ny + nh } };
    D2D1_POINT_2F tagS[4]; for (int i = 0; i < 4; i++) tagS[i] = { tag[i].x + 4, tag[i].y + 4 };
    R->FillPoly(tagS, 4, RGBA(0, 0, 0, 0.3f));
    R->FillPoly(tag, 4, RGBA(0.12f, 0.1f, 0.11f, 0.97f));
    R->FillRect(nx + 4, ny + 9, 6, 18, RGBA(0.78f, 0.2f, 0.16f));
    R->Text(d.who, nx + 14, ny + 4, nw, 30, 19, RGBA(0.98f, 0.95f, 0.88f), 0, true, false, 1);

    // セリフ（墨色・明朝）
    R->Text(d.text, bx + 150, by + 30, bw - 200, bh - 40, 21, RGBA(0.16f, 0.12f, 0.1f), 0, true, false, 1);
    // 次へ：糸のついた針がゆれる（▼ の代わり）。赤い糸の線と、ひし形の針
    float bob = std::sin(time * 4) * 3;
    float ax = bx + bw - 42, ay = by + bh - 28 + bob;
    R->Line(ax - 22, ay - 4, ax - 2, ay + 2, RGBA(0.72f, 0.18f, 0.16f, 0.9f), 1.5f);
    D2D1_POINT_2F nd[4] = { { ax, ay - 2 }, { ax + 14, ay + 4 }, { ax, ay + 7 }, { ax + 3, ay + 3 } };
    R->FillPoly(nd, 4, RGBA(0.2f, 0.16f, 0.15f));
}

// 操作のヒント：墨の帯（両端を斜めに切る）＋下に縫い目
//   「[E] 調べる」など、近くで何かできるときに出す。ダンジョン・町・建物の中で共通
//   cx：帯のまん中の x　y：帯の上の y　w：帯の幅
void Game::DrawInkHint(const std::wstring& s, float cx, float y, float w)
{
    float x0 = cx - w / 2, x1 = cx + w / 2, h = 38, sk = 14;
    D2D1_POINT_2F p[4] = { { x0 + sk, y }, { x1, y }, { x1 - sk, y + h }, { x0, y + h } };
    D2D1_POINT_2F sh[4]; for (int i = 0; i < 4; i++) sh[i] = { p[i].x + 4, p[i].y + 4 };
    R->FillPoly(sh, 4, RGBA(0, 0, 0, 0.3f));
    R->FillPoly(p, 4, RGBA(0.1f, 0.09f, 0.1f, 0.88f));
    R->DashedLine(x0 + sk + 4, y + h - 6, x1 - sk - 4, y + h - 6, RGBA(0.85f, 0.65f, 0.35f, 0.7f), 1.3f);
    R->Text(s, x0, y + 6, w, 28, 17, RGBA(0.98f, 0.94f, 0.85f), 1, true, false);
}

void Game::DrawToasts()
{
    // お知らせ：左から滑り込む墨の帯。左端の色の縦帯でお知らせの種類がわかる
    //   色は ToastMsg に渡した色（黄＝よいこと、赤＝失敗など）。上から順に 46px ずつ積む
    float W = (float)R->W;
    float y = 150;
    for (auto& t : toasts)
    {
        // a：不透明さ。出てから 0.2 秒でふわっと出て、2.8〜3.4 秒で消える
        float a = t.t < 0.2f ? t.t / 0.2f : (t.t > 2.8f ? Saturate((3.4f - t.t) / 0.6f) : 1.f);
        // slide：出てから 0.25 秒で、左 40px の位置から元の位置へ滑り込む
        float slide = (1 - SmoothStep(0.f, 0.25f, t.t)) * -40.f;
        float w = 640, h = 38, sk = 12, x0 = (W - w) / 2 + slide, x1 = x0 + w;
        D2D1_POINT_2F p[4] = { { x0 + sk, y }, { x1, y }, { x1 - sk, y + h }, { x0, y + h } };
        R->FillPoly(p, 4, RGBA(0.1f, 0.09f, 0.1f, 0.84f * a));
        D2D1_COLOR_F c = t.col; c.a = a;
        D2D1_POINT_2F bar[4] = { { x0 + sk, y }, { x0 + sk + 7, y }, { x0 + 7, y + h }, { x0, y + h } };
        R->FillPoly(bar, 4, c);
        D2D1_COLOR_F st = t.col; st.a = 0.45f * a;
        R->DashedLine(x0 + 24, y + h - 5, x1 - sk - 10, y + h - 5, st, 1.2f);
        R->Text(t.text, x0 + 10, y + 6, w - 20, 30, 18, RGBA(0.98f, 0.95f, 0.88f, a), 1, true, false);
        y += 46;
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
    a.pos = p; a.yaw = yaw; a.st = ASt::Normal;
    XMFLOAT3 right{ std::cos(yaw), 0, -std::sin(yaw) };
    int slot = 0;
    for (int i = 0; i < 3; i++)
    {
        if (i == active || !joined[i]) continue;
        XMFLOAT3 bp = Add(Sub(p, Mul(dir, 1.2f)), Mul(right, slot == 0 ? 1.6f : -1.6f));
        party[i].pos = Solid(bp.x, bp.z) ? p : bp;
        party[i].yaw = yaw; party[i].st = ASt::Normal;
        slot++;
    }
    camYaw = yaw;
    camPosNow = Add(p, F3(-dir.x * 7, 4.5f, -dir.z * 7));
    camTargetNow = Add(p, F3(0, 1.2f, 0));
}

// 敵が一番多い部屋へ操作キャラを移動させて戦闘を始める（撮影用）
void Game::SetupCombat()
{
    int best = -1, bestN = 0;
    for (int r = 0; r < (int)rooms.size(); r++)
    {
        int n = 0;
        for (auto& e : enemies) if (e.st != ESt::Dead && RoomAt(e.pos) == r) n++;
        if (n > bestN && r != 0) { bestN = n; best = r; }
    }
    if (best < 0) return;
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

void Game::BuildShotScript()
{
    steps.clear();
    DeleteFileW(SavePath().c_str());   // 撮影デモはセーブなしから始める
    DeleteFileW((shotDir + L"\\dungeon_test.txt").c_str());
    auto S = [this](float wait, std::function<void()> act, const wchar_t* shot = L"") { steps.push_back({ wait, act, shot }); };
    auto logD = [this](const char* s) {
        FILE* f = nullptr;
        if (_wfopen_s(&f, (shotDir + L"\\dungeon_test.txt").c_str(), L"a") == 0 && f) { fprintf(f, "%s\n", s); fclose(f); }
    };
    auto partyFresh = [this]() {
        for (int i = 0; i < 3; i++) { party[i].hp = party[i].maxHp; party[i].st = ASt::Normal; party[i].dur = party[i].maxDur; party[i].broken = false; }
        arriveT = 9; dialog.clear(); toasts.clear();
    };

    // ---- タイトル〜パーツ0からのオープニング
    S(1.6f, nullptr, L"01_title");
    S(0.1f, [this] { titleSel = 2; settingsOpen = true; setSel = 0; });
    S(0.3f, nullptr, L"01b_title_settings");
    S(0.1f, [this] { settingsOpen = false; titleSel = 3; });
    S(0.3f, nullptr, L"01c_title_quit_option");
    S(0.1f, [this] { NewGame(); EnterVillage(false); });
    S(1.5f, nullptr, L"02_opening_event");
    // ---- 入口へダッシュ（町では武器をしまっている）
    S(0.1f, [this] {
        dialog.clear();
        party[0].pos = F3(-0.5f, 0, 4.0f); party[0].yaw = 0; camYaw = 0.25f;
        camPosNow = Add(party[0].pos, F3(-1.5f, 4.5f, -6.5f)); camTargetNow = Add(party[0].pos, F3(0, 1.3f, 0));
        autoWalkOn = true; autoWalk = F3(-0.4f, 0, 21.f); autoDash = true;
    });
    S(1.4f, nullptr, L"03_dash_to_entrance");
    // ---- 入口の前
    S(0.1f, [this] {
        shopOpen = false; scene = SceneId::Village; vWalkCam = true; autoWalkOn = false; autoDash = false;
        EnterTownWalk(true);
        party[0].yaw = 0; camYaw = 0.3f;
        party[0].pos = Sub(kEntrancePos, F3(0.3f, 0, 2.4f));
        camPosNow = Add(party[0].pos, F3(-2.4f, 4.0f, -6.5f)); camTargetNow = Add(party[0].pos, F3(0, 1.3f, 0));
    });
    S(0.8f, nullptr, L"05_entrance");
    // ---- 入口で入る階を選ぶメニュー（7階まで行ったことがある想定で表示）
    S(0.1f, [this] { toasts.clear(); talkHint.clear(); deepest = 7; floorLoot = (1u << 1) | (1u << 2) | (1u << 3); floorGot[1] = floorGot[2] = floorGot[3] = 0xFF; floorGot[4] = 0x03; floorGot[5] = 0x01; floorMenu = true; floorSel = 4; });
    S(0.3f, nullptr, L"05b_floor_select");
    // ---- 入る演出（入口へ歩いて入り、暗転して題字）
    S(0.1f, [this] {
        deepest = 0; floorLoot = 0; floorMenu = false; for (auto& g : floorGot) g = 0;
        entering = true;
        StartTransition(L"根の洞", L"1階　― 浅層", [this] { entering = false; EnterDungeon(1, 777); });
    });
    S(1.25f, nullptr, L"06_enter_transition");
    // 暗転が明けると上から降り立つ
    S(1.75f, nullptr, L"07_dungeon_arrival");
    S(1.0f, nullptr, L"08_floor1_event");
    S(0.1f, [this] { dialog.clear(); for (int x = 0; x < DN; x++) for (int z = 0; z < DN; z++) seen[x][z] = true; });
    S(0.2f, nullptr, L"08b_big_map_10_spools");
    // ---- メニュー：目的
    S(0.1f, [this] { dialog.clear(); menuOpen = true; menuTab = 3; menuSel = 1; objective = 1; });
    S(0.2f, nullptr, L"09_menu_objective");
    // ---- 宝箱3種
    S(0.1f, [this] {
        menuOpen = false;
        Actor& a = party[active];
        XMFLOAT3 f = Fwd(a.yaw), r{ std::cos(a.yaw), 0, -std::sin(a.yaw) };
        auto put = [&](int kind, float fx2, float rx) {
            XMFLOAT3 p = Add(a.pos, Add(Mul(f, fx2), Mul(r, rx)));
            if (Solid(p.x, p.z)) return;
            if (kind == 2) { for (auto& ch : chests) if (ch.kind == 2 && !ch.opened) { ch.pos = p; ch.yaw = a.yaw + PI; return; } }   // この階の糸巻きを1つ手前に寄せて見せる
            chests.push_back({ p, a.yaw + PI, false, 0, kind });
        };
        put(0, 3.0f, -2.4f); put(1, 3.6f, 0.f); put(2, 3.0f, 2.4f);
    });
    S(0.4f, nullptr, L"10_chests");
    S(0.1f, [this] { for (auto& ch : chests) if (ch.kind == 2 && !ch.opened && DistXZ(ch.pos, party[active].pos) < 5) { OpenChest(ch); break; } });
    S(0.45f, nullptr, L"11_spool_open");
    // ---- 1階の糸巻きを全部開けて、全部そろうか確かめる（糸巻き1つにパーツ1つ）
    S(0.1f, [this] {
        int spools = 0, opened = 0;
        for (auto& ch : chests) if (ch.kind == 2 && ch.loot >= 0) spools++;
        int before = crystalsThisRun;
        for (auto& ch : chests) if (ch.kind == 2 && !ch.opened) { OpenChest(ch); opened++; }
        FILE* f = nullptr;
        if (_wfopen_s(&f, (shotDir + L"\\spool_test.txt").c_str(), L"w") == 0 && f)
        {
            fprintf(f, "floor1 loot total=%d, spool chests spawned on entry=%d, opened now=%d, parts gained %d -> %d, left=%d, floor complete=%d\n",
                    LootTotal(1), spools, opened, before, crystalsThisRun, LootLeft(1), (floorLoot >> 1) & 1u);
            fclose(f);
        }
        // もう一度1階に入ると糸巻きは出ない
        int again = 0;
        EnterDungeon(1, 778);
        for (auto& ch : chests) if (ch.kind == 2) again++;
        if (_wfopen_s(&f, (shotDir + L"\\spool_test.txt").c_str(), L"a") == 0 && f) { fprintf(f, "re-enter floor1: spool chests=%d\n", again); fclose(f); }
        arriveT = 9; party[active].pos.y = 0; dialog.clear();
    });
    S(0.6f, nullptr, L"11b_all_spools_collected");
    // ---- ソラの戦闘と回転攻撃
    S(0.1f, [this, partyFresh] { partyFresh(); EnterDungeon(2, 2024); arriveT = 9; eventsDone |= EV_F2; SetupCombat(); autopilot = true; });
    S(1.5f, nullptr, L"12_combat_sora");
    S(0.1f, [this] { autoSpecial = 0.75f; });
    S(1.05f, nullptr, L"13_sora_spin");
    // ロックオン：Q で敵を捕まえ、カメラが敵の方を向いて両方を画面に入れる
    S(0.1f, [this] {
        autopilot = false; autoSpecial = 0;
        Actor& a = party[active]; a.st = ASt::Normal;
        int best = -1; float bd = 1e9f;
        for (int i = 0; i < (int)enemies.size(); i++) if (enemies[i].st != ESt::Dead) { float d = DistXZ(enemies[i].pos, a.pos); if (d < bd) { bd = d; best = i; } }
        lockIdx = best;
        if (best >= 0) camYaw = WrapAngle(YawOf(Sub(enemies[best].pos, a.pos)) + 1.2f);   // わざとずらして、カメラが向き直るのを見せる
    });
    S(0.7f, nullptr, L"13b_lockon_camera");
    // ---- 2階：霧の巫女ナギ
    S(0.1f, [this, partyFresh] { autopilot = false; autoSpecial = 0; partyFresh(); eventsDone &= ~EV_F2; EnterDungeon(2, 31); arriveT = 1.0f; party[active].pos.y = 0; });
    S(1.3f, nullptr, L"14_floor2_nagi");
    // ---- 5階：ほつれ蜘蛛
    S(0.1f, [this, partyFresh] { partyFresh(); EnterDungeon(5, 5); arriveT = 1.0f; party[active].pos.y = 0; });
    S(0.6f, [this] { dialog.clear(); autopilot = true; });
    S(2.8f, nullptr, L"15_floor5_boss");
    // ---- 5階：ボスを倒すと武器と鍵の宝箱（ボスの階は宝箱・糸巻き・ふつうの敵なし）
    S(0.1f, [this, logD] {
        autopilot = false;
        int chestsBefore = 0, spools = 0, others = 0;
        for (auto& ch : chests) { chestsBefore++; if (ch.kind == 2) spools++; }
        for (auto& e : enemies) if (e.st != ESt::Dead && !IsBossType(e.t)) others++;
        Enemy* sp = nullptr;
        for (auto& e : enemies) if (e.t == EType::Spider) sp = &e;
        if (sp && sp->st != ESt::Dead) KillEnemy(*sp, &party[active]);
        int keyChest = 0; for (auto& ch : chests) if (ch.key) keyChest++;
        char b[200]; sprintf_s(b, "floor5: chests at start=%d spools=%d (boss floor: 0), summoned minions alive=%d, key+weapon chest after boss=%d", chestsBefore, spools, others, keyChest);
        logD(b);
        for (auto& ch : chests) if (ch.key && !ch.opened) { party[active].pos = Add(ch.pos, F3(0, 0, -1.6f)); OpenChest(ch); }
        sprintf_s(b, "floor5: opened boss chest -> hasKey=%d", hasKey ? 1 : 0);
        logD(b);
    });
    S(0.8f, nullptr, L"15b_floor5_key_chest");
    // ---- 操作キャラの決まった階（3階＝ソラ、7階＝クロベエ、14階＝カイリ）とリミテッド層
    S(0.1f, [this, logD, partyFresh] {
        partyFresh();
        bool j1 = joined[1], j2 = joined[2];
        joined[1] = false; joined[2] = false;
        std::wstring b7 = FloorBlock(7), b14 = FloorBlock(14);
        joined[1] = joined[2] = true;
        active = 2; EnterDungeon(3, 33); int a3 = active; int n3 = NextMember(active);
        EnterDungeon(7, 77); int a7 = active;
        EnterDungeon(14, 1414); int a14 = active; float wm = wearMul;
        Actor& A = party[active]; float d0 = A.dur; WeaponWear(A, 2); float used = d0 - A.dur;
        joined[1] = j1; joined[2] = j2;
        char b[240]; sprintf_s(b, "locks: floor7 blocked without kurobee=%d, floor14 blocked without kairi=%d, floor3 active=%d (0=sora) swap allowed=%d, floor7 active=%d (1=kurobee), floor14 active=%d (2=kairi) wearMul=%.0f wear for 2 hits=%.0f",
                               b7.empty() ? 0 : 1, b14.empty() ? 0 : 1, a3, n3 >= 0 ? 1 : 0, a7, a14, wm, used);
        logD(b);
        arriveT = 1.0f; party[active].pos.y = 0; dialog.clear();
    });
    S(1.2f, nullptr, L"15c_floor14_kairi_limited");
    // ---- 12階：マップのまん中へ行くと、うつろが現れて QTE
    S(0.1f, [this, partyFresh] {
        autopilot = false; partyFresh(); active = 0;
        EnterDungeon(12, 1212); arriveT = 9; party[active].pos.y = 0;
        XMFLOAT3 d = NormXZ(Sub(eventPos, party[active].pos));
        party[active].pos = Sub(eventPos, Mul(d, 4.0f));
        if (Solid(party[active].pos.x, party[active].pos.z)) party[active].pos = Add(eventPos, F3(0, 0, -3.5f));
        party[active].yaw = YawOf(d); camYaw = party[active].yaw;
        for (auto& e : enemies) if (!IsBossType(e.t)) e.pos = Add(e.pos, F3(0, 0, 0));
    });
    S(1.4f, nullptr, L"16_floor12_utsuro_appears");
    S(0.1f, [this] { dialog.clear(); autoQTE = false; });
    S(0.75f, nullptr, L"17_qte_prompt");
    // 2〜4つ目の合図の表示も撮る（キーの文字が1行に収まっているかの確認）。撮ったら1つ目に戻して、いつもの流れへ
    S(0.1f, [this] { qteStep = 1; qteT = 0; qteMash = 3; qteFailT = 0; });
    S(0.4f, nullptr, L"17b_qte_mash");
    S(0.1f, [this] { qteStep = 2; qteT = 0; qteMash = 0; });
    S(0.4f, nullptr, L"17c_qte_e");
    S(0.1f, [this] { qteStep = 3; qteT = 0; qteMash = 0; });
    S(0.4f, nullptr, L"17d_qte_rclick");
    S(0.1f, [this] { qteStep = 0; qteT = 0; qteMash = 0; qteFailT = 0; });
    S(0.1f, [this] { autoQTE = true; });
    S(4.2f, [this, logD] {
        int spools = 0; for (auto& ch : chests) if (ch.kind == 2 && !ch.opened) spools++;
        bool kairi = false; for (auto& ch : chests) if (ch.kind == 2 && ch.loot == 0) kairi = true;
        char b[160]; sprintf_s(b, "floor12: QTE done=%d, spool appeared after QTE=%d (kairi=%d)", Done(EV_F12) ? 1 : 0, spools, kairi ? 1 : 0);
        logD(b);
    }, L"18_qte_clear_kairi_spool");
    // 12階にもう一度入る：QTE は起きず、出口は開いている（敵も宝箱もない）
    S(0.1f, [this, logD] {
        autoQTE = false; dialog.clear();
        char b0[120]; sprintf_s(b0, "floor12 after QTE: hasKey=%d", hasKey ? 1 : 0); logD(b0);
        EnterDungeon(12, 1213); arriveT = 9; party[active].pos = Add(eventPos, F3(0, 0, -1.f));
    });
    S(0.5f, [this, logD] {
        int normal = 0; for (auto& ch : chests) if (ch.kind != 2) normal++;
        char b[200]; sprintf_s(b, "floor12 revisit: standing at center -> qte=%d utsuro=%d, enemies=%d, normal chests=%d, exit open(hasKey)=%d",
                               qteOn ? 1 : 0, (int)std::count_if(enemies.begin(), enemies.end(), [](const Enemy& e) { return e.t == EType::Utsuro; }), (int)enemies.size(), normal, hasKey ? 1 : 0);
        logD(b);
    }, L"18b_floor12_revisit_no_qte");
    // ---- 15階：うつろとの戦い（4本の糸の柱を全部壊さないと攻撃が効かない）
    S(0.1f, [this, partyFresh] { autoQTE = false; partyFresh(); EnterDungeon(15, 1515); arriveT = 1.0f; party[active].pos.y = 0; });
    S(0.6f, [this] { dialog.clear(); autopilot = true; });
    S(2.0f, nullptr, L"19_floor15_utsuro");
    // 突進を柱にぶつけさせる
    S(0.1f, [this, logD] {
        autopilot = false;
        Enemy* u = nullptr; for (auto& e : enemies) if (e.t == EType::Utsuro) u = &e;
        if (!u) return;
        float hp0 = u->hp;
        HitEnemy(*u, 100, party[active].pos, 0, false, &party[active]);
        float shielded = hp0 - u->hp;
        Pillar& p = pillars[0];
        u->pos = Add(p.pos, F3(4.5f, 0, 0)); u->dashDir = F3(-1, 0, 0); u->yaw = YawOf(u->dashDir);
        u->st = ESt::Attack; u->stT = 0; u->pattern = 1; u->hitDone = true;
        party[active].pos = Add(p.pos, F3(1.5f, 0, -5.f));
        camYaw = 0;
        char b[160]; sprintf_s(b, "floor15: utsuro maxHp=%.0f, 100 dmg with 4 pillars -> %.0f (immune)", u->maxHp, shielded);
        logD(b);
    });
    S(0.5f, [this, logD] {
        Enemy* u = nullptr; for (auto& e : enemies) if (e.t == EType::Utsuro) u = &e;
        int alive = 0; for (auto& p : pillars) if (p.alive) alive++;
        float hp0 = u ? u->hp : 0;
        if (u) HitEnemy(*u, 100, party[active].pos, 0, false, &party[active]);
        float hit3 = u ? hp0 - u->hp : 0.f;
        // 主人公の攻撃でも柱を壊せる：残りの柱を攻撃で壊す
        Actor& A = party[active];
        for (auto& p : pillars) if (p.alive) { A.pos = Add(p.pos, F3(0, 0, -1.8f)); A.yaw = 0; for (int k = 0; k < 6 && p.alive; k++) DamagePillars(A.pos, A.yaw, 2.9f, 150.f, 20.f); }
        int left = 0; for (auto& p : pillars) if (p.alive) left++;
        hp0 = u ? u->hp : 0;
        if (u) HitEnemy(*u, 100, party[active].pos, 0, false, &party[active]);
        char b[260]; sprintf_s(b, "floor15: charge into pillar -> pillars left=%d stunned=%d, 100 dmg with 3 pillars -> %.0f; after breaking the rest by attacks pillars=%d, 100 dmg -> %.0f",
                               alive, u && u->pattern == 9 ? 1 : 0, hit3, left, u ? hp0 - u->hp : 0.f);
        logD(b);
    }, L"19b_utsuro_stunned_pillar_broken");
    // 守りが消えて時間がたつと、柱が紡ぎなおされる
    S(0.1f, [this] { shieldDownT = 17.95f; });
    S(0.6f, [this, logD] {
        int alive = 0; for (auto& p : pillars) if (p.alive) alive++;
        char b[120]; sprintf_s(b, "floor15: 18s after shield down -> pillars regrown=%d", alive);
        logD(b);
        sprintf_s(b, "dodge: motion sora=%.2fs kurobee=%.2fs kairi=%.2fs (original), invulnerable %.2f/%.2f/%.2fs (+1.0s)", DodgeDur(0), DodgeDur(1), DodgeDur(2), DodgeInvuln(0), DodgeInvuln(1), DodgeInvuln(2));
        logD(b);
    }, L"19b2_pillars_regrow");
    S(0.1f, [this] { autopilot = true; });
    S(1.5f, nullptr, L"19c_floor15_fight");
    // 深い階の新しい敵（影法師・根の鎧武者・糸喰い蛾）
    S(0.1f, [this, partyFresh] { autopilot = false; partyFresh(); active = 0; EnterDungeon(11, 1111); arriveT = 9; party[active].pos.y = 0; SetupCombat(); autopilot = true; });
    S(1.4f, nullptr, L"19d_deep_enemies");
    // 会話している間は、敵は動かず攻撃もしない
    S(0.1f, [this, partyFresh] {
        autopilot = false; partyFresh(); active = 0; EnterDungeon(2, 2024); arriveT = 9; party[active].pos.y = 0; SetupCombat();
        for (auto& e : enemies) if (e.st != ESt::Dead) { e.st = ESt::Chase; e.atkCd = 0; }
        testPos.clear(); for (auto& e : enemies) testPos.push_back(e.pos);
        testHp = party[active].hp;
        Say(L"ツムギ", L"（会話のテスト）敵はいま止まっているはず…");
        Say(L"ツムギ", L"（会話のテスト）攻撃もしてこないよ。");
    });
    S(1.5f, [this, logD] {
        float moved = 0; for (size_t i = 0; i < enemies.size() && i < testPos.size(); i++) moved = std::max(moved, DistXZ(enemies[i].pos, testPos[i]));
        char b[160]; sprintf_s(b, "talking: enemies max moved=%.2fm (0 = frozen), player hp %.0f -> %.0f", moved, testHp, party[active].hp);
        logD(b);
    }, L"19e_enemies_frozen_while_talking");
    S(0.1f, [this] { dialog.clear(); });
    // 回避の距離：動き・硬直はもとのまま、そのあと無敵の1秒のあいだすべる
    S(0.1f, [this, partyFresh] {
        partyFresh(); active = 0; EnterDungeon(2, 2024); arriveT = 9; party[active].pos.y = 0; enemies.clear(); projs.clear();
        testPos.clear(); testPos.push_back(party[active].pos);
        dialog.clear(); party[active].st = ASt::Normal;
    });
    S(0.3f, [this] {
        dialog.clear(); testPos[0] = party[active].pos; dodgeDist = glideDist = 0; fakeIn.key[VK_SPACE] = true;
    });
    S(0.05f, [this] { fakeIn.key[VK_SPACE] = false; });
    S(0.42f, [this] { testHp = (party[active].st == ASt::Normal && party[active].glideT > 0) ? 1.f : 0.f; });   // 前転が終わった直後：もう動けて、すべっている
    S(1.3f, [this, logD] {
        char b[260]; sprintf_s(b, "dodge distance (sora): roll %.2fm + glide %.2fm = %.2fm (before: 2.40m), moved %.2fm; free to act while gliding=%d, invuln %.2fs",
                               dodgeDist, glideDist, dodgeDist + glideDist, DistXZ(party[active].pos, testPos[0]), (int)testHp, DodgeInvuln(0));
        logD(b);
    }, L"19f_dodge_glide");
    // ボスを倒したあとの5階・15階：ボスが現れず、何もない層（出口は開いている）
    S(0.1f, [this, partyFresh, logD] {
        unsigned evSave = eventsDone; bool knotSave = knot;
        char b[300]; int e5, p5, k5, e15, p15, k15, b5, b15;
        eventsDone &= ~(EV_BOSS5 | EV_F15); knot = false;
        EnterDungeon(5, 505); b5 = (int)enemies.size();
        EnterDungeon(15, 1515); b15 = (int)enemies.size();
        eventsDone |= EV_BOSS5 | EV_F15; knot = true;
        partyFresh(); active = 0;
        EnterDungeon(5, 505); e5 = (int)enemies.size(); p5 = (int)pillars.size(); k5 = hasKey ? 1 : 0;
        EnterDungeon(15, 1515); e15 = (int)enemies.size(); p15 = (int)pillars.size(); k15 = hasKey ? 1 : 0;
        sprintf_s(b, "boss floors before clear: f5 enemies=%d f15 enemies=%d | after clear: f5 enemies=%d pillars=%d exit open=%d, f15 enemies=%d pillars=%d exit open=%d",
                  b5, b15, e5, p5, k5, e15, p15, k15);
        logD(b);
        arriveT = 9; party[active].pos.y = 0; dialog.clear();
        eventsDone = evSave | EV_F15; knot = knotSave;   // 撮影用に15階の空の広間を見せる（このあとは元に戻す）
    });
    S(1.0f, [this] { dialog.clear(); }, L"19g_floor15_after_clear_empty");
    S(0.1f, [this] { eventsDone &= ~EV_F15; });
    S(0.1f, [this, logD] {
        char b[200]; int cnt[5]; const int fl[5] = { 2, 6, 9, 11, 13 };
        for (int i = 0; i < 5; i++) { EnterDungeon(fl[i], 900 + fl[i]); cnt[i] = (int)enemies.size(); }
        sprintf_s(b, "enemy count per floor (fewer, max 12 / deep max 10): f2=%d f6=%d f9=%d f11=%d f13=%d", cnt[0], cnt[1], cnt[2], cnt[3], cnt[4]);
        logD(b);
        EnterDungeon(11, 1111); arriveT = 9; party[active].pos.y = 0;
    });
    // ==== 村：置く → 住人と話して解放 → 自分ではめ込む → 完成のイベント（機能・仲間）====
    auto idxOf = [this](PartType t) { for (int i = 0; i < (int)placed.size(); i++) if (placed[i].t == t) return i; return -1; };
    auto logV = [this](const char* s) {
        FILE* f = nullptr;
        if (_wfopen_s(&f, (shotDir + L"\\village_test.txt").c_str(), L"a") == 0 && f) { fprintf(f, "%s\n", s); fclose(f); }
    };
    S(0.1f, [this] {
        autopilot = false;
        DeleteFileW((shotDir + L"\\village_test.txt").c_str());
        for (int f : { 1, 2, 4, 8 }) for (int k = 0; k < LOOT_N; k++) TakeLoot(f, k);   // 1・2・4・8階の糸巻きを全部開けた
        toasts.clear();
        EnterVillage(true);
    });
    S(1.2f, nullptr, L"20_return_with_parts");
    // 配置メニュー（WASD）：左のカード → 右のパーツ欄
    S(0.1f, [this] { dialog.clear(); toasts.clear(); vWalkCam = false; vYaw = 0.3f; vDist = 30; geoFocus = 0; geoCol = 0; geoRow = 0; cursorX = 12; cursorZ = 12; });
    S(0.6f, nullptr, L"20b_geo_menu_wasd_left");
    S(0.1f, [this] { geoCol = 1; geoRow = 2; });
    S(0.4f, nullptr, L"20c_geo_menu_wasd_right");
    // 置く（川・橋・石垣も見せる）
    S(0.1f, [this, logV] {
        auto P = [this](PartType t, int x, int z, int rot = 0) {
            if (inventory[t] <= 0) return;
            if (!CanPlace(t, x, z, rot)) { ToastMsg(L"置けない"); return; }
            placed.push_back({ t, x, z, rot, 0.f }); inventory[t]--;
        };
        P(P_Shop, 11, 15); P(P_Kiln, 3, 13); P(P_HouseKurobee, 3, 16); P(P_Wheel, 14, 2); P(P_Well, 12, 5);
        P(P_River, 1, 8); P(P_River, 1, 9); P(P_River, 1, 10);
        bool bridgeLand = CanPlace(P_Bridge, 5, 5, 0), bridgeRiver = CanPlace(P_Bridge, 14, 6, 0);
        { char w[200]; sprintf_s(w, "well far from river=%d, well 1 cell from river=%d, well on river=%d, wheel on land=%d, wheel on river=%d", CanPlace(P_Well, 5, 5, 0), CanPlace(P_Well, 12, 7, 0), CanPlace(P_Well, 14, 7, 0), CanPlace(P_Wheel, 5, 5, 0), CanPlace(P_Wheel, 14, 12, 0)); logV(w); }
        P(P_Bridge, 14, 6); P(P_Bridge, 14, 7);   // となり同士の橋はつながる
        inventory[P_River]++; P(P_River, 13, 12);   // もとの川のとなりに置いた川（岸があいて、もとの川とつながる）
        { char w[120]; int m = 0; for (auto& q : placed) if (q.t == P_River && q.x == 13 && q.z == 12) m = NeighborMask(q); sprintf_s(w, "river next to terrain river: mask=%d (1 = joined to the main river on +x)", m); logV(w); }
        { char w[220]; int mb = 0, mr = 0; for (auto& q : placed) { if (q.t == P_Bridge && q.z == 6) mb = NeighborMask(q); if (q.t == P_River && q.z == 9) mr = NeighborMask(q); }
          sprintf_s(w, "connect: bridge(14,6) mask=%d (4 or 8 = side joined), river(1,9) mask=%d (4|8 = joined above and below); well next to shop placeable=%d, well away from buildings=%d, bridge next to building=%d",
                    mb, mr, CanPlace(P_Well, 13, 15, 0), CanPlace(P_Well, 13, 7, 0), CanPlace(P_Bridge, 14, 15, 0)); logV(w); }
        P(P_Path, 9, 6); P(P_Path, 9, 7); P(P_Path, 9, 5); P(P_Path, 9, 4); P(P_Tree, 6, 12); P(P_Tree, 8, 8); P(P_Tree, 0, 16); P(P_Lantern, 11, 13); P(P_Lantern, 8, 3);
        XMFLOAT3 onBridge = CellCenter(14, 6), onRiver = CellCenter(14, 12), riverPart = CellCenter(1, 8), farBank = CellCenter(17, 6);
        { char w[160]; sprintf_s(w, "far bank (east of river) walkable=%d, reachable over bridge cells 13->14->15->16 walkable=%d%d%d%d", !VillageSolid(farBank.x, farBank.z), !VillageSolid(CellCenter(13, 6).x, CellCenter(13, 6).z), !VillageSolid(onBridge.x, onBridge.z), !VillageSolid(CellCenter(15, 6).x, CellCenter(15, 6).z), !VillageSolid(CellCenter(16, 6).x, CellCenter(16, 6).z)); logV(w); }
        char b[256];
        sprintf_s(b, "bridge on land placeable=%d, bridge on river placeable=%d, walk on bridge blocked=%d, walk on river blocked=%d, placed river blocked=%d, outside wall blocked=%d",
                  bridgeLand, bridgeRiver, VillageSolid(onBridge.x, onBridge.z), VillageSolid(onRiver.x, onRiver.z), VillageSolid(riverPart.x, riverPart.z), VillageSolid(-21.f, 0.f));
        logV(b);
        dialog.clear(); toasts.clear();
        RebuildWalkers();
        sprintf_s(b, "start: residents in town=%d (only chief), shop open=%d", (int)walkers.size(), placed[0].open ? 1 : 0);
        logV(b);
        vYaw = 0.55f; vDist = 48; cursorX = 12; cursorZ = 9; geoFocus = 1; vFocus = F3(0, 0, 0);
    });
    S(0.8f, nullptr, L"20d_placed_border_river_bridge");
    // 配置メニュー：ホイールでスクロールした位置が勝手に戻らないか、置いた建物が ×0 で一覧に残るか
    S(0.1f, [this, logV] { vYaw = 0.3f; vDist = 34; geoFocus = 0; geoCol = 0; geoRow = 0; geoScroll = 0; });
    S(0.1f, [this] { geoScroll = 3; });
    S(0.3f, [this, logV, idxOf] {
        std::vector<int> pal = PaletteList(); bool shop = false; for (int id : pal) if (id == P_Shop) shop = true;
        char b[260]; sprintf_s(b, "placement list: scroll kept after wheel=%d (scroll=%d), placed shop shown only as its building card (no extra 0/3 card)=%d (inventory %d)", geoScroll == 3 ? 1 : 0, geoScroll, (!shop && idxOf(P_Shop) >= 0) ? 1 : 0, inventory[P_Shop]);
        logV(b);
    }, L"20e_list_scroll_kept");
    // 置いたジオラマ：X で「移動」→ クリック/Enter で別の場所へ → もう一度 X で「片付け」。村長の家は片付けられない
    auto press = [this](int vk) { fakeIn.key[vk] = true; };
    auto release = [this](int vk) { fakeIn.key[vk] = false; };
    S(0.1f, [this, press] {
        toasts.clear(); vWalkCam = false; geoFocus = 1; cursorX = 6; cursorZ = 12;   // 木の上
        moveTest[0] = 0; for (auto& p : placed) if (p.t == P_Tree && p.x == 6 && p.z == 12) moveTest[0] = 1;
        press('X');
    });
    S(0.15f, [this, release] { release('X'); moveTest[1] = moving ? 1 : 0; cursorX = 5; cursorZ = 9; });
    S(0.3f, nullptr, L"20f_moving_tree");
    S(0.1f, [this, press] { press(VK_RETURN); });
    S(0.15f, [this, release, press] {
        release(VK_RETURN);
        moveTest[2] = 0; for (auto& p : placed) if (p.t == P_Tree && p.x == 5 && p.z == 9) moveTest[2] = 1;
        moveTest[3] = inventory[P_Tree];
        press('X');                                       // もう一度持ち上げて…
    });
    S(0.15f, [this, release] { release('X'); });
    S(0.1f, [this, press] { press('X'); });             // …移動中にもう一度 X で片付け
    S(0.15f, [this, release, press] {
        release('X');
        moveTest[4] = inventory[P_Tree] - moveTest[3];
        moveTest[5] = moving ? 1 : 0;
        cursorX = 12; cursorZ = 10;                       // 村長の家（村長がはめ込まれている）
        press('X');
    });
    S(0.15f, [this, release] { release('X'); });
    S(0.1f, [this, press] { press('X'); });             // 片付けようとしても…
    S(0.15f, [this, release, press] { release('X'); moveTest[6] = moving ? 1 : 0; press(VK_ESCAPE); });   // できないので Esc で元に戻す
    S(0.15f, [this, release, logV] {
        release(VK_ESCAPE);
        int touji = 0; for (auto& p : placed) if (p.t == P_HouseTouji && p.x == 12 && p.z == 10 && p.sub[0] == S_Chief) touji = 1;
        char b[300]; sprintf_s(b, "move: tree at (6,12)=%d, X -> moving=%d, Enter -> tree at (5,9)=%d, X again while moving -> stored (+%d tree, moving=%d), chief house X,X -> still moving (not stored)=%d, Esc -> back in place with chief=%d",
                               moveTest[0], moveTest[1], moveTest[2], moveTest[4], moveTest[5], moveTest[6], touji);
        logV(b);
        sprintf_s(b, "bridge end: well on bridge end cell(13,6)=%d, path on bridge end=%d, building next to bridge end(12,4 2x2 covers 13,5)=%d",
                  CanPlace(P_Well, 13, 6, 0), CanPlace(P_Path, 13, 6, 0), CanPlace(P_HouseVillager, 12, 6, 0));
        logV(b);
        // はめ込みパーツの数と枠の数（余り・不足がないか）
        int have[S_COUNT]{}, need[S_COUNT]{}, rivers = 0;
        have[S_Chief]++; for (int s = 0; s < MAX_SLOT; s++) if (SlotNeed(P_HouseTouji, s) >= 0) need[SlotNeed(P_HouseTouji, s)]++;
        for (int f = 1; f <= MAX_FLOOR; f++)
            for (int k = 0; k < LOOT_N; k++)
            {
                int id = kFloorLoot[f].ids[k];
                if (id < 0) break;
                if (id >= SUB_ID) have[id - SUB_ID]++;
                else { if (id == P_River) rivers++; for (int s = 0; s < MAX_SLOT; s++) if (SlotNeed((PartType)id, s) >= 0) need[SlotNeed((PartType)id, s)]++; }
            }
        std::string diff; int bad = 0;
        for (int s = 0; s < S_COUNT; s++) if (have[s] != need[s]) { bad++; char t[40]; sprintf_s(t, " sub%d have%d need%d", s, have[s], need[s]); diff += t; }
        sprintf_s(b, "fit parts balance: barrels have=%d need=%d, mismatched kinds=%d%s, river pieces from spools=%d, well slots=%d",
                  have[S_Barrel], need[S_Barrel], bad, diff.c_str(), rivers, SlotCount(P_Well));
        logV(b);
    });
    // 配置メニューで、右のパーツを左のカードの同じ形の枠へ運ぶ。人物（タヌキの商人）は解放前でもはめられる
    S(0.1f, [this, logV, idxOf] {
        dialog.clear(); toasts.clear();
        vWalkCam = false; geoFocus = 0; vYaw = 0.3f; vDist = 34; vFocus = F3(4, 0, 6);
        int si = idxOf(P_Shop);
        std::vector<int> pal = PaletteList();
        std::vector<std::pair<int, int>> left; std::vector<int> right;
        GeoLists(pal, left, right);
        for (int r = 0; r < (int)right.size(); r++) if (pal[right[r]] == SUB_ID + S_Merchant) { geoCol = 1; geoRow = r; palSel = right[r]; }
        carrySub = S_Merchant; carryByMouse = false; carrySlot = 0; carryCard = 0;
        for (int i = 0, k = 0; i < (int)placed.size(); i++) if (SlotCount(placed[i].t) > 0) { if (i == si) carryCard = k; k++; }
        bool signBefore = TryFit(si, 1, S_Sign);      // 解放前の看板ははめられない
        char b[160]; sprintf_s(b, "before unlock: shop open=%d, sign fits=%d (should be 0)", placed[si].open ? 1 : 0, signBefore ? 1 : 0);
        logV(b);
        toasts.clear();
    });
    S(0.6f, nullptr, L"21_carry_person_to_card");
    S(0.1f, [this, logV, idxOf] {
        int si = idxOf(P_Shop);
        bool wrong = TryFit(si, 2, S_Merchant);       // 形の違う枠には入らない
        bool ok = TryFit(si, 0, S_Merchant);
        carrySub = -1;
        int people = 0; for (auto& w : walkers) if (w.person == S_Merchant) people++;
        char b[160]; sprintf_s(b, "person before unlock: wrong-shape slot=%d, merchant fitted=%d, merchant in town=%d", wrong ? 1 : 0, ok ? 1 : 0, people);
        logV(b);
    });
    S(0.35f, nullptr, L"21b_person_fitted");
    // 住人（村長）と話して、はめ込みを解放する
    S(0.1f, [this, logV, idxOf] {
        toasts.clear();
        vWalkCam = true; EnterTownWalk(false);
        Walker* chief = nullptr;
        for (auto& w : walkers) if (w.person == S_Chief) chief = &w;
        if (chief)
        {
            party[0].pos = Add(chief->pos, F3(0, 0, -1.6f)); party[0].yaw = 0; chief->yaw = PI; chief->waitT = 9;
            camYaw = 0; camPosNow = Add(party[0].pos, F3(0, 3.5f, -5.5f)); camTargetNow = Add(party[0].pos, F3(0, 1.2f, 0));
            Say(WalkerName(*chief), WalkerLine(*chief, 0));
        }
        bool before = placed[idxOf(P_Shop)].open;
        UnlockByTalk(L"トウジ");
        dialog.pop_front();
        char b[160]; sprintf_s(b, "talk to chief: shop open %d -> %d, kiln open %d", before ? 1 : 0, placed[idxOf(P_Shop)].open ? 1 : 0, placed[idxOf(P_Kiln)].open ? 1 : 0);
        logV(b);
    });
    S(0.7f, nullptr, L"22_unlock_by_talk");
    // 話題を選ぶ会話：ジオラマのこと（パーツを教わる）・村のこと（復興率）・物語・コツ
    S(0.1f, [this, logV] {
        dialog.clear(); toasts.clear();
        int ci = -1;
        for (int i = 0; i < (int)walkers.size(); i++) if (walkers[i].person == S_Chief) ci = i;
        if (ci < 0) return;
        talkMenu = true; talkSel = 1; talkWalker = ci;
    });
    S(0.4f, nullptr, L"22a_talk_topics");
    S(0.1f, [this, logV] {
        int ci = talkWalker; talkMenu = false;
        if (ci < 0) return;
        // 村長の家に足りないパーツ（暖簾・植木鉢）を手元からいったん外して、会話でもらえるか確かめる
        int keepNoren = subInv[S_Noren], keepPot = subInv[S_Pot];
        subInv[S_Noren] = 0; subInv[S_Pot] = 0;
        float r0 = RestoreRate();
        TalkTopic(1);                       // ジオラマのこと → 村長の家のはめ込みパーツ
        int norenGift = subInv[S_Noren], potGift = subInv[S_Pot];
        int got3 = 0; for (int i = 0; i < LOOT_N; i++) if ((floorGot[3] >> i) & 1) got3++;
        TalkTopic(2);                       // 村のこと → 復興率アップ
        float r1 = RestoreRate();
        subInv[S_Noren] = keepNoren + norenGift; subInv[S_Pot] = keepPot + potGift;
        talkWalker = ci; TalkTopic(0); TalkTopic(3);
        talkWalker = -1;
        char b[240]; sprintf_s(b, "talk: chief gives own house part noren=%d pot=%d (taken from a spool not yet opened: floor3 got=%d), restore %.1f%% -> %.1f%%, dialog lines=%d", norenGift, potGift, got3, r0 * 100, r1 * 100, (int)dialog.size());
        logV(b);
    });
    S(0.5f, nullptr, L"22a2_talk_gift");
    // 解放後：カードの「？」が、はめるパーツの形に変わる
    S(0.1f, [this] {
        dialog.clear(); toasts.clear(); vWalkCam = false; geoFocus = 0; geoCol = 0; geoRow = 0; carrySub = -1;
        std::vector<int> pal = PaletteList(); std::vector<std::pair<int, int>> left; std::vector<int> right;
        GeoLists(pal, left, right);
        for (int r = (int)left.size() - 1; r >= 0; r--) if (left[r].first == 1) geoRow = r;   // 置いた建物のカードへ
        geoScroll = geoRow;
    });
    S(0.4f, nullptr, L"22b_card_shapes_after_unlock");
    // 看板と酒樽もはめて完成 → カードに「イベント！」（完成してから表示）
    S(0.1f, [this, logV, idxOf] {
        int si = idxOf(P_Shop);
        bool badgeBefore = IsComplete(placed[si]) && !placed[si].evDone;
        TryFit(si, 1, S_Sign); TryFit(si, 2, S_Barrel);
        bool badgeAfter = IsComplete(placed[si]) && !placed[si].evDone;
        char b[160]; sprintf_s(b, "event badge: before last fit=%d, after all fitted=%d", badgeBefore ? 1 : 0, badgeAfter ? 1 : 0);
        logV(b);
        toasts.clear();
    });
    S(0.7f, nullptr, L"22c_card_event_badge");
    // はめ込んだ建物も片付けられる（はめたパーツはついたまま）→ もう一度置くとパーツごと戻る
    S(0.1f, [this, idxOf, press] {
        int si = idxOf(P_Shop);
        moveTest[0] = placed[si].x; moveTest[1] = placed[si].z; moveTest[2] = placed[si].rot;
        toasts.clear(); vWalkCam = false; geoFocus = 1; cursorX = placed[si].x; cursorZ = placed[si].z; cursorRot = placed[si].rot;
        press('X');
    });
    S(0.15f, [this, release] { release('X'); });
    S(0.1f, [this, press] { press('X'); });
    S(0.15f, [this, release, idxOf] {
        release('X');
        moveTest[3] = (int)stored.size(); moveTest[4] = inventory[P_Shop]; moveTest[5] = idxOf(P_Shop);
        std::vector<int> pal = PaletteList(); std::vector<std::pair<int, int>> left; std::vector<int> right;
        GeoLists(pal, left, right);
        for (int r = 0; r < (int)left.size(); r++) if (left[r].first == 0 && left[r].second >= 0 && left[r].second < (int)pal.size() && pal[left[r].second] == P_Shop) { geoRow = r; geoScroll = r; palSel = left[r].second; }
        geoFocus = 0; geoCol = 0;
    });
    S(0.6f, nullptr, L"22d_stored_shop_card_complete");
    S(0.1f, [this, press] {
        std::vector<int> pal = PaletteList();
        for (int i = 0; i < (int)pal.size(); i++) if (pal[i] == P_Shop) palSel = i;
        geoFocus = 1; cursorX = moveTest[0]; cursorZ = moveTest[1]; cursorRot = moveTest[2];
        press(VK_RETURN);
    });
    S(0.15f, [this, release, logV, idxOf] {
        release(VK_RETURN);
        int si = idxOf(P_Shop), filled = 0;
        if (si >= 0) for (int s = 0; s < MAX_SLOT; s++) if (placed[si].sub[s] >= 0) filled++;
        char b[260]; sprintf_s(b, "store fitted building: shop X,X -> stored=%d inventory=%d on map=%d; placed again -> on map=%d fitted parts kept=%d/3, stored left=%d",
                               moveTest[3], moveTest[4], moveTest[5] >= 0 ? 1 : 0, si >= 0 ? 1 : 0, filled, (int)stored.size());
        dialog.clear();
        logV(b);
        toasts.clear();
    });
    // 町で完成した商店のそばへ行く → イベント
    S(0.1f, [this, idxOf] {
        vWalkCam = true;
        int si = idxOf(P_Shop);
        XMFLOAT3 door; int pat; DoorOf(placed[si], door, pat);
        party[0].pos = Add(door, F3(-0.6f, 0, -0.6f)); party[0].yaw = 0;
        camYaw = 0.2f; camPosNow = Add(party[0].pos, F3(-1.5f, 4.5f, -6.5f)); camTargetNow = Add(party[0].pos, F3(0, 1.3f, 0));
    });
    S(0.3f, nullptr);
    S(0.6f, [this, logV, idxOf] {
        char b[160]; sprintf_s(b, "shop complete=%d, completion event seen=%d", IsComplete(placed[idxOf(P_Shop)]) ? 1 : 0, placed[idxOf(P_Shop)].evDone ? 1 : 0);
        logV(b);
    }, L"23_shop_complete_event");
    // 完成した商店の中：買い物ができる
    S(0.1f, [this, idxOf] { dialog.clear(); toasts.clear(); EnterInterior(idxOf(P_Shop)); });
    S(1.3f, [this] { party[active].pos = F3(0.2f, 0, -0.2f); party[active].yaw = 0; shopOpen = true; shopSel = 4; });
    S(0.4f, nullptr, L"23b_shop_interior_open");
    // 店から出て、店の前を歩いて離れられるか（閉じ込めバグの確認）
    S(0.1f, [this] { shopOpen = false; LeaveInterior(); });
    S(1.2f, [this] { testStart = party[active].pos; autoWalkOn = true; autoWalk = Add(testStart, F3(-4.f, 0, -4.f)); });
    S(1.3f, [this] {
        float moved = DistXZ(party[active].pos, testStart);
        FILE* f = nullptr;
        if (_wfopen_s(&f, (shotDir + L"\\shop_exit_test.txt").c_str(), L"w") == 0 && f) { fprintf(f, "moved %.2f m\n", moved); fclose(f); }
        autoWalkOn = false;
    });
    // 炭焼き窯を完成させ、そばへ行く → クロベエが仲間に
    S(0.1f, [this, logV, idxOf] {
        dialog.clear(); toasts.clear();
        int ki = idxOf(P_Kiln);
        bool joinedBefore = joined[1];
        for (int s = 0; s < SlotCount(P_Kiln); s++) TryFit(ki, s, SlotNeed(P_Kiln, s));   // レシピどおりにはめる
        bool joinedAfterFit = joined[1];
        int w, h; PartSize(placed[ki].t, placed[ki].rot, w, h);
        party[0].pos = Add(CellCenter(placed[ki].x, placed[ki].z, w, h), F3(1.0f, 0, -3.2f)); party[0].yaw = -0.3f;
        camYaw = -0.3f; camPosNow = Add(party[0].pos, F3(1.5f, 4.5f, -6.5f)); camTargetNow = Add(party[0].pos, F3(0, 1.3f, 0));
        CheckTownEvents();
        char b[160]; sprintf_s(b, "kiln: kurobee joined before=%d, after fitting(no event yet)=%d, after event=%d", joinedBefore, joinedAfterFit, joined[1] ? 1 : 0);
        logV(b);
    });
    S(1.0f, nullptr, L"24_kurobee_joins_after_event");
    S(0.1f, [this, logV, idxOf] {
        dialog.clear(); toasts.clear();
        int wi = idxOf(P_Wheel);
        subInv[S_Kairi]++;                             // 12階の QTE のあとに出る「カイリ」をもらったことにする
        TryFit(wi, 0, S_Kairi);
        int w, h; PartSize(placed[wi].t, placed[wi].rot, w, h);
        party[0].pos = Add(CellCenter(placed[wi].x, placed[wi].z, w, h), F3(-2.6f, 0, -1.0f)); party[0].yaw = 1.2f;
        camYaw = 1.0f; camPosNow = Add(party[0].pos, F3(-5.5f, 4.5f, -3.f)); camTargetNow = Add(party[0].pos, F3(0, 1.3f, 0));
        CheckTownEvents();
        char b[96]; sprintf_s(b, "wheel: kairi joined after event=%d", joined[2] ? 1 : 0);
        logV(b);
    });
    S(1.0f, nullptr, L"24b_kairi_joins_after_event");
    // 町を歩くのは主人公だけ（仲間は仕事場のそば、はめ込んだ人物が住人として出てくる）
    S(0.1f, [this, logV] {
        dialog.clear(); toasts.clear();
        RebuildWalkers();
        party[0].pos = F3(-4.f, 0, -2.f); party[0].yaw = PI * 0.75f;
        camYaw = PI * 0.75f; camPosNow = Add(party[0].pos, F3(4.f, 5.f, 4.f)); camTargetNow = Add(party[0].pos, F3(0, 1.3f, 0));
        autoWalkOn = true; autoWalk = F3(-10.f, 0, 8.f);
        char b[160]; int k2 = 0, k5 = 0; for (auto& w : walkers) { if (w.kind == 2) k2++; if (w.kind == 5) k5++; }
        sprintf_s(b, "town: party drawn=1 (hero only), residents=%d (kurobee by kiln=%d, kairi by wheel=%d)", (int)walkers.size(), k2, k5);
        logV(b);
    });
    S(1.4f, nullptr, L"25_town_hero_only");
    // 村のメニュー（Esc）
    S(0.1f, [this] { autoWalkOn = false; townMenu = true; townSel = 4; });
    S(0.3f, nullptr, L"25b_town_menu");
    S(0.1f, [this] { townSel = 1; townWeapon = true; menuOpen = true; menuTab = 2; menuSel = 0; menuChar = 1; });
    S(0.4f, nullptr, L"25c_town_menu_weapon");
    // 村のメニュー：セーブのあと「続けますか」、ゲーム終了の前に「セーブしますか」
    S(0.1f, [this] { townWeapon = false; menuOpen = false; townMenu = true; townSel = 5; townPage = 0; townAsk = 0; });
    S(0.3f, nullptr, L"25f_menu_controls_placement");
    S(0.1f, [this] { townPage = 1; });
    S(0.3f, nullptr, L"25g_menu_controls_town");
    S(0.1f, [this] { townPage = 2; });
    S(0.3f, nullptr, L"25h_menu_controls_dungeon");
    S(0.1f, [this] { townSel = 6; townAsk = 1; askSel = 0; });
    S(0.3f, nullptr, L"25d_menu_save_continue");
    S(0.1f, [this] { townSel = 7; townAsk = 2; askSel = 0; });
    S(0.3f, nullptr, L"25e_menu_quit_save");
    S(0.1f, [this] { townAsk = 0; townMenu = false; });
    // 村がすべてそろった → 御神木の前でイベント
    S(0.1f, [this, logV] {
        townMenu = false; townWeapon = false; menuOpen = false; dialog.clear(); toasts.clear();
        for (int f = 1; f <= MAX_FLOOR; f++) { floorGot[f] = 0x3FF; floorLoot |= 1u << f; }
        for (int i = 0; i < P_COUNT; i++) inventory[i] = 0;
        for (int i = 0; i < S_COUNT; i++) subInv[i] = 0;
        placed.clear();
        auto P = [this](PartType t, int x, int z, int rot = 0) { Placed p{ t, x, z, rot, 10.f }; p.open = true; p.evDone = true; placed.push_back(p); };
        P(P_HouseTouji, 12, 10); P(P_Shop, 11, 15); P(P_Kiln, 3, 13); P(P_HouseKurobee, 3, 16); P(P_Loom, 16, 6); P(P_Wheel, 14, 2);
        P(P_Forge, 16, 12); P(P_HouseRabbit, 3, 4); P(P_Field, 5, 4); P(P_Field, 5, 1); P(P_HouseVillager, 11, 2); P(P_HouseVillager, 7, 14);
        P(P_Well, 12, 5); P(P_Well, 16, 15); P(P_Cloth, 6, 8); P(P_Cloth, 12, 18);
        for (int z = 8; z <= 13; z++) P(P_River, 1, z);
        P(P_Bridge, 14, 9); P(P_Bridge, 14, 16); P(P_Bridge, 1, 9, 1);
        for (int z = 0; z <= 8; z++) P(P_Path, 9, z);
        for (int z = 12; z <= 17; z++) P(P_Path, 10, z);
        const int tr[18][2] = { {0,0},{0,2},{0,4},{0,16},{0,18},{2,18},{4,19},{6,19},{18,19},{19,17},{19,14},{19,11},{12,0},{17,4},{19,0},{6,12},{8,8},{11,8} };
        for (auto& t : tr) P(P_Tree, t[0], t[1]);
        const int ln[12][2] = { {8,1},{8,3},{8,5},{8,7},{13,13},{13,17},{7,17},{13,9},{17,9},{6,6},{2,2},{18,16} };
        for (auto& t : ln) P(P_Lantern, t[0], t[1]);
        for (auto& p : placed) for (int s = 0; s < SlotCount(p.t); s++) p.sub[s] = SlotNeed(p.t, s);   // レシピどおりにはめる
        joined[1] = joined[2] = true;
        RebuildWalkers();
        rateShown = RestoreRate();
        vWalkCam = true;
        party[0].pos = F3(0, 0, -4.5f); party[0].yaw = 0;
        camYaw = 0; camPosNow = F3(0, 4.5f, -12.f); camTargetNow = F3(0, 2.0f, 0);
        bool complete = VillageComplete();
        CheckTownEvents();
        { std::vector<int> pal = PaletteList(); bool tree = false, river = false; for (int id : pal) { if (id == P_Tree) tree = true; if (id == P_River) river = true; }
          char k[120]; sprintf_s(k, "left list after placing all: tree card kept=%d (inventory %d), river card kept=%d", tree ? 1 : 0, inventory[P_Tree], river ? 1 : 0); logV(k); }
        char b[160]; sprintf_s(b, "full village: complete=%d, tree event=%d, restore rate=%.0f%%", complete ? 1 : 0, Done(EV_TREE) ? 1 : 0, RestoreRate() * 100);
        logV(b);
    });
    S(0.4f, [this] { if (!dialog.empty()) dialog.pop_front(); });
    S(1.0f, nullptr, L"26_tree_event");
    S(0.1f, [this] { dialog.clear(); toasts.clear(); vWalkCam = false; vYaw = 0.45f; vDist = 46; geoFocus = 0; geoCol = 0; geoRow = 0; vFocus = F3(0, 0, 0); cursorX = 12; cursorZ = 10; });
    S(0.8f, nullptr, L"26b_full_village_placement");
    // ---- 配置モードで指した場所へ Tab で移動して町歩き
    S(0.1f, [this] {
        cursorX = 6; cursorZ = 10;
        WalkFromCursor();
        XMFLOAT3 c = CellCenter(6, 10);
        FILE* f = nullptr;
        if (_wfopen_s(&f, (shotDir + L"\\tab_move_test.txt").c_str(), L"w") == 0 && f)
        {
            fprintf(f, "cursor cell (6,10) center %.2f %.2f -> player %.2f %.2f dist %.2f walk=%d\n", c.x, c.z, party[active].pos.x, party[active].pos.z, DistXZ(c, party[active].pos), vWalkCam ? 1 : 0);
            fclose(f);
        }
    });
    S(0.6f, nullptr, L"23c_tab_move_to_cursor");
    // ---- 建物の中（民家・工房）
    S(0.1f, [this] { autoWalkOn = false; for (int i = 0; i < (int)placed.size(); i++) if (placed[i].t == P_HouseTouji) { EnterInterior(i); break; } });
    S(1.3f, [this] { party[active].pos = F3(0.3f, 0, -1.2f); party[active].yaw = 0.6f; Say(L"トウジ", WalkerLine(roomNpc, 0)); });
    S(0.4f, nullptr, L"24_interior_house");
    S(0.1f, [this] { dialog.clear(); scene = SceneId::Village; for (int i = 0; i < (int)placed.size(); i++) if (placed[i].t == P_Loom) { EnterInterior(i); break; } });
    S(1.3f, [this] { party[active].pos = F3(-1.0f, 0, -0.6f); party[active].yaw = 0.3f; });
    S(0.4f, nullptr, L"25_interior_workshop");
    // ---- クロベエ：投げて巻き込む攻撃・潜る回避
    S(0.1f, [this, partyFresh] { partyFresh(); active = 1; EnterDungeon(4, 303); arriveT = 9; SetupCombat(); autopilot = true; });
    S(1.25f, nullptr, L"26_kurobee_throw");
    // 扇形に薙ぎ払っている途中の一瞬
    S(0.1f, [this] {
        autopilot = false;
        Actor& a = party[active];
        a.st = ASt::Attack; a.combo = 0; a.stT = 0.46f * Def(a).dur[0]; a.hitDone = true;
        camYaw = a.yaw;
    });
    S(0.02f, nullptr, L"26b_kurobee_fan_sweep");
    S(0.1f, [this] {
        autopilot = false;
        Actor& a = party[active];
        a.st = ASt::Dodge; a.stT = 0.3f; a.dodgeDir = Fwd(a.yaw + 1.2f); a.invuln = 0.3f;
    });
    S(0.02f, nullptr, L"27_kurobee_dig");
    // ---- カイリ：ミシン針
    S(0.1f, [this, partyFresh] { partyFresh(); active = 2; EnterDungeon(6, 404); arriveT = 9; SetupCombat(); autopilot = true; });
    S(1.3f, nullptr, L"28_kairi_attack");
    // ---- メニュー：武器変更・キャラ変更
    S(0.1f, [this] {
        autopilot = false; active = 0;
        auto give = [this](int def) { for (int i = 0; i < (int)weapons.size(); i++) if (weapons[i].def == def) return i; return GiveWeapon(def); };
        int w = give(W_SORA_FIRE); give(W_SORA_WATER);
        attInv[AT_STONE] = 2; attInv[AT_FIRE] = 1; attInv[AT_STEEL] = 1;
        EquipWeapon(party[0], w);
        weapons[w].att[0] = AT_STONE; attInv[AT_STONE]--; RecalcWeapon(party[0]);
        menuOpen = true; menuTab = 2; menuSel = 1;
    });
    S(0.2f, nullptr, L"29_menu_weapons");
    S(0.1f, [this] { menuChar = 1; menuSel = 0; });
    S(0.3f, nullptr, L"29b_menu_weapons_kurobee");
    S(0.1f, [this] { menuChar = 0; });
    S(0.1f, [this] { menuTab = 1; menuSel = 2; });
    S(0.2f, nullptr, L"30_menu_party");
    S(0.1f, [this] { menuTab = 5; menuSel = 2; });
    S(0.2f, nullptr, L"30b_menu_controls_dungeon");
    S(0.1f, [this] { menuOpen = false; SetupCombat(); autopilot = true; });
    S(1.5f, nullptr, L"31_fire_scissors");
    // ---- 武器が壊れる：最初の武器以外は消える（アタッチメントは戻る）→ 最初の武器は残り、繕い糸で直る
    S(0.1f, [this] {
        autopilot = false; toasts.clear();
        Actor& a = party[0];
        int stoneBefore = attInv[AT_STONE];
        size_t nBefore = weapons.size();
        a.st = ASt::Normal;
        BreakWeapon(a);                                   // 緋糸切り（硬い石つき）が壊れる
        bool fireGone = true;
        for (auto& w : weapons) if (w.def == W_SORA_FIRE) fireGone = false;
        FILE* f = nullptr;
        if (_wfopen_s(&f, (shotDir + L"\\weapon_break_test.txt").c_str(), L"w") == 0 && f)
        {
            fprintf(f, "non-starter break: weapons %d -> %d, fire scissors gone=%d, stone %d -> %d, now holding def=%d broken=%d\n",
                    (int)nBefore, (int)weapons.size(), fireGone ? 1 : 0, stoneBefore, attInv[AT_STONE], weapons[a.weapon].def, a.broken ? 1 : 0);
            fclose(f);
        }
    });
    S(0.5f, nullptr, L"31b_weapon_break_vanish");
    S(0.1f, [this] {
        toasts.clear();
        Actor& a = party[0];
        BreakWeapon(a);                                   // 最初の武器が壊れる（消えない）
        bool brokenAfter = a.broken; size_t n = weapons.size();
        items[4] = std::max(items[4], 1);
        UseItem(4);                                       // 繕い糸で直す
        FILE* f = nullptr;
        if (_wfopen_s(&f, (shotDir + L"\\weapon_break_test.txt").c_str(), L"a") == 0 && f)
        {
            fprintf(f, "starter break: broken=%d weapons=%d (kept) -> after mending thread broken=%d dur=%.0f/%.0f\n",
                    brokenAfter ? 1 : 0, (int)n, a.broken ? 1 : 0, a.dur, a.maxDur);
            fclose(f);
        }
    });
    S(0.5f, nullptr, L"31c_starter_repaired");
    // ---- モーション表
    const wchar_t* sheetNames[9] = { L"32_weapon_design", L"33_motion_combo1", L"34_motion_combo2", L"35_motion_combo3",
                                     L"36_motion_spin", L"37_motion_kurobee_guard", L"38_motion_kurobee_throw", L"39_motion_kurobee_dig", L"40_motion_kairi" };
    for (int i = 0; i < 9; i++)
    {
        S(0.1f, [this, i] { autopilot = false; scene = SceneId::Sheet; sheetMode = true; sheetPage = i; toasts.clear(); dialog.clear(); });
        S(0.25f, nullptr, sheetNames[i]);
    }
    // ---- セーブ（G）→ タイトル「つづきから」→ ロード
    S(0.1f, [this] {
        sheetMode = false; active = 0;
        scene = SceneId::Village; vWalkCam = true;
        SaveGame(true);
        scene = SceneId::Title; titleSel = 1;
        toasts.clear(); dialog.clear();
    });
    S(1.0f, nullptr, L"41_title_continue");
    S(0.1f, [this] {
        placed.clear();
        for (int i = 0; i < P_COUNT; i++) inventory[i] = 0;
        joined[1] = joined[2] = false;
        bool ok = LoadGame();
        EnterVillage(false);
        ToastMsg(ok ? L"セーブデータを読み込みました" : L"読み込みに失敗しました", RGBA(0.7f, 1, 0.75f));
        rateShown = RestoreRate();
    });
    S(1.2f, nullptr, L"42_loaded_village");
    S(0.3f, nullptr);
}
