// ダンジョンの描画・HUD・メニュー
#include "Game.h"

void Game::RenderDungeon()
{
    if (menuOpen && menuTab == 2) { RenderWeaponShowcase(); return; }
    FrameSettings fs;
    bool deep = floorNo >= 6;
    float lk = Saturate(lantern / 100.f);
    XMFLOAT3 sh{ 0, 0, 0 };
    if (shake > 0 && setShake) sh = F3(std::sin(time * 71) * shake * 0.25f, std::sin(time * 53) * shake * 0.2f, std::cos(time * 67) * shake * 0.25f);
    fs.camPos = Add(camPosNow, sh);
    fs.camTarget = Add(camTargetNow, Mul(sh, 0.5f));
    fs.time = time;
    fs.fogColor = deep ? F3(0.07f, 0.06f, 0.11f) : F3(0.06f, 0.09f, 0.13f);
    if (floorNo == 15) fs.fogColor = F3(0.08f, 0.06f, 0.1f);
    if (floorNo == 14) fs.fogColor = F3(0.1f, 0.06f, 0.06f);
    fs.clear = fs.fogColor;
    fs.fogStart = Lerpf(2.f, 8.f, lk);
    fs.fogEnd = Lerpf(12.f, deep ? 26.f : 32.f, SmoothStep(0.f, 0.5f, lk));
    fs.fogMax = 1.f;
    fs.ambient = deep ? F3(0.27f, 0.24f, 0.38f) : F3(0.3f, 0.34f, 0.44f);
    fs.lightDir = F3(0.35f, -1.f, 0.5f);
    fs.lightColor = deep ? F3(0.42f, 0.4f, 0.62f) : F3(0.45f, 0.52f, 0.66f);
    Actor& A = party[active];
    fs.lights.push_back({ Add(A.pos, F3(0, 2.4f, 0)), 6.f + 8.f * lk, Mul(F3(1.25f, 0.85f, 0.5f), 0.45f + 0.75f * lk) });
    for (auto& ch : chests)
        if (!ch.opened && ch.kind == 2) fs.lights.push_back({ Add(ch.pos, F3(0, 1.0f, 0)), 4.5f, F3(1.0f, 0.75f, 0.35f) });
        else if (!ch.opened && ch.kind == 1) fs.lights.push_back({ Add(ch.pos, F3(0, 1.0f, 0)), 4.f, F3(0.8f, 0.6f, 0.3f) });
    for (auto& k : pickups) if (!k.taken && k.k == PickKind::Key) fs.lights.push_back({ Add(k.pos, F3(0, 1.0f, 0)), 3.f, F3(1.0f, 0.8f, 0.3f) });
    for (auto& d : decos) if (d.kind == 1) fs.lights.push_back({ Add(d.pos, F3(0, 0.6f, 0)), 3.2f, F3(0.15f, 0.6f, 0.55f) });
    fs.lights.push_back({ Add(exitPos, F3(0, 1.6f, -1.0f)), 6.f, hasKey ? F3(1.0f, 0.8f, 0.35f) : F3(0.6f, 0.35f, 0.9f) });
    for (auto& s : springs) fs.lights.push_back({ Add(s.pos, F3(0, 0.8f, 0)), 5.f, s.used ? F3(0.2f, 0.3f, 0.4f) : F3(0.35f, 0.7f, 1.0f) });
    for (auto& p : projs) fs.lights.push_back({ p.pos, 3.f, p.friendly ? F3(0.6f, 0.8f, 1.0f) : p.kind == 0 ? F3(0.5f, 1.0f, 0.4f) : F3(0.8f, 0.8f, 1.0f) });
    for (auto& f : fx) if (f.kind == 0) fs.lights.push_back({ f.pos, 3.5f * (1 - f.t / f.dur), F3(1.2f, 0.8f, 0.4f) });
    for (auto& e : enemies)
        if (e.st != ESt::Dead && (e.t == EType::Guardian || e.t == EType::RootArm || e.t == EType::Utsuro))
            fs.lights.push_back({ Add(e.pos, F3(0, 4.f, 0)), 9.f, F3(0.9f, 0.55f, 0.2f) });
    R->BeginFrame(fs);

    // 地形
    DrawParams fl; fl.tex = T["cave_floor"]; fl.color = deep ? C4(0.8f, 0.75f, 0.95f) : C4(0.85f, 0.9f, 1.0f); fl.rim = 0;
    R->Draw(floorMesh, XMMatrixIdentity(), fl);
    DrawParams wl; wl.tex = T["cave_wall"]; wl.color = deep ? C4(0.75f, 0.68f, 0.9f) : C4(0.82f, 0.88f, 1.0f); wl.rim = 0.1f;
    R->Draw(wallMesh, XMMatrixIdentity(), wl);

    // 飾り
    for (auto& d : decos)
    {
        DrawParams p;
        switch (d.kind)
        {
        case 0:
        {
            p.tex = T["bark"]; p.color = C4(0.55f, 0.42f, 0.32f); p.outline = 0.02f;
            XMFLOAT3 in = Fwd(d.rot);
            XMFLOAT3 top = Add(d.pos, F3(0, WALL_H * 0.95f, 0));
            XMFLOAT3 mid = Add(d.pos, Add(Mul(in, 0.6f * d.scale), F3(std::sin(d.tilt) * 0.4f, 1.2f, std::cos(d.tilt) * 0.2f)));
            XMFLOAT3 bot = Add(d.pos, Add(Mul(in, 1.1f * d.scale), F3(d.tilt, 0, 0)));
            Seg(top, mid, 0.28f * d.scale, p);
            Seg(mid, bot, 0.2f * d.scale, p);
            break;
        }
        case 1:
        {
            float s = d.scale;
            p.color = C4(0.85f, 0.88f, 0.85f);
            R->Draw(R->cylinder, XMMatrixScaling(0.12f * s, 0.35f * s, 0.12f * s) * XMMatrixTranslation(d.pos.x, 0, d.pos.z), p);
            DrawParams cp; cp.color = C4(0.2f, 0.7f, 0.65f); cp.emissive = { 0.1f, 0.45f, 0.42f }; cp.outline = 0.015f;
            R->Draw(R->sphere, XMMatrixScaling(0.4f * s, 0.2f * s, 0.4f * s) * XMMatrixTranslation(d.pos.x, 0.36f * s, d.pos.z), cp);
            R->Draw(R->cylinder, XMMatrixScaling(0.08f * s, 0.22f * s, 0.08f * s) * XMMatrixTranslation(d.pos.x + 0.25f, 0, d.pos.z + 0.1f), p);
            R->Draw(R->sphere, XMMatrixScaling(0.24f * s, 0.12f * s, 0.24f * s) * XMMatrixTranslation(d.pos.x + 0.25f, 0.22f * s, d.pos.z + 0.1f), cp);
            break;
        }
        case 2:
            p.tex = T["cave_wall"]; p.color = C4(0.7f, 0.75f, 0.85f); p.outline = 0.015f;
            R->Draw(R->sphere, XMMatrixScaling(1.2f * d.scale, 0.7f * d.scale, 1.0f * d.scale) * XMMatrixRotationY(d.rot) * XMMatrixTranslation(d.pos.x, 0.1f, d.pos.z), p);
            break;
        case 3:
        {
            DrawParams cp; cp.color = deep ? C4(0.6f, 0.4f, 0.9f) : C4(0.45f, 0.55f, 0.95f);
            cp.emissive = deep ? F3(0.2f, 0.1f, 0.35f) : F3(0.08f, 0.15f, 0.35f); cp.rim = 0.5f;
            for (int i = 0; i < 3; i++)
            {
                float a = d.rot + i * 2.1f;
                R->Draw(R->octa, XMMatrixScaling(0.25f * d.scale, (0.9f - i * 0.2f) * d.scale, 0.25f * d.scale) * XMMatrixRotationZ(0.3f * std::sin(a)) * XMMatrixRotationY(a) *
                                 XMMatrixTranslation(d.pos.x + std::cos(a) * 0.25f, 0.3f * d.scale, d.pos.z + std::sin(a) * 0.25f), cp);
            }
            break;
        }
        }
    }

    // 出口（封印の扉）
    {
        float yaw = YawOf(exitFace);
        XMMATRIX base = XMMatrixRotationY(yaw) * XMMatrixTranslation(exitPos.x, 0, exitPos.z);
        DrawParams seal; seal.tex = hasKey ? T["seal_open"] : T["seal_closed"]; seal.unlit = true; seal.fog = 0.3f;
        R->Draw(R->quad, XMMatrixScaling(1.9f, 2.6f, 1) * XMMatrixTranslation(0, 1.35f, -0.03f) * base, seal);
        DrawParams wd; wd.color = C4(0.45f, 0.25f, 0.2f); wd.outline = 0.02f;
        R->Draw(R->cube, XMMatrixScaling(0.28f, 3.0f, 0.28f) * XMMatrixTranslation(-1.05f, 1.5f, -0.2f) * base, wd);
        R->Draw(R->cube, XMMatrixScaling(0.28f, 3.0f, 0.28f) * XMMatrixTranslation(1.05f, 1.5f, -0.2f) * base, wd);
        R->Draw(R->cube, XMMatrixScaling(2.8f, 0.25f, 0.4f) * XMMatrixTranslation(0, 3.0f, -0.2f) * base, wd);
        DrawParams rp; rp.tex = T["rope"]; rp.color = C4(1, 0.92f, 0.7f); rp.uv = { 4, 1, 0, 0.35f };
        R->Draw(R->cylinder, XMMatrixScaling(0.2f, 2.3f, 0.2f) * XMMatrixRotationZ(PI / 2) * XMMatrixTranslation(1.15f, 2.7f, -0.3f) * base, rp);
        if (floorNo == 7 && !hasKey)
        {
            // 根に塞がれた出口
            DrawParams rt; rt.tex = T["bark"]; rt.color = C4(0.5f, 0.38f, 0.28f); rt.outline = 0.02f;
            for (int i = 0; i < 4; i++)
                Seg(Add(exitPos, F3(-1.3f + i * 0.9f, 0, -0.2f)), Add(exitPos, F3(1.2f - i * 0.8f, 2.6f, -0.4f)), 0.18f, rt);
        }
    }

    for (auto& ch : chests) DrawChest(ch);
    for (auto& s : springs)
    {
        DrawParams st; st.tex = T["spring"]; st.outline = 0.02f;
        R->Draw(R->cylinder, XMMatrixScaling(2.4f, 0.45f, 2.4f) * XMMatrixTranslation(s.pos.x, 0, s.pos.z), st);
        DrawParams w; w.tex = T["water"]; w.uv = { 1, 1, time * 0.05f, 0 }; w.color = s.used ? C4(0.4f, 0.5f, 0.6f) : C4(0.6f, 0.9f, 1);
        w.emissive = s.used ? F3(0, 0, 0) : F3(0.1f, 0.3f, 0.45f);
        R->Draw(R->cylinder, XMMatrixScaling(2.0f, 0.02f, 2.0f) * XMMatrixTranslation(s.pos.x, 0.42f, s.pos.z), w);
    }

    // 拾い物
    for (auto& k : pickups)
    {
        if (k.taken) continue;
        float b = std::sin(k.t * 2.5f) * 0.15f;
        XMFLOAT3 p = Add(k.pos, F3(0, b * 0.5f, 0));
        if (k.k == PickKind::Key)
        {
            DrawParams g; g.color = C4(1.f, 0.8f, 0.3f); g.emissive = { 0.4f, 0.3f, 0.05f }; g.outline = 0.015f;
            XMMATRIX b2 = XMMatrixRotationY(k.t * 2) * XMMatrixTranslation(p.x, p.y + 0.3f, p.z);
            R->Draw(R->cylinder, XMMatrixScaling(0.4f, 0.08f, 0.4f) * XMMatrixRotationX(PI / 2) * XMMatrixTranslation(0, 0.3f, 0) * b2, g);
            R->Draw(R->cube, XMMatrixScaling(0.08f, 0.5f, 0.08f) * XMMatrixTranslation(0, -0.05f, 0) * b2, g);
            R->Draw(R->cube, XMMatrixScaling(0.16f, 0.08f, 0.08f) * XMMatrixTranslation(0.08f, -0.25f, 0) * b2, g);
            R->DrawBillboard(T["glint"], Add(p, F3(0, 0.3f, 0)), 2.f, 2.f, C4(1, 0.85f, 0.4f, 0.7f), Blend::Add);
        }
        else if (k.k == PickKind::Money)
        {
            DrawParams g; g.color = C4(0.95f, 0.75f, 0.3f); g.emissive = { 0.25f, 0.18f, 0.03f }; g.outline = 0.01f;
            R->Draw(R->cylinder, XMMatrixScaling(0.3f, 0.06f, 0.3f) * XMMatrixRotationX(PI / 2) * XMMatrixRotationY(k.t * 4) * XMMatrixTranslation(p.x, p.y, p.z), g);
            DrawParams h; h.color = C4(0.2f, 0.15f, 0.1f);
            R->Draw(R->cube, XMMatrixScaling(0.09f, 0.09f, 0.08f) * XMMatrixRotationY(k.t * 4) * XMMatrixTranslation(p.x, p.y, p.z), h);
        }
        else
        {
            DrawParams g; g.color = C4(0.4f, 0.8f, 0.35f); g.outline = 0.015f;
            R->Draw(R->sphere, XMMatrixScaling(0.35f, 0.2f, 0.35f) * XMMatrixTranslation(p.x, p.y + 0.1f, p.z), g);
            R->Draw(R->sphere, XMMatrixScaling(0.2f, 0.35f, 0.2f) * XMMatrixTranslation(p.x + 0.1f, p.y + 0.25f, p.z), g);
        }
    }

    // 予告円
    for (auto& m : markers)
    {
        float k = Saturate(m.t / m.delay);
        DrawParams mp; mp.tex = T["ring"]; mp.blend = Blend::Add; mp.unlit = true; mp.depthWrite = false; mp.uv = { 0.25f, 0.25f, 0.5f, 0.25f };
        mp.color = C4(1.f, 0.35f, 0.2f, 0.4f + 0.5f * k);
        R->Draw(R->plane, XMMatrixScaling(m.radius * 2.4f, 1, m.radius * 2.4f) * XMMatrixTranslation(m.pos.x, 0.05f, m.pos.z), mp);
        if (m.t >= m.delay)
        {
            DrawParams sp; sp.tex = T["bark"]; sp.color = C4(0.5f, 0.38f, 0.28f); sp.outline = 0.02f;
            float h = 2.4f * Saturate((m.t - m.delay) / 0.1f) * (1 - Saturate((m.t - m.delay - 0.3f) / 0.2f));
            for (int i = 0; i < 4; i++)
            {
                float a = i * 1.57f + m.pos.x;
                R->Draw(R->cone, XMMatrixScaling(0.45f, h, 0.45f) * XMMatrixRotationZ(0.2f * std::cos(a)) * XMMatrixTranslation(m.pos.x + std::cos(a) * 0.5f, 0, m.pos.z + std::sin(a) * 0.5f), sp);
            }
        }
    }

    for (auto& e : enemies) DrawEnemy(e);

    // 操作キャラ（仲間は画面に出ない）
    DrawActor(A);
    if (arriveT < 1.0f)
    {
        // 降り立つ光の柱
        float al = arriveT < 0.5f ? 0.9f : 0.9f * (1 - (arriveT - 0.5f) / 0.5f);
        for (int i = 0; i < 2; i++)
        {
            float a = i * PI / 2 + camYaw;
            XMFLOAT3 r{ std::cos(a) * 0.9f, 0, -std::sin(a) * 0.9f };
            XMFLOAT3 b0 = Add(A.pos, Mul(r, -1)), b1 = Add(A.pos, r);
            b0.y = b1.y = 0;
            R->DrawQuadWorld(Add(b0, F3(0, 9, 0)), Add(b1, F3(0, 9, 0)), b1, b0, C4(0.8f, 0.95f, 1.f, al), Blend::Add, T["pillar"]);
        }
    }
    if (eventState >= 0 && nagiAlpha > 0.01f) DrawNagi(nagiPos, YawOf(Sub(A.pos, nagiPos)), nagiAlpha);
    // 糸の妖精ツムギ：武器を持つ右側とは反対の、左うしろの高いところに浮かぶ（武器と重ならない）
    DrawTsumugi(Add(A.pos, Add(Mul(Fwd(A.yaw - 2.5f), 1.25f), F3(0, 2.45f + std::sin(time * 2.2f) * 0.12f, 0))));

    // 飛び道具
    for (auto& p : projs)
    {
        if (p.friendly)
        {
            XMFLOAT3 d = NormXZ(p.vel);
            float len = p.pierce ? 1.6f : 0.7f;
            DrawParams np; np.color = p.pierce ? C4(0.7f, 0.9f, 1.f) : C4(0.9f, 0.92f, 0.96f); np.emissive = p.pierce ? F3(0.3f, 0.5f, 0.7f) : F3(0.15f, 0.15f, 0.18f);
            Seg(Sub(p.pos, Mul(d, len)), p.pos, p.pierce ? 0.07f : 0.03f, np);
            R->DrawBillboard(T["glint"], p.pos, p.pierce ? 2.2f : 0.9f, p.pierce ? 2.2f : 0.9f, C4(0.7f, 0.9f, 1, 0.8f), Blend::Add, { 1, 1, 0, 0 }, time * 6);
            continue;
        }
        DrawParams g; g.unlit = true;
        g.color = p.kind == 0 ? C4(0.6f, 1.f, 0.45f) : C4(0.95f, 0.95f, 1.f);
        R->Draw(R->sphere, XMMatrixScaling(0.35f, 0.35f, 0.35f) * XMMatrixTranslation(p.pos.x, p.pos.y, p.pos.z), g);
        R->DrawBillboard(T["glint"], p.pos, 1.6f, 1.6f, p.kind == 0 ? C4(0.5f, 1, 0.4f, 0.8f) : C4(1, 1, 1, 0.7f), Blend::Add, { 1, 1, 0, 0 }, time * 4);
    }

    // 15階：糸の柱　／　12階：イベントの場所（マップのまん中）の目印
    if (!pillars.empty()) DrawPillars();
    if (floorNo == 12 && !Done(EV_F12) && eventState == 0)
    {
        DrawParams rp; rp.tex = T["ring"]; rp.blend = Blend::Add; rp.unlit = true; rp.depthWrite = false; rp.uv = { 0.25f, 0.25f, 0.5f, 0.25f };
        rp.color = C4(0.6f, 0.6f, 1.f, 0.6f + 0.3f * std::sin(time * 3));
        R->Draw(R->plane, XMMatrixScaling(7.f, 1, 7.f) * XMMatrixTranslation(eventPos.x, 0.05f, eventPos.z), rp);
        R->DrawBillboard(T["smoke"], Add(eventPos, F3(0, 1.5f, 0)), 4.f, 4.f, C4(0.5f, 0.5f, 0.65f, 0.5f), Blend::Alpha, { 1, 1, 0, 0 }, time * 0.3f);
    }
    // マーカー
    for (auto& e : enemies)
    {
        if (e.st == ESt::Dead) continue;
        float h = e.t == EType::Spider ? 4.2f : e.t == EType::Guardian ? 6.f : e.t == EType::Utsuro ? 5.f : e.t == EType::Knight ? 2.9f : e.t == EType::Moth ? 2.4f : 1.9f;
        if (e.st == ESt::Notice)
            R->DrawBillboard(T["exclaim"], Add(e.pos, F3(0, h + 0.3f, 0)), 0.9f, 0.9f, C4(1, 1, 1, 1), Blend::Alpha, { 1, 1, 0, 0 }, 0, false, 0);
        if (e.keyHolder)
            R->DrawBillboard(T["glint"], Add(e.pos, F3(0, h, 0)), 1.2f, 1.2f, C4(1, 0.85f, 0.3f, 0.9f), Blend::Add, { 1, 1, 0, 0 }, time * 2);
    }
    if (lockIdx >= 0 && lockIdx < (int)enemies.size())
    {
        const Enemy& e = enemies[lockIdx];
        bool big = e.t == EType::Spider || e.t == EType::Guardian || e.t == EType::Utsuro;
        R->DrawBillboard(T["lockon"], Add(e.pos, F3(0, big ? 1.8f : 0.8f, 0)), big ? 3.4f : 1.4f, big ? 3.4f : 1.4f,
                         C4(1, 0.85f, 0.4f, 0.95f), Blend::Alpha, { 1, 1, 0, 0 }, time * 1.5f, false, 0);
    }
    // 目的の目印
    XMFLOAT3 og;
    if (ObjectiveTarget(og))
        R->DrawBillboard(T["objective"], Add(og, F3(0, 2.6f + std::sin(time * 3) * 0.15f, 0)), 0.9f, 0.9f, C4(1, 0.9f, 0.5f, 0.95f), Blend::Alpha, { 1, 1, 0, 0 }, 0, false, 0);

    DrawFx();

    // 画面効果
    float hpK = A.hp / A.maxHp;
    if (lantern < 30)
        R->DrawScreenOverlay(T["inkborder"], C4(0, 0, 0, Saturate((30 - lantern) / 22.f) * 0.95f), Blend::Alpha);
    if (hpK < 0.4f || A.invuln > 0.2f && A.st == ASt::Hit)
    {
        float a = std::max(Saturate((0.4f - hpK) / 0.3f) * (0.6f + 0.25f * std::sin(time * 6)), A.st == ASt::Hit ? 0.6f : 0.f);
        R->DrawScreenOverlay(T["vignette"], C4(1, 1, 1, a), Blend::Alpha);
    }
}

