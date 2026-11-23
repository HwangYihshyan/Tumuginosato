#include "Game.h"

// 2点を結ぶ円柱
void Game::Seg(XMFLOAT3 a, XMFLOAT3 b, float r, DrawParams p)
{
    XMVECTOR va = XMLoadFloat3(&a), vb = XMLoadFloat3(&b);
    XMVECTOR d = vb - va;
    float len = XMVectorGetX(XMVector3Length(d));
    if (len < 1e-4f) return;
    XMVECTOR up = d / len;
    XMVECTOR ref = std::fabs(XMVectorGetY(up)) > 0.95f ? XMVectorSet(1, 0, 0, 0) : XMVectorSet(0, 1, 0, 0);
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, ref));
    XMVECTOR fwd = XMVector3Cross(right, up);
    XMMATRIX rot(right, up, fwd, XMVectorSet(0, 0, 0, 1));
    R->Draw(R->cylinder, XMMatrixScaling(r * 2, len, r * 2) * rot * XMMatrixTranslation(a.x, a.y, a.z), p);
}

static XMMATRIX S(float x, float y, float z) { return XMMatrixScaling(x, y, z); }
static XMMATRIX Tr(float x, float y, float z) { return XMMatrixTranslation(x, y, z); }

// ソラの描画は Sora.cpp

// ------------------------------------------------------------------
// クロベエ（アナグマ・ひも通し）
// クロベエ・カイリなどは Characters2.cpp

// ------------------------------------------------------------------
// ツムギ（蚕の精霊）
// ------------------------------------------------------------------
void Game::DrawTsumugi(XMFLOAT3 pos)
{
    XMMATRIX P = Tr(pos.x, pos.y, pos.z);
    DrawParams b; b.color = C4(1, 0.99f, 0.95f); b.emissive = { 0.25f, 0.24f, 0.2f }; b.outline = 0.018f;
    R->Draw(R->sphere, S(0.5f, 0.46f, 0.46f) * P, b);
    DrawParams band = b; band.color = C4(0.95f, 0.9f, 0.8f);
    R->Draw(R->cylinder, S(0.47f, 0.06f, 0.47f) * Tr(0, -0.1f, 0) * P, band);
    float flap = std::sin(time * 14) * 0.5f;
    DrawParams w; w.color = C4(1, 0.96f, 0.85f, 0.85f); w.emissive = { 0.2f, 0.18f, 0.12f }; w.cullNone = true; w.outline = 0.0f;
    for (int s = -1; s <= 1; s += 2)
        R->Draw(R->sphere, S(0.42f, 0.05f, 0.3f) * Tr(s * 0.2f, 0, 0) * XMMatrixRotationZ(s * (0.4f + flap)) * Tr(s * 0.18f, 0.08f, -0.05f) * P, w);
    DrawParams e; e.color = C4(0.1f, 0.1f, 0.15f);
    // 目（カメラ側を向く）
    XMFLOAT3 toCam = NormXZ(Sub(R->camPos, pos));
    float yy = YawOf(toCam);
    XMMATRIX face = XMMatrixRotationY(yy) * P;
    for (int s = -1; s <= 1; s += 2)
    {
        R->Draw(R->sphere, S(0.07f, 0.09f, 0.04f) * Tr(s * 0.1f, 0.03f, 0.21f) * face, e);
        DrawParams c; c.color = C4(1, 0.6f, 0.65f); c.unlit = true;
        R->Draw(R->sphere, S(0.07f, 0.04f, 0.02f) * Tr(s * 0.16f, -0.06f, 0.2f) * face, c);
    }
    DrawParams g; g.color = C4(0.9f, 0.7f, 0.3f); g.emissive = { 0.35f, 0.25f, 0.05f };
    for (int s = -1; s <= 1; s += 2)
        Seg(Add(pos, F3(s * 0.08f, 0.2f, 0)), Add(pos, F3(s * 0.25f, 0.5f, 0.02f)), 0.015f, g);
    // 金色の糸
    XMFLOAT3 prev = Add(pos, F3(0, -0.22f, 0));
    for (int i = 1; i <= 6; i++)
    {
        XMFLOAT3 q = Add(pos, F3(std::sin(time * 3 + i * 0.9f) * 0.12f * i * 0.3f, -0.22f - i * 0.14f, std::cos(time * 2 + i) * 0.05f));
        Seg(prev, q, 0.014f, g);
        prev = q;
    }
    R->DrawBillboard(T["glint"], pos, 1.6f, 1.6f, C4(1, 0.9f, 0.6f, 0.35f), Blend::Add, { 1, 1, 0, 0 }, time * 0.3f);
}

