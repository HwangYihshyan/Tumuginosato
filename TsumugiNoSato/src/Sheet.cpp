// 武器の設定画とモーションのキーフレーム表（--autoshot で画像に保存）
#include "Game.h"

namespace
{
    struct Key { ASt st; int combo; float k; const wchar_t* label; int def = 0; };
    struct Page
    {
        const wchar_t* title;
        const wchar_t* note;
        bool side;        // true = 横から見る（縦振り）
        Key keys[3];
    };
    const int kPageCount = 9;
    const Page kPages[kPageCount] = {
        { L"主人公の武器「大糸切り鋏」設定画", L"", false, {} },
        { L"モーション①　1段目「横挟み」（右→左）",
          L"全11f（30fps）　0〜3f 刃を開きながら右へ振りかぶる → 4f 刃が閉じて挟む（当たり判定・耐久−1）→ 5〜8f 一本の刃のまま振り抜く → 9〜11f 肩に担ぐ構えへ戻る",
          false, { { ASt::Attack, 0, 0.22f, L"2f　刃を開いて構える" }, { ASt::Attack, 0, 0.35f, L"4f　挟む！（当たり判定）" }, { ASt::Attack, 0, 0.62f, L"7f　一本の刃で振り抜く" } } },
        { L"モーション②　2段目「返し挟み」（左→右）",
          L"全11f（30fps）　1段目の逆向き。刃を裏返して左から振る → 4f 挟む（当たり判定）→ 一本の刃で右へ振り抜く。1段目の途中で攻撃を押すとつながる（先行入力）",
          false, { { ASt::Attack, 1, 0.22f, L"2f　左で刃を開く" }, { ASt::Attack, 1, 0.35f, L"4f　挟む！（当たり判定）" }, { ASt::Attack, 1, 0.62f, L"7f　一本の刃で振り抜く" } } },
        { L"モーション③　3段目「断ち落とし」（縦振り・両手）",
          L"全17f（30fps）　0〜4f 両手で振りかぶり、刃を縦に大きく開く → 5〜6f 振り下ろしながら閉じて挟む（当たり判定・耐久−2）→ 7〜12f 一本の刃を地面へ叩きつける → 13〜17f 戻り",
          true, { { ASt::Attack, 2, 0.22f, L"4f　振りかぶって大きく開く" }, { ASt::Attack, 2, 0.35f, L"6f　振り下ろしながら挟む！" }, { ASt::Attack, 2, 0.6f, L"10f　一本の刃で叩きつける" } } },
        { L"モーション④　ソラ　右クリック溜め攻撃「回転挟み」",
          L"右クリック長押しで腰を落とし、刃を最大（45°）まで開いて溜める（最大0.8秒・刃が光る）→ 離すと刃を開いたまま真横に突き出して体ごと2回転（9f・17fに360°の当たり判定）→ 24fで正面に振り込み、刃を閉じて挟む（当たり判定・ガード崩し）。溜めるほど威力アップ",
          false, { { ASt::Charging, 0, 1.0f, L"溜め　腰を落として最大まで開く" }, { ASt::ChargeAtk, 0, 0.42f, L"13f　開いた刃で回転斬り" }, { ASt::ChargeAtk, 0, 0.8f, L"24f　正面で挟む！" } } },
        { L"モーション⑤　クロベエ　右クリック「旋回防御」",
          L"右クリックを押している間、両腕を前に出し、ひも通しを紐で体の前に高速で回して「回転の盾」を作る（0〜4fで回し始め）。正面からの攻撃と飛び道具を完全に防ぐ（防ぐたびに耐久−1）。防御中の移動はゆっくり",
          false, { { ASt::Normal, 0, 0.0f, L"構え", 1 }, { ASt::Guard, 0, 0.1f, L"3f　ひも通しを回し始める", 1 }, { ASt::Guard, 0, 0.7f, L"回転の盾で弾をはじく", 1 } } },
        { L"モーション⑥　クロベエ　左クリック「薙ぎ縫い」（扇形の範囲攻撃）",
          L"全15f（30fps・前より約4割短い）　0〜3f ひも通しを振りかぶる → 3〜5f 扇の端へ投げ出す → 5〜10f 紐を張ったまま扇形に薙ぎ払う（8fに扇の中の敵すべてに当たり判定。1段目は左から、2段目は右から120°、3段目は160°で遠くまで）→ 10〜14f 紐をたぐって回収。回収中でもSpaceで回避できる",
          false, { { ASt::Attack, 0, 0.28f, L"4f　扇の端へ投げ出す", 1 }, { ASt::Attack, 0, 0.5f, L"8f　扇形に薙ぎ払う！（当たり判定）", 1 }, { ASt::Attack, 0, 0.78f, L"12f　たぐり寄せて回収", 1 } } },
        { L"モーション⑦　クロベエ　Space回避「穴掘り」",
          L"全18f（30fps）　0〜4f 頭から地面に潜る（土煙）→ 5〜13f 地中を進む。地面の盛り上がりだけが動く → 14〜18f 勢いよく飛び出す（土煙）。無敵は回避を始めてから1.6秒（動きが終わったあとも1秒続く）",
          false, { { ASt::Dodge, 0, 0.08f, L"2f　頭から潜る", 1 }, { ASt::Dodge, 0, 0.3f, L"9f　地中を進む（無敵）", 1 }, { ASt::Dodge, 0, 0.52f, L"16f　飛び出す！", 1 } } },
        { L"モーション⑧　カイリ　ミシン針（遠距離）",
          L"左クリック：指の間に挟んだミシン針を投げる（3段目は3本を扇状に）／右クリック長押し：大きな針に力を溜め、離すと敵を貫く1本を放つ／Space：腹ばいで滑る回避",
          false, { { ASt::Attack, 2, 0.2f, L"振りかぶる", 2 }, { ASt::Attack, 2, 0.55f, L"3本の針を扇状に投げる", 2 }, { ASt::Charging, 0, 1.0f, L"溜め：貫く大針", 2 } } },
    };