// 目的に合わせた次の目標
bool Game::ObjectiveTarget(XMFLOAT3& out) const
{
    const Actor& A = party[active];
    float bd = 1e9f; bool have = false;
    if (floorNo == 12 && !Done(EV_F12)) { out = eventPos; return true; }   // 12階はまずまん中へ
    if (objective == 0)
    {
        if (hasKey) { out = Sub(exitPos, Mul(exitFace, 1.0f)); return true; }
        for (auto& ch : chests) if (ch.key && !ch.opened) { out = ch.pos; return true; }
        for (auto& e : enemies) if (e.keyHolder && e.st != ESt::Dead) { out = e.pos; return true; }
        for (auto& k : pickups) if (k.k == PickKind::Key && !k.taken) { out = k.pos; return true; }
        out = Sub(exitPos, Mul(exitFace, 1.0f)); return true;
    }
    for (auto& ch : chests)
    {
        if (ch.opened) continue;
        if (objective == 1 && ch.kind != 2) continue;
        if (objective == 2 && ch.kind == 2) continue;
        float d = DistXZ(ch.pos, A.pos);
        if (d < bd) { bd = d; out = ch.pos; have = true; }
    }
    return have;
}

void Game::DrawFaceIcon(int def, float cx, float cy, float r)
{
    if (def == 0)
    {
        R->FillCircle(cx, cy, r, RGBA(0.98f, 0.84f, 0.72f));
        R->FillRound(cx - r * 0.95f, cy - r, r * 1.9f, r * 0.75f, r * 0.35f, RGBA(0.12f, 0.1f, 0.12f));
        R->FillRect(cx - r * 0.95f, cy - r * 0.35f, r * 1.9f, r * 0.2f, RGBA(0.97f, 0.97f, 0.95f));
        R->FillCircle(cx - r * 0.35f, cy + r * 0.15f, r * 0.12f, RGBA(0.1f, 0.08f, 0.1f));
        R->FillCircle(cx + r * 0.35f, cy + r * 0.15f, r * 0.12f, RGBA(0.1f, 0.08f, 0.1f));
    }
    else if (def == 1)
    {
        R->FillCircle(cx, cy, r, RGBA(0.96f, 0.95f, 0.92f));
        R->FillRound(cx - r * 0.62f, cy - r * 0.75f, r * 0.38f, r * 1.2f, r * 0.19f, RGBA(0.12f, 0.12f, 0.14f));
        R->FillRound(cx + r * 0.24f, cy - r * 0.75f, r * 0.38f, r * 1.2f, r * 0.19f, RGBA(0.12f, 0.12f, 0.14f));
        R->FillCircle(cx, cy + r * 0.55f, r * 0.18f, RGBA(0.12f, 0.12f, 0.14f));
    }
    else if (def == 2)
    {
        R->FillCircle(cx, cy, r, RGBA(0.55f, 0.38f, 0.25f));
        R->FillCircle(cx, cy + r * 0.35f, r * 0.5f, RGBA(0.95f, 0.88f, 0.72f));
        R->FillRect(cx - r * 0.95f, cy - r * 0.6f, r * 1.9f, r * 0.25f, RGBA(0.8f, 0.2f, 0.18f));
        R->FillCircle(cx - r * 0.35f, cy - r * 0.05f, r * 0.12f, RGBA(0.1f, 0.08f, 0.1f));
        R->FillCircle(cx + r * 0.35f, cy - r * 0.05f, r * 0.12f, RGBA(0.1f, 0.08f, 0.1f));
        R->FillCircle(cx, cy + r * 0.2f, r * 0.13f, RGBA(0.15f, 0.1f, 0.08f));
    }
}

