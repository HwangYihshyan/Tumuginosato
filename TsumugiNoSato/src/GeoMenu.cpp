// 配置／組み立てメニュー（村の配置モード）
//   左：主要なジオラマのカード（手持ち）と、置いた建物の「はめ込み」カード
//   右：村の名札、小物・はめ込みパーツの一覧（集めた数／全体の数）、下に説明
//   サムネイルは各部品の3Dの模型をオフスクリーンのアトラスに一度だけ描いて、UIに貼っている
#include "Game.h"

namespace
{
    constexpr int kCell = 128, kAtlasW = 1024, kAtlasH = 1024, kCols = kAtlasW / kCell;
    constexpr float kLeftX = 24, kLeftW = 320, kCardH = 100, kCardGap = 8, kListY = 128;
    constexpr float kPanelW = 380, kGridCell = 62, kGridGap = 8;
    constexpr int kGridCols = 5, kGridRows = 3;
    int CellOf(int id) { return id >= SIL_ID ? P_COUNT + S_COUNT + (id - SIL_ID) : id >= SUB_ID ? P_COUNT + (id - SUB_ID) : id; }
}

bool Game::IsMainPart(int id) const
{
    // 置くジオラマ（建物・道・川・橋・木・提灯など）は全部、画面左の一覧で選んでマップに置く。
    // 右の欄ははめ込みパーツだけ
    return id < SUB_ID;
}

// 置くと起きるイベントがまだ残っている部品
bool Game::PartHasEvent(PartType t) const
{
    switch (t)
    {
    case P_HouseTouji: return !Done(EV_TOUJI);
    case P_Kiln: return !Done(EV_KUROBEE);
    case P_Wheel: return !Done(EV_KAIRI);
    case P_Loom: return !Done(EV_LOOM);
    case P_Lantern: return !Done(EV_LANTERN);
    default: return false;
    }
}

// 部品の模型を1マス 128px のアトラスに描く（最初の1回だけ）
void Game::BuildThumbs()
{
    thumbsReady = true;
    if (!R->CreateAtlas(kAtlasW, kAtlasH)) return;
    R->BeginAtlas();
    XMFLOAT3 savedTint = tint;
    tint = F3(0, 0, 0);
    auto shoot = [&](int id, float size, float height, std::function<void()> draw, float zoom = 1.f) {
        int c = CellOf(id);
        FrameSettings fs;
        fs.fovY = 0.55f;
        fs.camTarget = F3(0, height * 0.42f, 0);
        float radius = std::max(size * 0.62f, height * 0.62f) + 0.35f;
        float dist = radius / std::tan(fs.fovY * 0.5f) * 1.05f / zoom;
        XMFLOAT3 dir = F3(0.62f, 0.62f, -1.f);
        XMStoreFloat3(&dir, XMVector3Normalize(XMLoadFloat3(&dir)));
        fs.camPos = Add(fs.camTarget, Mul(dir, dist));
        fs.lightDir = F3(-0.45f, -1.f, 0.55f);
        fs.lightColor = F3(0.95f, 0.9f, 0.82f);
        fs.ambient = F3(0.62f, 0.62f, 0.66f);
        fs.fogStart = 1000; fs.fogEnd = 2000; fs.fogMax = 0; fs.heightFog = 0;
        fs.time = 0;
        R->AtlasCell(fs, (float)(c % kCols) * kCell, (float)(c / kCols) * kCell, (float)kCell, (float)kCell);
        draw();
    };
    for (int t = 0; t < P_COUNT; t++)
    {
        float size = (float)std::max(kParts[t].w, kParts[t].h) * 2.f;
        float h = 3.4f;
        switch (t)
        {
        case P_Tree: h = 4.2f; break;
        case P_Lantern: h = 2.3f; break;
        case P_Path: case P_Field: h = 0.6f; break;
        case P_Cloth: h = 2.0f; break;
        case P_Well: h = 2.2f; break;
        case P_Wheel: h = 3.2f; break;
        case P_Jizo: h = 1.6f; break;
        case P_Bench: h = 2.3f; break;
        case P_Flower: h = 0.6f; break;
        case P_StoneLantern: h = 2.0f; break;
        case P_Scarecrow: h = 2.3f; break;
        default: break;
        }
        float zoom = (t == P_Tree || t == P_Lantern) ? 1.5f : t == P_Path ? 1.3f : (t >= P_Jizo && t <= P_Scarecrow && t != P_Bench) ? 1.4f : 1.f;
        shoot(t, size, h, [&] { DrawPart((PartType)t, F3(0, 0, 0), 0, 1.f, 0.3f); }, zoom);
    }
    for (int s = 0; s < S_COUNT; s++)
        shoot(SUB_ID + s, 1.0f, 1.5f, [&] { DrawSub(s, F3(0, 0, 0), PI, 1.f); }, 1.9f);
    // はめ込み枠に見せる「形」（灰色の影絵）
    silhouette = true;
    for (int s = 0; s < S_COUNT; s++)
        shoot(SIL_ID + s, 1.0f, 1.5f, [&] { DrawSub(s, F3(0, 0, 0), PI, 1.f); }, 1.9f);
    silhouette = false;
    tint = savedTint;
    R->EndAtlas();
}