    Actor MakePose(const Key& key, XMFLOAT3 pos, float yaw)
    {
        static const float durs[3][3] = { { 0.36f, 0.36f, 0.56f }, { 0.5f, 0.5f, 0.66f }, { 0.32f, 0.32f, 0.5f } };
        Actor a;
        a.def = key.def;
        a.pos = pos; a.yaw = yaw;
        a.st = key.st; a.combo = key.combo;
        a.dodgeDir = Fwd(yaw);
        a.stT = key.st == ASt::Attack ? key.k * durs[key.def][key.combo] : key.k;
        return a;
    }
    bool IsSnip(const Key& key)
    {
        if (key.def == 0) return (key.st == ASt::Attack && std::fabs(key.k - 0.35f) < 0.01f) || (key.st == ASt::ChargeAtk && std::fabs(key.k - 0.8f) < 0.01f);
        if (key.def == 1) return key.st == ASt::Attack && std::fabs(key.k - 0.5f) < 0.01f;
        return false;
    }
}

void Game::RenderSheet()
{
    const Page& pg = kPages[sheetPage];
    FrameSettings fs;
    fs.clear = F3(0.93f, 0.92f, 0.88f);
    fs.fogColor = fs.clear; fs.fogStart = 500; fs.fogEnd = 1000; fs.fogMax = 0;
    fs.ambient = F3(0.6f, 0.6f, 0.64f);
    fs.lightColor = F3(0.6f, 0.58f, 0.56f);
    fs.lightDir = F3(-0.35f, -1.f, 0.45f);
    fs.time = time;
    if (sheetPage == 0) { fs.camPos = F3(0.55f, 1.45f, -32.f); fs.camTarget = F3(0.55f, 1.35f, 0.f); fs.fovY = 0.158f; }
    else if (sheetPage == 5) { fs.camPos = F3(0.f, 6.0f, 13.f); fs.camTarget = F3(0.f, 0.9f, 0.5f); fs.fovY = 0.75f; }
    else if (pg.side) { fs.camPos = F3(0.6f, 3.4f, -16.5f); fs.camTarget = F3(0.6f, 1.3f, 0.f); fs.fovY = 0.72f; }
    else if (sheetPage >= 6) { fs.camPos = F3(0.f, 9.5f, -9.5f); fs.camTarget = F3(0.f, 0.5f, 2.4f); fs.fovY = 0.8f; }
    else { fs.camPos = F3(0.f, 10.5f, -10.5f); fs.camTarget = F3(0.f, 0.5f, 1.6f); fs.fovY = 0.75f; }
    R->BeginFrame(fs);

    DrawParams fl; fl.color = C4(0.88f, 0.87f, 0.83f); fl.rim = 0;
    R->Draw(R->plane, XMMatrixScaling(200, 1, 200), fl);
    DrawParams gr; gr.tex = T["grid_cell"]; gr.blend = Blend::Alpha; gr.unlit = true; gr.depthWrite = false;
    gr.color = C4(0.35f, 0.35f, 0.4f, 0.25f); gr.worldUV = true; gr.uv = { 1.f, 1.f, 0, 0 };
    R->Draw(R->plane, XMMatrixScaling(200, 1, 200) * XMMatrixTranslation(0, 0.01f, 0), gr);

    if (sheetPage == 0)
    {
        Actor s; s.def = 0; s.pos = F3(-2.5f, 0, 0); s.yaw = PI; s.st = ASt::Normal;
        DrawSora(s, 1);
        XMMATRIX up = XMMatrixRotationX(-PI / 2);
        DrawShadow(F3(-1.0f, 0, 0), 0.8f, 0.3f);
        DrawScissors(up * XMMatrixTranslation(-1.0f, 0.46f, 0), -0.01f, F3(0, 0, 0));
        DrawShadow(F3(0.9f, 0, 0), 0.8f, 0.3f);
        DrawScissors(up * XMMatrixTranslation(0.9f, 0.46f, 0), 0.28f, F3(0, 0, 0));
        DrawShadow(F3(3.4f, 0, 0), 0.8f, 0.3f);
        DrawScissors(up * XMMatrixTranslation(3.4f, 0.46f, 0), 0.7f, F3(0, 0, 0));
        return;
    }

    for (int i = 0; i < 3; i++)
    {
        const Key& key = pg.keys[i];
        float x = (i - 1) * 6.5f * (sheetPage == 5 ? -1.f : 1.f);
        float yaw = pg.side ? PI / 2 : 0.f;
        Actor a = MakePose(key, F3(x, 0, 0), yaw);
        bool snip = IsSnip(key);
        // 斬られ役の人形
        if (sheetPage != 5 && sheetPage != 7)
        {
            Enemy dummy;
            dummy.t = EType::Doll; dummy.st = ESt::Idle;
            dummy.yaw = pg.side ? -PI / 2 : PI;
            if (pg.side) dummy.pos = F3(x + 2.4f, 0, 0);
            else if (sheetPage == 6) dummy.pos = Add(a.pos, Mul(Fwd(yaw), kFanReach[0] - 0.6f));
            else if (sheetPage == 8) dummy.pos = F3(x, 0, 6.0f);
            else dummy.pos = F3(x, 0, 2.4f);
            dummy.flash = snip ? 0.55f : 0.f;
            DrawEnemy(dummy);
        }
        // ここまでの刃の通り道（ソラのみ）
        if (key.def == 0 && key.st != ASt::Charging)
        {
            float k0 = std::max(0.f, key.k - 0.3f);
            XMFLOAT3 pa{}, pb{};
            for (int j = 0; j <= 14; j++)
            {
                Key kk = key; kk.k = Lerpf(k0, key.k, j / 14.f);
                Actor g = MakePose(kk, a.pos, yaw);
                XMMATRIX W = SoraWeaponWorld(g);
                XMFLOAT3 ma, mb;
                XMStoreFloat3(&ma, XMVector3Transform(XMVectorSet(0, 0, 0.7f, 1), W));
                XMStoreFloat3(&mb, XMVector3Transform(XMVectorSet(0, 0, 2.34f, 1), W));
                if (j > 0) R->DrawQuadWorld(pa, pb, mb, ma, C4(1.f, 0.6f, 0.2f, 0.06f + 0.4f * j / 14.f), Blend::Alpha);
                pa = ma; pb = mb;
            }
        }
        // クロベエの投げ：ひも通しの通り道
        if (sheetPage == 6 && key.k > 0.22f)
        {
            XMFLOAT3 prev = KurobeeNeedle(MakePose({ ASt::Attack, 0, 0.2f, L"", 1 }, a.pos, yaw));
            for (int j = 1; j <= 20; j++)
            {
                float kk = Lerpf(0.2f, std::min(key.k, 0.65f), j / 20.f);
                XMFLOAT3 q = KurobeeNeedle(MakePose({ ASt::Attack, 0, kk, L"", 1 }, a.pos, yaw));
                DrawParams tp; tp.color = C4(1.f, 0.5f, 0.35f, 0.5f); tp.blend = Blend::Alpha; tp.unlit = true; tp.depthWrite = false;
                Seg(prev, q, 0.05f, tp);
                prev = q;
            }
            // 当たり判定の扇の輪郭
            DrawParams lp; lp.color = C4(0.95f, 0.4f, 0.3f, 0.85f); lp.blend = Blend::Alpha; lp.unlit = true; lp.depthWrite = false;
            float half = kFanArc[0] * 0.5f * PI / 180.f;
            XMFLOAT3 o = Add(a.pos, F3(0, 0.04f, 0));
            XMFLOAT3 pe = Add(o, Mul(Fwd(yaw - half), kFanReach[0]));
            Seg(o, pe, 0.03f, lp);
            for (int j = 1; j <= 16; j++)
            {
                XMFLOAT3 q = Add(o, Mul(Fwd(yaw - half + 2 * half * j / 16.f), kFanReach[0]));
                Seg(pe, q, 0.03f, lp);
                pe = q;
            }
            Seg(o, pe, 0.03f, lp);
        }
        // カイリ：投げた針
        if (sheetPage == 8 && key.st == ASt::Attack && key.k > 0.45f)
        {
            DrawParams np; np.color = C4(0.85f, 0.88f, 0.95f); np.emissive = { 0.1f, 0.1f, 0.15f };
            for (int n = -1; n <= 1; n++)
            {
                XMFLOAT3 d = Fwd(yaw + n * 0.2f);
                XMFLOAT3 p = Add(F3(x, 1.05f, 0), Mul(d, 3.2f));
                Seg(Sub(p, Mul(d, 0.7f)), p, 0.03f, np);
                DrawParams tr; tr.color = C4(0.4f, 0.6f, 1.f, 0.35f); tr.blend = Blend::Alpha; tr.unlit = true; tr.depthWrite = false;
                Seg(Add(F3(x, 1.05f, 0), Mul(d, 0.6f)), Sub(p, Mul(d, 0.7f)), 0.02f, tr);
            }
        }
        DrawActor(a);
        if (sheetPage == 5 && key.st == ASt::Guard && key.k > 0.5f)
        {
            XMFLOAT3 hit{ x + 0.3f, 1.1f, 1.25f };
            DrawParams g; g.unlit = true; g.color = C4(0.6f, 1.f, 0.45f);
            R->Draw(R->sphere, XMMatrixScaling(0.35f, 0.35f, 0.35f) * XMMatrixTranslation(hit.x, hit.y, hit.z + 0.3f), g);
            R->DrawBillboard(T["spark"], hit, 2.4f, 2.4f, C4(1, 0.8f, 0.5f, 1), Blend::Add, { 0.25f, 0.25f, 0.25f, 0.25f }, 0.6f, false, 0);
        }
        if (snip && key.def == 0)
        {
            XMMATRIX W = SoraWeaponWorld(a);
            XMFLOAT3 m; XMStoreFloat3(&m, XMVector3Transform(XMVectorSet(0, 0, 1.6f, 1), W));
            R->DrawBillboard(T["spark"], m, 2.6f, 2.6f, C4(1, 0.8f, 0.4f, 1), Blend::Add, { 0.25f, 0.25f, 0.25f, 0.25f }, 0.3f, false, 0);
        }
        if (sheetPage == 7 && (key.k < 0.15f || key.k > 0.45f))
            R->DrawBillboard(T["smoke"], F3(x, 0.4f, 0), 2.4f, 2.4f, C4(0.45f, 0.33f, 0.22f, 0.7f), Blend::Alpha, { 0.125f, 0.125f, 0.25f, 0.25f }, 0.3f, true, 0);
    }
}