void Game::DrawDungeonUI()
{
    if (menuOpen && menuTab == 2) { DrawWeaponScreen(); return; }   // 武器画面は全画面
    float W = (float)R->W, H = (float)R->H;
    wchar_t buf[160];
    if (!pillars.empty()) DrawPillarBars();   // 15階：糸の柱の体力
    Actor& A = party[active];
    const CharDef& d = Def(A);
    const OwnedWeapon& ow = weapons[A.weapon];

    for (auto& e : enemies)
    {
        if (e.st == ESt::Dead || IsBossType(e.t)) continue;
        if (e.hp >= e.maxHp && !e.keyHolder) continue;
        float sx, sy;
        if (!R->Project(Add(e.pos, F3(0, 1.75f, 0)), sx, sy)) continue;
        Bar(sx - 30, sy, 60, 7, e.hp / e.maxHp, RGBA(0.9f, 0.3f, 0.3f), RGBA(0, 0, 0, 0.6f));
        if (e.keyHolder)
        {
            R->FillCircle(sx - 44, sy + 3, 11, RGBA(0.9f, 0.7f, 0.25f));
            R->Text(L"鍵", sx - 55, sy - 7, 22, 20, 13, RGBA(0.2f, 0.12f, 0.05f), 1, true, false);
        }
    }
    for (auto& n : nums)
    {
        float sx, sy;
        if (!R->Project(n.pos, sx, sy)) continue;
        float pop = n.t < 0.1f ? 1.f + (0.1f - n.t) * 4 : 1.f;
        D2D1_COLOR_F c = n.col; c.a = Saturate((0.9f - n.t) / 0.3f);
        float s = n.size * pop;
        R->Text(std::to_wstring(n.val), sx - 60, sy - s * 0.6f, 120, s * 1.3f, s, c, 1, true, true);
    }

    // 左上：ステータス
    R->FillRound(16, 14, 410, 150, 12, RGBA(0.05f, 0.07f, 0.13f, 0.78f));
    DrawFaceIcon(A.def, 52, 52, 26);
    R->StrokeCircle(52, 52, 26, RGBA(0.9f, 0.65f, 0.3f), 2.5f);
    R->Text(d.name, 88, 20, 150, 30, 21, RGBA(1, 0.95f, 0.85f), 0, true);
    swprintf_s(buf, L"HP %d / %d", (int)std::ceil(A.hp), (int)A.maxHp);
    R->Text(buf, 260, 23, 150, 24, 14, RGBA(1, 1, 1, 0.85f), 2);
    float hk = A.hp / A.maxHp;
    Bar(88, 50, 320, 13, hk, hk > 0.35f ? RGBA(0.45f, 0.85f, 0.45f) : RGBA(0.95f, 0.35f, 0.3f), RGBA(0.15f, 0.15f, 0.2f, 0.9f));
    R->Text(L"灯", 88, 72, 24, 24, 15, RGBA(1, 0.8f, 0.4f), 0, true);
    float blink = lantern < 25 ? 0.6f + 0.4f * std::sin(time * 8) : 1.f;
    Bar(110, 78, 298, 11, lantern / 100.f, RGBA(1.f, 0.7f, 0.25f, blink), RGBA(0.15f, 0.15f, 0.2f, 0.9f));
    float dk = A.maxDur > 0 ? A.dur / A.maxDur : 0;
    D2D1_COLOR_F wc = RGBA(0.85f, 0.9f, 1);
    if (!A.broken && dk <= 0.1f) wc = RGBA(1, 0.3f, 0.25f, 0.55f + 0.45f * std::sin(time * 12));
    else if (!A.broken && dk <= 0.25f) wc = RGBA(1, 0.85f, 0.25f);
    swprintf_s(buf, L"%s  Lv%d", A.broken ? d.spare : kWeapons[ow.def].name, A.lvl);
    R->Text(buf, 24, 100, 260, 24, 16, A.broken ? RGBA(0.7f, 0.7f, 0.75f) : kWeapons[ow.def].element == 1 ? RGBA(1, 0.6f, 0.45f) : kWeapons[ow.def].element == 2 ? RGBA(0.6f, 0.85f, 1) : RGBA(1, 1, 1), 0, true);
    if (A.broken) R->Text(L"壊れている（繕い糸で直る）", 190, 101, 220, 24, 14, RGBA(1, 0.6f, 0.5f), 2);
    else { swprintf_s(buf, L"耐久 %d / %d", (int)A.dur, (int)A.maxDur); R->Text(buf, 220, 101, 190, 24, 14, wc, 2, true); }
    Bar(24, 126, 384, 9, A.broken ? 0.f : dk, wc, RGBA(0.15f, 0.15f, 0.2f, 0.9f));
    Bar(24, 142, 384, 5, A.exp / (100.f * A.lvl), RGBA(0.55f, 0.75f, 1.f), RGBA(0.15f, 0.15f, 0.2f, 0.9f));

    // 控え（画面には出ない仲間）
    float ry = 172;
    for (int i = 0; i < 3; i++)
    {
        if (i == active || !joined[i]) continue;
        R->FillRound(16, ry, 250, 46, 10, RGBA(0.05f, 0.07f, 0.13f, 0.7f));
        DrawFaceIcon(i, 40, ry + 23, 16);
        R->Text(std::wstring(Def(party[i]).name) + L"（控え）", 64, ry + 4, 190, 22, 14, RGBA(1, 1, 1, 0.9f), 0, true);
        Bar(64, ry + 29, 186, 8, party[i].hp / party[i].maxHp, party[i].st == ASt::Down ? RGBA(0.5f, 0.5f, 0.5f) : RGBA(0.45f, 0.85f, 0.45f), RGBA(0.15f, 0.15f, 0.2f, 0.9f));
        ry += 52;
    }
    swprintf_s(buf, L"%d 文", money);
    R->Text(buf, 24, ry + 4, 200, 24, 16, RGBA(1, 0.85f, 0.4f), 0, true);

    // 右上：ミニマップ
    float ms = (setBigMap ? 290.f : 200.f) / DN, mx0 = W - DN * ms - 18, my0 = 14;   // 設定で地図を大きくできる
    R->FillRound(mx0 - 8, my0, DN * ms + 16, DN * ms + 56, 10, RGBA(0.05f, 0.07f, 0.13f, 0.78f));
    float myb = my0 + 8 + DN * ms;
    for (int x = 0; x < DN; x++)
        for (int z = 0; z < DN; z++)
            if (seen[x][z] && cell[x][z]) R->FillRect(mx0 + x * ms, myb - (z + 1) * ms, ms, ms, RGBA(0.55f, 0.62f, 0.75f, 0.85f));
    if (seen[exitX][exitZ - 1] || seen[exitX][exitZ])
        R->FillRect(mx0 + exitX * ms - 2, myb - (exitZ + 1) * ms - 2, ms + 4, ms + 4, hasKey ? RGBA(0.35f, 0.65f, 1.f) : RGBA(0.2f, 0.4f, 0.95f));   // 階のゴール：青
    // 泉：青
    for (auto& s : springs)
    {
        int cx = (int)(s.pos.x / CELL), cz = (int)(s.pos.z / CELL);
        if (!seen[cx][cz]) continue;
        R->FillCircle(mx0 + (s.pos.x / CELL) * ms, myb - (s.pos.z / CELL) * ms, 3.5f, s.used ? RGBA(0.4f, 0.5f, 0.7f) : RGBA(0.3f, 0.6f, 1.f));
    }
    // 敵：赤
    for (auto& e : enemies)
    {
        if (e.st == ESt::Dead) continue;
        int cx = (int)(e.pos.x / CELL), cz = (int)(e.pos.z / CELL);
        if (cx < 0 || cz < 0 || cx >= DN || cz >= DN || !seen[cx][cz]) continue;
        R->FillCircle(mx0 + (e.pos.x / CELL) * ms, myb - (e.pos.z / CELL) * ms, IsBossType(e.t) ? 4.5f : 2.6f, RGBA(1.f, 0.2f, 0.18f));
    }
    for (auto& ch : chests)
    {
        int cx = (int)(ch.pos.x / CELL), cz = (int)(ch.pos.z / CELL);
        if (!seen[cx][cz] || ch.opened) continue;
        // ジオラマの糸巻き：黄緑　ふつうの宝箱：黄色　武器の宝箱：オレンジ
        D2D1_COLOR_F cc = ch.kind == 2 ? RGBA(0.62f, 0.95f, 0.25f) : ch.kind == 1 ? RGBA(1.f, 0.55f, 0.1f) : RGBA(1.f, 0.92f, 0.2f);
        R->FillRect(mx0 + (ch.pos.x / CELL) * ms - 2.5f, myb - (ch.pos.z / CELL) * ms - 2.5f, 5, 5, cc);
    }
    float px = mx0 + (A.pos.x / CELL) * ms, py = myb - (A.pos.z / CELL) * ms;
    R->FillCircle(px, py, 4.5f, RGBA(1, 0.85f, 0.3f));
    XMFLOAT3 fd = Fwd(A.yaw);
    R->Line(px, py, px + fd.x * 9, py - fd.z * 9, RGBA(1, 0.85f, 0.3f), 2);
    swprintf_s(buf, L"根の洞　%d / %d", floorNo, MAX_FLOOR);
    R->Text(buf, mx0, myb + 8, DN * ms, 26, 17, RGBA(1, 0.92f, 0.75f), 0, true);
    R->Text(hasKey ? L"鍵 ✓" : L"鍵 －", mx0, myb + 10, DN * ms, 24, 15, hasKey ? RGBA(1, 0.85f, 0.4f) : RGBA(1, 1, 1, 0.6f), 2, true);
    swprintf_s(buf, L"パーツ ×%d　この階 あと%d", crystalsThisRun, LootLeft(floorNo));
    R->Text(buf, mx0 - 250, my0 + 6, 240, 24, 15, RGBA(0.6f, 0.95f, 1), 2, true);

    float topY = 70;   // 上中央の表示は上から順に積む（ボスのHP → 守りの様子 → 目的 → ロックオン）。重ならない
    // ボスのHP
    for (auto& e : enemies)
    {
        if (!IsBossType(e.t) || e.st == ESt::Dead || (e.t == EType::Utsuro && floorNo != 15)) continue;
        float bw = 420, bx = (W - bw) / 2;   // 左の体力の欄・右上の「パーツ」表示と重ならない幅
        const wchar_t* nm = e.t == EType::Spider ? L"中ボス　ほつれ蜘蛛" : e.t == EType::Guardian ? L"主　大樹の番人" : e.t == EType::Utsuro ? L"主　うつろ" : L"根の主の腕";
        R->Text(nm, bx, 14, bw, 28, 20, RGBA(1, 0.9f, 0.85f), 1, true);
        Bar(bx, 44, bw, 14, e.hp / e.maxHp, RGBA(0.85f, 0.25f, 0.3f), RGBA(0.1f, 0.08f, 0.12f, 0.85f));
        topY = std::max(topY, 66.f);
        if (e.t == EType::Utsuro && floorNo == 15)
        {
            // うつろの守りの様子
            int left = 0; for (auto& p : pillars) if (p.alive) left++;
            wchar_t sb[96];
            if (left > 0) swprintf_s(sb, L"守り：糸の柱 %d / 4 本　― 柱を全部壊すまで攻撃が効かない", left);
            else swprintf_s(sb, L"守りが消えている！　あと %d 秒で柱が戻る", std::max(0, (int)std::ceil(18.f - shieldDownT)));
            R->FillRound(bx - 10, 62, bw + 20, 26, 13, left > 0 ? RGBA(0.15f, 0.2f, 0.45f, 0.85f) : RGBA(0.5f, 0.25f, 0.1f, 0.85f));
            R->Text(sb, bx - 10, 65, bw + 20, 22, 13, left > 0 ? RGBA(0.75f, 0.85f, 1.f) : RGBA(1, 0.9f, 0.6f), 1, true, false);
            topY = 96;
        }
    }
    // 目的
    {
        const wchar_t* objs[3] = { L"鍵持ちを倒して次の階へ", L"ジオラマパーツを集める", L"宝箱を探す" };
        std::wstring s = std::wstring(L"目的：") + objs[objective];
        R->FillRound(W / 2 - 190, topY, 380, 32, 16, RGBA(0.05f, 0.07f, 0.13f, 0.6f));
        R->Text(s, W / 2 - 190, topY + 5, 380, 26, 16, RGBA(1, 0.92f, 0.7f), 1, true);
        topY += 38;
    }

    // 右下：アイテム
    const wchar_t* inames[ITEM_N] = { L"薬草", L"灯油", L"研ぎ粉", L"帰り糸", L"繕い糸" };
    D2D1_COLOR_F icol[ITEM_N] = { RGBA(0.4f, 0.8f, 0.4f), RGBA(1, 0.7f, 0.3f), RGBA(0.75f, 0.8f, 0.9f), RGBA(0.9f, 0.75f, 0.4f), RGBA(0.95f, 0.5f, 0.7f) };
    for (int i = 0; i < ITEM_N; i++)
    {
        float x = W - ITEM_N * 78 - 14 + i * 78, y = H - 112;
        R->FillRound(x, y, 70, 70, 10, RGBA(0.05f, 0.07f, 0.13f, 0.78f));
        R->StrokeRound(x, y, 70, 70, 10, items[i] > 0 ? icol[i] : RGBA(1, 1, 1, 0.2f), 2);
        R->FillCircle(x + 35, y + 30, 13, items[i] > 0 ? icol[i] : RGBA(0.3f, 0.3f, 0.3f));
        R->Text(inames[i], x, y + 46, 70, 20, 13, RGBA(1, 1, 1, 0.9f), 1, true);
        swprintf_s(buf, L"%d", i + 1);
        R->Text(buf, x + 5, y + 2, 20, 20, 13, RGBA(1, 1, 1, 0.6f));
        swprintf_s(buf, L"×%d", items[i]);
        R->Text(buf, x + 30, y + 2, 36, 20, 14, RGBA(1, 0.95f, 0.8f), 2, true);
    }

    if (lantern < 25)
        R->Text(lantern <= 0 ? L"灯が消えた…体力が削られていく" : L"灯が消えかけている…", 0, H - 200, W, 30, 20, RGBA(1, 0.7f, 0.4f, 0.6f + 0.4f * std::sin(time * 4)), 1, true);
    if (!interactHint.empty() && !exitMenu && !menuOpen)
    {
        R->FillRound(W / 2 - 190, H - 170, 380, 40, 20, RGBA(0.05f, 0.07f, 0.13f, 0.8f));
        R->Text(interactHint, W / 2 - 190, H - 162, 380, 30, 18, RGBA(1, 0.92f, 0.7f), 1, true);
    }

    if (exitMenu)
    {
        R->FillRect(0, 0, W, H, RGBA(0, 0, 0, 0.35f));
        float bw = 520, bh = 250, bx = (W - bw) / 2, by = (H - bh) / 2 - 20;
        R->FillRound(bx, by, bw, bh, 16, RGBA(0.06f, 0.09f, 0.17f, 0.95f));
        R->StrokeRound(bx + 5, by + 5, bw - 10, bh - 10, 12, RGBA(0.85f, 0.6f, 0.25f), 2);
        R->Text(L"封印が解けた", bx, by + 20, bw, 34, 26, RGBA(1, 0.9f, 0.7f), 1, true);
        std::wstring block = floorNo < MAX_FLOOR ? FloorBlock(floorNo + 1) : L"";
        bool lockNext = !block.empty();
        std::wstring o1 = floorNo >= MAX_FLOOR ? L"章を終えて村へ" : L"次の階へ進む（" + std::to_wstring(floorNo + 1) + L"階）";
        if (lockNext) o1 += floorNo + 1 == 7 ? L"　※クロベエが必要" : L"　※カイリが必要";
        swprintf_s(buf, L"村へ戻る（パーツ %d 個を持ち帰る）", crystalsThisRun);
        std::wstring opts[2] = { o1, buf };
        for (int i = 0; i < 2; i++)
        {
            float y = by + 82 + i * 64;
            bool sel = exitSel == i;
            R->FillRound(bx + 40, y, bw - 80, 50, 10, sel ? RGBA(0.98f, 0.94f, 0.85f) : RGBA(1, 1, 1, 0.08f));
            R->Text(opts[i], bx + 40, y + 11, bw - 80, 30, 19, sel ? RGBA(0.12f, 0.15f, 0.25f) : RGBA(1, 1, 1, 0.9f), 1, true, !sel);
        }
        R->Text(L"W/S：選択　Enter：決定", bx, by + bh - 34, bw, 24, 14, RGBA(1, 1, 1, 0.6f), 1);
    }

    // ロックオン中の敵（名前と体力）
    if (lockIdx >= 0 && lockIdx < (int)enemies.size() && !menuOpen)
    {
        const Enemy& e = enemies[lockIdx];
        bool boss = IsBossType(e.t);
        bool bossShown = false;
        for (auto& o : enemies) if (IsBossType(o.t) && o.st != ESt::Dead && !(o.t == EType::Utsuro && floorNo != 15)) bossShown = true;
        // ボスをロックしているときは上のボスのHPで十分なので出さない。ボス戦で手下をロックしたときは、地図の下に出す（上中央と重ならない）
        float lw = 300, lx = (W - lw) / 2, ly = 14.f;
        if (bossShown) { lx = W - lw - 18; ly = my0 + DN * ms + 64; }
        if (!(boss && bossShown))
        {
        R->FillRound(lx, ly, lw, 44, 10, RGBA(0.05f, 0.07f, 0.13f, 0.8f));
        R->StrokeRound(lx, ly, lw, 44, 10, RGBA(1, 0.45f, 0.35f), 2);
        R->Text(std::wstring(L"ロックオン：") + EnemyName(e.t) + (e.keyHolder ? L"（鍵）" : L""), lx, ly + 3, lw, 22, 15, RGBA(1, 0.9f, 0.85f), 1, true);
        Bar(lx + 16, ly + 28, lw - 32, 8, e.hp / e.maxHp, RGBA(0.9f, 0.3f, 0.3f), RGBA(0.15f, 0.12f, 0.15f, 0.9f));
        }
    }
    if (qteOn) DrawQTEUI();
    if (menuOpen) DrawMenuUI();

    R->FillRect(0, H - 30, W, 30, RGBA(0.04f, 0.05f, 0.1f, 0.65f));
    const wchar_t* rc = d.badger ? L"右クリック 旋回防御" : L"右クリック長押し 溜め";
    swprintf_s(buf, L"WASD 移動　Shift ダッシュ　左クリック 攻撃　%s　Space 回避　Q ロックオン（R・ホイールで切替）　V カメラを背後へ　E 調べる　Tab 交代　Esc メニュー", rc);
    R->Text(buf, 0, H - 26, W, 24, 14, RGBA(1, 1, 1, 0.8f), 1, false, false);
}

