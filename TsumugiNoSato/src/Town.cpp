// 町歩き（村の中を歩いて住人と話し、建物に入り、根の洞の入口から潜る）とセーブ／ロード
#include "Game.h"
#include <cstdio>

static bool OverlapCell(int cx, int cz, int x, int z, int w, int h)
{
    return cx >= x && cx < x + w && cz >= z && cz < z + h;
}

// 村の当たり判定：建物・木・御神木・川・村の外は通れない（道と畑は歩ける）
// 北の入口へは 8〜11列 の参道だけ通れる
bool Game::VillageSolid(float x, float z) const
{
    int cx = (int)std::floor(x / 2.f + VG * 0.5f), cz = (int)std::floor(z / 2.f + VG * 0.5f);
    if (cz >= VG)
    {
        if (cx < 8 || cx > 11) return true;
        return z > kEntrancePos.z - 0.6f;           // 入口の岩
    }
    if (cx < 0 || cx >= VG || cz < 0) return true;                   // 村を囲む石垣の外
    if (IsRiverCol(cx)) return !BridgeCell(cx, cz);                  // もとの川：橋の上だけ歩ける（向こう岸へは橋で渡る）
    if (OverlapCell(cx, cz, 9, 9, 2, 2)) return true;
    for (auto& p : placed)
    {
        if (p.t == P_Path || p.t == P_Field || p.t == P_Bridge) continue;
        int w, h; PartSize(p.t, p.rot, w, h);
        if (!OverlapCell(cx, cz, p.x, p.z, w, h)) continue;
        if (p.t == P_River && BridgeCell(cx, cz)) continue;         // 置いた川も橋の上は歩ける
        return true;
    }
    return false;
}

void Game::EnterTownWalk(bool atEntrance)
{
    vWalkCam = true;
    XMFLOAT3 start = atEntrance ? Sub(kEntrancePos, F3(0, 0, 3.0f)) : XMFLOAT3{ -1.f, 0, -19.f };
    for (int i = 0; i < 20 && Solid(start.x, start.z); i++) start.x += 1.f;
    Actor& A = party[active];
    A.pos = start; A.yaw = atEntrance ? PI : 0.f; A.st = ASt::Normal; A.speedNow = 0; A.kb = {};
    int slot = 0;
    for (int i = 0; i < 3; i++)
    {
        if (i == active || !joined[i]) continue;
        Actor& B = party[i];
        B.pos = Add(start, F3(slot == 0 ? 1.6f : -1.6f, 0, atEntrance ? 0.6f : -0.6f));
        if (Solid(B.pos.x, B.pos.z)) B.pos = start;
        B.yaw = A.yaw; B.st = ASt::Normal; B.speedNow = 0; B.kb = {};
        slot++;
    }
    camYaw = A.yaw;
    camPosNow = Add(start, F3(-std::sin(camYaw) * 6.f, 4.5f, -std::cos(camYaw) * 6.f));
    camTargetNow = Add(start, F3(0, 1.3f, 0));
}

std::wstring Game::WalkerName(const Walker& w) const
{
    if (w.person >= 0) return w.person == S_Rabbits ? L"ウサギの姉妹" : kSubs[w.person].name;
    switch (w.kind)
    {
    case 1: return L"トウジ";
    case 2: return L"クロベエ";
    case 3: return L"ウサギの姉妹";
    case 4: return L"タヌキの商人";
    case 5: return L"カイリ";
    default: return L"里人";
    }
}

