// クロベエ（投げて巻き込む攻撃・地面に潜る回避）、カイリ、ナギ、タヌキ商人、
// 大樹の番人・根の主の腕、宝箱3種
#include "Game.h"

namespace
{
    XMMATRIX S(float x, float y, float z) { return XMMatrixScaling(x, y, z); }
    XMMATRIX Tr(float x, float y, float z) { return XMMatrixTranslation(x, y, z); }
    XMFLOAT3 ToW(XMFLOAT3 l, XMMATRIX m) { XMFLOAT3 o; XMStoreFloat3(&o, XMVector3Transform(XMLoadFloat3(&l), m)); return o; }
}

// ------------------------------------------------------------------
// クロベエのひも通しの位置（攻撃の進み具合から計算）
//   0〜20%：振りかぶる → 20〜35%：扇の端へ投げ出す → 35〜65%：紐を張ったまま扇形に薙ぎ払う
//   （50%＝扇の真ん中で当たり判定。扇の中の敵すべてに当たる）→ 65〜92%：紐をたぐって回収
// ------------------------------------------------------------------
XMFLOAT3 Game::KurobeeNeedle(const Actor& a, XMFLOAT3* handOut) const
{
    XMMATRIX P = XMMatrixRotationY(a.yaw) * Tr(a.pos.x, a.pos.y, a.pos.z);
    XMFLOAT3 hand = ToW(F3(0.62f, 0.62f, 0.15f), P);
    if (a.st != ASt::Attack)
    {
        if (handOut) *handOut = hand;
        return ToW(F3(0.67f, 1.15f, 0.25f), P);
    }
    float k = Saturate(a.stT / Def(a).dur[a.combo]);
    float half = kFanArc[a.combo] * 0.5f * PI / 180.f, dir = kFanDir[a.combo], reach = kFanReach[a.combo];
    auto fanPt = [&](float u) {           // u=0 扇の始まりの端 → u=1 反対の端
        float ang = a.yaw + dir * (u * 2.f - 1.f) * half;
        XMFLOAT3 q = Add(a.pos, Mul(Fwd(ang), reach)); q.y = 0.55f;
        return q;
    };
    XMFLOAT3 n;
    if (k < 0.2f)
    {
        float u = k / 0.2f;
        hand = ToW(Lerp3(F3(0.5f, 0.9f, 0.2f), F3(0.55f, 1.4f, -0.4f), u), P);
        n = Add(hand, F3(0, 0.35f, 0));
    }
    else if (k < 0.35f)
    {
        float u = (k - 0.2f) / 0.15f;
        hand = ToW(F3(0.45f, 1.1f, 0.55f), P);
        XMFLOAT3 s = fanPt(0);
        n = Lerp3(hand, s, u);
        n.y = Lerpf(hand.y, s.y, u) + std::sin(u * PI) * 0.6f;
    }
    else if (k < 0.65f)
    {
        float u = (k - 0.35f) / 0.3f;
        float e = u * u * (3 - 2 * u);
        float ang = a.yaw + dir * (e * 2.f - 1.f) * half;
        // 腕を大きく振って紐を回す
        hand = Add(a.pos, Add(Mul(Fwd(ang), 0.6f), F3(0, 1.05f, 0)));
        n = fanPt(e);
    }
    else if (k < 0.92f)
    {
        float u = (k - 0.65f) / 0.27f;
        float e = u * u * (3 - 2 * u);
        float tug = std::sin(u * PI * 3) * 0.12f;
        hand = ToW(F3(0.4f, 1.0f, 0.45f - 0.35f * std::abs(std::sin(u * PI * 2)) + tug), P);
        XMFLOAT3 s = fanPt(1);
        n = Lerp3(s, hand, e);
        n.y = Lerpf(s.y, hand.y, e * e);
    }
    else
    {
        hand = ToW(F3(0.62f, 0.62f, 0.15f), P);
        n = ToW(F3(0.67f, 1.15f, 0.25f), P);
    }
    if (handOut) *handOut = hand;
    return n;
}

