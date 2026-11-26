// 深い階の敵（糸喰い蛾・影法師・根の鎧武者）と、うつろ（12階の QTE・15階のボス）、
// 15階の「糸の柱」（うつろを守る、壊せる地形）
#include "Game.h"

namespace
{
    XMMATRIX S(float x, float y, float z) { return XMMatrixScaling(x, y, z); }
    XMMATRIX Tr(float x, float y, float z) { return XMMatrixTranslation(x, y, z); }
}

// ------------------------------------------------------------------
// 糸の柱
// ------------------------------------------------------------------
bool Game::PillarSolid(float x, float z) const
{
    for (auto& p : pillars) if (p.alive && DistXZ(p.pos, F3(x, 0, z)) < 0.95f) return true;
    return false;
}

void Game::DrawPillars()
{
    const Enemy* u = nullptr;
    for (auto& e : enemies) if (e.t == EType::Utsuro && e.st != ESt::Dead) u = &e;
    for (auto& p : pillars)
    {
        DrawParams st; st.tex = T["cave_wall"]; st.color = C4(0.5f, 0.48f, 0.55f); st.outline = 0.03f; st.uv = { 1, 2, 0, 0 };
        if (!p.alive)
        {
            // 砕けた柱：根元だけが残る（ほそい糸がまた伸びはじめている）
            R->Draw(R->cylinder, S(1.5f, 0.6f, 1.5f) * Tr(p.pos.x, 0, p.pos.z), st);
            for (int k = 0; k < 4; k++)
                R->Draw(R->cube, S(0.5f, 0.35f, 0.45f) * XMMatrixRotationY(k * 1.3f) * Tr(p.pos.x + std::cos(k * 1.6f) * 1.2f, 0.17f, p.pos.z + std::sin(k * 1.6f) * 1.2f), st);
            continue;
        }
        float g = SmoothStep(0.f, 1.f, p.grow);                     // 紡ぎなおされるときは下から伸びる
        st.emissive = Add(Mul(F3(0.6f, 0.6f, 1.f), p.flash), Mul(F3(0.3f, 0.35f, 0.8f), 1.f - g));
        R->Draw(R->cylinder, S(1.6f, 5.0f * g, 1.6f) * Tr(p.pos.x, 0, p.pos.z), st);
        // ひびの入り具合（体力が減ると赤く光る）
        float hurt = 1.f - p.hp / p.maxHp;
        if (hurt > 0.05f)
        {
            DrawParams cr; cr.color = C4(1.f, 0.4f, 0.3f, 0.6f * hurt); cr.unlit = true; cr.blend = Blend::Add; cr.depthWrite = false;
            for (int k = 0; k < 3; k++) R->Draw(R->cylinder, S(1.64f, 0.05f, 1.64f) * Tr(p.pos.x, 1.4f + k * 1.3f, p.pos.z), cr);
        }
        // 巻きついた光る糸
        DrawParams th; th.color = C4(0.75f, 0.8f, 1.f); th.emissive = { 0.35f, 0.4f, 0.7f }; th.outline = 0.01f;
        for (int k = 0; k < 4; k++)
            if (1.0f + k * 1.0f < 5.0f * g) R->Draw(R->cylinder, S(1.7f, 0.08f, 1.7f) * Tr(p.pos.x, 1.0f + k * 1.0f + std::sin(time * 2 + k) * 0.08f, p.pos.z), th);
        R->DrawBillboard(T["glint"], Add(p.pos, F3(0, 5.3f * g, 0)), 2.2f, 2.2f, C4(0.6f, 0.7f, 1.f, 0.8f), Blend::Add, { 1, 1, 0, 0 }, time);
        // うつろへ力を送る糸（守りの元）
        if (u && g > 0.9f)
        {
            DrawParams lk; lk.color = C4(0.55f, 0.6f, 1.f, 0.8f); lk.unlit = true; lk.blend = Blend::Add; lk.depthWrite = false;
            XMFLOAT3 a = Add(p.pos, F3(0, 4.5f, 0)), b = Add(u->pos, F3(0, 2.6f, 0));
            XMFLOAT3 m = Add(Lerp3(a, b, 0.5f), F3(0, std::sin(time * 3 + p.pos.x) * 0.4f, 0));
            Seg(a, m, 0.07f, lk); Seg(m, b, 0.07f, lk);
            // 糸の上を流れる光の粒
            float k = std::fmod(time * 0.8f + p.pos.x * 0.1f, 1.f);
            XMFLOAT3 q = k < 0.5f ? Lerp3(a, m, k * 2) : Lerp3(m, b, k * 2 - 1);
            R->DrawBillboard(T["glint"], q, 0.9f, 0.9f, C4(0.7f, 0.8f, 1.f, 0.9f), Blend::Add, { 1, 1, 0, 0 }, time * 4);
        }
    }
}