// 住人の台詞：要望がかなっているかで変わる
std::wstring Game::WalkerLine(const Walker& w, int idx) const
{
    switch (w.kind)
    {
    case 1:
        if (ReqSatisfied(0) && ReqSatisfied(1)) return L"ほう、ソラ。御神木のそばで機の音が聞こえる…これでこそハタオリ村じゃ。";
        if (ReqSatisfied(0)) return L"御神木の近くは落ち着くのう。あとは機織り小屋の音が聞こえれば言うことなしじゃ。";
        return L"わしは御神木のそばに住みたいのう。あの木を見守るのが村長の務めじゃ。";
    case 2: return L"窯の火は消しちゃいねえ。坊、洞に潜るなら声をかけな。";
    case 3:
        if (ReqSatisfied(3)) return L"畑のとなりだと朝の水やりが楽なの！ ありがとう、ソラ！";
        return L"畑の近くに住めたらうれしいなあ…";
    case 4: return L"へい、らっしゃい！ 店の支度がそろったら、中で品物を見ていきな。";
    case 5: return L"水車の音はいいねえ。洞に行くなら、あたしも連れてきな。";
    default:
        if (w.person == S_Smith) return L"鍛冶屋の炉は、はめ込みが全部そろえば火が入るさ。";
        if (w.person == S_Weaver) return L"機の音が好きなの。小屋が完成したら、ほつれ糸を紡いであげる。";
        if (w.person == S_Child) return L"ソラ兄ちゃん、洞の中ってどんなとこ？ 糸巻きが10個もあるんでしょ？";
    {
        const wchar_t* lines[3] = {
            L"霧が晴れてきたね。干してある布の色が戻ってきたよ。",
            L"ソラ、また洞に潜るのかい？ 灯油を忘れずにね。",
            ReqSatisfied(5) ? L"木に囲まれた家は涼しくていいよ。ありがとう。" : L"家のまわりに木があったら、もっと落ち着くんだけどなあ。",
        };
        return lines[idx % 3];
    }
    }
}

std::wstring Game::TownGoal() const
{
    // 目標：いまやると良いことを順に
    for (auto& p : placed) if (p.t == P_HouseTouji && !p.open) return L"村長トウジに話しかけて「ジオラマのこと」を聞こう（はめ込みが解放される）";
    if (VillageComplete()) return Done(EV_TREE) ? L"ハタオリ村は完成した！ 住人と話したり、洞で腕をみがこう" : L"村がすべてそろった！ 御神木の前へ行こう";
    for (auto& p : placed) if (IsComplete(p) && !p.evDone) return std::wstring(kParts[p.t].name) + L"が完成した。そばへ行ってみよう";
    if (totalDives == 0) return L"村の北にある「根の洞」の入口へ行こう（Shift でダッシュ）";
    for (int i = 0; i < P_COUNT; i++) if (inventory[i] > 0) return L"Tab で配置モードにして、ジオラマを置こう";
    for (auto& p : placed) if (SlotCount(p.t) > 0 && !p.open) return std::wstring(kParts[p.t].name) + L"のはめ込みは、住人に「ジオラマのこと」を聞くと解放される";
    bool haveSub = false;
    for (int i = 0; i < S_COUNT; i++) if (subInv[i] > 0) haveSub = true;
    if (haveSub) for (auto& p : placed) if (p.open && SlotCount(p.t) > 0 && !IsComplete(p)) return std::wstring(kParts[p.t].name) + L"の前で F を押して、はめ込もう";
    if (!joined[1]) return L"根の洞でパーツを集めよう（炭焼き窯を完成させるとクロベエが仲間に）";
    if (!joined[2]) return L"根の洞でパーツを集めよう（水車を完成させるとカイリが仲間に）";
    return L"根の洞でパーツを集めて、村の復興を進めよう（各階 10 個）";
}

// 住人の散歩と仲間の追従
void Game::UpdateWalkersAndFollowers(float dt)
{
    // 住人は建物・木・川・ほかの人を通り抜けない。ぶつかって進めなければ、別の行き先を選びなおす
    const Actor& A = party[active];
    for (size_t i = 0; i < walkers.size(); i++)
    {
        Walker& w = walkers[i];
        w.phase += dt;
        if (w.kind == 4) continue;                    // 商人は店の前から動かない
        if (w.waitT > 0) { w.waitT -= dt; continue; }
        if (talkWalker == (int)i) continue;           // 話している相手は立ち止まる
        XMFLOAT3 d = Sub(w.target, w.pos);
        float l = LenXZ(d);
        if (l < 0.2f || w.stuck > 0.7f)
        {
            w.waitT = RandF(1.0f, 3.5f); w.stuck = 0;
            for (int tries = 0; tries < 12; tries++)
            {
                // 近く（6マス以内）の歩ける場所へ。住人は川のこちら側を歩く
                int cx0 = (int)std::floor(w.pos.x / 2.f + VG * 0.5f), cz0 = (int)std::floor(w.pos.z / 2.f + VG * 0.5f);
                int cx = std::clamp(cx0 + RandI(-6, 6), 0, RIVER_X - 1), cz = std::clamp(cz0 + RandI(-6, 6), 0, VG - 1);
                XMFLOAT3 c = CellCenter(cx, cz);
                if (!VillageSolid(c.x, c.z)) { w.target = c; break; }
            }
            continue;
        }
        XMFLOAT3 n = Mul(d, 1.f / l);
        float step = std::min(l, 1.3f * dt);
        // 主人公やほかの住人にぶつかりそうなら待つ
        XMFLOAT3 next = Add(w.pos, Mul(n, 0.5f));
        bool bump = DistXZ(next, A.pos) < 0.8f && vWalkCam;
        for (size_t j = 0; j < walkers.size() && !bump; j++) if (j != i && DistXZ(next, walkers[j].pos) < 0.7f) bump = true;
        if (bump) { w.stuck += dt; continue; }
        XMFLOAT3 before = w.pos;
        MoveCircle(w.pos, 0.35f, n.x * step, n.z * step);
        w.pos.y = 0;
        if (DistXZ(before, w.pos) < step * 0.3f) w.stuck += dt; else w.stuck = 0;
        w.yaw = ApproachAngle(w.yaw, YawOf(n), dt * 6);
    }
    // 町を歩くのは主人公だけ（仲間は仕事場のそばにいる住人として描く）
}