void Game::DrawThumb(int id, float x, float y, float w, float h, float op)
{
    int c = CellOf(id);
    R->DrawAtlas(x, y, w, h, (float)(c % kCols) * kCell, (float)(c / kCols) * kCell, (float)kCell, (float)kCell, op);
}

// 画面の部品の位置（描画とクリック判定で同じものを使う）
std::vector<Game::GeoRect> Game::GeoLayout(const std::vector<int>& pal) const
{
    std::vector<GeoRect> out;
    float W = (float)R->W, H = (float)R->H;
    // 左の一覧：手持ちの主要なジオラマ → 置いた建物（はめ込み枠のあるもの）
    std::vector<GeoRect> list;
    for (int i = 0; i < (int)pal.size(); i++) if (IsMainPart(pal[i])) list.push_back({ 0, 0, kLeftW, kCardH, 0, i });
    for (int i = 0; i < (int)placed.size(); i++) if (SlotCount(placed[i].t) > 0) list.push_back({ 0, 0, kLeftW, kCardH, 1, i });
    int visible = std::max(1, (int)((H - 150 - kListY) / (kCardH + kCardGap)));
    int scroll = std::clamp(geoScroll, 0, std::max(0, (int)list.size() - visible));
    for (int i = scroll; i < (int)list.size() && i < scroll + visible; i++)
    {
        GeoRect g = list[i];
        g.x = kLeftX; g.y = kListY + (i - scroll) * (kCardH + kCardGap);
        out.push_back(g);
    }
    // 右のパーツ欄：小物とはめ込みパーツ
    float gx0 = W - kPanelW - 24 + 22, gy0 = 122 + 52;
    int k = 0;
    for (int i = 0; i < (int)pal.size(); i++)
    {
        if (IsMainPart(pal[i])) continue;
        if (k >= kGridCols * kGridRows) break;
        out.push_back({ gx0 + (k % kGridCols) * (kGridCell + kGridGap), gy0 + (k / kGridCols) * (kGridCell + kGridGap), kGridCell, kGridCell, 0, i });
        k++;
    }
    return out;
}