// 柱を砕く（主人公の攻撃でも、うつろの突進でも）
void Game::BreakPillar(Pillar& p, bool byCharge)
{
    if (!p.alive) return;
    p.alive = false; p.hp = 0;
    SpawnFx(1, Add(p.pos, F3(0, 2.5f, 0)), 8.f, C4(0.7f, 0.75f, 1.f), 0.7f);
    SpawnFx(3, Add(p.pos, F3(0, 1.f, 0)), 6.f, C4(0.5f, 0.48f, 0.55f, 0.9f), 0.9f);
    shake = std::max(shake, byCharge ? 0.6f : 0.35f); hitstop = std::max(hitstop, 0.1f);
    int left = 0; for (auto& q : pillars) if (q.alive) left++;
    if (left > 0) ToastMsg(L"糸の柱が砕けた！（あと " + std::to_wstring(left) + L" 本）", RGBA(1, 0.85f, 0.4f));
    else
    {
        ToastMsg(L"4本の柱がすべて砕けた！ うつろの守りが消えた。今のうちに攻めよう！", RGBA(1, 0.85f, 0.4f));
        for (auto& e : enemies)
            if (e.t == EType::Utsuro && e.st != ESt::Dead)
            {
                SpawnFx(1, Add(e.pos, F3(0, 2.5f, 0)), 10.f, C4(0.8f, 0.85f, 1.f), 0.8f);   // 守りの膜がはじける
                shieldDownT = 0.001f; shieldDownHp = e.hp;
            }
    }
}

// 主人公の攻撃が柱に当たる（近接の扇形の当たり判定と同じ考え方）
void Game::DamagePillars(XMFLOAT3 from, float yaw, float reach, float arcDeg, float dmg)
{
    float cosHalf = std::cos(arcDeg * 0.5f * PI / 180.f);
    for (auto& p : pillars)
    {
        if (!p.alive || p.grow < 0.99f) continue;
        XMFLOAT3 to = Sub(p.pos, from);
        float d = LenXZ(to) - 0.9f;
        if (d > reach) continue;
        if (DotXZ(NormXZ(to), Fwd(yaw)) < cosHalf && d > 0.4f) continue;
        p.hp -= dmg; p.flash = 1;
        AddNum(Add(p.pos, F3(0, 3.f, 0)), (int)dmg, RGBA(0.75f, 0.85f, 1.f), 26);
        SpawnFx(0, Add(p.pos, F3(0, 2.f, 0)), 2.f, C4(0.75f, 0.8f, 1.f), 0.35f);
        if (p.hp <= 0) BreakPillar(p, false);
    }
}

// 柱の体力のゲージ（画面）
void Game::DrawPillarBars()
{
    for (auto& p : pillars)
    {
        if (!p.alive || p.grow < 0.99f) continue;
        float sx, sy;
        if (!R->Project(Add(p.pos, F3(0, 5.8f, 0)), sx, sy)) continue;
        // 画面上のほうの UI（左のキャラ・仲間・お金の欄、上のボスの体力、右上の地図）に重なるときは出さない
        //   残りの本数はボスの体力の下の「守り：糸の柱 n/4 本」でわかる
        float W = (float)R->W;
        if (sx < 500.f && sy < 300.f) continue;
        if (sx > W - 260.f && sy < 290.f) continue;
        if (sy < 100.f) continue;
        Bar(sx - 40, sy, 80, 8, p.hp / p.maxHp, RGBA(0.55f, 0.65f, 1.f), RGBA(0.1f, 0.1f, 0.15f, 0.85f));
    }
}