void Game::UpdateTown(float dt, InputState& in)
{
    Controls c = autoshot ? Controls{} : ReadControls(in, dt);
    if (autoshot)
    {
        c.dash = autoDash;
        if (autoWalkOn && DistXZ(party[active].pos, autoWalk) > 0.6f)
        {
            XMFLOAT3 d = NormXZ(Sub(autoWalk, party[active].pos));
            XMFLOAT3 right{ std::cos(camYaw), 0, -std::sin(camYaw) }, fwd = Fwd(camYaw);
            c.mx = DotXZ(d, right); c.mz = DotXZ(d, fwd);
            camYaw = ApproachAngle(camYaw, party[active].yaw, dt * 1.2f);
        }
    }
    c.atkPressed = c.atkDown = false; c.guard = false; c.lockon = false; c.item = -1; c.dodge = false;
    bool talking = !dialog.empty();
    Actor& A = party[active];
    auto standStill = [&]() {
        Controls s = c; s.mx = s.mz = 0; s.interact = false; s.dash = false;
        UpdateActor(A, &s, dt, true);
        UpdateFollowCam(dt);
    };
    // 村のメニュー（Esc / M）
    if (townMenu && !talking && !transOn) { UpdateTownMenu(c); if (!townWeapon) standStill(); return; }
    // 話題を選ぶ
    if (talkMenu && !talking && !transOn) { UpdateTalkMenu(c); standStill(); return; }
    if (floorMenu && !talking && !transOn)
    {
        // 入る階を選んでいる間は動かない
        Controls m = c;
        standStill();
        UpdateFloorMenu(m);
        return;
    }
    // はめ込み（パーツを選ぶ・はめ込む動き）
    if (talking || transOn) { c.mx = c.mz = 0; c.interact = false; c.swap = false; }
    camYaw += c.camYaw;
    c.swap = false;   // 町では Tab は配置モードの切り替え
    if (entering)
    {
        // 入口の奥へ歩いて入っていく
        XMFLOAT3 goal = Add(kEntrancePos, F3(0, 0, 1.5f));
        XMFLOAT3 d = NormXZ(Sub(goal, A.pos));
        A.pos = Add(A.pos, Mul(d, 3.2f * dt));
        A.yaw = ApproachAngle(A.yaw, YawOf(d), dt * 8);
        A.walkT += dt; A.speedNow = 3.2f;
    }
    else UpdateActor(A, &c, dt, true);
    A.pos.y = BridgeHeight(A.pos.x, A.pos.z);     // 橋の上はアーチに沿って歩く
    // 主人公は住人を通り抜けない（押し返される）
    for (auto& w : walkers)
    {
        XMFLOAT3 d = Sub(A.pos, w.pos);
        float l = LenXZ(d);
        if (l < 0.75f && l > 0.001f) MoveCircle(A.pos, 0.4f, d.x / l * (0.75f - l), d.z / l * (0.75f - l));
    }

    talkHint.clear();
    if (!talking && !transOn && !entering)
    {
        // 完成したジオラマのそば・御神木の前のイベント
        CheckTownEvents();
        if (!dialog.empty()) { UpdateFollowCam(dt); return; }
        // 根の洞の入口
        if (DistXZ(A.pos, Sub(kEntrancePos, F3(0, 0, 2.0f))) < 2.6f)
        {
            talkHint = L"[E] 根の洞へ入る（階を選ぶ）";
            if (c.interact)
            {
                floorMenu = true;
                floorSel = std::clamp(deepest, 1, 10) - 1;     // いちばん深く行った階を選んでおく
                talkHint.clear();
                return;
            }
        }
        // 建物の入口
        if (talkHint.empty())
            for (int i = 0; i < (int)placed.size(); i++)
            {
                XMFLOAT3 door; int pat;
                if (!DoorOf(placed[i], door, pat)) continue;
                if (DistXZ(A.pos, door) < 1.4f)
                {
                    talkHint = L"[E] 入る：" + std::wstring(kParts[placed[i].t].name);
                    if (c.interact) { EnterInterior(i); return; }
                    break;
                }
            }
        // 住人（話すと、はめ込みを解放してくれる）
        if (talkHint.empty())
        {
            int nearIdx = -1; float bd = 2.4f;
            for (int i = 0; i < (int)walkers.size(); i++)
            {
                float d = DistXZ(walkers[i].pos, A.pos);
                if (d < bd) { bd = d; nearIdx = i; }
            }
            if (nearIdx >= 0)
            {
                Walker& w = walkers[nearIdx];
                talkHint = L"[E] 話す：" + WalkerName(w);
                if (c.interact)
                {
                    if (w.kind != 4) w.waitT = 4.f;
                    w.yaw = YawOf(Sub(A.pos, w.pos));
                    talkMenu = true; talkSel = 0; talkWalker = nearIdx;   // 話題を選ぶ（物語・ジオラマ・村・コツ）
                    return;
                }
            }
        }
        if (c.menu) { townMenu = true; townSel = 0; return; }
    }
    if (!talking && !entering && !transOn && in.Pressed(VK_TAB))
    {
        // 配置モードへ（カーソルは主人公の足もと）
        vWalkCam = false; talkHint.clear(); geoFocus = 0;
        cursorX = std::clamp((int)std::floor(A.pos.x / 2.f + VG * 0.5f), 0, VG - 1);
        cursorZ = std::clamp((int)std::floor(A.pos.z / 2.f + VG * 0.5f), 0, VG - 1);
        return;
    }
    UpdateFollowCam(dt);
}