void Game::DrawGeoMenu()
{
    float W = (float)R->W, H = (float)R->H;
    std::vector<int> pal = PaletteList();
    if (!pal.empty()) palSel = std::clamp(palSel, 0, (int)pal.size() - 1);
    // 選んでいるカードが一覧から見えるようにスクロール
    {
        int idx = 0, selRow = -1, total = 0;
        for (int i = 0; i < (int)pal.size(); i++) if (IsMainPart(pal[i])) { if (i == palSel) selRow = idx; idx++; }
        if ((geoFocus == 0 && geoCol == 0) || carrySub >= 0) selRow = geoRow;   // キーボードで選んでいる行（運んでいる先のカード）を見せる
        total = idx;
        for (auto& p : placed) if (SlotCount(p.t) > 0) total++;
        int visible = std::max(1, (int)((H - 150 - kListY) / (kCardH + kCardGap)));
        // 選んだ行が変わったときだけ、その行が見えるようにスクロールする（ホイールで動かした位置は勝手に戻さない）
        if (selRow >= 0 && selRow != geoRowShown)
        {
            if (selRow < geoScroll) geoScroll = selRow;
            if (selRow >= geoScroll + visible) geoScroll = selRow - visible + 1;
        }
        geoRowShown = selRow;
        geoScroll = std::clamp(geoScroll, 0, std::max(0, total - visible));
    }
    std::vector<GeoRect> geo = GeoLayout(pal);
    int hover = PartAtCursor();
    wchar_t buf[160];

    // ---- 左上：紋章と題字
    float ex = 52, ey = 44;
    R->FillCircle(ex, ey, 25, RGBA(0.95f, 0.5f, 0.15f));
    R->FillCircle(ex + 5, ey - 4, 17, RGBA(0.25f, 0.6f, 0.95f));
    R->FillCircle(ex - 2, ey + 9, 10, RGBA(0.45f, 0.8f, 0.35f));
    R->FillRound(ex - 10, ey - 2, 20, 6, 3, RGBA(1, 0.95f, 0.7f));
    R->Text(L"配置／組み立て", ex + 36, ey - 22, 330, 44, 28, RGBA(0.75f, 0.85f, 1), 0, true);
    // 復興率
    float rate = rateShown;
    swprintf_s(buf, L"復興率 %d%%", (int)std::round(rate * 100));
    R->Text(buf, kLeftX + 4, 78, 140, 24, 16, RGBA(1, 0.92f, 0.75f), 0, true);
    Bar(kLeftX + 120, 86, kLeftW - 120, 10, rate, RGBA(0.9f, 0.6f, 0.22f), RGBA(0.15f, 0.15f, 0.2f, 0.85f));


    // ---- 左：カード
    int mainCount = 0;
    for (int id : pal) if (IsMainPart(id)) mainCount++;
    int listTotal = mainCount;
    for (auto& p : placed) if (SlotCount(p.t) > 0) listTotal++;
    for (auto& g : geo)
    {
        if (g.x > W / 2) continue;
        float x = g.x, y = g.y;
        if (g.kind == 0)
        {
            int id = pal[g.ref];
            PartType t = (PartType)id;
            bool sel = g.ref == palSel;
            int ns = SlotCount(t);
            if (ns > 0)
            {
                // はめ込みのある建物は、置いた建物のカードと同じ形（ゲージと 0/数・絵・名前・枠・右下の×数）
                // はめ込んだまま片付けた建物なら、そのはめ込みのようすを出す
                int si = StoredIndex(t);
                int sub[MAX_SLOT] = { -1, -1, -1 };
                if (si >= 0) for (int s = 0; s < MAX_SLOT; s++) sub[s] = stored[si].sub[s];
                int filled = 0;
                for (int s = 0; s < ns; s++) if (sub[s] >= 0) filled++;
                bool done = filled == ns;
                R->FillRound(x, y, g.w, g.h, 8, done ? RGBA(0.48f, 0.5f, 0.34f, 0.92f) : sel ? RGBA(0.4f, 0.5f, 0.47f, 0.95f) : RGBA(0.32f, 0.4f, 0.38f, 0.85f));
                R->StrokeRound(x + 3, y + 3, g.w - 6, g.h - 6, 6, sel ? RGBA(0.95f, 0.55f, 0.15f) : done ? RGBA(0.95f, 0.8f, 0.4f) : RGBA(0.6f, 0.72f, 0.68f, 0.8f), sel ? 2.5f : 1.5f);
                R->FillRound(x + 8, y + 26, 8, 64, 4, RGBA(0.15f, 0.18f, 0.18f));
                float k = (float)filled / ns;
                if (k > 0) R->FillRound(x + 8, y + 26 + 64 * (1 - k), 8, 64 * k, 4, RGBA(0.95f, 0.6f, 0.2f));
                swprintf_s(buf, L"%d/%d", filled, ns);
                R->Text(buf, x + 6, y + 4, 60, 20, 13, RGBA(1, 1, 1), 0, true);
                swprintf_s(buf, L"×%d", inventory[t]);
                R->Text(buf, x + g.w - 70, y + 62, 60, 30, 20, RGBA(1, 0.8f, 0.4f), 2, true);
                DrawThumb(id, x + 22, y + 10, 96, 84, 0.9f);
                R->Text(kParts[t].name, x + 124, y + 8, g.w - 130, 24, 16, RGBA(1, 0.97f, 0.9f), 0, true);
                if (done) R->Text(L"完成", x + 196, y + 10, g.w - 200, 20, 13, RGBA(1, 0.9f, 0.5f), 2, true);
                for (int s = 0; s < ns; s++)
                {
                    float cx = x + 146 + s * 52, cy = y + 66;
                    R->FillCircle(cx, cy, 21, sub[s] >= 0 ? RGBA(0.95f, 0.9f, 0.78f, 0.95f) : RGBA(0.2f, 0.25f, 0.25f, 0.9f));
                    if (sub[s] >= 0) DrawThumb(SUB_ID + sub[s], cx - 22, cy - 24, 44, 44);
                    else DrawThumb(SIL_ID + SlotNeed(t, s), cx - 23, cy - 25, 46, 46);
                    R->StrokeCircle(cx, cy, 21, RGBA(0.75f, 0.85f, 0.82f), 1.5f);
                }
                if (sel) R->Text(L"☞", x - 24, y + 30, 26, 36, 26, RGBA(1, 1, 1), 1, true);
                continue;
            }
            R->FillRound(x, y, g.w, g.h, 8, sel ? RGBA(0.9f, 0.8f, 0.6f, 0.97f) : RGBA(0.42f, 0.36f, 0.27f, 0.85f));
            R->StrokeRound(x + 3, y + 3, g.w - 6, g.h - 6, 6, sel ? RGBA(0.95f, 0.55f, 0.15f) : RGBA(0.75f, 0.65f, 0.45f, 0.8f), sel ? 2.5f : 1.5f);
            R->FillRound(x + 10, y + 8, 112, 84, 6, sel ? RGBA(0.98f, 0.95f, 0.88f) : RGBA(0.3f, 0.26f, 0.2f, 0.8f));
            DrawThumb(id, x + 12, y + 4, 108, 92, inventory[t] > 0 ? 1.f : 0.4f);   // 手持ちが0のものはうすく（一覧には残る）
            R->Text(kParts[t].name, x + 130, y + 10, g.w - 140, 26, 17, sel ? RGBA(0.2f, 0.13f, 0.08f) : RGBA(1, 0.95f, 0.85f), 0, true, !sel);
            swprintf_s(buf, L"%d×%d", kParts[t].w, kParts[t].h);   // 説明は大きさ（何マス）だけ
            R->Text(buf, x + 130, y + 38, g.w - 140, 22, 13, sel ? RGBA(0.35f, 0.28f, 0.2f) : RGBA(1, 1, 1, 0.75f), 0, false, !sel);
            swprintf_s(buf, L"×%d", inventory[t]);
            R->Text(buf, x + g.w - 70, y + 62, 60, 30, 20, sel ? RGBA(0.75f, 0.35f, 0.08f) : RGBA(1, 0.8f, 0.4f), 2, true, !sel);
            if (sel) R->Text(L"☞", x - 24, y + 30, 26, 36, 26, RGBA(1, 1, 1), 1, true);
        }
        else
        {
            const Placed& p = placed[g.ref];
            int n = SlotCount(p.t), filled = 0;
            for (int s = 0; s < n; s++) if (p.sub[s] >= 0) filled++;
            bool done = filled == n;
            R->FillRound(x, y, g.w, g.h, 8, done ? RGBA(0.48f, 0.5f, 0.34f, 0.92f) : RGBA(0.32f, 0.4f, 0.38f, 0.85f));
            R->StrokeRound(x + 3, y + 3, g.w - 6, g.h - 6, 6, done ? RGBA(0.95f, 0.8f, 0.4f) : RGBA(0.6f, 0.72f, 0.68f, 0.8f), 1.5f);
            // 進み具合（左のゲージと数）
            R->FillRound(x + 8, y + 26, 8, 64, 4, RGBA(0.15f, 0.18f, 0.18f));
            float k = n > 0 ? (float)filled / n : 0.f;
            if (k > 0) R->FillRound(x + 8, y + 26 + 64 * (1 - k), 8, 64 * k, 4, RGBA(0.95f, 0.6f, 0.2f));
            swprintf_s(buf, L"%d/%d", filled, n);
            R->Text(buf, x + 6, y + 4, 60, 20, 13, RGBA(1, 1, 1), 0, true);
            swprintf_s(buf, L"×%d", inventory[p.t]);   // この建物の手持ち（置いたぶんは ×0）
            R->Text(buf, x + g.w - 70, y + 62, 60, 30, 20, RGBA(1, 0.8f, 0.4f), 2, true);   // 数はどのカードも右下
            DrawThumb(p.t, x + 22, y + 10, 96, 84, 0.9f);
            R->Text(kParts[p.t].name, x + 124, y + 8, g.w - 130, 24, 16, RGBA(1, 0.97f, 0.9f), 0, true);
            if (!p.open && !done) R->Text(L"未解放：住人と話す", x + 196, y + 10, g.w - 200, 20, 12, RGBA(1, 0.75f, 0.6f), 2, true);
            // 枠：解放前は「？」（人物は最初から形が見える）→ 解放後ははめるパーツの形（うすく）→ はめると色がつく
            for (int s = 0; s < n; s++)
            {
                float cx = x + 146 + s * 52, cy = y + 66;
                int need = SlotNeed(p.t, s);
                bool shown = true;   // どのカードも、はめるパーツの形を出してそろえる（解放前は「未解放」の文字で知らせる）
                bool target = carrySub >= 0 && p.sub[s] < 0 && need == carrySub && (p.open || IsPerson(need));
                R->FillCircle(cx, cy, 21, p.sub[s] >= 0 ? RGBA(0.95f, 0.9f, 0.78f, 0.95f) : RGBA(0.2f, 0.25f, 0.25f, 0.9f));
                if (p.sub[s] >= 0) DrawThumb(SUB_ID + p.sub[s], cx - 22, cy - 24, 44, 44);
                else if (shown)
                {
                    DrawThumb(SIL_ID + need, cx - 23, cy - 25, 46, 46);          // はめるパーツの形（灰色の影絵）
                }
                else R->Text(L"？", cx - 20, cy - 13, 40, 26, 18, RGBA(1, 1, 1, 0.6f), 1, true, false);
                R->StrokeCircle(cx, cy, 21, target ? RGBA(0.5f, 1.f, 0.55f, 0.6f + 0.4f * std::sin(time * 8.f)) : RGBA(0.75f, 0.85f, 0.82f), target ? 3.f : 1.5f);
                // キーボードで運ぶ先
                if (carrySub >= 0 && !carryByMouse)
                {
                    int ci = 0;
                    for (int i = 0; i < (int)placed.size(); i++) if (SlotCount(placed[i].t) > 0) { if (i == g.ref) break; ci++; }
                    if (ci == carryCard && s == carrySlot) R->StrokeCircle(cx, cy, 26, RGBA(1, 0.9f, 0.3f, 0.6f + 0.4f * std::sin(time * 6.f)), 3.f);
                }
            }
            // 全部はめ込んで完成したら「イベント！」（町でそばへ行くと起きる）
            if (done && !p.evDone)
            {
                float bx = x + 196, by = y + 6;
                R->FillRound(bx, by, 110, 28, 14, RGBA(0.97f, 0.55f, 0.25f));
                R->FillCircle(bx + 10, by + 24, 6, RGBA(0.97f, 0.55f, 0.25f));
                R->StrokeRound(bx, by, 110, 28, 14, RGBA(1, 0.9f, 0.75f), 1.5f);
                R->Text(L"イベント！", bx, by + 3, 110, 24, 15, RGBA(1, 1, 1), 1, true);
            }
            else if (done) R->Text(L"完成", x + 196, y + 10, g.w - 200, 20, 13, RGBA(1, 0.9f, 0.5f), 2, true);
        }
    }
    // はめ込む演出：枠に光がはじける
    if (flyT >= 0 && flyPart >= 0)
    {
        float cx, cy;
        if (SlotScreen(flyPart, flySlot, cx, cy))
        {
            float k = Saturate(flyT / 0.6f);
            R->StrokeCircle(cx, cy, 21 + 30 * k, RGBA(1, 0.9f, 0.5f, 1 - k), 4.f * (1 - k) + 1);
            R->StrokeCircle(cx, cy, 21 + 14 * k, RGBA(1, 1, 1, 1 - k), 2.f);
        }
    }
    {
        int visible = std::max(1, (int)((H - 150 - kListY) / (kCardH + kCardGap)));
        if (geoScroll > 0) R->Text(L"▲", kLeftX, kListY - 26, kLeftW, 22, 16, RGBA(1, 0.9f, 0.6f), 1, true);
        if (geoScroll + visible < listTotal) R->Text(L"▼", kLeftX, kListY + visible * (kCardH + kCardGap) - 4, kLeftW, 22, 16, RGBA(1, 0.9f, 0.6f), 1, true);
        if (listTotal == 0)
        {
            R->FillRound(kLeftX, kListY, kLeftW, 80, 8, RGBA(0.42f, 0.36f, 0.27f, 0.8f));
            R->Text(L"手持ちの主要なジオラマはない\nTab で町を歩き、北の入口から根の洞へ", kLeftX + 10, kListY + 14, kLeftW - 20, 56, 15, RGBA(1, 0.95f, 0.85f), 1, true);
        }
    }

    // ---- 右：村の名札とパーツ欄
    float px = W - kPanelW - 24;
    R->FillRound(px + 20, 66, kPanelW - 40, 44, 10, RGBA(0.62f, 0.5f, 0.32f));
    R->FillRound(px + 26, 70, kPanelW - 52, 36, 8, RGBA(0.78f, 0.66f, 0.45f));
    R->StrokeRound(px + 20, 66, kPanelW - 40, 44, 10, RGBA(0.35f, 0.25f, 0.15f), 2);
    R->Text(L"ハタオリ村", px + 20, 72, kPanelW - 40, 32, 22, RGBA(0.95f, 0.85f, 0.35f), 1, true);
    float gy = 122, gh = kGridRows * (kGridCell + kGridGap) + 70;
    R->FillRound(px, gy, kPanelW, gh, 14, RGBA(0.45f, 0.55f, 0.52f, 0.95f));
    R->FillRound(px + 14, gy + 46, kPanelW - 28, gh - 58, 10, RGBA(0.3f, 0.36f, 0.36f, 0.95f));
    R->StrokeRound(px, gy, kPanelW, gh, 14, RGBA(0.75f, 0.85f, 0.8f), 2);
    // 左上の糸巻きの印
    R->FillRound(px + 18, gy + 10, 30, 30, 6, RGBA(0.6f, 0.4f, 0.25f));
    R->FillRect(px + 22, gy + 16, 22, 18, RGBA(0.85f, 0.3f, 0.25f));
    for (int i = 0; i < 3; i++) R->Line(px + 22, gy + 19 + i * 6, px + 44, gy + 19 + i * 6, RGBA(1, 0.7f, 0.65f), 1.5f);
    R->Text(L"小物・はめ込みパーツ", px + 56, gy + 12, 220, 26, 15, RGBA(1, 1, 1), 0, true);
    // 集めた数／全体の数
    int got = 0, all = 0;
    for (int f = 1; f <= MAX_FLOOR; f++) { all += LootTotal(f); got += LootTotal(f) - LootLeft(f); }   // 糸巻きから集めた数／全部の数
    float bcx = px + kPanelW - 30, bcy = gy + 6;
    R->FillCircle(bcx, bcy, 28, RGBA(0.2f, 0.27f, 0.3f));
    R->StrokeCircle(bcx, bcy, 28, RGBA(0.85f, 0.9f, 0.88f), 2);
    R->Text(std::to_wstring(got), bcx - 26, bcy - 24, 52, 22, 16, RGBA(1, 1, 1), 1, true, false);
    R->Line(bcx - 16, bcy, bcx + 16, bcy, RGBA(1, 1, 1, 0.8f), 1.5f);
    R->Text(std::to_wstring(all), bcx - 26, bcy + 2, 52, 22, 16, RGBA(1, 1, 1), 1, true, false);
    // マス（空きマスも描く）
    float gx0 = px + 22, gy0 = gy + 52;
    for (int k = 0; k < kGridCols * kGridRows; k++)
    {
        float x = gx0 + (k % kGridCols) * (kGridCell + kGridGap), y = gy0 + (k / kGridCols) * (kGridCell + kGridGap);
        R->FillRound(x, y, kGridCell, kGridCell, 8, RGBA(0.22f, 0.27f, 0.27f, 0.9f));
    }
    for (auto& g : geo)
    {
        if (g.x < W / 2) continue;
        int id = pal[g.ref];
        bool sel = g.ref == palSel, isSub = id >= SUB_ID;
        if (sel) R->FillRound(g.x - 2, g.y - 2, g.w + 4, g.h + 4, 9, RGBA(0.98f, 0.94f, 0.82f));
        R->StrokeRound(g.x, g.y, g.w, g.h, 8, sel ? RGBA(0.95f, 0.55f, 0.15f) : isSub ? RGBA(0.5f, 0.8f, 1.f, 0.7f) : RGBA(0.75f, 0.7f, 0.55f, 0.7f), sel ? 2.5f : 1.5f);
        DrawThumb(id, g.x + 1, g.y - 2, g.w - 2, g.h - 2);
        swprintf_s(buf, L"×%d", isSub ? subInv[id - SUB_ID] : inventory[id]);   // 手持ちの数
        R->Text(buf, g.x + 2, g.y + g.h - 22, g.w - 6, 22, 15, RGBA(1, 0.8f, 0.4f), 2, true);   // 数は右下（カードと同じ書き方）
    }

    // ---- 右下：説明
    float dy = gy + gh + 12, dh = std::min(140.f, H - 44 - dy);
    R->FillRound(px, dy, kPanelW, dh, 12, RGBA(0.1f, 0.12f, 0.28f, 0.95f));
    R->StrokeRound(px + 4, dy + 4, kPanelW - 8, dh - 8, 10, RGBA(0.75f, 0.8f, 0.95f), 1.5f);
    std::wstring title, desc;
    if (!pal.empty())
    {
        int id = pal[palSel];
        if (id >= SUB_ID) { title = kSubs[id - SUB_ID].name; desc = kSubs[id - SUB_ID].desc; }
        else { title = kParts[id].name; desc = kParts[id].desc; }
    }
    if (!title.empty()) R->Text(L"「" + title + L"」", px + 18, dy + 10, kPanelW - 36, 26, 17, RGBA(1, 1, 1), 0, true);
    if (!desc.empty()) R->Text(desc, px + 18, dy + 36, kPanelW - 36, 24, 14, RGBA(1, 1, 1, 0.85f));
    if (hover >= 0 && dh > 90)
    {
        const Placed& p = placed[hover];
        std::wstring s = L"カーソル：" + std::wstring(kParts[p.t].name);
        for (int r = 0; r < kReqCount; r++)
            if (kReqs[r].owner == p.t) { s += std::wstring(L"　") + (ReqSatisfied(r) ? L"✓" : L"○") + kReqs[r].text; break; }
        R->Text(s, px + 18, dy + 62, kPanelW - 36, 40, 13, RGBA(1, 0.9f, 0.65f));
    }

    // 運んでいるはめ込みパーツ（マウスについてくる／キーボードなら運ぶ先の枠のそば）
    if (carrySub >= 0)
    {
        float cx = uiMouseX + 18, cy = uiMouseY + 18;
        if (!carryByMouse)
        {
            int ci = 0;
            for (int i = 0; i < (int)placed.size(); i++)
                if (SlotCount(placed[i].t) > 0) { if (ci == carryCard) { float sx, sy; if (SlotScreen(i, carrySlot, sx, sy)) { cx = sx + 30; cy = sy - 34; } break; } ci++; }
        }
        R->FillCircle(cx, cy, 34, RGBA(1, 0.97f, 0.88f, 0.92f));
        R->StrokeCircle(cx, cy, 34, RGBA(0.95f, 0.6f, 0.2f), 3);
        DrawThumb(SUB_ID + carrySub, cx - 34, cy - 38, 68, 68);
        R->FillRound(W / 2 - 260, 112, 520, 34, 17, RGBA(0.35f, 0.22f, 0.08f, 0.9f));
        R->Text(std::wstring(L"運んでいる：") + kSubs[carrySub].name + L"（×" + std::to_wstring(subInv[carrySub]) + L"）　→ 左のカードの同じ形の枠へ（Esc でやめる）", W / 2 - 260, 118, 520, 24, 15, RGBA(1, 0.95f, 0.8f), 1, true);
    }
    // キーボード操作の場所（メニュー／マップ）
    {
        std::vector<std::pair<int, int>> left; std::vector<int> right;
        GeoLists(pal, left, right);
        float pulse = 0.6f + 0.4f * std::sin(time * 6.f);
        if (geoFocus == 0)
        {
            for (auto& g : geo)
            {
                bool hit = geoCol == 0 ? (geoRow < (int)left.size() && g.kind == left[geoRow].first && g.ref == left[geoRow].second && (g.x < W / 2))
                                       : (geoRow < (int)right.size() && g.kind == 0 && g.ref == right[geoRow] && g.x > W / 2);
                if (hit) R->StrokeRound(g.x - 4, g.y - 4, g.w + 8, g.h + 8, 10, RGBA(1, 0.9f, 0.3f, pulse), 3.5f);
            }
        }
        const wchar_t* mode = moving ? L"移動中（クリックでここへ移動／もう一度 X・右クリックで片付け）" : carrySub >= 0 ? L"はめ込み中（W/S：カード　A/D：枠　Enter：はめる）" : geoFocus == 0 ? L"メニューを操作中（Enter で決定）" : L"マップを操作中（Esc でメニューへ）";
        InkPanel(W / 2 - 230, H - 60, 460, 34, 0.88f);
        R->Text(mode, W / 2 - 230, H - 54, 460, 24, 15, RGBA(1, 0.95f, 0.8f), 1, true);
    }
}