// ------------------------------------------------------------------
// クロベエ（アナグマ・ひも通し）
// ------------------------------------------------------------------
void Game::DrawKurobee(const Actor& a, float alpha)
{
    float moving = Saturate(a.speedNow / 3.f);
    float t = a.walkT * 8.f;
    float bob = std::abs(std::sin(t)) * 0.06f * moving;
    float sway = std::sin(t) * 0.08f * moving;
    XMMATRIX P = XMMatrixRotationZ(sway) * XMMatrixRotationY(a.yaw) * Tr(a.pos.x, a.pos.y + bob, a.pos.z);
    float sink = 0;
    if (a.st == ASt::Dodge)
    {
        // 地面に潜る回避：頭から潜って進み、勢いよく飛び出す
        float s = a.stT / DodgeDur(1) * DodgeBase(1);   // のばした回避の時間を、もとの動きの時間に直す
        sink = s < 0.15f ? s / 0.15f : s < 0.45f ? 1.f : 1.f - (s - 0.45f) / 0.15f;
        sink = Saturate(sink);
        float pop = s > 0.45f ? std::sin(Saturate((s - 0.45f) / 0.15f) * PI) * 0.6f : 0.f;
        P = XMMatrixRotationX(0.9f * sink) * XMMatrixRotationY(YawOf(a.dodgeDir)) * Tr(a.pos.x, a.pos.y - 1.9f * sink * sink + pop, a.pos.z);
        // 地面の盛り上がり（土の山が進む）
        if (sink > 0.2f)
        {
            DrawParams mp; mp.tex = T["soil"]; mp.color = C4(0.6f, 0.45f, 0.32f); mp.outline = 0.015f;
            float mh = 0.35f * sink;
            R->Draw(R->sphere, S(1.3f, mh * 2, 1.3f) * Tr(a.pos.x, 0, a.pos.z), mp);
            for (int i = 0; i < 4; i++)
            {
                float an = i * 1.57f + a.stT * 9.f;
                R->Draw(R->sphere, S(0.25f, 0.18f, 0.25f) * Tr(a.pos.x + std::cos(an) * 0.75f, 0.05f + 0.15f * std::abs(std::sin(a.stT * 20 + i)), a.pos.z + std::sin(an) * 0.75f), mp);
            }
        }
    }
    else if (a.st == ASt::Hit) P = XMMatrixRotationX(-0.3f) * P;
    else if (a.st == ASt::Down) P = Tr(0, 0.4f, 0) * XMMatrixRotationX(-PI / 2) * XMMatrixRotationY(a.yaw) * Tr(a.pos.x, 0, a.pos.z);
    if (sink < 0.9f) DrawShadow(a.pos, 1.6f);
    XMFLOAT3 em = Mul(F3(1, 0.5f, 0.5f), a.flash * 0.8f);
    auto D = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, float ol = 0.022f) {
        DrawParams p; p.color = c; p.outline = ol; p.emissive = em; p.rim = 0.3f; R->Draw(m, L * P, p);
    };
    XMFLOAT4 grey = C4(0.48f, 0.48f, 0.52f), black = C4(0.12f, 0.12f, 0.14f), white = C4(0.96f, 0.95f, 0.92f);
    for (int s = -1; s <= 1; s += 2)
    {
        float sw = std::sin(t) * 0.5f * moving * s;
        if (a.st == ASt::Dodge) sw = std::sin(a.stT * 40.f) * 0.9f * s;   // 掘る手足
        D(R->cylinder, S(0.24f, 0.3f, 0.24f) * Tr(0, -0.3f, 0) * XMMatrixRotationX(sw) * Tr(s * 0.25f, 0.32f, 0), black);
    }
    D(R->sphere, S(1.0f, 0.95f, 0.88f) * Tr(0, 0.72f, 0), grey);
    D(R->cube, S(0.62f, 0.6f, 0.08f) * Tr(0, 0.66f, 0.42f), C4(0.55f, 0.35f, 0.2f), 0.012f);
    D(R->cube, S(0.25f, 0.16f, 0.03f) * Tr(0, 0.58f, 0.47f), C4(0.4f, 0.25f, 0.14f), 0);
    D(R->cylinder, S(0.82f, 0.14f, 0.74f) * Tr(0, 1.05f, 0), C4(0.8f, 0.2f, 0.16f), 0.012f);
    D(R->cube, S(0.18f, 0.35f, 0.05f) * XMMatrixRotationZ(0.3f) * Tr(0.22f, 0.95f, 0.38f), C4(0.8f, 0.2f, 0.16f), 0.01f);
    D(R->sphere, S(0.74f, 0.62f, 0.78f) * Tr(0, 1.42f, 0.0f), white);
    D(R->sphere, S(0.36f, 0.3f, 0.42f) * Tr(0, 1.3f, 0.3f), white, 0.015f);
    for (int s = -1; s <= 1; s += 2)
    {
        D(R->sphere, S(0.15f, 0.2f, 0.78f) * XMMatrixRotationX(-0.45f) * XMMatrixRotationY(s * 0.12f) * Tr(s * 0.17f, 1.5f, 0.05f), black, 0);
        D(R->sphere, S(0.07f, 0.08f, 0.04f) * Tr(s * 0.17f, 1.42f, 0.33f), white, 0);
        D(R->sphere, S(0.045f, 0.055f, 0.03f) * Tr(s * 0.17f, 1.42f, 0.35f), black, 0);
        D(R->sphere, S(0.2f, 0.16f, 0.1f) * Tr(s * 0.31f, 1.68f, -0.12f), grey, 0.015f);
        D(R->sphere, S(0.1f, 0.06f, 0.03f) * Tr(s * 0.25f, 1.28f, 0.28f), C4(1, 0.6f, 0.62f), 0);
    }
    D(R->sphere, S(0.15f, 0.11f, 0.11f) * Tr(0, 1.33f, 0.52f), black, 0);

    DrawParams arm; arm.color = black; arm.outline = 0.02f; arm.emissive = em;
    // 旋回防御（右クリック）
    if (a.st == ASt::Guard)
    {
        float open = SmoothStep(0.f, 0.15f, a.stT);
        float th = a.stT * 16.f;
        const float rad = 0.8f * open + 0.15f;
        XMFLOAT3 C{ 0, 1.05f, 0.55f + 0.5f * open };
        D(R->cylinder, S(0.2f, 0.42f, 0.2f) * Tr(0, -0.42f, 0) * XMMatrixRotationX(-1.45f) * XMMatrixRotationZ(0.2f) * Tr(0.4f, 1.05f, 0), black);
        D(R->cylinder, S(0.2f, 0.42f, 0.2f) * Tr(0, -0.42f, 0) * XMMatrixRotationX(-1.3f) * XMMatrixRotationZ(-0.25f) * Tr(-0.4f, 1.0f, 0), black);
        XMFLOAT3 hand = ToW(F3(0.12f, 1.08f, 0.5f), P);
        for (int i = 0; i < 24; i++)
        {
            float a0 = 2 * PI * i / 24, a1 = 2 * PI * (i + 1) / 24;
            float r0 = rad * 0.45f, r1 = rad + 0.25f;
            XMFLOAT3 p0 = ToW(F3(C.x + std::cos(a0) * r0, C.y + std::sin(a0) * r0, C.z), P), p1 = ToW(F3(C.x + std::cos(a0) * r1, C.y + std::sin(a0) * r1, C.z), P);
            XMFLOAT3 p2 = ToW(F3(C.x + std::cos(a1) * r1, C.y + std::sin(a1) * r1, C.z), P), p3 = ToW(F3(C.x + std::cos(a1) * r0, C.y + std::sin(a1) * r0, C.z), P);
            float lag = WrapAngle(th - a0);
            float al = (lag > 0 && lag < 2.4f ? (1 - lag / 2.4f) * 0.5f : 0.f) + 0.12f;
            if (sheetMode) R->DrawQuadWorld(p0, p1, p2, p3, C4(0.95f, 0.42f, 0.3f, std::min(1.f, al * open * 1.6f)), Blend::Alpha);
            else R->DrawQuadWorld(p0, p1, p2, p3, C4(1.f, 0.62f, 0.5f, al * open), Blend::Add);
        }
        for (int g = 3; g >= 0; g--)
        {
            float ang = th - g * 0.32f;
            XMMATRIX nm = Tr(0, -0.55f, 0) * XMMatrixRotationZ(ang) * Tr(C.x + std::cos(ang) * rad, C.y + std::sin(ang) * rad, C.z);
            if (g == 0)
            {
                DrawParams p; p.color = C4(0.88f, 0.9f, 0.95f); p.outline = 0.012f; p.rim = 0.7f; p.emissive = Add(em, F3(0.1f, 0.1f, 0.12f));
                R->Draw(R->cube, S(0.14f, 1.1f, 0.04f) * nm * P, p);
                XMFLOAT3 eye = ToW(F3(C.x + std::cos(ang) * rad + std::sin(ang) * 0.45f, C.y + std::sin(ang) * rad - std::cos(ang) * 0.45f, C.z), P);
                DrawParams cp; cp.color = C4(0.85f, 0.15f, 0.12f);
                Seg(hand, eye, 0.03f, cp);
            }
            else
            {
                DrawParams gp; gp.color = C4(0.9f, 0.95f, 1.f, 0.35f - g * 0.08f); gp.blend = Blend::Add; gp.unlit = true; gp.depthWrite = false;
                R->Draw(R->cube, S(0.14f, 1.1f, 0.04f) * nm * P, gp);
            }
        }
        return;
    }

    // 腕とひも通し（町では武器をしまう）
    XMFLOAT3 shR = ToW(F3(0.45f, 1.0f, 0), P), shL = ToW(F3(-0.45f, 1.0f, 0), P);
    D(R->cylinder, S(0.2f, 0.42f, 0.2f) * Tr(0, -0.42f, 0) * XMMatrixRotationZ(-0.35f) * Tr(-0.45f, 1.0f, 0), black);
    if (!WeaponsOut() || a.st == ASt::Dodge)
    {
        float sw = std::sin(t) * 0.5f * moving;
        D(R->cylinder, S(0.2f, 0.42f, 0.2f) * Tr(0, -0.42f, 0) * XMMatrixRotationX(sw) * XMMatrixRotationZ(0.35f) * Tr(0.45f, 1.0f, 0), black);
        return;
    }
    XMFLOAT3 hand;
    XMFLOAT3 needle = KurobeeNeedle(a, &hand);
    Seg(shR, hand, 0.1f, arm);
    if (a.st == ASt::Attack)
    {
        float k = Saturate(a.stT / Def(a).dur[a.combo]);
        if (k > 0.65f) Seg(shL, Lerp3(hand, needle, 0.15f), 0.1f, arm);    // 両手でたぐる
    }
    // 針の向き：飛んでいる間は進む方向、刺さっているときは地面へ斜め
    XMFLOAT3 dir = Sub(needle, hand);
    if (Len(dir) < 0.3f) dir = F3(0, 1, 0.2f);
    XMVECTOR dv = XMVector3Normalize(XMLoadFloat3(&dir));
    XMStoreFloat3(&dir, dv);
    XMFLOAT3 tip = Add(needle, Mul(dir, 0.55f)), tail = Sub(needle, Mul(dir, 0.55f));
    if (a.st == ASt::Attack)
    {
        // 薙ぎ払った扇形の跡（地面すれすれに光る帯）
        float k = Saturate(a.stT / Def(a).dur[a.combo]);
        if (k > 0.35f && k < 0.85f)
        {
            float u = Saturate((k - 0.35f) / 0.3f);
            float e = u * u * (3 - 2 * u);
            float fade = k < 0.65f ? 1.f : 1.f - (k - 0.65f) / 0.2f;
            float half = kFanArc[a.combo] * 0.5f * PI / 180.f, fdir = kFanDir[a.combo], reach = kFanReach[a.combo];
            const int N = 16;
            for (int i = 0; i < N; i++)
            {
                float u0 = i / (float)N, u1 = (i + 1) / (float)N;
                if (u0 > e) break;
                u1 = std::min(u1, e);
                float a0 = a.yaw + fdir * (u0 * 2.f - 1.f) * half, a1 = a.yaw + fdir * (u1 * 2.f - 1.f) * half;
                float r0 = 0.9f, r1 = reach;
                XMFLOAT3 p0 = Add(a.pos, Add(Mul(Fwd(a0), r0), F3(0, 0.08f, 0))), p1 = Add(a.pos, Add(Mul(Fwd(a0), r1), F3(0, 0.08f, 0)));
                XMFLOAT3 p2 = Add(a.pos, Add(Mul(Fwd(a1), r1), F3(0, 0.08f, 0))), p3 = Add(a.pos, Add(Mul(Fwd(a1), r0), F3(0, 0.08f, 0)));
                float al = (0.15f + 0.35f * (1.f - (e - u1))) * fade;
                if (sheetMode) R->DrawQuadWorld(p0, p1, p2, p3, C4(0.95f, 0.42f, 0.3f, std::min(1.f, al * 1.4f)), Blend::Alpha);
                else R->DrawQuadWorld(p0, p1, p2, p3, C4(1.f, 0.55f, 0.4f, al), Blend::Add);
            }
        }
    }
    DrawParams np; np.color = C4(0.85f, 0.88f, 0.92f); np.outline = 0.012f; np.rim = 0.6f; np.emissive = em;
    Seg(tail, tip, 0.06f, np);
    DrawParams hole; hole.color = C4(0.15f, 0.12f, 0.12f);
    R->Draw(R->sphere, S(0.08f, 0.08f, 0.08f) * Tr(Lerp3(tail, tip, 0.12f).x, Lerp3(tail, tip, 0.12f).y, Lerp3(tail, tip, 0.12f).z), hole);
    // 赤い組紐（たるみつき）
    DrawParams cp; cp.color = C4(0.85f, 0.15f, 0.12f);
    XMFLOAT3 mid = Lerp3(hand, tail, 0.5f);
    float slack = a.st == ASt::Attack ? 0.15f : 0.3f;
    mid.y -= slack;
    Seg(hand, mid, 0.025f, cp);
    Seg(mid, tail, 0.025f, cp);
    if (a.st == ASt::Attack)
    {
        float k = Saturate(a.stT / Def(a).dur[a.combo]);
        if (k > 0.2f && k < 0.65f) R->DrawBillboard(T["glint"], needle, 1.2f, 1.2f, C4(1, 0.7f, 0.6f, 0.7f), Blend::Add, { 1, 1, 0, 0 }, time * 8);
    }
}

