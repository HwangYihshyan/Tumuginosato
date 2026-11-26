// 村の進め方：はめ込みの解放（住人と話す）、自分ではめ込む（建物の前で F）、
// 完成したジオラマのイベントと機能の解放、御神木のイベント、村を囲む石垣、橋、村のメニュー
#include "Game.h"

namespace
{
    bool OverlapC(int cx, int cz, int x, int z, int w, int h) { return cx >= x && cx < x + w && cz >= z && cz < z + h; }
}

// ------------------------------------------------------------------
// 配置メニューの並び（左：手持ちの主要なジオラマ → 置いた建物、右：小物とはめ込みパーツ）
// ------------------------------------------------------------------
void Game::GeoLists(const std::vector<int>& pal, std::vector<std::pair<int, int>>& left, std::vector<int>& right) const
{
    left.clear(); right.clear();
    for (int i = 0; i < (int)pal.size(); i++) if (IsMainPart(pal[i])) left.push_back({ 0, i });
    for (int i = 0; i < (int)placed.size(); i++) if (SlotCount(placed[i].t) > 0) left.push_back({ 1, i });
    for (int i = 0; i < (int)pal.size(); i++) if (!IsMainPart(pal[i])) right.push_back(i);
}

// ------------------------------------------------------------------
// 完成：はめ込み枠が全部うまったジオラマ
// ------------------------------------------------------------------
bool Game::IsComplete(const Placed& p) const
{
    int n = SlotCount(p.t);
    if (n == 0) return false;
    for (int s = 0; s < n; s++) if (p.sub[s] < 0) return false;
    return true;
}

// 村の完成：100個のパーツを全部集めて置き、すべてのはめ込みが終わった
bool Game::VillageComplete() const
{
    for (int f = 1; f <= MAX_FLOOR; f++) if (LootLeft(f) > 0) return false;
    for (int i = 0; i < P_COUNT; i++) if (inventory[i] > 0) return false;
    for (int i = 0; i < S_COUNT; i++) if (subInv[i] > 0) return false;
    for (auto& p : placed) if (SlotCount(p.t) > 0 && !IsComplete(p)) return false;
    return true;
}

// ------------------------------------------------------------------
// はめ込みの解放：住人と話すと、まだ解放していない建物のはめ込みを手伝ってくれる
// ------------------------------------------------------------------
bool Game::UnlockByTalk(const std::wstring& who)
{
    std::wstring names;
    int n = 0;
    for (auto& p : placed)
    {
        if (SlotCount(p.t) == 0 || p.open) continue;
        p.open = true;
        if (n < 3) { if (!names.empty()) names += L"・"; names += kParts[p.t].name; }
        n++;
    }
    if (n == 0) return false;
    if (n > 3) names += L" など";
    Say(who, L"そうじゃ、" + names + L"のはめ込みなら手伝えるぞ。");
    Say(L"ツムギ", L"はめ込みが解放されたよ！ Tab の配置メニューで、右のパーツを左のカードの同じ形の枠へ運んで、はめ込もう。");
    ToastMsg(L"はめ込み解放：" + names, RGBA(1, 0.85f, 0.4f));
    SaveGame(false);
    return true;
}

// ------------------------------------------------------------------
// はめ込む：運んできたパーツを、左のカードの同じ形の枠へ
//   人物は最初からはめられる。ほかのパーツは、住人と話して解放してから
// ------------------------------------------------------------------
bool Game::TryFit(int idx, int s, int sub)
{
    if (idx < 0 || idx >= (int)placed.size() || sub < 0 || sub >= S_COUNT || subInv[sub] <= 0) return false;
    Placed& p = placed[idx];
    if (s < 0 || s >= SlotCount(p.t)) return false;
    if (p.sub[s] >= 0) { ToastMsg(L"その枠はもう、はまっている", RGBA(1, 0.7f, 0.6f)); return false; }
    if (SlotNeed(p.t, s) != sub) { ToastMsg(L"形が合わない…（" + std::wstring(kSubs[SlotNeed(p.t, s)].name) + L" の形の枠）", RGBA(1, 0.6f, 0.5f)); return false; }
    if (!p.open && !IsPerson(sub)) { ToastMsg(L"まだはめ込めない。住人と話すと解放される", RGBA(1, 0.6f, 0.5f)); return false; }
    p.sub[s] = sub;
    subInv[sub]--;
    // カードの枠へ吸い込まれる演出と、村の建物の上の光
    float cx, cy;
    if (SlotScreen(idx, s, cx, cy)) { flyT = 0; flyPart = idx; flySlot = s; flySub = sub; }
    SpawnFx(2, Add(SlotPos(p, s), F3(0, 0.9f, 0)), 2.6f, C4(1, 0.9f, 0.6f), 0.6f);
    int n = SlotCount(p.t), filled = 0;
    for (int k = 0; k < n; k++) if (p.sub[k] >= 0) filled++;
    wchar_t b[32]; swprintf_s(b, L"（%d/%d）", filled, n);
    ToastMsg(std::wstring(kSubs[sub].name) + L" を " + kParts[p.t].name + L" にはめ込んだ" + b, RGBA(1, 0.9f, 0.6f));
    if (IsPerson(sub)) ToastMsg(std::wstring(kSubs[sub].name) + L" が村に戻ってきた！", RGBA(0.7f, 1, 0.75f));
    if (filled == n) ToastMsg(std::wstring(kParts[p.t].name) + L" が完成した！ 町で近くへ行くとイベント", RGBA(1, 0.8f, 0.35f));
    RebuildWalkers();
    SaveGame(false);
    return true;
}