// ------------------------------------------------------------------
// 新しく始める（パーツ0から）
// ------------------------------------------------------------------
void Game::NewGame()
{
    for (int i = 0; i < P_COUNT; i++) inventory[i] = 0;
    placed.clear();
    storyQueue = { P_HouseTouji, P_Loom, P_Tree, P_Kiln, P_Lantern, P_HouseKurobee, P_Field, P_HouseRabbit,
                   P_Wheel, P_Path, P_Tree, P_Cloth, P_Well, P_HouseVillager, P_Tree, P_Forge, P_Lantern };
    items[0] = 2; items[1] = 1; items[2] = 1; items[3] = 1; items[4] = 1;
    for (int i = 0; i < S_COUNT; i++) subInv[i] = 0;
    floorLoot = 0; floorMenu = false; talkGift = talkBoost = 0; moving = false; stored.clear();
    for (auto& g : floorGot) g = 0;
    money = 100; threads = 0; deepest = 0; knot = false; totalDives = 0; eventsDone = 0; chapterClear = false;
    joined[0] = true; joined[1] = joined[2] = false;
    for (int i = 0; i < AT_COUNT; i++) attInv[i] = 0;
    active = 0;
    InitWeapons();
    EnsureStart();
}

// ------------------------------------------------------------------
// セーブ／ロード（テキスト形式。メモ帳で中身を確認できる）
// ------------------------------------------------------------------
std::wstring Game::SavePath() const
{
    if (autoshot) return shotDir + L"\\demo_save.txt";   // 撮影デモでは本番のセーブを上書きしない
    return assets + L"..\\save.txt";
}

