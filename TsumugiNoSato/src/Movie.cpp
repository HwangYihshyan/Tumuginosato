// 紹介動画（--movie 出力.mp4）
//   約1分。撮影デモ（autoshot）と同じ「台本（steps）」のしくみで自動操作し、main.cpp が毎フレームを MP4 に書き出す。
//   字幕と、最初と最後の題字カードは DrawMovieOverlay でゲームの画面に重ねる（ゲームの UI と同じ「和紙と墨と朱」）。
//   場面のつなぎは、題字なしの短い暗転（StartTransition の quick）
//
//   流れ（秒はおおよそ）
//     0  題字カード → タイトル画面 → ハタオリ村を歩いて根の洞へ（01）→ 1階に降り立つ
//     15 ソラの戦闘と回転斬り（02）→ クロベエ（03）→ カイリ（04）→ 中ボス ほつれ蜘蛛（05）
//     29 12階の QTE（06）→ 15階のボス うつろ、突進を柱にぶつける（07）
//     40 ジオラマを置いてはめ込む（08）→ 完成した村と御神木（09）→ 終わりの題字カード
#include "Game.h"

void Game::BuildMovieScript()
{
    steps.clear();
    DeleteFileW(SavePath().c_str());
    auto S = [this](float wait, std::function<void()> act) { steps.push_back({ wait, act, L"" }); };
    // 字幕：番号・見出し・説明・出しておく秒数
    auto Cap = [this](const wchar_t* no, const wchar_t* title, const wchar_t* sub, float dur) {
        movieNo = no; movieTitle = title; movieSub = sub; movieCapT = 0; movieCapDur = dur;
    };
    // 場面のつなぎ：0.35 秒で暗くなり、暗いあいだに mid で場面を切りかえ、0.7 秒で明ける（合計 1.05 秒）
    auto Cut = [this](std::function<void()> mid) {
        toasts.clear();
        StartTransition(L"", L"", [this, mid] { toasts.clear(); dialog.clear(); movieTick = nullptr; mid(); }, true);
    };
    auto partyFresh = [this]() {
        for (int i = 0; i < 3; i++) { party[i].hp = party[i].maxHp; party[i].st = ASt::Normal; party[i].dur = party[i].maxDur; party[i].broken = false; }
        arriveT = 9; dialog.clear(); toasts.clear(); autopilot = false; autoSpecial = 0; autoQTE = false; lockIdx = -1;
    };
    // カメラを主人公の背後へ置き直す（前の場面のカメラから流れてこないように）
    auto SnapCam = [this]() {
        Actor& a = party[active];
        camYaw = a.yaw;
        camPosNow = Add(a.pos, F3(-std::sin(a.yaw) * 7.f, 4.5f, -std::cos(a.yaw) * 7.f));
        camTargetNow = Add(a.pos, F3(0, 1.2f, 0));
    };

    // ---- 始まりの題字カード（タイトル画面の上に重ね、3.2 秒から消えていく）
    S(0.0f, [this] { movieCard = 1; movieCardT = 0; titleSel = 0; });
    S(4.0f, [this] { movieCard = 0; });
    // ---- タイトル画面 → ハタオリ村（題字つきの暗転）
    S(1.4f, [this] {
        StartTransition(L"ハタオリ村", L"― 霧に呑まれた村 ―", [this] {
            NewGame(); EnterVillage(false);
            dialog.clear(); toasts.clear();
            vWalkCam = true;
            Actor& a = party[0];
            a.pos = F3(-3.5f, 0, -9.f);
            XMFLOAT3 goal = F3(-0.4f, 0, 21.f);
            a.yaw = YawOf(Sub(goal, a.pos)); camYaw = a.yaw;
            camPosNow = Add(a.pos, F3(-std::sin(camYaw) * 6.f, 4.5f, -std::cos(camYaw) * 6.f)); camTargetNow = Add(a.pos, F3(0, 1.3f, 0));
            autoWalkOn = true; autoWalk = goal; autoDash = true;
        });
    });
    S(2.0f, [Cap] { Cap(L"01", L"ハタオリ村", L"霧に呑まれた機織りの村から、地下の迷宮「根の洞」へ", 4.6f); });
    // ---- 入口の奥へ歩いて入り、根の洞の1階へ
    S(4.0f, [this] {
        dialog.clear(); toasts.clear(); autoWalkOn = false; autoDash = false;
        eventsDone |= EV_F1;                 // 1階の会話は出さない
        entering = true;
        StartTransition(L"根の洞", L"1階　― 浅層", [this] { entering = false; EnterDungeon(1, 777); dialog.clear(); });
    });
    // ---- 02 ソラ：3段コンボと回転斬り
    S(4.4f, [this, Cut, Cap, partyFresh, SnapCam] {
        Cut([this, Cap, partyFresh, SnapCam] {
            partyFresh(); active = 0; eventsDone |= EV_F2;
            EnterDungeon(2, 2024); arriveT = 9; SetupCombat(); autopilot = true;
            Cap(L"02", L"大糸切り鋏", L"開いて、挟んで、振り抜く ― 溜めると刃を開いたまま回転斬り", 4.2f);
        });
    });
    S(2.4f, [this] { autoSpecial = 0.75f; });
    // ---- 03 クロベエ
    S(2.0f, [this, Cut, Cap, partyFresh, SnapCam] {
        Cut([this, Cap, partyFresh, SnapCam] {
            partyFresh(); joined[1] = joined[2] = true; active = 1;
            EnterDungeon(4, 303); arriveT = 9; SetupCombat(); autopilot = true;
            Cap(L"03", L"仲間　クロベエ", L"ひも通しで扇形に薙ぎ払い、地面に潜ってかわす", 3.0f);
        });
    });
    // ---- 04 カイリ
    S(3.3f, [this, Cut, Cap, partyFresh, SnapCam] {
        Cut([this, Cap, partyFresh, SnapCam] {
            partyFresh(); active = 2;
            EnterDungeon(2, 2026); arriveT = 9; SetupCombat(); autopilot = true;
            Cap(L"04", L"仲間　カイリ", L"ミシン針で、遠くの敵を縫い留める", 2.6f);
        });
    });
    // ---- 05 中ボス ほつれ蜘蛛
    S(3.0f, [this, Cut, Cap, partyFresh, SnapCam] {
        Cut([this, Cap, partyFresh, SnapCam] {
            partyFresh(); active = 0; eventsDone |= EV_F5;
            EnterDungeon(5, 5); arriveT = 9; party[active].pos.y = 0; SetupCombat(); SnapCam(); autopilot = true;
            Cap(L"05", L"中ボス　ほつれ蜘蛛", L"子分を呼ぶ前に、一気に攻める", 4.0f);
        });
    });
    // ---- 06 12階の QTE
    S(4.5f, [this, Cut, Cap, partyFresh, SnapCam] {
        Cut([this, Cap, partyFresh, SnapCam] {
            partyFresh(); active = 0;
            EnterDungeon(12, 1212); arriveT = 9; party[active].pos.y = 0;
            XMFLOAT3 d = NormXZ(Sub(eventPos, party[active].pos));
            party[active].pos = Sub(eventPos, Mul(d, 4.0f));
            if (Solid(party[active].pos.x, party[active].pos.z)) party[active].pos = Add(eventPos, F3(0, 0, -3.5f));
            party[active].yaw = YawOf(d); camYaw = party[active].yaw; SnapCam();
            Cap(L"06", L"12階　QTE", L"刺繍枠の縫い目がほどける前に、合図に合わせて動く", 5.4f);
        });
    });
    S(2.35f, [this] { dialog.clear(); autoQTE = true; });
    // ---- 07 15階のボス うつろ
    S(3.7f, [this, Cut, Cap, partyFresh, SnapCam] {
        Cut([this, Cap, partyFresh, SnapCam] {
            partyFresh(); active = 0;
            EnterDungeon(15, 1515); arriveT = 9; party[active].pos.y = 0;
            eventState = 1;                  // 15階の始まりの会話は出さない（暗転が明けてから始まるため）
            SnapCam();
            Cap(L"07", L"ボス　うつろ", L"4本の糸の柱が守りの要。突進を柱にぶつけて、守りを崩す", 5.2f);
        });
    });
    S(0.5f, [this] { dialog.clear(); autopilot = true; });
    S(2.0f, [this] {
        // 突進の構えから、柱へ向かってまっすぐ突っ込ませる
        autopilot = false; dialog.clear();
        Enemy* u = nullptr; for (auto& e : enemies) if (e.t == EType::Utsuro) u = &e;
        if (!u || pillars.empty()) return;
        Pillar& p = pillars[0];
        u->pos = Add(p.pos, F3(6.f, 0, 0)); u->dashDir = F3(-1, 0, 0); u->yaw = YawOf(u->dashDir);
        u->st = ESt::Attack; u->stT = 0; u->pattern = 1; u->hitDone = true;
        Actor& a = party[active]; a.st = ASt::Normal;
        a.pos = Add(p.pos, F3(2.5f, 0, -6.5f)); a.yaw = 0;
        camYaw = -0.25f;
    });
    // ---- 08 ジオラマを置いて、はめ込む
    S(2.0f, [this, Cut, Cap, partyFresh, SnapCam] {
        Cut([this, Cap, partyFresh, SnapCam] {
            partyFresh();
            EnterVillage(true);
            dialog.clear(); toasts.clear();
            vWalkCam = false; geoFocus = 1; geoCol = 0; geoRow = 0; carrySub = -1;
            vFocus = F3(0, 0, 0); vYaw = 0.25f; vDist = 42; cursorX = 11; cursorZ = 15;
            movieTick = [this](float dt) { vYaw += dt * 0.07f; };
            Cap(L"08", L"ジオラマの村づくり", L"集めたパーツを村に置き、建物の枠にはめ込んで完成させる", 6.0f);
        });
    });
    struct PutDef { PartType t; int x, z, rot; };
    const PutDef puts[10] = {
        { P_Shop, 11, 15, 0 }, { P_Kiln, 3, 13, 0 }, { P_HouseKurobee, 3, 16, 0 }, { P_Wheel, 14, 2, 0 }, { P_Well, 12, 5, 0 },
        { P_Bridge, 14, 6, 0 }, { P_Bridge, 14, 7, 0 }, { P_Tree, 6, 12, 0 }, { P_Lantern, 11, 13, 0 }, { P_Tree, 8, 8, 0 },
    };
    for (int i = 0; i < 10; i++)
    {
        PutDef pd = puts[i];
        S(i == 0 ? 1.1f : 0.35f, [this, pd] {
            Placed p{ pd.t, pd.x, pd.z, pd.rot, 0.f };   // appearT = 0：下から伸びて現れる
            placed.push_back(p);
            cursorX = pd.x; cursorZ = pd.z;
            int w, h; PartSize(pd.t, pd.rot, w, h);
            SpawnFx(2, Add(CellCenter(pd.x, pd.z, w, h), F3(0, 1.5f, 0)), 3.f, C4(1, 0.9f, 0.6f), 0.5f);
            RebuildWalkers();
        });
    }
    // 右の欄のタヌキの商人を、左の商人の店のカードの枠へ運んで、はめる
    S(0.4f, [this] {
        geoFocus = 0; subInv[S_Merchant] = 1;
        int si = -1; for (int i = 0; i < (int)placed.size(); i++) if (placed[i].t == P_Shop) si = i;
        int k = 0; for (int i = 0; i < (int)placed.size(); i++) if (SlotCount(placed[i].t) > 0) { if (i == si) carryCard = k; k++; }
        carrySub = S_Merchant; carryByMouse = false; carrySlot = 0;
        std::vector<int> pal = PaletteList(); std::vector<std::pair<int, int>> left; std::vector<int> right;
        GeoLists(pal, left, right);
        for (int r = 0; r < (int)left.size(); r++) if (left[r].first == 1 && left[r].second == si) { geoRow = r; geoScroll = r; }
    });
    S(1.0f, [this] {
        int si = -1; for (int i = 0; i < (int)placed.size(); i++) if (placed[i].t == P_Shop) si = i;
        if (si >= 0) TryFit(si, 0, S_Merchant);
        carrySub = -1;
    });
    // ---- 09 完成した村と御神木
    S(1.2f, [this, Cut, Cap] {
        Cut([this, Cap] {
            townMenu = false; townWeapon = false; menuOpen = false;
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
            for (auto& p : placed) for (int s = 0; s < SlotCount(p.t); s++) p.sub[s] = SlotNeed(p.t, s);
            joined[1] = joined[2] = true;
            RebuildWalkers();
            rateShown = RestoreRate();
            vWalkCam = true; carrySub = -1; geoFocus = 0;
            party[0].pos = F3(0, 0, -4.5f); party[0].yaw = 0; party[0].st = ASt::Normal;
            camYaw = -0.5f; camPosNow = F3(2.5f, 4.5f, -12.f); camTargetNow = F3(0, 2.0f, 0);
            CheckTownEvents();               // 村がそろった → 御神木のイベント（会話と光）
            movieTick = [this](float dt) { camYaw += dt * 0.16f; };   // 主人公のまわりをゆっくり回る
            Cap(L"09", L"よみがえる村", L"すべてのジオラマがそろうと、御神木が花を咲かせる", 4.6f);
        });
    });
    // ---- 終わりの題字カード
    S(5.5f, [this] { movieCard = 2; movieCardT = 0; });
    S(5.0f, nullptr);
}