// ------------------------------------------------------------------
// うつろ（15階のボス）：浮かんで糸の弾をばらまき、ときどき突進する。
//   4本の糸の柱が1本でも残っている間はまったく効かない。柱は攻撃でも、うつろの突進をぶつけても壊せる
//   （突進でぶつけると3秒気絶）。全部壊すと守りが消えるが、18秒たつか体力が1/4減ると、柱が紡ぎなおされる
// ------------------------------------------------------------------
void Game::UpdateUtsuro(Enemy& e, float dt)
{
    Actor& A = party[active];
    e.stT += dt; e.atkCd -= dt;
    immuneMsgT -= dt;
    for (auto& p : pillars) { p.flash = std::max(0.f, p.flash - dt * 4); if (p.alive && p.grow < 1.f) p.grow = std::min(1.f, p.grow + dt * 0.8f); }
    if (LenXZ(e.kb) > 0.02f) { MoveCircle(e.pos, e.radius * 0.7f, e.kb.x * dt, e.kb.z * dt); e.kb = Mul(e.kb, std::max(0.f, 1 - dt * 7)); }
    XMFLOAT3 to = Sub(A.pos, e.pos);
    float d = LenXZ(to);
    XMFLOAT3 dir = NormXZ(to);
    if (A.st == ASt::Down || arriveT < 1.f) return;
    // 守りが消えている時間：18秒たつか、体力が1/4減ると、柱が紡ぎなおされて守りが戻る
    bool anyAlive = false;
    for (auto& p : pillars) if (p.alive) anyAlive = true;
    if (!anyAlive && !pillars.empty())
    {
        if (shieldDownT <= 0) { shieldDownT = 0.001f; shieldDownHp = e.hp; }
        shieldDownT += dt;
        if (shieldDownT > 18.f || shieldDownHp - e.hp > e.maxHp * 0.25f)
        {
            for (auto& p : pillars)
            {
                p.alive = true; p.hp = p.maxHp; p.grow = 0; p.flash = 1;
                SpawnFx(3, Add(p.pos, F3(0, 1.f, 0)), 5.f, C4(0.55f, 0.6f, 1.f, 0.9f), 1.0f);
            }
            shieldDownT = 0;
            if (e.st == ESt::Hit) { e.st = ESt::Chase; e.pattern = 0; }
            SpawnFx(1, Add(e.pos, F3(0, 2.5f, 0)), 9.f, C4(0.55f, 0.6f, 1.f), 0.8f);
            ToastMsg(L"うつろが糸の柱を紡ぎなおした！ また柱を壊そう", RGBA(0.7f, 0.8f, 1.f));
        }
    }
    else shieldDownT = 0;
    // 手下を呼ぶ：24秒ごとに1体だけ（影法師と糸喰い蛾を交互に）。生きている手下は2体まで
    summonT += dt;
    int alive = 0;
    for (auto& o : enemies) if (o.st != ESt::Dead && !IsBossType(o.t)) alive++;
    if (summonT > 24.f && alive < 2 && e.st != ESt::Hit)
    {
        summonT = 0;
        static int turn = 0;
        pendingSpawns.push_back({ (turn++ & 1) ? EType::Moth : EType::Shade, Add(e.pos, Mul(Fwd(e.yaw + 1.3f), 3.5f)) });
        ToastMsg(L"うつろが霧から手下を呼び出した！", RGBA(1, 0.7f, 0.6f));
    }
    auto move = [&](XMFLOAT3 m, float sp) { MoveCircle(e.pos, e.radius * 0.7f, m.x * sp * dt, m.z * sp * dt); };
    switch (e.st)
    {
    case ESt::Idle:
    case ESt::Notice:
        e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 3);
        if (e.stT > 0.5f) { e.st = ESt::Chase; e.stT = 0; e.atkCd = 1.f; }
        break;
    case ESt::Chase:
        e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 4);
        if (d > 8.f) move(dir, 2.4f);
        else if (d < 4.5f) move(Mul(dir, -1), 2.0f);
        else move(F3(-dir.z, 0, dir.x), 0.6f * std::sin(e.bob * 0.5f));
        if (e.atkCd <= 0)
        {
            e.pattern = (d > 4.f && RandI(0, 9) < 6) ? 1 : 0;   // 1=突進 0=糸の弾
            e.st = ESt::Windup; e.stT = 0;
            if (e.pattern == 1) ToastMsg(L"うつろが突進の構え！ 柱の前へ誘い込め！", RGBA(1, 0.75f, 0.5f));
        }
        break;
    case ESt::Windup:
    {
        float wu = e.pattern == 1 ? 1.1f : 0.8f;
        if (e.stT < wu * 0.75f) { e.yaw = ApproachAngle(e.yaw, YawOf(dir), dt * 8); e.dashDir = dir; }
        if (e.stT >= wu)
        {
            e.st = ESt::Attack; e.stT = 0; e.hitDone = false;
            if (e.pattern == 0)
            {
                XMFLOAT3 o = Add(e.pos, F3(0, 1.8f, 0));
                for (int k = -3; k <= 3; k++)
                {
                    float yy = e.yaw + k * 0.2f;
                    projs.push_back({ o, Mul(Fwd(yy), 8.f), 2.8f, 10.f, 0.45f, 1 });
                }
            }
        }
        break;
    }
    case ESt::Attack:
        if (e.pattern == 0) { if (e.stT > 0.5f) { e.st = ESt::Recover; e.stT = 0; } break; }
        {
            // 突進
            XMFLOAT3 before = e.pos;
            move(e.dashDir, 15.f);
            if (e.stT > 0.05f) SpawnFx(3, Add(e.pos, F3(0, 1.f, 0)), 2.5f, C4(0.35f, 0.32f, 0.45f, 0.6f), 0.4f);
            if (!e.hitDone && d < e.radius + 0.7f) { e.hitDone = true; DamageActor(A, 22, e.pos, 10); }
            // 柱にぶつかった？
            for (auto& p : pillars)
            {
                if (!p.alive || DistXZ(p.pos, e.pos) > e.radius + 1.6f) continue;
                e.st = ESt::Hit; e.stT = 0; e.pattern = 9;     // ぶつかって目を回す
                ToastMsg(L"突進が柱にぶつかった！ うつろが目を回している", RGBA(1, 0.9f, 0.6f));
                BreakPillar(p, true);
                break;
            }
            if (e.st != ESt::Attack) break;
            // 壁にぶつかった（ほとんど進めない）→ 少しだけよろける
            if (e.stT > 0.1f && DistXZ(before, e.pos) < 15.f * dt * 0.3f) { e.st = ESt::Hit; e.stT = 0; e.pattern = 8; shake = 0.3f; break; }
            if (e.stT > 1.1f) { e.st = ESt::Recover; e.stT = 0; }
        }
        break;
    case ESt::Recover:
        if (e.stT > 0.8f) { e.st = ESt::Chase; e.stT = 0; e.atkCd = 1.6f; }
        break;
    case ESt::Hit:
        if (e.stT > (e.pattern == 9 ? 3.0f : 1.2f)) { e.st = ESt::Chase; e.stT = 0; e.pattern = 0; e.atkCd = 1.2f; }
        break;
    default: break;
    }
    e.pos.y = 0;
}