// ------------------------------------------------------------------
// メニュー（アイテム／キャラ変更／武器変更／目的／退出）
// ------------------------------------------------------------------
static const int kTabN = 6;
static const wchar_t* kTabs[kTabN] = { L"アイテム", L"キャラ変更", L"武器変更", L"目的", L"退出", L"操作" };

void Game::UpdateMenu(const Controls& c)
{
    if (c.menu || c.back) { menuOpen = false; return; }
    if (c.left) { menuTab = (menuTab + kTabN - 1) % kTabN; menuSel = 0; }
    if (c.right) { menuTab = (menuTab + 1) % kTabN; menuSel = 0; }
    if (!joined[menuChar]) menuChar = active;
    if (menuTab == 2)
    {
        // 武器画面：Z/C で見るキャラを切り替え、R 研ぎ粉／F 繕い糸 をそのキャラの武器に使う
        int dir = c.prevChar ? 2 : c.nextChar ? 1 : 0;
        if (dir)
        {
            for (int k = 1; k <= 2; k++) { int n = (menuChar + dir * k) % 3; if (joined[n]) { menuChar = n; menuSel = 0; break; } }
        }
        if (c.useA) UseItemOn(2, party[menuChar]);
        if (c.useB) UseItemOn(4, party[menuChar]);
    }
    Actor& A = menuTab == 2 ? party[menuChar] : party[active];
    std::vector<int> myWeapons;
    for (int i = 0; i < (int)weapons.size(); i++) if (kWeapons[weapons[i].def].owner == A.def) myWeapons.push_back(i);
    int rows = menuTab == 0 ? ITEM_N : menuTab == 1 ? 3 : menuTab == 2 ? (int)myWeapons.size() + 2 : menuTab == 3 ? 3 : menuTab == 5 ? 3 : 2;   // 操作：W/S でページ
    if (c.up) menuSel = (menuSel + rows - 1) % rows;
    if (c.down) menuSel = (menuSel + 1) % rows;
    if (!c.confirm) return;
    switch (menuTab)
    {
    case 0:
        UseItem(menuSel);
        if (menuSel == 3) menuOpen = false;
        break;
    case 1:
        if (!joined[menuSel]) ToastMsg(L"まだ仲間になっていない", RGBA(1, 0.8f, 0.7f));
        else if (party[menuSel].st == ASt::Down) ToastMsg(L"倒れていて交代できない", RGBA(1, 0.6f, 0.5f));
        else if (menuSel != active) { SwapTo(menuSel); menuOpen = false; }
        break;
    case 2:
        if (menuSel < (int)myWeapons.size())
        {
            if (myWeapons[menuSel] != A.weapon) { EquipWeapon(A, myWeapons[menuSel]); ToastMsg(std::wstring(kWeapons[weapons[A.weapon].def].name) + L" を装備した"); }
        }
        else
        {
            // アタッチメントの付け替え（持っている種類を順に試す）
            int slot = menuSel - (int)myWeapons.size();
            OwnedWeapon& w = weapons[A.weapon];
            int cur = w.att[slot];
            if (cur >= 0) attInv[cur]++;
            int next = -1;
            for (int k = 1; k <= AT_COUNT; k++)
            {
                int t = (cur + k) % (AT_COUNT + 1);
                if (t == AT_COUNT) { next = -1; break; }
                if (attInv[t] > 0) { next = t; break; }
            }
            if (next >= 0) attInv[next]--;
            w.att[slot] = next;
            RecalcWeapon(A);
        }
        break;
    case 3:
        objective = menuSel;
        ToastMsg(L"目的を変えた", RGBA(1, 0.92f, 0.7f));
        menuOpen = false;
        break;
    case 4:
        if (menuSel == 0) { menuOpen = false; LeaveDungeon(false); }
        else menuOpen = false;
        break;
    }
}