// 左のカード（置いた建物）の枠の画面上の位置（GeoMenu の描画と同じ並び）
bool Game::SlotScreen(int idx, int s, float& cx, float& cy) const
{
    std::vector<GeoRect> geo = GeoLayout(PaletteList());
    for (auto& g : geo)
        if (g.kind == 1 && g.ref == idx) { cx = g.x + 146 + s * 52.f; cy = g.y + 66; return true; }
    return false;
}

// ------------------------------------------------------------------
// 完成したジオラマのそばに行くと起きるイベント（機能の解放・仲間の加入もここ）
// ------------------------------------------------------------------
void Game::OnPartCompleted(int idx)
{
    Placed& p = placed[idx];
    p.evDone = true;
    int w, h; PartSize(p.t, p.rot, w, h);
    XMFLOAT3 c = CellCenter(p.x, p.z, w, h);
    XMFLOAT3 nearPos = Add(party[active].pos, F3(1.5f, 0, -0.5f));
    switch (p.t)
    {
    case P_HouseTouji:
        Say(L"トウジ", L"おお…暖簾に鉢植え、わしの家がすっかり元どおりじゃ。ありがとう、ソラ。");
        Say(L"トウジ", L"村のみんなにはそれぞれ「望み」がある。望みをかなえれば、村はもっと元に戻るじゃろう。");
        break;
    case P_Shop:
        Say(L"タヌキの商人", L"看板よし、樽よし！ へい、よろず屋の店開きだよ！");
        Say(L"ツムギ", L"商店が使えるようになった！ 中に入ると買い物ができるよ。");
        break;
    case P_Kiln:
        Say(L"クロベエ", L"おお…オレの窯だ！ 薪も提灯もそろって、火が戻ってきやがった。");
        Say(L"クロベエ", L"坊、礼を言う。洞へ潜るなら、オレも一緒に行かせてくれ。力仕事なら任せな！");
        Say(L"ツムギ", L"クロベエが仲間になった！ ダンジョンでは Tab かメニューで交代できるよ。");
        Recruit(1, nearPos);
        eventsDone |= EV_KUROBEE;
        break;
    case P_Wheel:
        Say(L"カイリ", L"へえ、水車が回ってる！ あたしはカイリ、ミナモの船大工さ。霧に流されてここまで来ちまってね。");
        Say(L"カイリ", L"水車の軸を直したお礼だ。遠くの敵はあたしのミシン針で縫い留めてやるよ！");
        Say(L"ツムギ", L"カイリが仲間になった！");
        Recruit(2, nearPos);
        eventsDone |= EV_KAIRI;
        break;
    case P_Loom:
        Say(L"機織りの娘", L"機の音が戻ったわ！ 壊れた武器の「ほつれ糸」があれば、アタッチメントに紡いであげる。");
        Say(L"ツムギ", L"機織り小屋が使えるようになった！ 中で、ほつれ糸3本をアタッチメントに変えられるよ。");
        eventsDone |= EV_LOOM;
        break;
    case P_Forge:
        Say(L"鍛冶屋の親方", L"炉に火が入った。傷んだ武器は持ってきな、まとめて叩き直してやる。");
        Say(L"ツムギ", L"鍛冶屋が使えるようになった！ 中で武器をまとめて直せるよ（壊れた最初の武器も）。");
        break;
    default:
        Say(L"ツムギ", std::wstring(kParts[p.t].name) + L"が完成したね！ 村がまたひとつ、元の姿に戻ったよ。");
        break;
    }
    SpawnFx(2, Add(c, F3(0, 2.5f, 0)), 6.f, C4(1, 0.9f, 0.6f), 0.9f);
    SpawnFx(1, Add(c, F3(0, 1.5f, 0)), 5.f, C4(1, 0.85f, 0.5f), 0.6f);
    ToastMsg(std::wstring(kParts[p.t].name) + L" 完成！", RGBA(1, 0.85f, 0.4f));
    RebuildWalkers();
    SaveGame(false);
}