// ------------------------------------------------------------------
// 敵
// ------------------------------------------------------------------
void Game::DrawEnemy(const Enemy& e)
{
    if (DrawEnemy2(e)) return;   // 深い階の敵とうつろ
    if (e.t == EType::Guardian || e.t == EType::RootArm) { DrawBigEnemy(e); return; }
    float sc = 1.f;
    if (e.st == ESt::Dead)
    {
        if (e.deadT > 0.6f) return;
        sc = 1.f - e.deadT / 0.6f;
    }
    float squash = 1.f;
    XMFLOAT3 em = Mul(F3(1, 1, 1), e.flash * 0.9f);
    if (e.st == ESt::Windup)
    {
        float pulse = 0.5f + 0.5f * std::sin(e.stT * 30);
        em = Add(em, F3(0.6f * pulse, 0.08f * pulse, 0.05f * pulse));
        squash = 0.85f;
    }
    float bob = 0;
    if (e.t == EType::Spore) bob = 0.9f + std::sin(e.bob * 2.5f) * 0.2f;
    XMMATRIX P = S(sc, sc * squash, sc) * XMMatrixRotationY(e.yaw) * Tr(e.pos.x, e.pos.y + bob, e.pos.z);
    auto D = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, float ol = 0.02f, Texture* tx = nullptr, XMFLOAT3 extraEm = { 0, 0, 0 }) {
        DrawParams p; p.color = c; p.outline = ol; p.emissive = Add(em, extraEm); p.tex = tx; p.rim = 0.35f; R->Draw(m, L * P, p);
    };
    float walk = e.bob * 10;
    switch (e.t)
    {
    case EType::Rat:
    {
        DrawShadow(e.pos, 1.3f * sc);
        XMFLOAT4 fur = C4(0.78f, 0.78f, 0.88f);
        D(R->sphere, S(0.72f, 0.56f, 1.0f) * Tr(0, 0.4f, 0), fur);
        D(R->sphere, S(0.52f, 0.46f, 0.58f) * Tr(0, 0.55f, 0.48f), fur);
        D(R->sphere, S(0.14f, 0.1f, 0.12f) * Tr(0, 0.5f, 0.78f), C4(0.95f, 0.6f, 0.65f), 0.01f);
        for (int s = -1; s <= 1; s += 2)
        {
            D(R->sphere, S(0.26f, 0.26f, 0.06f) * Tr(s * 0.18f, 0.82f, 0.42f), C4(0.95f, 0.7f, 0.75f), 0.012f);
            D(R->sphere, S(0.08f, 0.08f, 0.05f) * Tr(s * 0.13f, 0.62f, 0.73f), C4(0.9f, 0.1f, 0.1f), 0, nullptr, F3(0.8f, 0.05f, 0.05f));
            for (int k = -1; k <= 1; k += 2)
            {
                float sw = std::sin(walk + k + s) * 0.4f;
                D(R->cylinder, S(0.12f, 0.22f, 0.12f) * Tr(0, -0.22f, 0) * XMMatrixRotationX(sw) * Tr(s * 0.22f, 0.25f, k * 0.28f), fur, 0.01f);
            }
        }
        DrawParams tp; tp.color = C4(0.9f, 0.7f, 0.75f); tp.emissive = em;
        XMVECTOR a = XMVector3Transform(XMVectorSet(0, 0.35f, -0.48f, 1), P), b = XMVector3Transform(XMVectorSet(0, 0.25f + std::sin(walk) * 0.1f, -1.2f, 1), P);
        XMFLOAT3 fa, fb; XMStoreFloat3(&fa, a); XMStoreFloat3(&fb, b);
        Seg(fa, fb, 0.035f, tp);
        R->DrawBillboard(T["blob"], Add(e.pos, F3(0, 0.5f, 0)), 2.0f * sc, 1.4f * sc, C4(0.85f, 0.85f, 1.f, 0.25f), Blend::Alpha);
        break;
    }
    case EType::Doll:
    {
        DrawShadow(e.pos, 1.4f * sc);
        XMFLOAT4 wood = C4(0.72f, 0.52f, 0.36f);
        D(R->cylinder, S(0.6f, 0.95f, 0.52f) * Tr(0, 0.25f, 0), wood, 0.02f, T["bark"]);
        D(R->sphere, S(0.6f, 0.55f, 0.55f) * Tr(0, 1.45f, 0), wood, 0.02f, T["bark"]);
        D(R->sphere, S(0.4f, 0.28f, 0.4f) * Tr(0.05f, 1.78f, -0.05f), C4(0.4f, 0.65f, 0.3f));
        for (int s = -1; s <= 1; s += 2)
        {
            D(R->sphere, S(0.1f, 0.12f, 0.05f) * Tr(s * 0.14f, 1.47f, 0.25f), C4(0.8f, 1, 0.3f), 0, nullptr, F3(0.6f, 0.8f, 0.15f));
            float sw = std::sin(walk + s) * 0.35f;
            D(R->cylinder, S(0.18f, 0.32f, 0.18f) * Tr(0, -0.32f, 0) * XMMatrixRotationX(sw) * Tr(s * 0.18f, 0.32f, 0), wood, 0.015f, T["bark"]);
            float armA = 0.6f;
            if (e.st == ESt::Windup) armA = 2.4f;
            else if (e.st == ESt::Attack) armA = Lerpf(2.4f, -0.6f, Saturate(e.stT / 0.2f));
            DrawParams ap; ap.color = wood; ap.tex = T["bark"]; ap.outline = 0.015f; ap.emissive = em;
            XMMATRIX arm = XMMatrixRotationX(-armA) * XMMatrixRotationZ(s * 0.5f) * Tr(s * 0.32f, 1.0f, 0);
            R->Draw(R->cylinder, S(0.12f, 0.75f, 0.12f) * Tr(0, -0.75f, 0) * arm * P, ap);
            R->Draw(R->cone, S(0.12f, 0.35f, 0.12f) * XMMatrixRotationX(PI) * Tr(0, -0.72f, 0) * arm * P, ap);
        }
        break;
    }
    case EType::Spore:
    {
        DrawShadow(e.pos, 1.2f * sc, 0.3f);
        D(R->sphere, S(0.85f, 0.8f, 0.85f) * Tr(0, 0.4f, 0), C4(0.6f, 0.42f, 0.75f), 0.02f, nullptr, F3(0.05f, 0.02f, 0.08f));
        for (int i = 0; i < 6; i++)
        {
            float a = i * 1.05f, h = 0.25f + (i % 3) * 0.15f;
            D(R->sphere, S(0.15f, 0.15f, 0.15f) * Tr(std::cos(a) * 0.38f, h, std::sin(a) * 0.38f), C4(0.55f, 0.95f, 0.4f), 0, nullptr, F3(0.25f, 0.5f, 0.15f));
        }
        for (int s = -1; s <= 1; s += 2)
            D(R->sphere, S(0.13f, 0.16f, 0.06f) * Tr(s * 0.15f, 0.48f, 0.38f), C4(1, 1, 0.9f), 0, nullptr, F3(0.4f, 0.4f, 0.3f));
        D(R->cone, S(0.25f, 0.3f, 0.25f) * Tr(0, 0.75f, 0), C4(0.5f, 0.35f, 0.65f), 0.015f);
        R->DrawBillboard(T["glint"], Add(e.pos, F3(0, bob + 0.4f, 0)), 1.6f * sc, 1.6f * sc, C4(0.6f, 1, 0.5f, 0.35f), Blend::Add, { 1, 1, 0, 0 }, e.bob);
        break;
    }
    case EType::Spider:
    {
        DrawShadow(e.pos, 5.5f * sc, 0.5f);
        float bs = 2.0f;
        XMMATRIX B = S(bs, bs, bs);
        XMFLOAT4 body = C4(0.25f, 0.22f, 0.36f), body2 = C4(0.34f, 0.28f, 0.45f);
        float rear = e.st == ESt::Windup ? 0.35f : (e.st == ESt::Attack && e.pattern == 0 ? -0.15f : 0.f);
        XMMATRIX tilt = XMMatrixRotationX(-rear);
        D(R->sphere, S(0.95f, 0.65f, 0.95f) * Tr(0, 0.65f, 0.2f) * tilt * B, body);
        D(R->sphere, S(1.35f, 1.05f, 1.5f) * Tr(0, 0.85f, -0.85f) * tilt * B, body2);
        D(R->sphere, S(0.9f, 0.08f, 0.9f) * Tr(0, 1.25f, -0.85f) * tilt * B, C4(0.85f, 0.8f, 0.75f), 0.01f);
        for (int i = 0; i < 4; i++)
        {
            float x = (i % 2 ? 0.14f : -0.14f) * (i < 2 ? 1.f : 1.6f);
            float y = i < 2 ? 0.78f : 0.68f;
            D(R->sphere, S(0.12f, 0.12f, 0.08f) * Tr(x, y, 0.62f) * tilt * B, C4(1, 0.2f, 0.15f), 0, nullptr, F3(0.9f, 0.1f, 0.08f));
        }
        // 牙
        for (int s = -1; s <= 1; s += 2)
            D(R->cone, S(0.08f, 0.25f, 0.08f) * XMMatrixRotationX(PI) * Tr(s * 0.12f, 0.5f, 0.65f) * tilt * B, C4(0.9f, 0.85f, 0.75f), 0.01f);
        // 8本の脚
        DrawParams lp; lp.color = body; lp.outline = 0.02f; lp.emissive = em;
        for (int s = -1; s <= 1; s += 2)
            for (int i = 0; i < 4; i++)
            {
                float ang = (i - 1.5f) * 0.5f;
                float ph = walk * 0.6f + i * 1.3f + (s > 0 ? PI : 0);
                float lift = std::max(0.f, std::sin(ph)) * 0.35f * (e.st == ESt::Chase ? 1.f : 0.3f);
                XMFLOAT3 hipL{ s * 0.45f, 0.7f, 0.25f - i * 0.18f };
                XMFLOAT3 dirL{ s * std::cos(ang), 0, std::sin(ang) };
                XMFLOAT3 kneeL = Add(hipL, Add(Mul(dirL, 0.75f), F3(0, 0.55f, 0)));
                XMFLOAT3 footL = Add(hipL, Add(Mul(dirL, 1.45f), F3(0, -0.7f + lift, 0)));
                if (i == 0 && e.st == ESt::Windup) footL = Add(kneeL, F3(s * 0.2f, 0.6f, 0.4f));
                auto toW = [&](XMFLOAT3 l) {
                    XMVECTOR v = XMVector3Transform(XMLoadFloat3(&l), B * P);
                    XMFLOAT3 o; XMStoreFloat3(&o, v); return o;
                };
                XMFLOAT3 h = toW(hipL), k = toW(kneeL), f = toW(footL);
                Seg(h, k, 0.13f * sc, lp);
                Seg(k, f, 0.09f * sc, lp);
            }
        // ほつれた糸
        DrawParams tp; tp.color = C4(0.95f, 0.95f, 1.f); tp.emissive = { 0.2f, 0.2f, 0.25f };
        for (int i = 0; i < 5; i++)
        {
            XMVECTOR a = XMVector3Transform(XMVectorSet(-0.6f + i * 0.3f, 1.2f, -1.4f, 1), B * P);
            XMFLOAT3 fa; XMStoreFloat3(&fa, a);
            XMFLOAT3 fb = Add(fa, F3(std::sin(time + i) * 0.4f, -1.2f - (i % 2) * 0.4f, -0.6f));
            Seg(fa, fb, 0.025f, tp);
        }
        break;
    }
    }
}