void Game::DrawMenuUI()
{
    if (menuTab == 2) { DrawWeaponScreen(); return; }
    float W = (float)R->W, H = (float)R->H;
    R->FillRect(0, 0, W, H, RGBA(0.02f, 0.03f, 0.06f, 0.55f));
    float bw = 900, bh = 470, bx = (W - bw) / 2, by = (H - bh) / 2;
    R->FillRound(bx, by, bw, bh, 16, RGBA(0.06f, 0.09f, 0.17f, 0.96f));
    R->StrokeRound(bx + 5, by + 5, bw - 10, bh - 10, 12, RGBA(0.85f, 0.6f, 0.25f), 2);
    for (int i = 0; i < kTabN; i++)
    {
        float tx = bx + 24 + i * 143;
        bool sel = menuTab == i;
        R->FillRound(tx, by + 18, 134, 40, 20, sel ? RGBA(0.85f, 0.55f, 0.2f) : RGBA(1, 1, 1, 0.08f));
        R->Text(kTabs[i], tx, by + 26, 134, 30, 17, RGBA(1, 1, 1), 1, true, false);
    }
    float lx = bx + 40, ly = by + 84;
    wchar_t buf[160];
    auto row = [&](int i, const std::wstring& s, const std::wstring& right, D2D1_COLOR_F c = RGBA(1, 1, 1)) {
        float y = ly + i * 50.f;
        bool sel = menuSel == i;
        R->FillRound(lx, y, bw - 80, 42, 10, sel ? RGBA(0.98f, 0.94f, 0.85f) : RGBA(1, 1, 1, 0.06f));
        R->Text(s, lx + 16, y + 9, 520, 28, 18, sel ? RGBA(0.12f, 0.15f, 0.25f) : c, 0, true, !sel);
        R->Text(right, lx + 300, y + 10, bw - 400, 28, 15, sel ? RGBA(0.3f, 0.35f, 0.45f) : RGBA(1, 1, 1, 0.7f), 2, false, !sel);
    };
    Actor& A = party[active];
    switch (menuTab)
    {
    case 0:
    {
        const wchar_t* n[ITEM_N] = { L"薬草", L"灯油", L"研ぎ粉", L"帰り糸", L"繕い糸" };
        const wchar_t* ds[ITEM_N] = { L"HPを40回復", L"灯を50回復", L"武器の耐久を30%回復", L"すぐに村へ戻る", L"壊れた最初の武器を直す" };
        for (int i = 0; i < ITEM_N; i++) { swprintf_s(buf, L"%s　×%d", n[i], items[i]); row(i, buf, ds[i]); }
        swprintf_s(buf, L"所持金 %d 文　／　アタッチメント：硬い石×%d 火の欠片×%d 軽い羽×%d 鋼糸×%d", money, attInv[0], attInv[1], attInv[2], attInv[3]);
        R->Text(buf, lx, ly + ITEM_N * 50 + 14, bw - 80, 26, 15, RGBA(1, 0.9f, 0.7f));
        break;
    }
    case 1:
        for (int i = 0; i < 3; i++)
        {
            std::wstring s = joined[i] ? std::wstring(Def(party[i]).name) + (i == active ? L"（操作中）" : L"") : std::wstring(L"？？？");
            std::wstring r;
            if (joined[i]) { swprintf_s(buf, L"HP %d/%d　%s", (int)party[i].hp, (int)party[i].maxHp, kWeapons[weapons[party[i].weapon].def].name); r = buf; }
            else r = i == 1 ? L"炭焼き窯を村に戻すと仲間になる" : L"水車を村に戻すと仲間になる";
            row(i, s, r, joined[i] ? RGBA(1, 1, 1) : RGBA(1, 1, 1, 0.4f));
            if (joined[i]) DrawFaceIcon(i, lx - 18, ly + i * 50.f + 21, 14);
        }
        R->Text(L"仲間は画面には出ず、交代すると入れ替わる。控えのあいだは少しずつ回復する（Tab でも交代）", lx, ly + 3 * 50 + 14, bw - 80, 26, 15, RGBA(1, 0.9f, 0.7f));
        break;
    case 2:
    {
        int r = 0;
        for (int i = 0; i < (int)weapons.size(); i++)
        {
            const OwnedWeapon& w = weapons[i];
            if (kWeapons[w.def].owner != A.def) continue;
            const wchar_t* el = kWeapons[w.def].element == 1 ? L"（火）" : kWeapons[w.def].element == 2 ? L"（水）" : L"";
            std::wstring s = std::wstring(i == A.weapon ? L"▶ " : L"　") + kWeapons[w.def].name + el;
            float du = i == A.weapon ? A.dur : w.dur;
            swprintf_s(buf, L"攻撃×%.2f　耐久 %d/%d　Lv%d", kWeapons[w.def].atk, (int)du, kWeapons[w.def].maxDur, i == A.weapon ? A.lvl : w.lvl);
            row(r++, s, (i == A.weapon ? A.broken : w.broken) ? std::wstring(L"壊れている") : std::wstring(buf),
                kWeapons[w.def].element == 1 ? RGBA(1, 0.65f, 0.5f) : kWeapons[w.def].element == 2 ? RGBA(0.65f, 0.85f, 1) : RGBA(1, 1, 1));
        }
        const OwnedWeapon& cw = weapons[A.weapon];
        for (int s = 0; s < 2; s++)
        {
            std::wstring nm = cw.att[s] >= 0 ? std::wstring(kAttach[cw.att[s]].name) : std::wstring(L"（空き）");
            std::wstring ds = cw.att[s] >= 0 ? std::wstring(kAttach[cw.att[s]].desc) : std::wstring(L"Enterで付ける");
            row(r++, L"　スロット" + std::to_wstring(s + 1) + L"：" + nm, ds, RGBA(0.85f, 0.95f, 0.8f));
        }
        R->Text(L"武器を選んで Enter で装備。スロットで Enter を押すとアタッチメントを付け替える", lx, ly + r * 50 + 10, bw - 80, 26, 15, RGBA(1, 0.9f, 0.7f));
        break;
    }
    case 3:
    {
        const wchar_t* o[3] = { L"鍵持ちを倒して次の階へ", L"ジオラマパーツを集める", L"宝箱を探す" };
        const wchar_t* ds[3] = { L"鍵持ちと出口に目印", L"糸巻きの宝箱に目印", L"宝箱・武器の宝箱に目印" };
        for (int i = 0; i < 3; i++) row(i, std::wstring(i == objective ? L"▶ " : L"　") + o[i], ds[i]);
        break;
    }
    case 4:
        swprintf_s(buf, L"ダンジョンから退出する（パーツ %d 個を持ち帰る）", crystalsThisRun);
        row(0, buf, L"村へ戻る");
        row(1, L"やめる", L"");
        break;
    case 5:
        DrawControlsPage(std::clamp(menuSel, 0, 2), bx + 30, by + 74, bw - 60, bh - 116);   // 操作方法（W/S でページ）
        break;
    }
    R->Text(L"A/D：タブ　W/S：選択　Enter：決定　Esc：閉じる", bx, by + bh - 34, bw, 24, 14, RGBA(1, 1, 1, 0.6f), 1);
}