void Game::DrawUtsuro(const Enemy& e)
{
    float sc = 1.f, fade = 1.f;
    if (e.st == ESt::Dead)
    {
        if (e.deadT > 2.0f) return;
        fade = 1.f - e.deadT / 2.0f;
        sc = 1.f + e.deadT * 0.4f;            // ふくらんで霧に戻っていく
    }
    float bob = 0.6f + std::sin(e.bob * 1.6f) * 0.25f;
    XMFLOAT3 em = Mul(F3(1, 1, 1), e.flash * 0.5f);
    if (e.st == ESt::Windup) { float pulse = 0.5f + 0.5f * std::sin(e.stT * 25); em = Add(em, F3(0.6f * pulse, 0.05f, 0.1f * pulse)); }
    XMMATRIX P = S(sc, sc, sc) * XMMatrixRotationY(e.yaw) * Tr(e.pos.x, e.pos.y + bob, e.pos.z);
    auto D = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, float ol = 0.03f) {
        DrawParams p; p.color = c; p.color.w *= fade; p.outline = fade < 1 ? 0.f : ol; p.emissive = em; p.rim = 0.6f;
        if (fade < 1) { p.blend = Blend::Alpha; p.depthWrite = false; }
        R->Draw(m, L * P, p);
    };
    DrawShadow(e.pos, 4.5f, 0.45f * fade);
    XMFLOAT4 cloak = C4(0.22f, 0.2f, 0.3f), cloak2 = C4(0.32f, 0.3f, 0.42f);
    D(R->cone, S(2.6f, 3.4f, 2.6f) * Tr(0, 0.2f, 0), cloak);                       // 霧の衣
    D(R->sphere, S(1.9f, 1.2f, 1.9f) * Tr(0, 0.6f, 0), cloak2);
    D(R->sphere, S(1.3f, 1.4f, 1.3f) * Tr(0, 3.0f, 0), cloak);                     // 頭巾
    D(R->sphere, S(0.95f, 1.05f, 0.5f) * Tr(0, 2.9f, 0.45f), C4(0.95f, 0.93f, 0.9f), 0.015f);   // 白い面
    for (int k = -1; k <= 1; k += 2)
    {
        D(R->sphere, S(0.22f, 0.3f, 0.1f) * Tr(k * 0.22f, 3.0f, 0.72f), C4(0.05f, 0.04f, 0.08f), 0);   // うつろな目
        R->DrawBillboard(T["glint"], Add(e.pos, F3(0, bob + 3.0f * sc, 0)), 0.6f, 0.6f, C4(0.7f, 0.6f, 1.f, 0.7f * fade), Blend::Add, { 1, 1, 0, 0 }, time * 3 + k);
    }
    D(R->cube, S(0.5f, 0.06f, 0.05f) * Tr(0, 2.55f, 0.74f), C4(0.05f, 0.04f, 0.08f), 0);         // 口
    // ほどけた糸のすそ
    DrawParams tp; tp.color = C4(0.8f, 0.82f, 0.95f, 0.8f * fade); tp.emissive = { 0.2f, 0.2f, 0.3f }; tp.blend = Blend::Alpha;
    for (int i = 0; i < 8; i++)
    {
        float a = i * PI / 4 + e.bob * 0.3f;
        XMFLOAT3 base = Add(e.pos, F3(std::cos(a) * 1.2f * sc, bob + 0.4f, std::sin(a) * 1.2f * sc));
        XMFLOAT3 tip = Add(base, F3(std::cos(a) * 0.6f + std::sin(time * 2 + i) * 0.3f, -0.9f, std::sin(a) * 0.6f));
        Seg(base, tip, 0.04f, tp);
    }
    if (e.st == ESt::Hit && e.pattern == 9)
    {
        // 目を回している
        for (int k = 0; k < 3; k++)
        {
            float a = time * 4 + k * 2.1f;
            R->DrawBillboard(T["spark"], Add(e.pos, F3(std::cos(a) * 1.0f, bob + 4.3f, std::sin(a) * 1.0f)), 0.8f, 0.8f, C4(1, 0.9f, 0.4f, 1), Blend::Add, { 0.25f, 0.25f, 0.25f, 0.25f }, a);
        }
    }
    // 守りの膜：柱が1本でも残っている間、うつろを包む青い膜（攻撃が効かない印）
    bool shield = false;
    for (auto& p : pillars) if (p.alive) shield = true;
    if (shield && e.st != ESt::Dead && floorNo == 15)
    {
        float pulse = 0.5f + 0.5f * std::sin(time * 3.f);
        DrawParams sh; sh.color = C4(0.45f, 0.55f, 1.f, 0.16f + 0.08f * pulse); sh.unlit = true; sh.blend = Blend::Add; sh.depthWrite = false; sh.cullNone = true;
        R->Draw(R->sphere, S(4.4f, 5.2f, 4.4f) * Tr(e.pos.x, e.pos.y + bob + 1.9f, e.pos.z), sh);
        DrawParams rg = sh; rg.color = C4(0.6f, 0.7f, 1.f, 0.5f);
        for (int k = 0; k < 3; k++)
            R->Draw(R->cylinder, S(4.6f, 0.05f, 4.6f) * XMMatrixRotationX(0.6f * k + time * 0.7f) * Tr(e.pos.x, e.pos.y + bob + 1.9f, e.pos.z), rg);
    }
    if (e.st != ESt::Dead && (int)(time * 10) % 3 == 0)
        SpawnFx(3, Add(e.pos, F3(RandF(-1, 1), bob + RandF(0.3f, 2.f), RandF(-1, 1))), 1.8f, C4(0.4f, 0.38f, 0.5f, 0.5f), 0.7f);
}

