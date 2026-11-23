// 武器の画面（ダンジョンのメニュー「武器変更」）
//   中央に武器を持ったキャラを3Dで見せ、まわりに能力・耐久と経験のゲージ・道具・説明を並べる
#include "Game.h"

namespace
{
    XMMATRIX S(float x, float y, float z) { return XMMatrixScaling(x, y, z); }
    XMMATRIX Tr(float x, float y, float z) { return XMMatrixTranslation(x, y, z); }
    const wchar_t* kElem[3] = { L"なし", L"火", L"水" };
    const wchar_t* kTabsW[6] = { L"アイテム", L"キャラ変更", L"武器", L"目的", L"退出", L"操作" };
}

// 見ているキャラが持っている武器（weapons の番号）
static std::vector<int> OwnedOf(const std::vector<OwnedWeapon>& ws, int def)
{
    std::vector<int> v;
    for (int i = 0; i < (int)ws.size(); i++) if (kWeapons[ws[i].def].owner == def) v.push_back(i);
    return v;
}

void Game::RenderWeaponShowcase()
{
    if (!joined[menuChar]) menuChar = active;
    FrameSettings fs;
    fs.camTarget = F3(0, 1.25f, 0);
    fs.camPos = F3(0, 2.1f, -6.6f);
    fs.fovY = 0.6f;
    fs.clear = F3(0.06f, 0.05f, 0.06f);
    fs.fogColor = fs.clear; fs.fogStart = 7; fs.fogEnd = 20; fs.fogMax = 0.85f;
    fs.ambient = F3(0.45f, 0.42f, 0.46f);
    fs.lightDir = F3(-0.45f, -1.f, 0.55f);
    fs.lightColor = F3(1.0f, 0.9f, 0.8f);
    fs.time = time;
    fs.lights.push_back({ F3(-2.4f, 2.6f, -1.4f), 7.f, F3(1.3f, 0.75f, 0.35f) });
    fs.lights.push_back({ F3(2.6f, 2.3f, -1.0f), 6.f, F3(0.45f, 0.6f, 1.1f) });
    R->BeginFrame(fs);

    // 背景：洞の石壁と柱、赤い敷物と丸い台
    DrawParams wall; wall.tex = T["cave_wall"]; wall.color = C4(0.55f, 0.52f, 0.55f); wall.uv = { 5, 2, 0, 0 };
    R->Draw(R->cube, S(18, 9, 0.4f) * Tr(0, 4.f, 4.5f), wall);
    DrawParams pil; pil.tex = T["cave_wall"]; pil.color = C4(0.45f, 0.42f, 0.45f); pil.outline = 0.02f; pil.uv = { 1, 3, 0, 0 };
    for (int s = -1; s <= 1; s += 2)
    {
        R->Draw(R->cylinder, S(1.0f, 8.f, 1.0f) * Tr(s * 3.6f, 0, 2.6f), pil);
        DrawParams cap = pil; cap.color = C4(0.35f, 0.3f, 0.3f);
        R->Draw(R->cube, S(1.4f, 0.3f, 1.4f) * Tr(s * 3.6f, 0.15f, 2.6f), cap);
    }
    DrawParams fl; fl.tex = T["stonepave"]; fl.color = C4(0.5f, 0.48f, 0.5f); fl.uv = { 6, 5, 0, 0 };
    R->Draw(R->cube, S(18, 0.2f, 14) * Tr(0, -0.1f, 2.f), fl);
    DrawParams carpet; carpet.color = C4(0.55f, 0.12f, 0.1f);
    R->Draw(R->cube, S(3.4f, 0.03f, 14) * Tr(0, 0.015f, 2.f), carpet);
    DrawParams trim; trim.color = C4(0.85f, 0.65f, 0.25f);
    for (int s = -1; s <= 1; s += 2) R->Draw(R->cube, S(0.12f, 0.035f, 14) * Tr(s * 1.6f, 0.02f, 2.f), trim);
    DrawParams ped; ped.color = C4(0.32f, 0.28f, 0.3f); ped.outline = 0.02f; ped.tex = T["stonepave"];
    R->Draw(R->cylinder, S(2.4f, 0.14f, 2.4f) * Tr(0, 0.0f, 0), ped);

    // 武器を持ったキャラ（選んでいる武器を持たせて見せる）
    Actor a = party[menuChar];
    std::vector<int> mine = OwnedOf(weapons, a.def);
    if (menuSel < (int)mine.size() && mine[menuSel] != a.weapon)
    {
        const OwnedWeapon& w = weapons[mine[menuSel]];
        a.weapon = mine[menuSel]; a.dur = w.dur; a.broken = w.broken; a.lvl = w.lvl; a.exp = w.exp;
    }
    a.pos = F3(0, 0.14f, 0);
    a.yaw = PI + std::sin(time * 0.7f) * 0.55f;
    a.st = ASt::Normal; a.stT = 0; a.speedNow = 0; a.walkT = 0; a.flash = 0; a.invuln = 0; a.kb = {};
    DrawActor(a);
    DrawFx();
}