void Game::CheckTownEvents()
{
    if (!dialog.empty() || transOn) return;
    const Actor& A = party[active];
    for (int i = 0; i < (int)placed.size(); i++)
    {
        const Placed& p = placed[i];
        if (p.evDone || !IsComplete(p)) continue;
        int w, h; PartSize(p.t, p.rot, w, h);
        if (DistXZ(A.pos, CellCenter(p.x, p.z, w, h)) < std::max(w, h) * 1.0f + 3.5f) { OnPartCompleted(i); return; }
    }
    // 村が全部そろったら、御神木の前でイベント
    if (!Done(EV_TREE) && VillageComplete() && DistXZ(A.pos, F3(0, 0, 0)) < 6.f)
    {
        eventsDone |= EV_TREE;
        Say(L"ツムギ", L"ソラ、見て…御神木が光ってる！");
        Say(L"トウジ", L"村のすべてが紡ぎなおされた…。御神木が、霧を吸い込んでおる。");
        Say(L"ナギ", L"……思い出すことを選んだのね。なら、この糸はあなたたちに託すわ。");
        Say(L"ソラ", L"ハタオリ村が、帰ってきた。みんな、おかえり！");
        Say(L"ツムギ", L"ハタオリ村、完成！ 第1章　おしまい。ありがとう、ソラ！");
        for (int k = 0; k < 8; k++)
            SpawnFx(2, F3(std::cos(k * 0.785f) * 3.f, 3.f + k * 0.6f, std::sin(k * 0.785f) * 3.f), 5.f, C4(1, 0.9f, 0.6f), 1.2f);
        SpawnFx(1, F3(0, 6, 0), 14.f, C4(1, 0.95f, 0.7f), 1.0f);
        ToastMsg(L"村の完成！ 御神木が花を咲かせた", RGBA(1, 0.85f, 0.4f));
        chapterClear = true;
        SaveGame(false);
    }
}

// ------------------------------------------------------------------
// 橋：アーチの上を歩く高さ
// ------------------------------------------------------------------
float Game::BridgeHeight(float x, float z) const
{
    int cx = (int)std::floor(x / 2.f + VG * 0.5f), cz = (int)std::floor(z / 2.f + VG * 0.5f);
    for (auto& p : placed)
    {
        if (p.t != P_Bridge) continue;
        int w, h; PartSize(p.t, p.rot, w, h);
        if (!OverlapC(cx, cz, p.x, p.z, w, h)) continue;
        XMFLOAT3 c = CellCenter(p.x, p.z, w, h);
        if (NeighborMask(p) & 3) return 0.7f;                     // つながった橋は平ら
        float l = (p.rot & 1) ? (z - c.z) : (x - c.x);
        return 0.3f + std::cos(Clampf(l / 2.f, -1, 1) * PI * 0.5f) * 0.55f;
    }
    return 0.f;
}

// ------------------------------------------------------------------
// 村を囲む石垣（ここが配置できる範囲）。北は根の洞への参道、東は川
// ------------------------------------------------------------------
void Game::DrawVillageBorder()
{
    const float x0 = -20.f, x1 = (RIVER_X - VG * 0.5f) * 2.f, xr = VG * 1.f, z0 = -20.f, z1 = 20.f;
    DrawParams st; st.tex = T["stonepave"]; st.color = C4(0.68f, 0.64f, 0.58f); st.outline = 0.02f; st.uv = { 1.2f, 0.5f, 0, 0 };
    DrawParams moss; moss.color = C4(0.38f, 0.55f, 0.3f); moss.outline = 0.015f;
    DrawParams bush; bush.color = C4(0.3f, 0.5f, 0.28f); bush.outline = 0.03f;
    auto wall = [&](XMFLOAT3 a, XMFLOAT3 b) {
        XMFLOAT3 d = Sub(b, a);
        float len = LenXZ(d);
        int n = std::max(1, (int)std::round(len / 2.f));
        float yaw = YawOf(d);
        for (int i = 0; i < n; i++)
        {
            XMFLOAT3 m = Lerp3(a, b, (i + 0.5f) / n);
            float hh = 0.75f + 0.12f * std::sin(m.x * 1.7f + m.z * 2.3f);
            XMMATRIX L = XMMatrixScaling(0.7f, hh, len / n + 0.05f) * XMMatrixRotationY(yaw);
            R->Draw(R->cube, L * XMMatrixTranslation(m.x, hh * 0.5f, m.z), st);
            R->Draw(R->cube, XMMatrixScaling(0.76f, 0.08f, len / n) * XMMatrixRotationY(yaw) * XMMatrixTranslation(m.x, hh, m.z), moss);
            if ((i & 1) == 0)
            {
                XMFLOAT3 out = Add(m, Mul(F3(std::cos(yaw), 0, -std::sin(yaw)), 1.2f));
                R->Draw(R->sphere, XMMatrixScaling(1.4f, 1.1f, 1.4f) * XMMatrixTranslation(out.x, 0.45f, out.z), bush);
            }
        }
        // 角の石柱
        for (XMFLOAT3 q : { a, b })
            R->Draw(R->cube, XMMatrixScaling(1.0f, 1.3f, 1.0f) * XMMatrixTranslation(q.x, 0.65f, q.z), st);
    };
    // 川は村のまん中（14・15列）を流れ、その東が「向こう岸」。石垣は川のところだけ切れている
    const float xR = x1 + 4.f;                                                // 川の東岸
    wall(F3(x0 - 0.4f, 0, z1 + 0.4f), F3(x0 - 0.4f, 0, z0 - 0.4f));          // 西
    wall(F3(x0 - 0.4f, 0, z0 - 0.4f), F3(x1, 0, z0 - 0.4f));                  // 南（川の手前まで）
    wall(F3(xR, 0, z0 - 0.4f), F3(xr + 0.4f, 0, z0 - 0.4f));                  // 南（向こう岸）
    wall(F3(xr + 0.4f, 0, z0 - 0.4f), F3(xr + 0.4f, 0, z1 + 0.4f));          // 東
    wall(F3(x0 - 0.4f, 0, z1 + 0.4f), F3(-4.f, 0, z1 + 0.4f));               // 北（参道の左）
    wall(F3(4.f, 0, z1 + 0.4f), F3(x1, 0, z1 + 0.4f));                        // 北（参道の右）
    wall(F3(xR, 0, z1 + 0.4f), F3(xr + 0.4f, 0, z1 + 0.4f));                  // 北（向こう岸）
    // 配置できる地面をうっすら明るく（こちら岸と向こう岸）
    DrawParams g; g.color = C4(1.f, 0.92f, 0.7f, 0.1f); g.blend = Blend::Alpha; g.unlit = true; g.depthWrite = false;
    R->Draw(R->plane, XMMatrixScaling(x1 - x0, 1, z1 - z0) * XMMatrixTranslation((x0 + x1) * 0.5f, 0.05f, 0), g);
    R->Draw(R->plane, XMMatrixScaling(xr - xR, 1, z1 - z0) * XMMatrixTranslation((xR + xr) * 0.5f, 0.05f, 0), g);
}