bool Game::HasSave() const
{
    DWORD a = GetFileAttributesW(SavePath().c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

bool Game::SaveGame(bool showToast)
{
    for (int i = 0; i < 3; i++) SyncWeapon(party[i]);
    FILE* f = nullptr;
    if (_wfopen_s(&f, SavePath().c_str(), L"w") != 0 || !f)
    {
        if (showToast) ToastMsg(L"セーブに失敗しました", RGBA(1, 0.5f, 0.45f));
        return false;
    }
    fprintf(f, "TSUMUGI_SAVE 4\n");
    fprintf(f, "inventory %d", (int)P_COUNT);
    for (int i = 0; i < P_COUNT; i++) fprintf(f, " %d", inventory[i]);
    fprintf(f, "\nitems %d", ITEM_N);
    for (int i = 0; i < ITEM_N; i++) fprintf(f, " %d", items[i]);
    fprintf(f, "\n");
    fprintf(f, "progress %d %d %d %d %d %u %d\n", threads, deepest, knot ? 1 : 0, totalDives, money, eventsDone, chapterClear ? 1 : 0);
    fprintf(f, "joined %d %d %d %d\n", joined[0] ? 1 : 0, joined[1] ? 1 : 0, joined[2] ? 1 : 0, active);
    fprintf(f, "attach %d %d %d %d\n", attInv[0], attInv[1], attInv[2], attInv[3]);
    fprintf(f, "queue %d", (int)storyQueue.size());
    for (auto q : storyQueue) fprintf(f, " %d", (int)q);
    fprintf(f, "\nplaced %d\n", (int)placed.size());
    for (auto& p : placed) fprintf(f, "%d %d %d %d %d %d %d %d %d\n", (int)p.t, p.x, p.z, p.rot, p.sub[0], p.sub[1], p.sub[2], p.open ? 1 : 0, p.evDone ? 1 : 0);
    fprintf(f, "weapons %d\n", (int)weapons.size());
    for (auto& w : weapons) fprintf(f, "%d %.1f %d %d %.1f %d %d\n", w.def, w.dur, w.broken ? 1 : 0, w.lvl, w.exp, w.att[0], w.att[1]);
    fprintf(f, "equip %d %d %d\n", party[0].weapon, party[1].weapon, party[2].weapon);
    fprintf(f, "subs %d", (int)S_COUNT);
    for (int i = 0; i < S_COUNT; i++) fprintf(f, " %d", subInv[i]);
    fprintf(f, "\nfloorloot %u\nfloorgot %d", floorLoot, MAX_FLOOR + 1);
    for (int i = 0; i <= MAX_FLOOR; i++) fprintf(f, " %d", (int)floorGot[i]);
    fprintf(f, "\ntalk %u %u\n", talkGift, talkBoost);
    fprintf(f, "stored %d\n", (int)stored.size());   // はめ込んだまま片付けた建物
    for (auto& p : stored) fprintf(f, "%d %d %d %d %d %d\n", (int)p.t, p.sub[0], p.sub[1], p.sub[2], p.open ? 1 : 0, p.evDone ? 1 : 0);
    fclose(f);
    if (showToast) ToastMsg(L"セーブしました", RGBA(0.7f, 1, 0.75f));
    return true;
}

bool Game::LoadGame()
{
    FILE* f = nullptr;
    if (_wfopen_s(&f, SavePath().c_str(), L"r") != 0 || !f) return false;
    char tag[32]; int ver = 0;
    bool ok = fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &ver) == 2 && strcmp(tag, "TSUMUGI_SAVE") == 0 && ver == 4;
    if (ok)
    {
        NewGame();
        int n = 0, kn = 0, cc = 0, j0, j1, j2, act;
        fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &n);
        for (int i = 0; i < n; i++) { int v = 0; fscanf_s(f, "%d", &v); if (i < P_COUNT) inventory[i] = v; }
        if (ver == 2) fscanf_s(f, "%31s %d %d %d %d", tag, (unsigned)sizeof(tag), &items[0], &items[1], &items[2], &items[3]);
        else
        {
            fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &n);
            for (int i = 0; i < n; i++) { int v = 0; fscanf_s(f, "%d", &v); if (i < ITEM_N) items[i] = v; }
        }
        fscanf_s(f, "%31s %d %d %d %d %d %u %d", tag, (unsigned)sizeof(tag), &threads, &deepest, &kn, &totalDives, &money, &eventsDone, &cc);
        knot = kn != 0; chapterClear = cc != 0;
        fscanf_s(f, "%31s %d %d %d %d", tag, (unsigned)sizeof(tag), &j0, &j1, &j2, &act);
        joined[0] = true; joined[1] = j1 != 0; joined[2] = j2 != 0; active = joined[act] ? act : 0;
        fscanf_s(f, "%31s %d %d %d %d", tag, (unsigned)sizeof(tag), &attInv[0], &attInv[1], &attInv[2], &attInv[3]);
        fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &n);
        storyQueue.clear();
        for (int i = 0; i < n; i++) { int v = 0; fscanf_s(f, "%d", &v); storyQueue.push_back((PartType)v); }
        fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &n);
        placed.clear();
        for (int i = 0; i < n; i++)
        {
            int t, x, z, r, s0 = -1, s1 = -1, s2 = -1;
            if (fscanf_s(f, "%d %d %d %d", &t, &x, &z, &r) != 4) break;
            int op = 0, ev = 0;
            if (ver >= 3) fscanf_s(f, "%d %d %d %d %d", &s0, &s1, &s2, &op, &ev);
            if (t >= 0 && t < P_COUNT) placed.push_back({ (PartType)t, x, z, r, 10.f, { s0, s1, s2 }, op != 0, ev != 0 });
        }
        fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &n);
        if (n > 0)
        {
            weapons.clear();
            for (int i = 0; i < n; i++)
            {
                OwnedWeapon w{}; int br = 0;
                fscanf_s(f, "%d %f %d %d %f %d %d", &w.def, &w.dur, &br, &w.lvl, &w.exp, &w.att[0], &w.att[1]);
                w.broken = br != 0;
                weapons.push_back(w);
            }
            int e0 = 0, e1 = 1, e2 = 2;
            fscanf_s(f, "%31s %d %d %d", tag, (unsigned)sizeof(tag), &e0, &e1, &e2);
            int eq[3] = { e0, e1, e2 };
            for (int i = 0; i < 3; i++)
            {
                party[i].weapon = -1;
                Actor& a = party[i];
                int idx = (eq[i] >= 0 && eq[i] < (int)weapons.size()) ? eq[i] : 0;
                a.weapon = idx;
                const OwnedWeapon& w = weapons[idx];
                a.dur = w.dur; a.maxDur = (float)kWeapons[w.def].maxDur; a.broken = w.broken; a.lvl = w.lvl; a.exp = w.exp;
                RecalcWeapon(a);
            }
        }
        if (ver >= 3)
        {
            if (fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &n) == 2)
                for (int i = 0; i < n; i++) { int v = 0; fscanf_s(f, "%d", &v); if (i < S_COUNT) subInv[i] = v; }
            fscanf_s(f, "%31s %u", tag, (unsigned)sizeof(tag), &floorLoot);
            if (fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &n) == 2 && strcmp(tag, "floorgot") == 0)
                for (int i = 0; i < n; i++) { int v = 0; fscanf_s(f, "%d", &v); if (i <= MAX_FLOOR) floorGot[i] = (unsigned short)v; }
            else
                for (int i = 1; i <= MAX_FLOOR; i++) if ((floorLoot >> i) & 1u) floorGot[i] = 0x3FF;   // 前の形式：全部回収した階
            // 会話：ジオラマを教えてもらった人・思い出話を聞いた人
            if (fscanf_s(f, "%31s %u %u", tag, (unsigned)sizeof(tag), &talkGift, &talkBoost) != 3 || strcmp(tag, "talk") != 0) talkGift = talkBoost = 0;
            // はめ込んだまま片付けた建物
            if (fscanf_s(f, "%31s %d", tag, (unsigned)sizeof(tag), &n) == 2 && strcmp(tag, "stored") == 0)
                for (int i = 0; i < n; i++)
                {
                    int t, s0, s1, s2, op, ev;
                    if (fscanf_s(f, "%d %d %d %d %d %d", &t, &s0, &s1, &s2, &op, &ev) != 6) break;
                    if (t >= 0 && t < P_COUNT) stored.push_back({ (PartType)t, 0, 0, 0, 0.f, { s0, s1, s2 }, op != 0, ev != 0 });
                }
        }
        EnsureStart();
    }
    fclose(f);
    return ok;
}