void Game::DrawWeaponScreen()
{
    float W = (float)R->W, H = (float)R->H;
    if (!joined[menuChar]) menuChar = active;
    Actor& A = party[menuChar];
    const CharDef& cd = Def(A);
    std::vector<int> mine = OwnedOf(weapons, A.def);
    int rows = (int)mine.size() + 2;
    int sel = std::clamp(menuSel, 0, rows - 1);
    // 見せる武器（武器の行を選んでいればその武器、スロットの行なら装備中の武器）
    int show = sel < (int)mine.size() ? mine[sel] : A.weapon;
    const OwnedWeapon& ow = weapons[show];
    const WeaponDef& wd = kWeapons[ow.def];
    bool equipped = show == A.weapon;
    float dur = equipped ? A.dur : ow.dur;
    bool broken = equipped ? A.broken : ow.broken;
    int lvl = equipped ? A.lvl : ow.lvl;
    float exp = equipped ? A.exp : ow.exp;
    wchar_t buf[160];

    // うっすら周辺を暗く
    R->FillRect(0, 0, W, 60, RGBA(0, 0, 0, 0.35f));
    R->FillRect(0, H - 170, W, 170, RGBA(0, 0, 0, 0.3f));

    // ---- 左上：紋章と「武器」
    float ex = 70, ey = 58;
    R->FillCircle(ex, ey, 30, RGBA(0.95f, 0.45f, 0.15f));
    R->FillCircle(ex + 8, ey - 6, 21, RGBA(0.2f, 0.5f, 0.95f));
    R->FillCircle(ex - 9, ey + 8, 12, RGBA(1.f, 0.8f, 0.25f));
    R->Line(ex - 18, ey + 16, ex + 18, ey - 16, RGBA(1, 1, 1), 4);       // 交差した針
    R->Line(ex - 18, ey - 16, ex + 18, ey + 16, RGBA(0.25f, 0.95f, 0.85f), 4);
    R->Text(L"武器", ex + 42, ey - 26, 200, 50, 36, RGBA(0.75f, 0.88f, 1), 0, true);

    // ---- 上：タブ（A/D）
    for (int i = 0; i < 6 && scene == SceneId::Dungeon; i++)
    {
        float tx = W - 6 * 92 - 20 + i * 92;
        bool on = i == 2;
        R->FillRound(tx, 12, 86, 28, 14, on ? RGBA(0.85f, 0.55f, 0.2f) : RGBA(1, 1, 1, 0.1f));
        R->Text(kTabsW[i], tx, 16, 86, 22, 13, RGBA(1, 1, 1, on ? 1.f : 0.7f), 1, on, false);
    }

    // ---- 上中央：キャラの顔と切り替え（Z ◀ ▶ C）
    float pcx = W / 2, pcy = 82;
    R->FillCircle(pcx, pcy, 52, RGBA(0.95f, 0.93f, 0.88f));
    DrawFaceIcon(A.def, pcx, pcy, 46);
    R->StrokeCircle(pcx, pcy, 52, RGBA(0.25f, 0.25f, 0.3f), 3);
    int nJoined = joined[0] + joined[1] + joined[2];
    if (nJoined > 1)
    {
        R->Text(L"◀", pcx - 104, pcy - 16, 30, 32, 24, RGBA(1, 0.9f, 0.2f), 1, true);
        R->Text(L"▶", pcx + 74, pcy - 16, 30, 32, 24, RGBA(1, 0.9f, 0.2f), 1, true);
        for (int s = -1; s <= 1; s += 2)
        {
            float kx = pcx + s * 96 - 22;
            R->FillRound(kx, pcy - 60, 44, 26, 6, RGBA(0.1f, 0.1f, 0.12f, 0.85f));
            R->StrokeRound(kx, pcy - 60, 44, 26, 6, RGBA(1, 1, 1, 0.7f), 1.5f);
            R->Text(s < 0 ? L"Z" : L"C", kx, pcy - 57, 44, 22, 16, RGBA(1, 1, 1), 1, true, false);
        }
    }
    R->Text(cd.name, pcx - 100, pcy + 50, 200, 26, 16, RGBA(1, 0.95f, 0.85f), 1, true);

    // ---- 名前の巻物
    float sw = 380, sx = pcx - sw / 2, sy = 168;
    R->FillRound(sx, sy, sw, 46, 6, RGBA(0.86f, 0.8f, 0.68f));
    R->FillRect(sx + 6, sy + 4, sw - 12, 4, RGBA(1, 0.97f, 0.88f, 0.6f));
    for (int s = 0; s < 2; s++)
    {
        float cx = s ? sx + sw : sx;
        R->FillRound(cx - 10, sy - 6, 20, 58, 9, RGBA(0.7f, 0.62f, 0.48f));
        R->StrokeRound(cx - 10, sy - 6, 20, 58, 9, RGBA(0.45f, 0.38f, 0.28f), 1.5f);
    }
    const wchar_t* el = wd.element == 1 ? L"（火）" : wd.element == 2 ? L"（水）" : L"";
    R->Text(std::wstring(wd.name) + el, sx, sy + 8, sw, 32, 22, wd.element == 1 ? RGBA(0.65f, 0.15f, 0.08f) : wd.element == 2 ? RGBA(0.1f, 0.3f, 0.65f) : RGBA(0.2f, 0.15f, 0.1f), 1, true, false);
    // 装備中かどうか
    float bx = pcx - 70, by = sy + 54;
    R->FillRound(bx, by, 140, 26, 13, equipped ? RGBA(0.08f, 0.08f, 0.1f, 0.9f) : RGBA(0.85f, 0.55f, 0.2f, 0.95f));
    R->Text(equipped ? L"装備中" : L"Enter で装備", bx, by + 3, 140, 22, 15, equipped ? RGBA(1, 0.85f, 0.4f) : RGBA(1, 1, 1), 1, true, false);

    // ---- キャラの左：能力
    float stx = pcx - 330, sty = 270;
    R->FillRound(stx - 12, sty - 14, 190, 196, 10, RGBA(0.03f, 0.04f, 0.08f, 0.55f));
    swprintf_s(buf, L"Lv %d", lvl);
    R->FillCircle(stx + 10, sty + 6, 9, RGBA(0.95f, 0.95f, 1));
    R->Text(buf, stx + 26, sty - 6, 140, 24, 18, RGBA(1, 1, 1), 0, true);
    float atk = cd.dmg[0] * (broken ? 0.45f : wd.atk) * (1.f + 0.06f * (lvl - 1)) * 10.f;
    float atkMul = 1, spd = 1;
    for (int s = 0; s < 2; s++)
    {
        if (ow.att[s] == AT_STONE) atkMul += 0.15f;
        if (ow.att[s] == AT_FIRE) atkMul += 0.10f;
        if (ow.att[s] == AT_FEATHER) spd += 0.12f;
    }
    struct Row { const wchar_t* k; D2D1_COLOR_F c; std::wstring v; };
    Row rowsS[5] = {
        { L"攻撃", RGBA(1.f, 0.55f, 0.4f),  std::to_wstring((int)std::round(atk * atkMul)) },
        { L"耐久", RGBA(1.f, 0.85f, 0.35f), std::to_wstring(wd.maxDur) },
        { L"速さ", RGBA(0.5f, 1.f, 0.6f),   std::to_wstring((int)std::round(cd.speed * spd * 10)) },
        { L"属性", RGBA(0.6f, 0.8f, 1.f),   kElem[wd.element] },
        { L"体力", RGBA(1.f, 0.7f, 0.85f),  std::to_wstring((int)A.hp) + L"/" + std::to_wstring((int)A.maxHp) },
    };
    for (int i = 0; i < 5; i++)
    {
        float y = sty + 26 + i * 30.f;
        R->Text(rowsS[i].k, stx, y, 60, 26, 17, rowsS[i].c, 0, true);
        R->Text(rowsS[i].v, stx + 56, y, 100, 26, 18, RGBA(1, 1, 1), 2, true);
    }

    // ---- キャラの右上：成長の札
    float gx = pcx + 120, gy = 262;
    R->FillRound(gx, gy, 300, 52, 10, RGBA(0.05f, 0.07f, 0.13f, 0.85f));
    R->StrokeRound(gx, gy, 300, 52, 10, RGBA(0.6f, 0.75f, 0.95f), 2);
    R->FillRound(gx + 10, gy + 10, 32, 32, 6, RGBA(0.85f, 0.85f, 0.9f));
    R->Line(gx + 16, gy + 36, gx + 36, gy + 16, RGBA(0.3f, 0.35f, 0.45f), 4);
    swprintf_s(buf, L"成長：あと %d で Lv%d", (int)std::ceil(std::max(0.f, 100.f * lvl - exp)), lvl + 1);
    R->Text(buf, gx + 52, gy + 13, 240, 28, 17, RGBA(1, 1, 1), 0, true);

    // ---- アタッチメント（スロット2つ）
    float ax = pcx + 120, ay = 328;
    for (int s = 0; s < 2; s++)
    {
        int row = (int)mine.size() + s;
        bool on = sel == row;
        float y = ay + s * 44.f;
        R->FillRound(ax, y, 300, 38, 9, on ? RGBA(0.98f, 0.94f, 0.85f) : RGBA(0.05f, 0.07f, 0.13f, 0.8f));
        int at = weapons[A.weapon].att[s];
        swprintf_s(buf, L"枠%d：%s", s + 1, at >= 0 ? kAttach[at].name : L"（空き）");
        R->Text(buf, ax + 14, y + 7, 160, 26, 16, on ? RGBA(0.12f, 0.15f, 0.25f) : RGBA(0.85f, 0.95f, 0.8f), 0, true, !on);
        R->Text(at >= 0 ? kAttach[at].desc : L"Enter で付ける", ax + 130, y + 9, 160, 24, 13, on ? RGBA(0.3f, 0.35f, 0.45f) : RGBA(1, 1, 1, 0.65f), 2, false, !on);
    }

    // ---- 右端：持っている武器の一覧（W/S）
    float lx = W - 270, ly = 70;
    R->Text(L"持っている武器", lx, ly, 250, 24, 15, RGBA(1, 0.9f, 0.7f), 0, true);
    for (int i = 0; i < (int)mine.size(); i++)
    {
        const OwnedWeapon& w = weapons[mine[i]];
        bool on = sel == i;
        float y = ly + 28 + i * 40.f;
        R->FillRound(lx, y, 250, 34, 8, on ? RGBA(0.98f, 0.94f, 0.85f) : RGBA(0.05f, 0.07f, 0.13f, 0.8f));
        if (on) R->Text(L"☞", lx - 30, y + 2, 30, 30, 22, RGBA(1, 1, 1), 1, true);
        std::wstring nm = std::wstring(mine[i] == A.weapon ? L"▶ " : L"　") + kWeapons[w.def].name;
        bool br = mine[i] == A.weapon ? A.broken : w.broken;
        R->Text(nm, lx + 8, y + 6, 180, 24, 15, on ? RGBA(0.12f, 0.15f, 0.25f) : RGBA(1, 1, 1), 0, true, !on);
        if (br) R->Text(L"壊れ", lx + 190, y + 7, 52, 22, 13, RGBA(1, 0.45f, 0.4f), 2, true, !on);
        else if (kWeapons[w.def].element) R->Text(kElem[kWeapons[w.def].element], lx + 190, y + 7, 52, 22, 13, kWeapons[w.def].element == 1 ? RGBA(1, 0.5f, 0.35f) : RGBA(0.45f, 0.7f, 1), 2, true, !on);
    }

    // ---- 下中央：耐久と経験のゲージ
    float gbx = pcx - 230, gby = 478;
    R->FillRound(gbx, gby, 460, 76, 10, RGBA(0.28f, 0.34f, 0.5f, 0.92f));
    R->StrokeRound(gbx, gby, 460, 76, 10, RGBA(0.75f, 0.82f, 0.95f), 2);
    float dk = Saturate(dur / std::max(1.f, (float)wd.maxDur));
    R->Text(L"耐久", gbx + 16, gby + 10, 60, 24, 16, RGBA(1, 1, 1), 0, true);
    swprintf_s(buf, broken ? L"壊れている" : L"%d / %d", (int)dur, wd.maxDur);
    R->Text(buf, gbx + 70, gby + 10, 130, 24, 16, broken ? RGBA(1, 0.5f, 0.45f) : RGBA(1, 1, 1), 0, true);
    Bar(gbx + 200, gby + 16, 240, 12, broken ? 0.f : dk, dk < 0.25f ? RGBA(1, 0.3f, 0.25f) : RGBA(0.95f, 0.45f, 0.2f), RGBA(0.12f, 0.12f, 0.16f, 0.95f));
    R->Text(L"経験", gbx + 16, gby + 42, 60, 24, 16, RGBA(1, 1, 1), 0, true);
    swprintf_s(buf, L"%d / %d", (int)exp, 100 * lvl);
    R->Text(buf, gbx + 70, gby + 42, 130, 24, 16, RGBA(1, 1, 1), 0, true);
    Bar(gbx + 200, gby + 48, 240, 12, Saturate(exp / (100.f * lvl)), RGBA(0.35f, 0.75f, 1.f), RGBA(0.12f, 0.12f, 0.16f, 0.95f));

    // ---- 左下：武器に使う道具
    float ibx = 60, iby = H - 156;
    R->FillRound(ibx, iby, 430, 110, 12, RGBA(0.28f, 0.34f, 0.5f, 0.92f));
    R->StrokeRound(ibx, iby, 430, 110, 12, RGBA(0.75f, 0.82f, 0.95f), 2);
    struct It { int idx; const wchar_t* key; D2D1_COLOR_F c; const wchar_t* note; };
    It its[2] = { { 2, L"R", RGBA(0.75f, 0.8f, 0.9f), L"耐久を30%回復" }, { 4, L"F", RGBA(0.95f, 0.5f, 0.7f), L"壊れた最初の武器を直す" } };
    const wchar_t* inm[ITEM_N] = { L"薬草", L"灯油", L"研ぎ粉", L"帰り糸", L"繕い糸" };
    for (int i = 0; i < 2; i++)
    {
        float y = iby + 12 + i * 48.f;
        R->FillCircle(ibx + 34, y + 20, 17, its[i].c);
        R->StrokeCircle(ibx + 34, y + 20, 17, RGBA(1, 1, 1, 0.8f), 2);
        R->Text(its[i].key, ibx + 22, y + 8, 24, 24, 16, RGBA(0.1f, 0.1f, 0.15f), 1, true, false);
        R->Text(inm[its[i].idx], ibx + 62, y + 2, 120, 26, 19, RGBA(1, 1, 1), 0, true);
        R->Text(its[i].note, ibx + 62, y + 24, 260, 20, 13, RGBA(1, 1, 1, 0.75f));
        swprintf_s(buf, L"× %d", items[its[i].idx]);
        R->Text(buf, ibx + 300, y + 6, 110, 30, 22, RGBA(1, 1, 1), 2, true);
    }

    // ---- 右下：説明
    float dbx = W / 2 + 20, dby = H - 156;
    R->FillRound(dbx, dby, W - dbx - 40, 110, 12, RGBA(0.04f, 0.05f, 0.12f, 0.92f));
    R->StrokeRound(dbx, dby, W - dbx - 40, 110, 12, RGBA(0.75f, 0.82f, 0.95f), 2);
    R->Text(L"「" + std::wstring(wd.name) + L"」", dbx + 18, dby + 10, W - dbx - 76, 28, 19, RGBA(1, 0.95f, 0.85f), 0, true);
    std::wstring desc = wd.desc;
    if (IsStarter(ow.def)) desc += L"。壊れても消えない";
    else desc += L"。壊れると消える（アタッチメントは戻る）";
    R->Text(desc, dbx + 18, dby + 42, W - dbx - 76, 60, 16, RGBA(1, 1, 1, 0.9f));

    R->FillRect(0, H - 30, W, 30, RGBA(0.04f, 0.05f, 0.1f, 0.75f));
    R->Text(L"W/S：武器・枠を選ぶ　Enter：装備／付け替え　Z/C：キャラ切り替え　R：研ぎ粉　F：繕い糸　A/D：タブ　Esc：閉じる",
            0, H - 26, W, 24, 14, RGBA(1, 1, 1, 0.85f), 1, false, false);
}