// ------------------------------------------------------------------
// 村のメニュー（町で Esc / M）
// ------------------------------------------------------------------
namespace
{
    const int kTownN = 9;
    const wchar_t* kTownMenu[kTownN] = { L"ジオラマ配置", L"武器", L"持ち物", L"仲間", L"村の様子", L"操作方法", L"セーブ", L"ゲーム終了", L"閉じる" };
    const wchar_t* kTownMenuDesc[kTownN] = {
        L"配置／組み立てメニューを開く（Tab でも開ける）",
        L"武器の画面：装備・アタッチメント・研ぎ粉や繕い糸",
        L"手持ちのアイテム・お金・アタッチメント",
        L"仲間の様子（洞では Tab で交代）",
        L"復興率・住人の要望・はめ込みの進み具合",
        L"操作方法：ジオラマ配置・町を歩く・ダンジョン探索（A/D でページ切り替え）",
        L"いまの状態をセーブする（セーブのあと、ゲームを続けるか聞く）",
        L"ゲームを終わる（終わる前にセーブするか聞く）",
        L"メニューを閉じる",
    };
}

void Game::UpdateTownMenu(const Controls& c)
{
    if (townWeapon)
    {
        // 武器の画面（洞のメニューと同じ操作。Esc で村のメニューへ戻る）
        menuOpen = true; menuTab = 2;
        Controls cc = c; cc.left = cc.right = false;
        UpdateMenu(cc);
        if (!menuOpen) { townWeapon = false; menuOpen = false; }
        return;
    }
    // 確かめる窓（セーブのあと：続けますか／終了：セーブしますか）
    if (townAsk != 0)
    {
        int n = townAsk == 1 ? 2 : 3;
        if (c.up) askSel = (askSel + n - 1) % n;
        if (c.down) askSel = (askSel + 1) % n;
        if (c.left) askSel = (askSel + n - 1) % n;
        if (c.right) askSel = (askSel + 1) % n;
        if (c.menu || c.back) { townAsk = 0; return; }
        if (!c.confirm) return;
        if (townAsk == 1)
        {
            if (askSel == 0) { townAsk = 0; townMenu = false; ToastMsg(L"ゲームを続けよう", RGBA(1, 0.92f, 0.75f)); }   // はい：続ける
            else { townAsk = 0; townMenu = false; scene = SceneId::Title; titleSel = HasSave() ? 1 : 0; sceneT = 0; }   // いいえ：タイトルへ
        }
        else
        {
            if (askSel == 0) { SaveGame(true); quit = true; }   // セーブして終わる
            else if (askSel == 1) quit = true;                 // セーブせずに終わる
            townAsk = 0;
        }
        return;
    }
    if (c.menu || c.back) { townMenu = false; return; }
    if (townSel == 5 && (c.left || c.right)) townPage = (townPage + (c.right ? 1 : 2)) % 3;   // 操作方法のページ
    if (c.up) townSel = (townSel + kTownN - 1) % kTownN;
    if (c.down) townSel = (townSel + 1) % kTownN;
    if (!c.confirm) return;
    switch (townSel)
    {
    case 0: townMenu = false; vWalkCam = false; geoFocus = 0; cursorX = std::clamp((int)std::floor(party[active].pos.x / 2.f + VG * 0.5f), 0, VG - 1); cursorZ = std::clamp((int)std::floor(party[active].pos.z / 2.f + VG * 0.5f), 0, VG - 1); break;
    case 1: townWeapon = true; menuOpen = true; menuTab = 2; menuSel = 0; menuChar = 0; break;
    case 5: townPage = (townPage + 1) % 3; break;
    case 6: if (SaveGame(true)) { townAsk = 1; askSel = 0; } break;
    case 7: townAsk = 2; askSel = 0; break;
    case 8: townMenu = false; break;
    default: break;
    }
}