// ------------------------------------------------------------------
// 根の洞の入口：入る階を選ぶメニュー（行ったことのある階まで選べる）
// ------------------------------------------------------------------
std::wstring FloorSubtitle(int floor)
{
    std::wstring sub = std::to_wstring(floor) + L"階";
    switch (floor)
    {
    case 3: sub += L"　― 細い糸の道（ソラだけ）"; break;
    case 5: sub += L"　― 中間の間（ほつれ蜘蛛）"; break;
    case 7: sub += L"　― 土の道（クロベエだけ）"; break;
    case 12: sub += L"　― うつろの影"; break;
    case 14: sub += L"　― リミテッド層（カイリだけ）"; break;
    case 15: sub += L"　― うつろの間"; break;
    default: sub += floor >= 11 ? L"　― 最深層" : floor >= 6 ? L"　― 深層" : L"　― 浅層"; break;
    }
    return sub;
}

// その階に入れない理由（入れるなら空）：7階はクロベエ、14階はカイリが仲間にいないと入れない
std::wstring Game::FloorBlock(int floor) const
{
    if (floor == 7 && !joined[1]) return L"7階はクロベエしか入れない。炭焼き窯を完成させて仲間にしよう";
    if (floor == 14 && !joined[2]) return L"14階はカイリしか入れない。12階で「カイリ」のパーツを手に入れ、水車にはめて完成させよう";
    return L"";
}