// ------------------------------------------------------------------
// 深い階の敵の描画
// ------------------------------------------------------------------
bool Game::DrawEnemy2(const Enemy& e)
{
    if (e.t == EType::Utsuro) { DrawUtsuro(e); return true; }
    if (e.t != EType::Moth && e.t != EType::Shade && e.t != EType::Knight) return false;
    float sc = 1.f;
    if (e.st == ESt::Dead)
    {
        if (e.deadT > 0.6f) return true;
        sc = 1.f - e.deadT / 0.6f;
    }
    XMFLOAT3 em = Mul(F3(1, 1, 1), e.flash * 0.9f);
    if (e.st == ESt::Windup) { float pulse = 0.5f + 0.5f * std::sin(e.stT * 30); em = Add(em, F3(0.6f * pulse, 0.08f * pulse, 0.05f * pulse)); }
    float bob = e.t == EType::Moth ? 1.3f + std::sin(e.bob * 3.f) * 0.25f : 0.f;
    XMMATRIX P = S(sc, sc, sc) * XMMatrixRotationY(e.yaw) * Tr(e.pos.x, e.pos.y + bob, e.pos.z);
    auto D = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, float ol = 0.02f, XMFLOAT3 ex = { 0, 0, 0 }) {
        DrawParams p; p.color = c; p.outline = ol; p.emissive = Add(em, ex); p.rim = 0.35f; R->Draw(m, L * P, p);
    };
    DrawShadow(e.pos, e.t == EType::Knight ? 1.8f : 1.2f, e.t == EType::Moth ? 0.25f : 0.45f);
    switch (e.t)
    {
    case EType::Moth:
    {
        // 糸喰い蛾：ふわふわの胴と、ばたつく2対の羽
        XMFLOAT4 fur = C4(0.62f, 0.55f, 0.45f), wing = C4(0.78f, 0.72f, 0.6f);
        D(R->sphere, S(0.35f, 0.35f, 0.75f) * Tr(0, 0, 0), fur);
        D(R->sphere, S(0.3f, 0.3f, 0.3f) * Tr(0, 0.05f, 0.4f), fur);
        for (int k = -1; k <= 1; k += 2)
        {
            float flap = std::sin(e.bob * 18.f) * 0.7f;
            XMMATRIX W = XMMatrixRotationZ(k * (0.25f + flap));
            D(R->sphere, S(0.9f, 0.06f, 0.6f) * Tr(k * 0.55f, 0, 0.1f) * W, wing, 0.012f);
            D(R->sphere, S(0.6f, 0.05f, 0.45f) * Tr(k * 0.45f, 0, -0.4f) * W, C4(0.68f, 0.6f, 0.5f), 0.012f);
            D(R->sphere, S(0.18f, 0.02f, 0.18f) * Tr(k * 0.7f, 0.04f, 0.15f) * W, C4(0.3f, 0.2f, 0.35f), 0, F3(0.3f, 0.1f, 0.4f));   // 目玉模様
            D(R->sphere, S(0.08f, 0.08f, 0.06f) * Tr(k * 0.1f, 0.12f, 0.62f), C4(0.9f, 0.3f, 0.2f), 0, F3(0.8f, 0.2f, 0.1f));
            DrawParams an; an.color = C4(0.3f, 0.25f, 0.2f); an.emissive = em;
            XMVECTOR a0 = XMVector3Transform(XMVectorSet(k * 0.08f, 0.2f, 0.55f, 1), P), a1 = XMVector3Transform(XMVectorSet(k * 0.35f, 0.55f, 0.85f, 1), P);
            XMFLOAT3 fa, fb; XMStoreFloat3(&fa, a0); XMStoreFloat3(&fb, a1);
            Seg(fa, fb, 0.025f, an);
        }
        break;
    }
    case EType::Shade:
    {
        // 影法師：すらりとした黒い影。突進のときは残像を残す
        XMFLOAT4 dark = C4(0.12f, 0.1f, 0.18f);
        D(R->cone, S(0.8f, 1.6f, 0.8f) * Tr(0, 0.0f, 0), dark);
        D(R->sphere, S(0.5f, 0.55f, 0.5f) * Tr(0, 1.65f, 0), dark);
        for (int k = -1; k <= 1; k += 2)
            D(R->sphere, S(0.1f, 0.06f, 0.05f) * Tr(k * 0.12f, 1.7f, 0.22f), C4(1, 1, 1), 0, F3(0.9f, 0.9f, 1.f));
        for (int k = -1; k <= 1; k += 2)
            D(R->cube, S(0.12f, 0.8f, 0.12f) * XMMatrixRotationX(e.st == ESt::Attack ? -1.2f : -0.3f) * Tr(k * 0.4f, 1.0f, 0.15f), dark);
        if (e.st == ESt::Attack)
            for (int k = 1; k <= 3; k++)
                R->DrawBillboard(T["smoke"], Add(Sub(e.pos, Mul(e.dashDir, k * 0.7f)), F3(0, 1.0f, 0)), 1.4f, 2.0f, C4(0.25f, 0.2f, 0.35f, 0.5f - k * 0.12f), Blend::Alpha, { 1, 1, 0, 0 }, 0);
        break;
    }
    case EType::Knight:
    {
        // 根の鎧武者：木の根でできた鎧と兜、左手に盾（正面の攻撃を受け流す）
        DrawParams bark; bark.tex = T["bark"]; bark.color = C4(0.7f, 0.55f, 0.4f); bark.outline = 0.025f; bark.emissive = em;
        R->Draw(R->cylinder, S(0.9f, 1.1f, 0.75f) * Tr(0, 0.35f, 0) * P, bark);
        R->Draw(R->sphere, S(0.95f, 0.8f, 0.85f) * Tr(0, 1.45f, 0) * P, bark);
        D(R->sphere, S(0.55f, 0.55f, 0.55f) * Tr(0, 2.05f, 0), C4(0.3f, 0.22f, 0.15f));
        D(R->cone, S(0.75f, 0.45f, 0.75f) * Tr(0, 2.35f, 0), C4(0.25f, 0.2f, 0.14f));
        D(R->cube, S(0.08f, 0.5f, 0.4f) * Tr(0, 2.6f, 0.1f), C4(0.85f, 0.65f, 0.2f), 0.01f);   // 前立て
        for (int k = -1; k <= 1; k += 2) D(R->sphere, S(0.1f, 0.06f, 0.05f) * Tr(k * 0.14f, 2.05f, 0.27f), C4(1, 0.8f, 0.3f), 0, F3(0.9f, 0.6f, 0.1f));
        D(R->cube, S(0.85f, 1.1f, 0.12f) * Tr(-0.6f, 1.2f, 0.45f), C4(0.45f, 0.32f, 0.2f), 0.02f);   // 盾
        D(R->cube, S(0.3f, 0.3f, 0.05f) * Tr(-0.6f, 1.3f, 0.53f), C4(0.85f, 0.65f, 0.2f), 0.01f);
        float swing = e.st == ESt::Windup ? -1.3f : e.st == ESt::Attack ? 1.1f : 0.f;
        D(R->cube, S(0.12f, 1.5f, 0.12f) * Tr(0, 0.75f, 0) * XMMatrixRotationX(-0.4f) * XMMatrixRotationY(swing) * Tr(0.65f, 1.2f, 0.2f), C4(0.75f, 0.75f, 0.8f), 0.015f);
        break;
    }
    default: break;
    }
    return true;
}