void Game::DrawTownMenu()
{
    if (townWeapon) return;
    float W = (float)R->W, H = (float)R->H;
    // 村のメニュー：機織りの村らしく「糸巻き」と「巻物」で見せる
    //   左：縦糸に通した糸巻きが項目。選んだ糸巻きから糸がのびて、右の巻物へつながる
    //   右：木の軸のついた和紙の巻物に、墨の文字で中身を書く
    R->FillRect(0, 0, W, H, RGBA(0.05f, 0.04f, 0.03f, 0.55f));
    R->FillGradientRect(0, 0, 420, H, RGBA(0.04f, 0.03f, 0.03f, 0.6f), RGBA(0.04f, 0.03f, 0.03f, 0.f));
    const D2D1_COLOR_F threadCol[4] = { RGBA(0.22f, 0.32f, 0.62f), RGBA(0.75f, 0.22f, 0.2f), RGBA(0.88f, 0.64f, 0.22f), RGBA(0.32f, 0.6f, 0.36f) };
    const D2D1_COLOR_F ink = RGBA(0.18f, 0.13f, 0.1f), inkSoft = RGBA(0.18f, 0.13f, 0.1f, 0.55f), wood = RGBA(0.42f, 0.27f, 0.16f), woodDark = RGBA(0.25f, 0.15f, 0.09f);

    // 題字：朱の判と村の名前
    R->FillRect(60, 24, 46, 46, RGBA(0.75f, 0.2f, 0.16f));
    R->StrokeRect(63, 27, 40, 40, RGBA(1, 0.9f, 0.8f, 0.8f), 1.5f);
    R->Text(L"紡", 60, 29, 46, 40, 28, RGBA(1, 0.96f, 0.9f), 1, true, false, 1);
    R->Text(L"ハタオリ村", 118, 20, 300, 40, 30, RGBA(1, 0.97f, 0.9f), 0, true, true, 1);
    R->Text(L"村のしおり", 120, 54, 200, 22, 14, RGBA(1, 0.88f, 0.7f, 0.85f), 0, true, true);

    // 左：縦糸と糸巻き
    float lx = 60, ly = 96, lw = 300, step = 60.f;
    float tx = lx + 22;
    R->Line(tx, ly - 6, tx, ly + (kTownN - 1) * step + 46, RGBA(0.95f, 0.9f, 0.8f, 0.7f), 2.f);   // 縦糸
    float rx = 420, ry = 84, rw = W - rx - 56, rh = H - ry - 40;
    for (int i = 0; i < kTownN; i++)
    {
        float y = ly + i * step;
        bool sel = i == townSel;
        D2D1_COLOR_F tc = threadCol[i % 4];
        if (sel)
        {
            // 選んだ項目：和紙の札と、巻物までのびる糸（少したるむ）
            float mx = (lx + lw + rx) * 0.5f, my = y + 26 + 10;
            R->Line(lx + lw, y + 26, mx, my, tc, 2.5f);
            R->Line(mx, my, rx + 2, ry + 40, tc, 2.5f);
            InkRow(lx + 40, y, lw - 40, 52, true);
        }
        else R->FillGradientRect(lx + 40, y + 4, lw - 40, 44, RGBA(0.06f, 0.05f, 0.05f, 0.75f), RGBA(0.06f, 0.05f, 0.05f, 0.1f));
        // 糸巻き（木のつば＋巻いた糸）。選ぶと少し大きく、ゆっくり回るように縞が動く
        float s = sel ? 1.25f : 1.f, cy = y + 26;
        R->FillRect(tx - 11 * s, cy - 11 * s, 22 * s, 4 * s, wood);
        R->FillRect(tx - 11 * s, cy + 7 * s, 22 * s, 4 * s, wood);
        R->FillRect(tx - 8 * s, cy - 7 * s, 16 * s, 14 * s, tc);
        float ph = sel ? std::fmod(time * 6.f, 4.f) : 0.f;
        for (int k = 0; k < 3; k++) R->Line(tx - 8 * s, cy - 6 * s + std::fmod(k * 4.f + ph, 12.f) * s, tx + 8 * s, cy - 4 * s + std::fmod(k * 4.f + ph, 12.f) * s, RGBA(1, 1, 1, 0.3f), 1.f);
        R->StrokeRect(tx - 11 * s, cy - 11 * s, 22 * s, 22 * s, woodDark, 1.f);
        R->Text(kTownMenu[i], lx + 60, y + 12, lw - 70, 30, 20, sel ? ink : RGBA(1, 0.97f, 0.9f), 0, true, !sel, sel ? 1 : 0);
    }

    // 右：巻物（上下に木の軸、あいだに和紙）
    R->FillRect(rx + 6, ry + 18, rw, rh - 26, RGBA(0, 0, 0, 0.35f));                          // 落ち影
    R->FillRect(rx, ry + 10, rw, rh - 20, RGBA(0.96f, 0.93f, 0.86f, 0.98f));                  // 和紙
    R->FillGradientRect(rx, ry + 10, 60, rh - 20, RGBA(0.75f, 0.68f, 0.55f, 0.25f), RGBA(0.75f, 0.68f, 0.55f, 0.f));   // 紙のまるみ
    for (int i = 0; i < 30; i++)
    {
        float fx = rx + 20 + std::fmod(i * 157.3f, rw - 60), fy = ry + 30 + std::fmod(i * 61.7f, rh - 70);
        float len = 10 + std::fmod(i * 7.1f, 18.f), ang = std::fmod(i * 0.83f, 3.14f);
        R->Line(fx, fy, fx + std::cos(ang) * len, fy + std::sin(ang) * len * 0.4f, RGBA(0.55f, 0.42f, 0.3f, 0.09f), 1.f);
    }
    for (int k = 0; k < 2; k++)
    {
        float yy = k == 0 ? ry : ry + rh - 20;
        R->FillRound(rx - 16, yy, rw + 32, 20, 10, wood);
        R->FillRect(rx - 16 + 10, yy + 3, rw + 12, 3, RGBA(1, 0.85f, 0.65f, 0.25f));
        R->FillCircle(rx - 18, yy + 10, 11, woodDark);
        R->FillCircle(rx + rw + 18, yy + 10, 11, woodDark);
    }
    R->DashedLine(rx + 14, ry + 30, rx + rw - 14, ry + 30, RGBA(0.72f, 0.18f, 0.16f, 0.7f), 1.5f);
    R->DashedLine(rx + 14, ry + rh - 30, rx + rw - 14, ry + rh - 30, RGBA(0.72f, 0.18f, 0.16f, 0.7f), 1.5f);
    // 見出し
    R->Text(kTownMenu[townSel], rx + 30, ry + 40, rw - 60, 36, 24, ink, 0, true, false, 1);
    R->FillRect(rx + 30, ry + 78, 60, 3, threadCol[townSel % 4]);
    R->Text(kTownMenuDesc[townSel], rx + 30, ry + 86, rw - 60, 24, 15, inkSoft, 0, true, false);
    wchar_t buf[160];
    float y = ry + 124;
    auto line = [&](const std::wstring& s, D2D1_COLOR_F c = RGBA(0.18f, 0.13f, 0.1f)) { R->Text(s, rx + 34, y, rw - 68, 26, 16, c, 0, true, false); y += 30; };
    float rh0 = rh;
    ry += 38; rh = rh0 - 60;   // 操作方法のページは見出しの下から
    switch (townSel)
    {
    case 2:
    {
        const wchar_t* n[ITEM_N] = { L"薬草", L"灯油", L"研ぎ粉", L"帰り糸", L"繕い糸" };
        for (int i = 0; i < ITEM_N; i++) { swprintf_s(buf, L"%s　×%d", n[i], items[i]); line(buf); }
        swprintf_s(buf, L"お金　%d 文　　ほつれ糸　%d 本", money, threads); line(buf, RGBA(0.62f, 0.3f, 0.08f));
        swprintf_s(buf, L"アタッチメント：硬い石×%d　火の欠片×%d　軽い羽×%d　鋼糸×%d", attInv[0], attInv[1], attInv[2], attInv[3]); line(buf, RGBA(0.15f, 0.28f, 0.55f));
        int nParts = 0, nSubs = 0;
        for (int i = 0; i < P_COUNT; i++) nParts += inventory[i];
        for (int i = 0; i < S_COUNT; i++) nSubs += subInv[i];
        swprintf_s(buf, L"手持ちのジオラマパーツ　%d 個／はめ込みパーツ　%d 個", nParts, nSubs); line(buf, RGBA(0.18f, 0.45f, 0.3f));
        break;
    }
    case 3:
        for (int i = 0; i < 3; i++)
        {
            DrawFaceIcon(i, rx + 54, y + 20, 18);
            if (joined[i]) swprintf_s(buf, L"%s　HP %d/%d　%s", Def(party[i]).name, (int)party[i].hp, (int)party[i].maxHp, kWeapons[weapons[party[i].weapon].def].name);
            else swprintf_s(buf, L"？？？　（%s）", i == 1 ? L"炭焼き窯を完成させ、そばに行くと仲間になる" : L"水車を完成させ、そばに行くと仲間になる");
            R->Text(buf, rx + 84, y + 8, rw - 110, 26, 16, joined[i] ? ink : inkSoft, 0, true, false);
            y += 50;
        }
        line(L"町を歩くのはソラだけ。仲間は仕事場のそばにいる");
        break;
    case 4:
    {
        swprintf_s(buf, L"復興率 %d%%", (int)std::round(RestoreRate() * 100)); line(buf, RGBA(0.62f, 0.3f, 0.08f));
        int got = 0, all = 0;
        for (int f = 1; f <= MAX_FLOOR; f++) { all += LootTotal(f); got += LootTotal(f) - LootLeft(f); }
        swprintf_s(buf, L"集めたジオラマパーツ　%d / %d", got, all); line(buf);
        int done = 0, total = 0, locked = 0;
        for (auto& p : placed) if (SlotCount(p.t) > 0) { total++; if (IsComplete(p)) done++; if (!p.open) locked++; }
        swprintf_s(buf, L"完成したジオラマ　%d / %d　（はめ込みが未解放：%d）", done, total, locked); line(buf);
        for (int r = 0; r < kReqCount; r++)
        {
            bool present = false;
            for (auto& p : placed) if (p.t == kReqs[r].owner) present = true;
            swprintf_s(buf, L"%s %s：%s", ReqSatisfied(r) ? L"✓" : L"○", present ? kReqs[r].who : L"？？？", present ? kReqs[r].text : L"（まだ村に戻っていない）");
            line(buf, ReqSatisfied(r) ? RGBA(0.18f, 0.45f, 0.2f) : RGBA(0.18f, 0.13f, 0.1f, present ? 0.95f : 0.45f));
        }
        if (VillageComplete()) line(Done(EV_TREE) ? L"村は完成した！" : L"村がすべてそろった！ 御神木の前へ行こう", RGBA(0.62f, 0.3f, 0.08f));
        break;
    }
    case 5:
        DrawControlsPage(townPage, rx + 30, ry + 84, rw - 60, rh - 100, true);
        break;
    default:
        line(L"W/S：選ぶ　Enter/E：決定　Esc：閉じる", inkSoft);
        break;
    }
}