void Game::DrawSheetUI()
{
    const Page& pg = kPages[sheetPage];
    float W = (float)R->W, H = (float)R->H;
    D2D1_COLOR_F ink = RGBA(0.12f, 0.16f, 0.28f), acc = RGBA(0.85f, 0.5f, 0.15f), sub = RGBA(0.3f, 0.32f, 0.38f);
    R->FillRect(0, 0, W, 58, RGBA(0.12f, 0.16f, 0.28f, 0.95f));
    R->Text(pg.title, 24, 12, W - 260, 40, 26, RGBA(1, 0.97f, 0.9f), 0, true, false);
    R->Text(L"ツムギノサト", W - 260, 18, 236, 30, 17, RGBA(1, 0.8f, 0.5f), 2, true, false);

    auto label = [&](XMFLOAT3 p, float dx, float dy, const std::wstring& s, float w = 220) {
        float sx, sy;
        if (!R->Project(p, sx, sy)) return;
        float tx = sx + dx, ty = sy + dy;
        R->Line(sx, sy, tx + (dx < 0 ? w : 0), ty + 12, sub, 1.5f);
        R->FillCircle(sx, sy, 3.5f, acc);
        R->Text(s, tx, ty, w, 26, 16, ink, dx < 0 ? 2 : 0, true, false);
    };
    auto under = [&](XMFLOAT3 p, const std::wstring& s, D2D1_COLOR_F c) {
        float sx, sy;
        if (!R->Project(p, sx, sy)) return;
        R->Text(s, sx - 160, sy, 320, 30, 18, c, 1, true, false);
    };

    if (sheetPage == 0)
    {
        float x0, y0, x1, y1;
        if (R->Project(F3(-1.45f, 0, 0), x0, y0) && R->Project(F3(-1.45f, 2.80f, 0), x1, y1))
        {
            R->Line(x0, y0, x1, y1, acc, 2);
            R->Line(x0 - 8, y0, x0 + 8, y0, acc, 2); R->Line(x1 - 8, y1, x1 + 8, y1, acc, 2);
            R->Text(L"全長 280cm", x1 - 130, (y0 + y1) / 2 - 30, 120, 26, 17, acc, 2, true, false);
            R->Text(L"（ソラの2倍）", x1 - 130, (y0 + y1) / 2 - 6, 120, 26, 14, acc, 2, true, false);
        }
        if (R->Project(F3(-3.05f, 0, 0), x0, y0) && R->Project(F3(-3.05f, 1.4f, 0), x1, y1))
        {
            R->Line(x0, y0, x1, y1, sub, 1.5f);
            R->Line(x0 - 6, y0, x0 + 6, y0, sub, 1.5f); R->Line(x1 - 6, y1, x1 + 6, y1, sub, 1.5f);
            R->Text(L"140cm", x1 - 70, (y0 + y1) / 2 - 12, 60, 24, 15, sub, 2, true, false);
        }
        label(F3(-0.98f, 1.9f, 0), 60, -40, L"刃（鋼）× 2枚");
        label(F3(-0.98f, 0.68f, 0), 70, -36, L"支点の結び（オレンジ）");
        label(F3(-0.98f, 0.36f, 0), 70, -6, L"持ち手：オレンジの糸巻き");
        label(F3(-0.98f, 0.04f, 0), 70, 4, L"U字ばね", 120);
        under(F3(-2.5f, -0.2f, 0), L"ソラ（担いだ構え）", ink);
        under(F3(-1.0f, -0.2f, 0), L"閉じた状態＝一本の刃", ink);
        under(F3(0.9f, -0.2f, 0), L"開いた状態（挟む直前）", ink);
        under(F3(3.4f, -0.2f, 0), L"溜め：最大まで開く", ink);
        R->FillRect(0, H - 92, W, 92, RGBA(1, 1, 1, 0.7f));
        R->Text(L"全長 280cm ／ 刃長 212cm ／ 刃幅 片側15cm（閉じると30cmの一本の刃）／ 刃の厚み 背5cm・刃先0.8cm ／ 持ち手 56cm",
                24, H - 84, W - 48, 26, 16, ink, 0, true, false);
        R->Text(L"開き角：構え 0°・挟む直前 16°・溜め 45°（片側）／ 支点を軸に2枚の刃がV字に開き、閉じると重なって一本の刃になる",
                24, H - 58, W - 48, 26, 16, ink, 0, false, false);
        R->Text(L"色：刃＝鋼（火の鋏は緋色・水の鋏は青）、刃先に研ぎの白い帯／持ち手と支点の結び＝オレンジ／ばね＝銀",
                24, H - 32, W - 48, 26, 16, sub, 0, false, false);
        return;
    }

    for (int i = 0; i < 3; i++)
    {
        const Key& key = pg.keys[i];
        float x = (i - 1) * 6.5f * (sheetPage == 5 ? -1.f : 1.f);
        bool snip = IsSnip(key);
        XMFLOAT3 p = pg.side ? F3(x + 0.6f, -0.4f, 0) : sheetPage == 5 ? F3(x, -0.2f, 2.2f) : F3(x, 0, -1.6f);
        float sx, sy;
        if (R->Project(p, sx, sy))
        {
            R->FillRound(sx - 160, sy + 4, 320, 36, 18, snip ? RGBA(0.85f, 0.45f, 0.12f) : RGBA(0.12f, 0.16f, 0.28f, 0.9f));
            R->Text(key.label, sx - 160, sy + 9, 320, 30, 18, RGBA(1, 1, 1), 1, true, false);
        }
    }
    R->FillRect(0, H - 70, W, 70, RGBA(1, 1, 1, 0.78f));
    R->Text(pg.note, 24, H - 62, W - 48, 60, 16, ink, 0, false, false);
}