// ------------------------------------------------------------------
// カイリ（カワウソ・ミシン針）
// ------------------------------------------------------------------
void Game::DrawKairi(const Actor& a, float alpha)
{
    float moving = Saturate(a.speedNow / 3.f);
    float t = a.walkT * 9.f;
    float bob = std::abs(std::sin(t)) * 0.06f * moving;
    XMMATRIX P = XMMatrixRotationY(a.yaw) * Tr(a.pos.x, a.pos.y + bob, a.pos.z);
    if (a.st == ASt::Dodge)
    {
        // 腹ばいで滑る回避
        float k = Saturate(a.stT / DodgeDur(2));
        float lie = std::sin(Saturate(k * 1.4f) * PI * 0.5f) * (k < 0.8f ? 1.f : 1.f - (k - 0.8f) / 0.2f);
        P = Tr(0, -0.4f * lie, 0) * XMMatrixRotationX(1.35f * lie) * Tr(0, 0.4f * lie, 0) * XMMatrixRotationY(YawOf(a.dodgeDir)) * Tr(a.pos.x, a.pos.y, a.pos.z);
    }
    else if (a.st == ASt::Hit) P = XMMatrixRotationX(-0.3f) * P;
    else if (a.st == ASt::Down) P = Tr(0, 0.3f, 0) * XMMatrixRotationX(-PI / 2) * XMMatrixRotationY(a.yaw) * Tr(a.pos.x, 0, a.pos.z);
    DrawShadow(a.pos, 1.3f);
    XMFLOAT3 em = Mul(F3(1, 0.5f, 0.5f), a.flash * 0.8f);
    auto D = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, float ol = 0.02f) {
        DrawParams p; p.color = c; p.outline = ol; p.emissive = em; p.rim = 0.3f; R->Draw(m, L * P, p);
    };
    XMFLOAT4 fur = C4(0.55f, 0.38f, 0.25f), cream = C4(0.95f, 0.88f, 0.72f), teal = C4(0.2f, 0.55f, 0.6f), red = C4(0.8f, 0.2f, 0.18f);
    for (int s = -1; s <= 1; s += 2)
    {
        float sw = std::sin(t) * 0.5f * moving * s;
        D(R->cylinder, S(0.18f, 0.28f, 0.18f) * Tr(0, -0.28f, 0) * XMMatrixRotationX(sw) * Tr(s * 0.17f, 0.3f, 0), fur);
        D(R->sphere, S(0.2f, 0.08f, 0.28f) * Tr(0, -0.28f, 0.06f) * XMMatrixRotationX(sw) * Tr(s * 0.17f, 0.3f, 0), C4(0.4f, 0.27f, 0.18f));
    }
    // しっぽ
    float tsw = std::sin(t * 0.5f) * 0.25f;
    D(R->cone, S(0.34f, 0.85f, 0.3f) * XMMatrixRotationX(-PI / 2 + 0.45f) * XMMatrixRotationY(tsw) * Tr(0, 0.32f, -0.3f), fur);
    D(R->sphere, S(0.78f, 0.82f, 0.7f) * Tr(0, 0.66f, 0), fur);
    D(R->sphere, S(0.5f, 0.55f, 0.25f) * Tr(0, 0.64f, 0.25f), cream, 0.01f);
    // 法被と縄帯
    D(R->cylinder, S(0.84f, 0.48f, 0.76f) * Tr(0, 0.42f, 0), teal);
    D(R->cube, S(0.08f, 0.45f, 0.03f) * XMMatrixRotationZ(0.35f) * Tr(-0.12f, 0.72f, 0.36f), C4(0.95f, 0.95f, 0.92f), 0);
    D(R->cube, S(0.08f, 0.45f, 0.03f) * XMMatrixRotationZ(-0.35f) * Tr(0.12f, 0.72f, 0.36f), C4(0.95f, 0.95f, 0.92f), 0);
    D(R->cylinder, S(0.86f, 0.08f, 0.78f) * Tr(0, 0.58f, 0), C4(0.85f, 0.65f, 0.3f), 0.01f);
    D(R->cylinder, S(0.07f, 0.3f, 0.07f) * Tr(-0.42f, 0.32f, 0.1f), C4(0.45f, 0.3f, 0.18f), 0.01f);
    D(R->cube, S(0.14f, 0.1f, 0.1f) * Tr(-0.42f, 0.32f, 0.1f), C4(0.55f, 0.55f, 0.6f), 0.01f);
    // 頭
    D(R->sphere, S(0.66f, 0.58f, 0.62f) * Tr(0, 1.25f, 0), fur);
    D(R->sphere, S(0.38f, 0.26f, 0.3f) * Tr(0, 1.15f, 0.27f), cream, 0.012f);
    D(R->sphere, S(0.13f, 0.09f, 0.08f) * Tr(0, 1.21f, 0.42f), C4(0.15f, 0.1f, 0.08f), 0);
    for (int s = -1; s <= 1; s += 2)
    {
        D(R->sphere, S(0.07f, 0.09f, 0.04f) * Tr(s * 0.13f, 1.31f, 0.28f), C4(0.1f, 0.08f, 0.1f), 0);
        D(R->sphere, S(0.14f, 0.11f, 0.08f) * Tr(s * 0.25f, 1.5f, 0), fur, 0.012f);
    }
    // 水玉の手ぬぐい
    D(R->cylinder, S(0.68f, 0.1f, 0.64f) * Tr(0, 1.38f, 0), red, 0.01f);
    for (int i = 0; i < 6; i++)
    {
        float an = i * 1.05f;
        D(R->sphere, S(0.06f, 0.05f, 0.03f) * XMMatrixRotationY(-an + PI / 2) * Tr(std::sin(an) * 0.34f, 1.43f, std::cos(an) * 0.32f), C4(1, 1, 1), 0);
    }
    D(R->cube, S(0.08f, 0.1f, 0.3f) * XMMatrixRotationX(0.5f) * Tr(-0.3f, 1.38f, -0.32f), red, 0.01f);

    // 腕と針
    XMFLOAT3 shR = ToW(F3(0.36f, 0.98f, 0), P), shL = ToW(F3(-0.36f, 0.98f, 0), P);
    DrawParams arm; arm.color = fur; arm.outline = 0.018f; arm.emissive = em;
    XMFLOAT3 handL = ToW(F3(-0.42f, 0.62f, 0.12f), P);
    XMFLOAT3 hl = F3(0.42f, 0.62f, 0.12f);
    bool showNeedles = WeaponsOut() && !a.broken;
    bool big = false;
    if (WeaponsOut())
    {
        if (a.st == ASt::Attack)
        {
            float k = Saturate(a.stT / Def(a).dur[a.combo]);
            float h = Def(a).hitFrac;
            if (k < h) hl = Lerp3(F3(0.35f, 1.0f, 0.1f), F3(0.4f, 1.5f, -0.4f), k / h);
            else hl = Lerp3(F3(0.3f, 1.1f, 0.6f), F3(0.4f, 0.8f, 0.3f), Saturate((k - h) / (1 - h)));
            if (k >= h && k < h + 0.3f) showNeedles = false;
        }
        else if (a.st == ASt::Charging) { hl = F3(0.45f, 1.55f, -0.45f); big = true; }
        else if (a.st == ASt::ChargeAtk) { hl = a.stT < 0.12f ? F3(0.45f, 1.55f, -0.45f) : F3(0.3f, 1.1f, 0.65f); big = a.stT < 0.12f; showNeedles = a.stT < 0.12f; }
        else hl = F3(0.45f, 0.75f, 0.25f);
    }
    else
    {
        float sw = std::sin(t) * 0.25f * moving;
        hl = F3(0.42f, 0.62f, 0.12f + sw);
        handL = ToW(F3(-0.42f, 0.62f, 0.12f - sw), P);
    }
    XMFLOAT3 handR = ToW(hl, P);
    Seg(shR, handR, 0.085f, arm);
    Seg(shL, handL, 0.085f, arm);
    DrawParams hp; hp.color = cream; hp.outline = 0.01f;
    R->Draw(R->sphere, S(0.13f, 0.13f, 0.13f) * Tr(handR.x, handR.y, handR.z), hp);
    if (showNeedles)
    {
        DrawParams np; np.color = C4(0.88f, 0.9f, 0.95f); np.rim = 0.7f; np.emissive = em;
        if (big)
        {
            np.emissive = Add(em, Mul(F3(0.3f, 0.5f, 0.7f), 0.5f + 0.5f * std::sin(time * 14)));
            XMFLOAT3 tip = Add(handR, Mul(Fwd(a.yaw), 1.0f));
            Seg(handR, tip, 0.05f, np);
            R->DrawBillboard(T["glint"], tip, 1.0f, 1.0f, C4(0.6f, 0.9f, 1, 0.7f), Blend::Add, { 1, 1, 0, 0 }, time * 5);
        }
        else
            for (int i = -1; i <= 1; i++)
            {
                XMFLOAT3 tip = ToW(Add(hl, F3(i * 0.07f, 0.35f, 0.18f)), P);
                Seg(handR, tip, 0.014f, np);
            }
    }
}