// いちばん近い歩ける場所（川の上の水車のそばに住人を立たせるときなど）
XMFLOAT3 Game::NearWalkable(XMFLOAT3 p) const
{
    if (!VillageSolid(p.x, p.z)) return p;
    for (int r = 1; r <= 6; r++)
        for (int i = 0; i < r * 8; i++)
        {
            float a = i * 2 * PI / (r * 8);
            XMFLOAT3 q = Add(p, F3(std::cos(a) * r * 1.0f, 0, std::sin(a) * r * 1.0f));
            if (!VillageSolid(q.x, q.z)) return q;
        }
    return p;
}

// 確かめる窓（セーブのあと「ゲームを続けますか？」／終了の前に「セーブしますか？」）
void Game::DrawTownAsk()
{
    if (!townMenu || townAsk == 0) return;
    float W = (float)R->W, H = (float)R->H;
    R->FillRect(0, 0, W, H, RGBA(0, 0, 0, 0.45f));
    float bw = 520, bh = townAsk == 1 ? 210.f : 260.f, bx = (W - bw) / 2, by = (H - bh) / 2;
    InkPanel(bx, by, bw, bh, 0.97f);
    R->Text(townAsk == 1 ? L"セーブしました。ゲームを続けますか？" : L"ゲームを終了します。セーブしますか？", bx, by + 22, bw, 30, 20, RGBA(1, 0.92f, 0.75f), 1, true);
    const wchar_t* opt1[2] = { L"はい（続ける）", L"いいえ（タイトル画面へ）" };
    const wchar_t* opt2[3] = { L"セーブして終了する", L"セーブせずに終了する", L"やめる" };
    int n = townAsk == 1 ? 2 : 3;
    for (int i = 0; i < n; i++)
    {
        float y = by + 70 + i * 50.f;
        bool sel = askSel == i;
        InkRow(bx + 60, y, bw - 120, 40, sel);
        R->Text(townAsk == 1 ? opt1[i] : opt2[i], bx + 60, y + 8, bw - 120, 26, 18, sel ? RGBA(0.16f, 0.12f, 0.1f) : RGBA(1, 1, 1), 1, true, !sel);
    }
}