// エフェクト（連番画像）
void Game::DrawFx()
{
    for (auto& f : fx)
    {
        float k = f.t / f.dur;
        if (f.kind == 0 || f.kind == 1)
        {
            int frame = std::min(15, (int)(k * 16));
            XMFLOAT4 uv{ 0.25f, 0.25f, (frame % 4) * 0.25f, (frame / 4) * 0.25f };
            XMFLOAT4 col = f.color; col.w = 1.f;
            R->DrawBillboard(f.kind == 0 ? T["spark"] : T["ring"], f.pos, f.size, f.size, col, Blend::Add, uv, f.rot, true, 0.3f);
        }
        else if (f.kind == 3)
        {
            int frame = std::min(63, (int)(k * 64));
            XMFLOAT4 uv{ 0.125f, 0.125f, (frame % 8) * 0.125f, (frame / 8) * 0.125f };
            XMFLOAT4 col = f.color; col.w *= (1 - k * 0.6f);
            R->DrawBillboard(T["smoke"], Add(f.pos, F3(0, k * 0.4f, 0)), f.size * (0.6f + k * 0.6f), f.size * (0.6f + k * 0.6f), col, Blend::Alpha, uv, f.rot, true, 0.4f);
        }
        else
        {
            XMFLOAT4 col = f.color; col.w = 1 - k;
            R->DrawBillboard(T["glint"], f.pos, f.size * (0.5f + k), f.size * (0.5f + k), col, Blend::Add, { 1, 1, 0, 0 }, f.rot + k * 2);
        }
    }

}

void Game::TickFx(float dt)
{
    for (auto& f : fx) f.t += dt;
    fx.erase(std::remove_if(fx.begin(), fx.end(), [](const FxInst& f) { return f.t >= f.dur; }), fx.end());
    for (auto& n : nums) { n.t += dt; n.pos.y += dt * 0.9f; }
    nums.erase(std::remove_if(nums.begin(), nums.end(), [](const DmgNum& n) { return n.t > 0.9f; }), nums.end());
}