// ------------------------------------------------------------------
// ナギ（霧の巫女）
// ------------------------------------------------------------------
void Game::DrawNagi(XMFLOAT3 pos, float yaw, float alpha)
{
    float fl = std::sin(time * 1.5f) * 0.06f;
    XMMATRIX P = XMMatrixRotationY(yaw) * Tr(pos.x, pos.y + fl, pos.z);
    auto D = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, float ol = 0.02f) {
        DrawParams p; p.color = c; p.color.w = alpha; p.outline = alpha > 0.95f ? ol : 0; p.rim = 0.6f; p.emissive = { 0.08f, 0.08f, 0.12f };
        if (alpha < 0.99f) { p.blend = Blend::Alpha; p.depthWrite = false; }
        R->Draw(m, L * P, p);
    };
    XMFLOAT4 hair = C4(0.78f, 0.76f, 0.9f), white = C4(0.96f, 0.96f, 0.98f), hakama = C4(0.42f, 0.5f, 0.68f), skin = C4(0.98f, 0.88f, 0.8f);
    D(R->cone, S(1.0f, 0.95f, 0.9f) * Tr(0, 0, 0), hakama);
    D(R->cylinder, S(0.58f, 0.5f, 0.48f) * Tr(0, 0.75f, 0), white);
    for (int s = -1; s <= 1; s += 2)
        D(R->cube, S(0.32f, 0.55f, 0.32f) * Tr(s * 0.42f, 0.95f, 0.05f), white, 0.015f);
    D(R->cylinder, S(0.6f, 0.08f, 0.5f) * Tr(0, 0.86f, 0), C4(0.75f, 0.2f, 0.2f), 0.01f);
    D(R->sphere, S(0.66f, 0.85f, 0.45f) * Tr(0, 1.15f, -0.18f), hair);
    D(R->sphere, S(0.58f, 0.56f, 0.56f) * Tr(0, 1.4f, 0), skin);
    D(R->sphere, S(0.64f, 0.38f, 0.6f) * Tr(0, 1.55f, -0.03f), hair);
    for (int s = -1; s <= 1; s += 2)
        D(R->cube, S(0.1f, 0.015f, 0.02f) * Tr(s * 0.12f, 1.38f, 0.28f), C4(0.25f, 0.2f, 0.3f), 0);
    D(R->cone, S(0.12f, 0.18f, 0.06f) * XMMatrixRotationZ(0.5f) * Tr(0.28f, 1.66f, -0.05f), C4(0.8f, 0.2f, 0.2f), 0.01f);
    // 神楽鈴
    D(R->cylinder, S(0.04f, 0.5f, 0.04f) * Tr(0, 0.65f, 0.38f), C4(0.6f, 0.4f, 0.2f), 0);
    for (int i = 0; i < 5; i++)
        D(R->sphere, S(0.09f, 0.09f, 0.09f) * Tr(std::cos(i * 1.26f) * 0.1f, 1.18f + (i % 2) * 0.06f, 0.38f + std::sin(i * 1.26f) * 0.1f), C4(0.95f, 0.78f, 0.3f), 0);
    for (int i = 0; i < 3; i++)
    {
        XMFLOAT4 rc[3] = { C4(0.8f, 0.2f, 0.2f), C4(0.3f, 0.6f, 0.35f), C4(0.95f, 0.8f, 0.3f) };
        D(R->cube, S(0.03f, 0.5f, 0.01f) * Tr(-0.05f + i * 0.05f, 0.38f, 0.4f), rc[i], 0);
    }
    R->DrawBillboard(T["smoke"], Add(pos, F3(0, 0.3f, 0)), 2.6f, 1.2f, C4(0.9f, 0.92f, 1.f, 0.45f * alpha), Blend::Alpha, { 0.125f, 0.125f, 0.125f * ((int)(time * 6) % 8), 0.25f }, 0, true, 0.3f);
}