// ------------------------------------------------------------------
// 操作方法のページ（0=ジオラマ配置 1=町を歩く 2=ダンジョン探索）。村のメニューと洞のメニューで使う
// ------------------------------------------------------------------
void Game::DrawControlsPage(int page, float x, float y, float w, float h, bool onPaper)
{
    struct Row { const wchar_t* key; const wchar_t* act; };
    static const Row place[] = {
        { L"W / S", L"左の一覧・右の欄で選ぶ（メニュー操作中）" },
        { L"A / D", L"左の一覧と右のはめ込みパーツ欄を行き来" },
        { L"Enter / Space", L"決定：置くジオラマを選ぶ／はめ込みパーツを持ち上げる" },
        { L"WASD（マップ操作中）", L"カーソルを動かす。Enter / Space / クリックで置く" },
        { L"運んでいるとき", L"W/S でカード、A/D で枠、Enter ではめる（クリック・ドラッグでも）" },
        { L"R", L"置くジオラマを回す" },
        { L"X / 右クリック", L"置いたジオラマを移動 → 移動中にもう一度押すと片付け（はめ込んだパーツは外れない）" },
        { L"F", L"置いた建物へカーソルを合わせる" },
        { L"Q / E・ホイール", L"視点を回す・寄る／引く" },
        { L"Esc", L"マップ操作 → メニュー操作 → 町へ戻る" },
        { L"Tab", L"カーソルの場所へ移動して町を歩く" },
    };
    static const Row town[] = {
        { L"WASD", L"歩く" },
        { L"Shift", L"ダッシュ" },
        { L"マウス・← →", L"カメラを回す" },
        { L"E", L"話す（話題を選ぶ）・建物に入る・根の洞へ入る" },
        { L"Tab", L"配置／組み立てメニュー（ジオラマを置く・はめ込む）" },
        { L"Esc / M", L"村のメニュー" },
        { L"セーブ", L"Esc のメニュー → セーブ（G キーではセーブしない）" },
        { L"話題", L"物語のこと／ジオラマのこと／村のこと／ちょっとしたコツ" },
    };
    static const Row dungeon[] = {
        { L"WASD・Shift", L"歩く・ダッシュ" },
        { L"左クリック", L"攻撃（3段コンボ）" },
        { L"右クリック", L"ソラ：溜め回転／クロベエ：旋回防御／カイリ：貫く大針" },
        { L"Space", L"回避" },
        { L"Q", L"ロックオン／解除（R・ホイールで次の敵へ）" },
        { L"V", L"カメラを背後へ戻す（ホイールで寄る／引く）" },
        { L"E", L"宝箱・糸巻きを開ける、泉・出口を調べる" },
        { L"Tab", L"仲間と交代（3・7・14階はできない）" },
        { L"1〜5", L"薬草・灯油・研ぎ粉・帰り糸・繕い糸" },
        { L"Esc", L"メニュー（アイテム・キャラ・武器・目的・退出・操作）" },
        { L"地図の色", L"黄緑＝糸巻き　黄＝宝箱　橙＝武器の宝箱　赤＝敵　青＝泉・出口" },
    };
    const wchar_t* titles[3] = { L"ジオラマ配置のとき", L"町を歩くとき", L"ダンジョン探索のとき" };
    const Row* rows = page == 0 ? place : page == 1 ? town : dungeon;
    int n = page == 0 ? (int)(sizeof(place) / sizeof(place[0])) : page == 1 ? (int)(sizeof(town) / sizeof(town[0])) : (int)(sizeof(dungeon) / sizeof(dungeon[0]));
    // ページの見出し（3つのタブ）
    for (int i = 0; i < 3; i++)
    {
        float tw = (w - 20) / 3, tx = x + i * (tw + 10);
        InkTab(tx, y, tw, 30, i == page);
        R->Text(titles[i], tx, y + 5, tw, 22, 14, RGBA(1, 1, 1, i == page ? 1.f : 0.65f), 1, i == page, false);
    }
    float rowH = std::min(34.f, (h - 50) / n);
    for (int i = 0; i < n; i++)
    {
        float ry = y + 44 + i * rowH;
        R->FillRect(x, ry, 190, rowH - 6, onPaper ? RGBA(0.14f, 0.11f, 0.1f, 0.92f) : RGBA(0.95f, 0.92f, 0.84f, 0.95f)); R->FillRect(x, ry, 4, rowH - 6, RGBA(0.72f, 0.2f, 0.16f));
        R->Text(rows[i].key, x, ry + (rowH - 6) * 0.5f - 10, 190, 22, 14, onPaper ? RGBA(0.98f, 0.94f, 0.86f) : RGBA(0.15f, 0.15f, 0.25f), 1, true, false);
        R->Text(rows[i].act, x + 204, ry + (rowH - 6) * 0.5f - 10, w - 210, 22, 14, onPaper ? RGBA(0.18f, 0.13f, 0.1f) : RGBA(1, 1, 1, 0.92f), 0, onPaper, !onPaper);
        if (onPaper) R->DashedLine(x + 200, ry + rowH - 4, x + w, ry + rowH - 4, RGBA(0.5f, 0.38f, 0.28f, 0.25f), 1.f);
    }
}