void Game::UpdateFloorMenu(const Controls& c)
{
    int maxF = std::clamp(deepest, 1, MAX_FLOOR);
    floorSel = std::clamp(floorSel, 0, maxF - 1);
    if (c.menu || c.back) { floorMenu = false; return; }
    if (c.up) floorSel = (floorSel + maxF - 1) % maxF;
    if (c.down) floorSel = (floorSel + 1) % maxF;
    if (c.confirm) StartDive(floorSel + 1);
}

void Game::StartDive(int floor)
{
    std::wstring block = FloorBlock(floor);
    if (!block.empty()) { ToastMsg(block, RGBA(1, 0.6f, 0.5f)); return; }
    floorMenu = false;
    entering = true;
    uint32_t seed = autoshot ? 777u + floor : (uint32_t)(GetTickCount() ^ (totalDives * 7919) ^ (floor * 131));
    StartTransition(L"根の洞", FloorSubtitle(floor), [this, floor, seed] { entering = false; EnterDungeon(floor, seed); });
}

void Game::DrawFloorMenu()
{
    float W = (float)R->W, H = (float)R->H;
    int maxF = std::clamp(deepest, 1, MAX_FLOOR);
    float rowH = std::min(40.f, (H - 210) / maxF), bw = 680, bh = 120 + maxF * rowH, bx = (W - bw) / 2, by = (H - bh) / 2 - 10;
    R->FillRect(0, 0, W, H, RGBA(0.02f, 0.03f, 0.06f, 0.45f));
    InkPanel(bx, by, bw, bh, 0.96f);
    R->Text(L"根の洞　― 入る階を選ぶ", bx, by + 16, bw, 32, 22, RGBA(1, 0.92f, 0.75f), 1, true);
    for (int i = 0; i < maxF; i++)
    {
        int fl = i + 1;
        float y = by + 62 + i * rowH;
        bool sel = floorSel == i;
        InkRow(bx + 24, y, bw - 48, rowH - 6, sel);
        D2D1_COLOR_F tc = sel ? RGBA(0.16f, 0.12f, 0.1f) : RGBA(1, 1, 1);
        R->Text(FloorSubtitle(fl), bx + 44, y + rowH * 0.5f - 15, 380, 26, rowH < 36 ? 15.f : 17.f, tc, 0, true, !sel);
        int left = LootLeft(fl), total = LootTotal(fl);
        bool taken = left == 0;
        wchar_t buf[64];
        if (!FloorBlock(fl).empty()) swprintf_s(buf, fl == 7 ? L"入れない（クロベエが必要）" : L"入れない（カイリが必要）");
        else if (fl == 5 || fl == 15) swprintf_s(buf, L"ボスの階（宝箱なし）");
        else if (fl == 12 && left > 0) swprintf_s(buf, L"QTE のあとに糸巻き");
        else if (taken) swprintf_s(buf, L"パーツ回収済み（%d/%d）", total, total);
        else swprintf_s(buf, L"糸巻き あと %d 個（%d/%d）", left, total - left, total);
        R->Text(buf, bx + bw - 300, y + rowH * 0.5f - 13, 250, 24, 14, taken ? (sel ? RGBA(0.4f, 0.45f, 0.5f) : RGBA(1, 1, 1, 0.45f)) : (sel ? RGBA(0.75f, 0.4f, 0.1f) : RGBA(1, 0.8f, 0.4f)), 2, true, !sel);
    }
    R->Text(L"W/S：選ぶ　E/Enter：入る　Esc：やめる　（行ったことのある階まで選べる）", bx, by + bh - 40, bw, 24, 14, RGBA(1, 1, 1, 0.65f), 1);
}