// ------------------------------------------------------------------
// タヌキの商人
// ------------------------------------------------------------------
void Game::DrawTanuki(XMFLOAT3 pos, float yaw, float phase)
{
    float bob = std::sin(phase * 2.f) * 0.03f;
    XMMATRIX P = XMMatrixRotationY(yaw) * Tr(pos.x, pos.y + bob, pos.z);
    auto D = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, float ol = 0.02f) { DrawParams p; p.color = c; p.outline = ol; p.rim = 0.3f; R->Draw(m, L * P, p); };
    XMFLOAT4 fur = C4(0.55f, 0.42f, 0.3f), dark = C4(0.2f, 0.16f, 0.14f), cream = C4(0.92f, 0.85f, 0.7f);
    DrawShadow(pos, 1.3f);
    for (int s = -1; s <= 1; s += 2) D(R->cylinder, S(0.2f, 0.25f, 0.2f) * Tr(s * 0.2f, 0, 0), dark);
    D(R->sphere, S(0.95f, 0.95f, 0.85f) * Tr(0, 0.65f, 0), fur);
    D(R->sphere, S(0.6f, 0.6f, 0.3f) * Tr(0, 0.6f, 0.28f), cream, 0.01f);
    D(R->cube, S(0.55f, 0.5f, 0.05f) * Tr(0, 0.55f, 0.42f), C4(0.2f, 0.3f, 0.55f), 0.01f);
    // しま模様のしっぽ
    for (int i = 0; i < 4; i++)
        D(R->sphere, S(0.3f - i * 0.03f, 0.3f - i * 0.03f, 0.3f) * Tr(0.1f, 0.35f + i * 0.08f, -0.45f - i * 0.18f), i % 2 ? dark : fur, 0.01f);
    D(R->sphere, S(0.62f, 0.55f, 0.58f) * Tr(0, 1.25f, 0.02f), fur);
    D(R->sphere, S(0.3f, 0.2f, 0.25f) * Tr(0, 1.15f, 0.27f), cream, 0.01f);
    for (int s = -1; s <= 1; s += 2)
    {
        D(R->sphere, S(0.2f, 0.14f, 0.08f) * Tr(s * 0.14f, 1.27f, 0.25f), dark, 0);
        D(R->sphere, S(0.05f, 0.06f, 0.03f) * Tr(s * 0.14f, 1.27f, 0.29f), C4(1, 1, 1), 0);
        D(R->sphere, S(0.14f, 0.12f, 0.08f) * Tr(s * 0.22f, 1.48f, 0), dark, 0.01f);
    }
    D(R->sphere, S(0.1f, 0.07f, 0.07f) * Tr(0, 1.2f, 0.4f), dark, 0);
    // 編み笠
    D(R->cone, S(1.0f, 0.3f, 1.0f) * Tr(0, 1.48f, 0), C4(0.85f, 0.72f, 0.45f), 0.015f);
    // そろばん
    D(R->cube, S(0.35f, 0.2f, 0.05f) * XMMatrixRotationX(-0.5f) * Tr(0.25f, 0.75f, 0.45f), C4(0.4f, 0.25f, 0.15f), 0.01f);
}