// ------------------------------------------------------------------
// 紹介動画の重ね描き：字幕（左下の墨の帯）と、始まり・終わりの題字カード
// ------------------------------------------------------------------
void Game::DrawMovieOverlay()
{
    float W = (float)R->W, H = (float)R->H;

    // ---- 字幕：左から滑り込む墨の帯。左に朱の判（番号）、見出しは明朝、下に説明と縫い目
    if (movieCard == 0 && !movieTitle.empty() && movieCapT < movieCapDur)
    {
        float in = SmoothStep(0.f, 0.35f, movieCapT);
        float out = 1.f - SmoothStep(movieCapDur - 0.4f, movieCapDur, movieCapT);
        float a = in * out;
        float slide = (1 - in) * -36.f;
        float w = 600, h = 76, sk = 18;
        float x0 = 34 + slide, y0 = H - h - 40;
        if (!dialog.empty()) y0 = H - h - 210;      // 会話の囲いと重ならないよう上へ
        D2D1_POINT_2F p[4] = { { x0 + sk, y0 }, { x0 + w, y0 }, { x0 + w - sk, y0 + h }, { x0, y0 + h } };
        D2D1_POINT_2F sh[4]; for (int i = 0; i < 4; i++) sh[i] = { p[i].x + 5, p[i].y + 5 };
        R->FillPoly(sh, 4, RGBA(0, 0, 0, 0.35f * a));
        R->FillPoly(p, 4, RGBA(0.08f, 0.07f, 0.08f, 0.9f * a));
        R->StrokePoly(p, 4, RGBA(0.8f, 0.64f, 0.4f, 0.5f * a), 1.f);
        R->DashedLine(x0 + 96, y0 + h - 8, x0 + w - sk - 12, y0 + h - 8, RGBA(0.85f, 0.65f, 0.35f, 0.6f * a), 1.3f);
        // 朱の判（番号）
        float sx = x0 + 22, sy = y0 + 10;
        R->FillRect(sx, sy, 56, 56, RGBA(0.75f, 0.2f, 0.16f, a));
        R->StrokeRect(sx + 3, sy + 3, 50, 50, RGBA(1, 0.92f, 0.82f, 0.8f * a), 1.2f);
        R->Text(movieNo, sx, sy + 9, 56, 40, 26, RGBA(1, 0.97f, 0.92f, a), 1, true, false, 1);
        R->Text(movieTitle, x0 + 96, y0 + 6, w - 120, 36, 25, RGBA(1, 0.97f, 0.9f, a), 0, true, false, 1);
        R->Text(movieSub, x0 + 98, y0 + 40, w - 130, 24, 15, RGBA(1, 0.88f, 0.7f, 0.95f * a), 0, true, false);
    }

    // ---- 題字カード（始まり：3.2 秒から 0.8 秒で消える／終わり：0.8 秒で現れる）
    float a = 0;
    if (movieCard == 1) a = movieCardT < 3.2f ? 1.f : Saturate(1.f - (movieCardT - 3.2f) / 0.8f);
    if (movieCard == 2) a = Saturate(movieCardT / 0.8f);
    if (a > 0.005f)
    {
        R->FillRect(0, 0, W, H, RGBA(0.075f, 0.065f, 0.065f, a));
        // 縁の赤い縫い目
        D2D1_POINT_2F bd[4] = { { 28, 28 }, { W - 28, 28 }, { W - 28, H - 28 }, { 28, H - 28 } };
        R->StrokePoly(bd, 4, RGBA(0.75f, 0.25f, 0.2f, 0.6f * a), 1.5f, true);
        // 文字は少し遅れて浮かび上がる（始まりのカード）
        float ta = movieCard == 1 ? a * SmoothStep(0.2f, 1.0f, movieCardT) : a * SmoothStep(0.4f, 1.2f, movieCardT);
        float cy = H / 2 - 30;
        R->FillRect(W / 2 - 40, cy - 190, 80, 80, RGBA(0.75f, 0.2f, 0.16f, ta));
        R->StrokeRect(W / 2 - 35, cy - 185, 70, 70, RGBA(1, 0.92f, 0.82f, 0.8f * ta), 1.5f);
        R->Text(L"紡", W / 2 - 40, cy - 180, 80, 64, 48, RGBA(1, 0.97f, 0.92f, ta), 1, true, false, 1);
        R->Text(L"ツムギノサト", 0, cy - 92, W, 110, 80, RGBA(1, 0.97f, 0.92f, ta), 1, true, false, 1);
        R->Text(L"― 霧海の記憶 ―", 0, cy + 18, W, 44, 28, RGBA(0.95f, 0.88f, 0.75f, ta), 1, false, false, 1);
        R->FillRect(W / 2 - 140, cy + 74, 280 * SmoothStep(0.3f, 1.4f, movieCardT), 3, RGBA(0.75f, 0.2f, 0.16f, ta));
        R->Text(L"ダンジョン探索 × 町の復興　アクション RPG", 0, cy + 92, W, 30, 19, RGBA(1, 0.95f, 0.88f, ta), 1, false, false);
        R->Text(L"C++17 ／ DirectX 11 ／ HLSL　―　ゲームエンジン不使用", 0, cy + 128, W, 26, 15, RGBA(0.8f, 0.64f, 0.4f, ta), 1, true, false);
    }
}