// 川・橋の「となりにつながる」印（DrawPart の partMask と同じ並び）
//   川：1=+x 2=-x 4=+z 8=-z に川（置いた川・もとの川）がある
//   橋：橋の向きで、1=+x 2=-x の端・4=+z 8=-z の横に、ほかの橋がある
int Game::NeighborMask(const Placed& p) const
{
    if (p.t == P_River)
    {
        int m = 0;
        if (RiverCell(p.x + 1, p.z)) m |= 1;
        if (RiverCell(p.x - 1, p.z)) m |= 2;
        if (RiverCell(p.x, p.z + 1)) m |= 4;
        if (RiverCell(p.x, p.z - 1)) m |= 8;
        return m;
    }
    if (p.t != P_Bridge) return 0;
    int w, h; PartSize(p.t, p.rot, w, h);
    XMFLOAT3 c = CellCenter(p.x, p.z, w, h);
    XMMATRIX rm = XMMatrixRotationY(p.rot * PI * 0.5f);
    auto bridgeAt = [&](float lx, float lz) {
        XMFLOAT3 o; XMStoreFloat3(&o, XMVector3Transform(XMVectorSet(lx, 0, lz, 0), rm));
        int cx = (int)std::floor((c.x + o.x) / 2.f + VG * 0.5f), cz = (int)std::floor((c.z + o.z) / 2.f + VG * 0.5f);
        return BridgeCell(cx, cz);
    };
    int m = 0;
    if (bridgeAt(3.f, 0)) m |= 1;
    if (bridgeAt(-3.f, 0)) m |= 2;
    if (bridgeAt(-1.f, 2.f) || bridgeAt(1.f, 2.f)) m |= 4;
    if (bridgeAt(-1.f, -2.f) || bridgeAt(1.f, -2.f)) m |= 8;
    return m;
}

// 動かしているジオラマを、元の場所へ戻す（Esc・町へ戻るとき）
void Game::CancelMove()
{
    if (!moving) return;
    Placed p = movingPart;
    p.x = moveFromX; p.z = moveFromZ;
    placed.push_back(p);
    moving = false;
    RebuildWalkers();
}

// 橋の端（橋を渡った先の1マス）。ここは通り道なので、道と川のほかは置けない
bool Game::BridgeEndCell(int cx, int cz) const
{
    for (auto& p : placed)
    {
        if (p.t != P_Bridge) continue;
        int w, h; PartSize(p.t, p.rot, w, h);
        if (p.rot & 1)
        {
            if (cx >= p.x && cx < p.x + w && (cz == p.z - 1 || cz == p.z + h)) return true;
        }
        else if (cz >= p.z && cz < p.z + h && (cx == p.x - 1 || cx == p.x + w)) return true;
    }
    return false;
}