// ------------------------------------------------------------------
// 大樹の番人（10階の主）／根の主の腕（7階のQTE）
// ------------------------------------------------------------------
void Game::DrawBigEnemy(const Enemy& e)
{
    float sc = 1.f, sinkY = 0;
    if (e.st == ESt::Dead)
    {
        if (e.deadT > 1.6f) return;
        sinkY = -e.deadT * 3.f;
        sc = 1.f - e.deadT * 0.3f;
    }
    XMFLOAT3 em = Mul(F3(1, 1, 1), e.flash * 0.6f);
    if (e.st == ESt::Windup) { float pulse = 0.5f + 0.5f * std::sin(e.stT * 25); em = Add(em, F3(0.5f * pulse, 0.08f * pulse, 0.05f * pulse)); }
    XMMATRIX P = S(sc, sc, sc) * XMMatrixRotationY(e.yaw) * Tr(e.pos.x, e.pos.y + sinkY, e.pos.z);
    auto toW = [&](XMFLOAT3 l) { return ToW(l, P); };
    DrawParams bark; bark.tex = T["bark"]; bark.color = C4(0.6f, 0.46f, 0.34f); bark.outline = 0.025f; bark.emissive = em; bark.uv = { 2, 2, 0, 0 };
    if (e.t == EType::Guardian)
    {
        DrawShadow(e.pos, 7.f, 0.5f);
        R->Draw(R->cylinder, S(2.6f, 4.4f, 2.4f) * P, bark);
        // 根の脚
        for (int i = 0; i < 6; i++)
        {
            float an = i * 1.05f + 0.3f;
            XMFLOAT3 a = toW(F3(std::cos(an) * 1.0f, 0.9f, std::sin(an) * 1.0f));
            XMFLOAT3 b = toW(F3(std::cos(an) * 2.4f, 0.0f, std::sin(an) * 2.4f));
            Seg(a, b, 0.35f, bark);
        }
        // 顔
        DrawParams eye; eye.color = C4(1, 0.7f, 0.2f); eye.emissive = { 0.9f, 0.55f, 0.1f }; eye.unlit = true;
        for (int s = -1; s <= 1; s += 2)
            R->Draw(R->sphere, S(0.4f, 0.26f, 0.15f) * Tr(s * 0.55f, 3.1f, 1.15f) * P, eye);
        DrawParams mouth; mouth.color = C4(0.08f, 0.05f, 0.04f);
        R->Draw(R->sphere, S(0.8f, 0.4f, 0.15f) * Tr(0, 2.3f, 1.15f) * P, mouth);
        DrawParams moss; moss.color = C4(0.35f, 0.5f, 0.28f); moss.outline = 0.02f;
        for (int i = 0; i < 5; i++) R->Draw(R->sphere, S(0.4f, 0.5f, 0.3f) * Tr(-0.8f + i * 0.4f, 1.8f - (i % 2) * 0.2f, 1.1f) * P, moss);
        // 枝の腕
        bool raise = e.st == ESt::Windup && e.pattern == 0;
        bool slam = e.st == ESt::Attack && e.pattern == 0;
        bool dig = (e.st == ESt::Windup || e.st == ESt::Attack) && e.pattern == 1;
        for (int s = -1; s <= 1; s += 2)
        {
            XMFLOAT3 sh = toW(F3(s * 1.25f, 3.5f, 0));
            XMFLOAT3 el = toW(F3(s * 2.4f, 3.4f + (raise ? 1.5f : 0), raise ? 0.2f : 0.9f));
            XMFLOAT3 hd = toW(raise ? F3(s * 2.0f, 6.5f, 0.6f) : slam ? F3(s * 1.2f, 0.4f, 3.0f) : dig ? F3(s * 2.6f, -0.2f, 1.4f) : F3(s * 2.8f, 1.8f + std::sin(e.bob * 1.5f + s) * 0.3f, 2.0f));
            Seg(sh, el, 0.45f, bark);
            Seg(el, hd, 0.32f, bark);
            for (int f = -1; f <= 1; f++) Seg(hd, Add(hd, F3(f * 0.4f, -0.3f, 0.6f)), 0.1f, bark);
        }
        // 枝葉の冠
        DrawParams lf; lf.color = C4(0.22f, 0.36f, 0.22f); lf.outline = 0.03f; lf.emissive = Mul(em, 0.3f);
        R->Draw(R->sphere, S(4.6f, 3.0f, 4.6f) * Tr(0, 5.6f, 0) * P, lf);
        lf.color = C4(0.28f, 0.42f, 0.25f);
        R->Draw(R->sphere, S(3.0f, 2.2f, 3.0f) * Tr(1.8f, 5.0f, -0.6f) * P, lf);
        R->Draw(R->sphere, S(3.2f, 2.4f, 3.2f) * Tr(-1.8f, 5.2f, 0.4f) * P, lf);
        DrawParams rp; rp.tex = T["rope"]; rp.color = C4(1, 0.92f, 0.7f); rp.uv = { 6, 1, 0, 0.35f }; rp.emissive = em;
        R->Draw(R->cylinder, S(2.75f, 0.25f, 2.55f) * Tr(0, 3.75f, 0) * P, rp);
        return;
    }
    // 根の主の腕：地面の穴からうねりながら伸びる
    float rise = Saturate(rootAnim) * (e.st == ESt::Dead ? 1.f : 1.f);
    DrawParams hole; hole.color = C4(0.03f, 0.03f, 0.04f);
    R->Draw(R->cylinder, S(4.2f, 0.04f, 4.2f) * Tr(e.pos.x, 0.02f, e.pos.z), hole);
    DrawParams rock; rock.tex = T["cave_wall"]; rock.color = C4(0.6f, 0.62f, 0.7f); rock.outline = 0.02f;
    for (int i = 0; i < 7; i++)
    {
        float an = i * 0.9f;
        R->Draw(R->sphere, S(0.9f, 0.5f, 0.8f) * XMMatrixRotationY(an) * Tr(e.pos.x + std::cos(an) * 2.2f, 0.15f, e.pos.z + std::sin(an) * 2.2f), rock);
    }
    const int N = 10;
    XMFLOAT3 prev = toW(F3(0, -0.5f, 0));
    float hitBend = qteOn && qteStep >= 1 ? 0.3f : 0.f;
    for (int i = 1; i <= N; i++)
    {
        float u = (float)i / N;
        float sway = std::sin(time * 1.4f + i * 0.55f) * 0.5f * u;
        float fwd = std::sin(u * PI * 0.75f) * 3.2f * rise + hitBend * u * 2;
        float h = u * 8.5f * rise;
        if (qteOn && qteStep == 0) h -= std::sin(time * 6.f) * 0.4f * u;
        XMFLOAT3 p = toW(F3(sway, h, fwd));
        DrawParams b = bark;
        b.color = C4(0.45f, 0.33f, 0.25f);
        Seg(prev, p, Lerpf(1.1f, 0.28f, u), b);
        if (i % 2 == 0)
            for (int s = -1; s <= 1; s += 2)
                R->Draw(R->cone, S(0.25f, 0.8f * (1 - u * 0.5f), 0.25f) * XMMatrixRotationZ(s * 1.2f) * Tr(p.x, p.y, p.z), b);
        // ツムギの糸で縫い留め（3つ目の合図以降）
        if (qteOn && qteStep >= 3 && i % 3 == 0)
        {
            DrawParams g; g.color = C4(1, 0.85f, 0.35f); g.emissive = { 0.6f, 0.45f, 0.1f }; g.unlit = true;
            R->Draw(R->cylinder, S(Lerpf(2.4f, 0.7f, u), 0.08f, Lerpf(2.4f, 0.7f, u)) * Tr(p.x, p.y, p.z), g);
        }
        prev = p;
    }
    DrawParams eye; eye.color = C4(1, 0.65f, 0.2f); eye.emissive = { 0.9f, 0.45f, 0.1f }; eye.unlit = true;
    for (int s = -1; s <= 1; s += 2)
        R->Draw(R->sphere, S(0.22f, 0.16f, 0.12f) * Tr(prev.x + s * 0.2f, prev.y + 0.1f, prev.z - 0.2f), eye);
}

// ------------------------------------------------------------------
// 宝箱（通常／武器の大箱／ジオラマパーツの糸巻き）
// ------------------------------------------------------------------
void Game::DrawChest(const Chest& ch)
{
    XMMATRIX b = XMMatrixRotationY(ch.yaw) * Tr(ch.pos.x, 0, ch.pos.z);
    if (ch.kind == 2)
    {
        // 糸巻き：木の糸巻きに太い糸が巻かれた形。開けると糸がほどける
        float unwind = ch.opened ? Saturate(ch.openT / 0.8f) : 0.f;
        float fl = ch.opened ? 0.f : 0.25f + std::sin(time * 2.f + ch.pos.x) * 0.12f;
        XMMATRIX sp = XMMatrixRotationY(time * (ch.opened ? 8.f * (1 - unwind) : 0.6f)) * Tr(ch.pos.x, fl, ch.pos.z);
        DrawShadow(ch.pos, 1.4f, 0.4f);
        DrawParams wd; wd.tex = T["wood"]; wd.color = C4(0.85f, 0.65f, 0.45f); wd.outline = 0.02f;
        R->Draw(R->cylinder, S(1.15f, 0.12f, 1.15f) * Tr(0, 0.05f, 0) * sp, wd);
        R->Draw(R->cylinder, S(1.15f, 0.12f, 1.15f) * Tr(0, 1.13f, 0) * sp, wd);
        R->Draw(R->cylinder, S(0.5f, 1.0f, 0.5f) * Tr(0, 0.15f, 0) * sp, wd);
        XMFLOAT4 cols[4] = { C4(0.25f, 0.35f, 0.7f), C4(0.8f, 0.25f, 0.22f), C4(0.95f, 0.7f, 0.25f), C4(0.35f, 0.65f, 0.4f) };
        XMFLOAT4 tc = cols[((int)(std::fabs(ch.pos.x * 3.1f + ch.pos.z * 1.7f))) % 4];
        float r = Lerpf(1.0f, 0.5f, unwind);
        if (r > 0.52f)
        {
            DrawParams th; th.tex = T["rope"]; th.color = tc; th.uv = { 5, 4, 0, 0 }; th.outline = 0.02f; th.emissive = Mul(F3(tc.x, tc.y, tc.z), ch.opened ? 0.1f : 0.25f);
            R->Draw(R->cylinder, S(r, 0.86f, r) * Tr(0, 0.18f, 0) * sp, th);
        }
        // ほどけた糸の端
        DrawParams ts; ts.color = tc; ts.emissive = Mul(F3(tc.x, tc.y, tc.z), 0.3f);
        XMFLOAT3 prev = ToW(F3(r * 0.5f, 0.6f, 0), sp);
        int seg = ch.opened ? 10 : 4;
        for (int i = 1; i <= seg; i++)
        {
            float u = (float)i / seg;
            XMFLOAT3 q = ToW(F3(r * 0.5f + u * (ch.opened ? 1.8f * unwind : 0.5f), 0.6f + std::sin(u * 6 + time * 3) * 0.25f + (ch.opened ? u * 1.5f * unwind : -u * 0.5f), std::cos(u * 5 + time * 2) * 0.3f), sp);
            Seg(prev, q, 0.03f, ts);
            prev = q;
        }
        if (!ch.opened)
            R->DrawBillboard(T["glint"], ToW(F3(0, 0.7f, 0), sp), 2.6f, 2.6f, C4(1, 0.85f, 0.5f, 0.55f), Blend::Add, { 1, 1, 0, 0 }, time * 0.6f);
        return;
    }
    float s = ch.kind == 1 ? 1.55f : 1.f;
    XMMATRIX B = S(s, s, s) * b;
    DrawShadow(ch.pos, 1.6f * s);
    DrawParams p; p.color = ch.kind == 1 ? C4(0.12f, 0.1f, 0.13f) : C4(0.6f, 0.15f, 0.12f); p.outline = 0.02f; p.rim = ch.kind == 1 ? 0.6f : 0.25f;
    R->Draw(R->cube, S(1.0f, 0.55f, 0.65f) * Tr(0, 0.3f, 0) * B, p);
    DrawParams g; g.color = C4(0.95f, 0.75f, 0.3f); g.emissive = { 0.1f, 0.07f, 0.02f };
    R->Draw(R->cube, S(1.04f, 0.1f, 0.69f) * Tr(0, 0.5f, 0) * B, g);
    if (ch.kind == 1)
    {
        R->Draw(R->cube, S(0.08f, 0.56f, 0.69f) * Tr(-0.3f, 0.3f, 0) * B, g);
        R->Draw(R->cube, S(0.08f, 0.56f, 0.69f) * Tr(0.3f, 0.3f, 0) * B, g);
    }
    float open = ch.opened ? -1.4f * Saturate(ch.openT / 0.3f) : 0;
    R->Draw(R->cube, S(1.0f, 0.22f, 0.65f) * Tr(0, 0.11f, 0.32f) * XMMatrixRotationX(open) * Tr(0, 0.58f, -0.32f) * B, p);
    R->Draw(R->cube, S(0.18f, 0.2f, 0.05f) * Tr(0, 0.5f, -0.34f) * B, g);
    if (ch.kind == 1)
    {
        DrawParams crest; crest.color = C4(0.95f, 0.92f, 0.85f); crest.emissive = { 0.2f, 0.2f, 0.18f };
        R->Draw(R->cylinder, S(0.22f, 0.02f, 0.22f) * XMMatrixRotationX(PI / 2) * Tr(0, 0.32f, -0.335f) * B, crest);
        DrawParams cord; cord.color = C4(0.85f, 0.2f, 0.18f);
        Seg(ToW(F3(-0.5f, 0.55f, -0.33f), B), ToW(F3(0.5f, 0.55f, -0.33f), B), 0.025f, cord);
        if (!ch.opened) R->DrawBillboard(T["glint"], ToW(F3(0, 0.9f, 0), B), 2.4f, 2.4f, C4(0.9f, 0.6f, 1.f, 0.45f), Blend::Add, { 1, 1, 0, 0 }, time * 0.4f);
    }
}
