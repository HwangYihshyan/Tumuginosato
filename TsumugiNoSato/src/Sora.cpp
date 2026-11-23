// ソラと武器「大糸切り鋏」（再設計版）
//
// 武器の設計（スケッチより）
//   ・2枚の刃が支点でつながった巨大な糸切りばさみ。全長はソラ（140cm）の2倍の 280cm
//   ・持ち手と支点の結びはオレンジ。刃は鋼
//   ・開いた状態は V 字、閉じると2枚が重なって「一本の刃」になる
// モーション：開いて構える → 挟む（刃が閉じる瞬間＝当たり判定）→ 一本の刃のまま振り抜く
//
// 武器のローカル座標（単位 m）
//   原点 = 右手で握る位置。+Z が刃先の方向、Y が刃の厚み方向
//   U字ばね  z = -0.46〜-0.34
//   持ち手    z = -0.34〜 0.22（オレンジの糸巻き）
//   支点      z =  0.22（オレンジの結び。ここを軸に刃が開く）
//   刃        z =  0.22〜 2.34（刃長 212cm）
#include "Game.h"

namespace
{
    const float kPivotZ = 0.22f;
    const float kBladeLen = 2.12f;
    const float kBladeW = 0.15f;      // 片側の刃の幅（閉じると 30cm の一本の刃）
    const float kTipZ = kPivotZ + kBladeLen;

    XMMATRIX S(float x, float y, float z) { return XMMatrixScaling(x, y, z); }
    XMMATRIX Tr(float x, float y, float z) { return XMMatrixTranslation(x, y, z); }

    // 葉の形をした片刃。sign=+1 で刃の背が +X 側、-1 で -X 側
    MeshBuilder MakeBlade(float sign, bool edgeOnly)
    {
        MeshBuilder mb;
        const int N = 30;
        const float ts = 0.05f, te = 0.008f;   // 背の厚み・刃先の厚み
        auto spineW = [](float s) {
            float f = s < 0.12f ? 0.55f + 0.45f * (s / 0.12f) : std::pow(std::max(0.f, 1.f - (s - 0.12f) / 0.88f), 0.7f);
            return kBladeW * f;
        };
        auto edgeX = [](float s) {
            float k = s > 0.8f ? (s - 0.8f) / 0.2f : 0.f;
            return -0.012f + k * k * 0.02f;
        };
        std::vector<XMFLOAT3> eT, eB, sT, sB;
        for (int i = 0; i <= N; i++)
        {
            float s = (float)i / N, z = s * kBladeLen;
            float xe = edgeX(s);
            float xs = std::max(spineW(s), xe + 0.001f);
            eT.push_back({ xe * sign, te * 0.5f, z }); eB.push_back({ xe * sign, -te * 0.5f, z });
            sT.push_back({ xs * sign, ts * 0.5f, z }); sB.push_back({ xs * sign, -ts * 0.5f, z });
        }
        auto quad = [&](XMFLOAT3 a, XMFLOAT3 b, XMFLOAT3 c, XMFLOAT3 d, XMFLOAT3 want) {
            XMVECTOR n = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&b) - XMLoadFloat3(&a), XMLoadFloat3(&d) - XMLoadFloat3(&a)));
            if (XMVectorGetX(XMVector3Dot(n, XMLoadFloat3(&want))) < 0) n = -n;
            XMFLOAT3 nn; XMStoreFloat3(&nn, n);
            if (XMVectorGetX(XMVector3LengthSq(n)) < 1e-6f) nn = want;
            mb.Quad(a, b, c, d, nn, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 });
        };
        if (edgeOnly)
        {
            // 刃先に沿った明るい帯（研いだ面）
            for (int i = 0; i < N; i++)
            {
                for (int side = -1; side <= 1; side += 2)
                {
                    auto P = [&](int j, float f) {
                        XMFLOAT3 e = side > 0 ? eT[j] : eB[j], s = side > 0 ? sT[j] : sB[j];
                        float w = std::fabs(s.x - e.x);
                        float t = w > 1e-4f ? std::min(1.f, 0.03f / w) * f : 0.f;
                        return XMFLOAT3{ Lerpf(e.x, s.x, t), Lerpf(e.y, s.y, t) + side * 0.0025f, e.z };
                    };
                    quad(P(i, 0), P(i + 1, 0), P(i + 1, 1), P(i, 1), { 0, (float)side, 0 });
                }
            }
            return mb;
        }
        for (int i = 0; i < N; i++)
        {
            quad(eT[i], eT[i + 1], sT[i + 1], sT[i], { 0, 1, 0 });            // 表
            quad(eB[i], eB[i + 1], sB[i + 1], sB[i], { 0, -1, 0 });           // 裏
            quad(sT[i], sT[i + 1], sB[i + 1], sB[i], { sign, 0, 0 });         // 刃の背
            quad(eT[i], eT[i + 1], eB[i + 1], eB[i], { -sign, 0, 0 });        // 刃先
        }
        quad(eT[0], sT[0], sB[0], eB[0], { 0, 0, -1 });                       // 根元
        return mb;
    }

    WeaponPose CarryPose()
    {
        WeaponPose p;
        p.yaw = PI - 0.35f; p.pitch = 0.7f; p.roll = 1.35f;
        p.grip = { 0.3f, 0.95f, 0.14f };
        return p;
    }

    WeaponPose MixPose(const WeaponPose& a, const WeaponPose& b, float t)
    {
        WeaponPose r;
        r.yaw = a.yaw + WrapAngle(b.yaw - a.yaw) * t;
        r.pitch = Lerpf(a.pitch, b.pitch, t);
        r.roll = a.roll + WrapAngle(b.roll - a.roll) * t;
        r.grip = Lerp3(a.grip, b.grip, t);
        r.open = Lerpf(a.open, b.open, t);
        r.twist = Lerpf(a.twist, b.twist, t);
        r.twoHand = t < 0.5f ? a.twoHand : b.twoHand;
        return r;
    }

    // 横に振るときの握り位置（振りに合わせて手も弧を描く）
    XMFLOAT3 SweepGrip(float yaw, float reach)
    {
        return { 0.05f + reach * std::sin(yaw * 0.55f), 0.85f, reach * std::cos(yaw * 0.55f) };
    }

    float EaseOut(float v) { return 1 - (1 - v) * (1 - v); }
}

void Game::BuildWeaponMeshes()
{
    bladeA = R->CreateMesh(MakeBlade(1.f, false));
    bladeB = R->CreateMesh(MakeBlade(-1.f, false));
    edgeA = R->CreateMesh(MakeBlade(1.f, true));
    edgeB = R->CreateMesh(MakeBlade(-1.f, true));
}

// ------------------------------------------------------------------
// 武器の姿勢（モーション）
// k = 攻撃の進み具合（0〜1）。H = 刃が閉じる瞬間＝当たり判定（Dungeon.cpp と同じ 35%）
// ------------------------------------------------------------------
WeaponPose Game::SoraPose(const Actor& a) const
{
    const CharDef& d = Def(a);
    WeaponPose carry = CarryPose();
    WeaponPose p = carry;
    const float H = 0.35f;
    if (a.st == ASt::Attack && (a.combo == 0 || a.combo == 1))
    {
        // 1段目：右から左へ／2段目：左から右へ。開いて構え→挟む→一本で振り抜く
        float dir = a.combo == 0 ? 1.f : -1.f;
        float k = Saturate(a.stT / d.dur[a.combo]);
        if (k < H)
        {
            float u = k / H;
            p.yaw = dir * Lerpf(1.9f, 0.1f, u * u);
            p.pitch = Lerpf(0.2f, -0.06f, u);
            p.open = 0.5f * SmoothStep(0.f, 0.45f, u) * (1 - SmoothStep(0.8f, 1.f, u));
        }
        else
        {
            float e = EaseOut(Saturate((k - H) / 0.4f));
            p.yaw = dir * Lerpf(0.1f, -1.8f, e);
            p.pitch = -0.08f;
            p.open = 0;
        }
        p.roll = a.combo == 0 ? 0.f : PI;
        p.grip = SweepGrip(p.yaw, 0.42f);
        p.twist = p.yaw * 0.35f;
        p.twoHand = false;
        if (k < 0.1f) p = MixPose(carry, p, SmoothStep(0.f, 0.1f, k));
        if (k > 0.78f) p = MixPose(p, carry, SmoothStep(0.78f, 1.f, k));
    }
    else if (a.st == ASt::Attack && a.combo == 2)
    {
        // 3段目：振りかぶって縦に大きく開き、振り下ろしながら挟んで叩きつける
        float k = Saturate(a.stT / d.dur[2]);
        if (k < H)
        {
            float u = k / H;
            if (u < 0.65f)
            {
                float e = EaseOut(u / 0.65f);
                p.pitch = Lerpf(0.4f, 2.05f, e);
                p.open = 0.65f * e;
                p.grip = Lerp3(F3(0.15f, 0.95f, 0.3f), F3(0.05f, 1.3f, 0.0f), e);
            }
            else
            {
                float r = (u - 0.65f) / 0.35f;
                p.pitch = Lerpf(2.05f, -0.45f, r * r);
                p.open = 0.65f * (1 - SmoothStep(0.55f, 1.f, r));
                p.grip = Lerp3(F3(0.05f, 1.3f, 0.0f), F3(0.0f, 0.72f, 0.5f), r);
            }
        }
        else
        {
            float v = Saturate((k - H) / 0.3f);
            p.pitch = -0.45f + std::sin(v * PI) * 0.08f;
            p.open = 0;
            p.grip = { 0.0f, 0.72f, 0.5f };
        }
        p.yaw = 0; p.roll = PI / 2; p.twist = 0; p.twoHand = true;
        if (k < 0.08f) p = MixPose(carry, p, SmoothStep(0.f, 0.08f, k));
        if (k > 0.75f) p = MixPose(p, carry, SmoothStep(0.75f, 1.f, k));
    }
    else if (a.st == ASt::Charging)
    {
        // 溜め：右後ろで最大まで開いて力をためる（刃が震える）
        p.yaw = 1.75f; p.pitch = 0.12f; p.roll = 0;
        p.open = 0.78f + std::sin(time * 30.f) * 0.03f;
        p.grip = { 0.4f, 0.85f, -0.05f };
        p.twist = 0.5f; p.twoHand = true;
        p = MixPose(carry, p, SmoothStep(0.f, 0.15f, a.stT));
    }
    else if (a.st == ASt::ChargeAtk)
    {
        // 回転攻撃（右クリックを離す）：刃を開いて真横に突き出したまま体ごと2回転
        // → 回転の終わりに正面へ振り込み、刃を閉じて挟む（0.8秒＝24f）
        float t = a.stT;
        if (t < 0.72f)
        {
            p.yaw = 1.5f;
            p.open = Lerpf(0.78f, 0.5f, Saturate(t / 0.1f)) + std::sin(t * 40.f) * 0.02f;
        }
        else if (t < 0.8f)
        {
            float u = (t - 0.72f) / 0.08f;
            p.yaw = Lerpf(1.5f, 0.1f, u * u);
            p.open = 0.5f * (1 - SmoothStep(0.6f, 1.f, u));
        }
        else
        {
            p.yaw = Lerpf(0.1f, -0.8f, EaseOut(Saturate((t - 0.8f) / 0.15f)));
            p.open = 0;
        }
        p.pitch = 0.06f; p.roll = 0;
        p.grip = SweepGrip(p.yaw, 0.5f);
        p.twist = 0; p.twoHand = true;
        if (t > 0.9f) p = MixPose(p, carry, SmoothStep(0.9f, 1.05f, t));
    }
    else if (a.st == ASt::Guard)
    {
        // ガード：閉じた刃を体の前に立てて盾にする
        WeaponPose g;
        g.yaw = 0; g.pitch = 1.45f; g.roll = 0; g.grip = { 0.02f, 0.5f, 0.5f }; g.twoHand = true;
        p = MixPose(carry, g, SmoothStep(0.f, 0.1f, a.stT));
    }
    return p;
}

XMMATRIX Game::SoraBody(const Actor& a) const
{
    float moving = Saturate(a.speedNow / 3.f);
    float t = a.walkT * 9.f;
    float bob = std::abs(std::sin(t)) * 0.07f * moving;
    XMMATRIX P = XMMatrixRotationY(a.yaw) * Tr(a.pos.x, a.pos.y + bob, a.pos.z);
    if (a.st == ASt::Dodge)
    {
        float k = a.stT / DodgeDur(0);
        float ry = YawOf(a.dodgeDir) - a.yaw;
        P = Tr(0, -0.55f, 0) * XMMatrixRotationX(k * 2 * PI) * Tr(0, 0.55f, 0) * XMMatrixRotationY(ry) * XMMatrixRotationY(a.yaw) * Tr(a.pos.x, a.pos.y, a.pos.z);
    }
    else if (a.st == ASt::Hit) P = XMMatrixRotationX(-0.35f) * P;
    else if (a.st == ASt::Down) P = Tr(0, 0.25f, 0) * XMMatrixRotationX(-PI / 2) * XMMatrixRotationY(a.yaw) * Tr(a.pos.x, 0, a.pos.z);
    else if (a.st == ASt::ChargeAtk)
    {
        // 体ごと2回転（720°）。回り終わると元の向きに戻る
        float spin = -4.f * PI * SmoothStep(0.05f, 0.78f, a.stT);
        P = XMMatrixRotationZ(0.08f) * XMMatrixRotationY(a.yaw + spin) * Tr(a.pos.x, a.pos.y, a.pos.z);
    }
    else if (a.st == ASt::Charging)
    {
        // 溜め：腰を落として踏ん張る
        P = Tr(0, -0.06f, 0) * XMMatrixRotationX(0.12f) * P;
    }
    return P;
}

XMMATRIX Game::SoraWeaponWorld(const Actor& a, WeaponPose* out) const
{
    WeaponPose p = SoraPose(a);
    if (out) *out = p;
    XMMATRIX Pt = XMMatrixRotationY(p.twist) * SoraBody(a);
    XMMATRIX orient = XMMatrixRotationZ(p.roll) * XMMatrixRotationX(-p.pitch) * XMMatrixRotationY(p.yaw);
    return orient * Tr(p.grip.x, p.grip.y, p.grip.z) * Pt;
}

// ------------------------------------------------------------------
// 大糸切り鋏の描画。world = 握る位置の行列
// ------------------------------------------------------------------
void Game::DrawScissors(XMMATRIX W, float open, XMFLOAT3 em, int element)
{
    XMFLOAT4 steel = C4(0.82f, 0.86f, 0.92f), steel2 = C4(0.66f, 0.72f, 0.82f), white = C4(1, 1, 1);
    // 火の鋏は緋色、水の鋏は青みの刃
    if (element == 1) { steel = C4(0.95f, 0.55f, 0.45f); steel2 = C4(0.8f, 0.35f, 0.3f); em = Add(em, F3(0.18f, 0.05f, 0.02f)); }
    if (element == 2) { steel = C4(0.55f, 0.8f, 0.98f); steel2 = C4(0.4f, 0.62f, 0.88f); em = Add(em, F3(0.02f, 0.08f, 0.16f)); }
    XMFLOAT4 orange = C4(0.96f, 0.56f, 0.16f), orangeDark = C4(0.78f, 0.38f, 0.1f), silver = C4(0.9f, 0.9f, 0.92f);
    auto D = [&](const Mesh& m, XMMATRIX L, XMFLOAT4 c, float ol, XMFLOAT3 e2 = { 0, 0, 0 }) {
        DrawParams p; p.color = c; p.outline = ol; p.emissive = Add(em, e2); p.rim = 0.7f;
        R->Draw(m, L * W, p);
    };
    // 刃（支点を軸に左右へ開く）
    XMMATRIX A = Tr(0, 0.024f, 0) * XMMatrixRotationY(open) * Tr(0, 0, kPivotZ);
    XMMATRIX B = Tr(0, -0.024f, 0) * XMMatrixRotationY(-open) * Tr(0, 0, kPivotZ);
    D(bladeA, A, steel, 0.012f);
    D(edgeA, A, white, 0, { 0.35f, 0.35f, 0.35f });
    D(bladeB, B, steel2, 0.012f);
    D(edgeB, B, white, 0, { 0.35f, 0.35f, 0.35f });
    // 持ち手（オレンジの糸巻き）
    D(R->cube, S(0.11f, 0.08f, 0.56f) * Tr(0, 0, -0.06f), orange, 0.012f);
    for (int i = 0; i < 6; i++) D(R->cube, S(0.122f, 0.092f, 0.022f) * Tr(0, 0, -0.29f + i * 0.1f), orangeDark, 0);
    // 支点の結び（オレンジ）と留め金
    D(R->cylinder, S(0.18f, 0.12f, 0.18f) * Tr(0, -0.06f, kPivotZ), orange, 0.012f);
    D(R->sphere, S(0.08f, 0.04f, 0.08f) * Tr(0, 0.065f, kPivotZ), silver, 0);
    D(R->sphere, S(0.08f, 0.04f, 0.08f) * Tr(0, -0.065f, kPivotZ), silver, 0);
    // U字ばね
    DrawParams sp; sp.color = silver; sp.outline = 0.008f; sp.emissive = em; sp.rim = 0.6f;
    XMFLOAT3 prev{};
    for (int i = 0; i <= 8; i++)
    {
        float ang = PI * i / 8.f;
        XMFLOAT3 lp{ 0.05f * std::cos(ang), 0, -0.36f - 0.06f * std::sin(ang) };
        XMFLOAT3 wp; XMStoreFloat3(&wp, XMVector3Transform(XMLoadFloat3(&lp), W));
        if (i > 0) Seg(prev, wp, 0.016f, sp);
        prev = wp;
    }
}

// ------------------------------------------------------------------
// ソラ
// ------------------------------------------------------------------
void Game::DrawSora(const Actor& a, float alpha)
{
    float moving = Saturate(a.speedNow / 3.f);
    float t = a.walkT * 9.f;
    XMMATRIX P = SoraBody(a);
    WeaponPose wp;
    XMMATRIX W = SoraWeaponWorld(a, &wp);
    XMMATRIX Pt = XMMatrixRotationY(wp.twist) * P;

    DrawShadow(a.pos, 1.2f);
    XMFLOAT3 em = Mul(F3(1, 0.5f, 0.5f), a.flash * 0.8f);
    if (a.invuln > 0 && a.st != ASt::Dodge && std::fmod(time, 0.12f) < 0.06f) em = Add(em, F3(0.3f, 0.3f, 0.3f));
    auto D = [&](const Mesh& m, XMMATRIX L, XMMATRIX parent, XMFLOAT4 c, float ol = 0.022f) {
        DrawParams p; p.color = c; p.outline = ol; p.emissive = em; p.rim = 0.35f; R->Draw(m, L * parent, p);
    };
    XMFLOAT4 indigo = C4(0.2f, 0.3f, 0.62f), navy = C4(0.14f, 0.17f, 0.3f), skin = C4(0.98f, 0.84f, 0.72f), ochre = C4(0.88f, 0.64f, 0.22f);
    auto toW = [](XMFLOAT3 l, XMMATRIX m) { XMFLOAT3 o; XMStoreFloat3(&o, XMVector3Transform(XMLoadFloat3(&l), m)); return o; };

    // 脚（腰から下はひねらない）
    for (int s = -1; s <= 1; s += 2)
    {
        float sw = std::sin(t) * 0.6f * moving * s;
        D(R->cylinder, S(0.17f, 0.44f, 0.17f) * Tr(0, -0.44f, 0) * XMMatrixRotationX(sw) * Tr(s * 0.11f, 0.47f, 0), P, navy);
        D(R->sphere, S(0.17f, 0.1f, 0.24f) * Tr(0, -0.44f, 0.04f) * XMMatrixRotationX(sw) * Tr(s * 0.11f, 0.47f, 0), P, C4(0.45f, 0.3f, 0.2f));
    }
    // 胴（上半身は振りに合わせてひねる）
    D(R->cylinder, S(0.52f, 0.52f, 0.44f) * Tr(0, 0.42f, 0), Pt, indigo);
    D(R->cylinder, S(0.54f, 0.11f, 0.46f) * Tr(0, 0.55f, 0), Pt, ochre, 0.01f);
    D(R->cube, S(0.2f, 0.2f, 0.02f) * XMMatrixRotationZ(PI / 4) * Tr(0, 0.86f, 0.22f), Pt, C4(0.95f, 0.95f, 0.92f), 0);
    D(R->sphere, S(0.14f, 0.14f, 0.14f) * Tr(0.24f, 0.52f, 0.1f), Pt, C4(0.95f, 0.85f, 0.4f), 0.01f);

    // 腕：肩から握る位置へ
    DrawParams arm; arm.color = indigo; arm.outline = 0.02f; arm.emissive = em; arm.rim = 0.35f;
    DrawParams hand; hand.color = skin; hand.outline = 0.01f; hand.emissive = em;
    XMFLOAT3 shR = toW(F3(0.29f, 0.9f, 0), Pt), shL = toW(F3(-0.29f, 0.9f, 0), Pt);
    bool out = WeaponsOut();
    if (!out)
    {
        // 町では武器をしまって、両腕を振って歩く
        for (int s = -1; s <= 1; s += 2)
        {
            float armSw = std::sin(t) * 0.5f * moving * -s;
            D(R->cylinder, S(0.14f, 0.38f, 0.14f) * Tr(0, -0.38f, 0) * XMMatrixRotationX(armSw) * XMMatrixRotationZ(s * 0.3f) * Tr(s * 0.29f, 0.9f, 0), Pt, indigo);
            D(R->sphere, S(0.13f, 0.13f, 0.13f) * Tr(0, -0.4f, 0) * XMMatrixRotationX(armSw) * XMMatrixRotationZ(s * 0.3f) * Tr(s * 0.29f, 0.9f, 0), Pt, skin, 0.01f);
        }
        // 背中に小さく畳んだ鋏（糸で縛って背負っている）
        DrawParams bk; bk.color = C4(0.96f, 0.56f, 0.16f); bk.outline = 0.012f;
        R->Draw(R->cube, S(0.1f, 0.6f, 0.06f) * XMMatrixRotationZ(0.5f) * Tr(0, 0.7f, -0.28f) * Pt, bk);
    }
    XMFLOAT3 hR = toW(F3(0, 0, 0), W);
    if (out)
    {
        Seg(shR, hR, 0.07f, arm);
        R->Draw(R->sphere, S(0.14f, 0.14f, 0.14f) * Tr(hR.x, hR.y, hR.z), hand);
    }
    if (!out) {}
    else if (wp.twoHand)
    {
        XMFLOAT3 hL = toW(F3(0, 0, -0.22f), W);
        Seg(shL, hL, 0.07f, arm);
        R->Draw(R->sphere, S(0.14f, 0.14f, 0.14f) * Tr(hL.x, hL.y, hL.z), hand);
    }
    else
    {
        float armSw = std::sin(t) * 0.5f * moving;
        D(R->cylinder, S(0.14f, 0.38f, 0.14f) * Tr(0, -0.38f, 0) * XMMatrixRotationX(-armSw) * XMMatrixRotationZ(-0.3f) * Tr(-0.29f, 0.9f, 0), Pt, indigo);
        D(R->sphere, S(0.13f, 0.13f, 0.13f) * Tr(0, -0.4f, 0) * XMMatrixRotationX(-armSw) * XMMatrixRotationZ(-0.3f) * Tr(-0.29f, 0.9f, 0), Pt, skin, 0.01f);
    }

    // 武器
    if (!out) {}
    else if (a.broken)
    {
        DrawParams nd; nd.color = C4(0.85f, 0.88f, 0.92f); nd.outline = 0.008f; nd.emissive = em;
        Seg(toW(F3(0, 0, -0.1f), W), toW(F3(0, 0, 0.9f), W), 0.025f, nd);
    }
    else
    {
        // 溜め中は刃が光る（最大まで溜まると速く明滅）
        XMFLOAT3 wem = em;
        if (a.st == ASt::Charging)
        {
            float lv = Saturate(a.stT / 0.8f);
            float pulse = 0.5f + 0.5f * std::sin(time * (lv >= 1.f ? 18.f : 8.f));
            wem = Add(wem, Mul(F3(0.5f, 0.28f, 0.06f), lv * pulse));
        }
        int el = (a.weapon >= 0 && a.weapon < (int)weapons.size()) ? kWeapons[weapons[a.weapon].def].element : 0;
        DrawScissors(W, wp.open, wem, el);
    }

    // 頭
    D(R->sphere, S(0.64f, 0.6f, 0.6f) * Tr(0, 1.22f, 0), Pt, skin);
    D(R->sphere, S(0.68f, 0.46f, 0.66f) * Tr(0, 1.36f, -0.04f), Pt, C4(0.12f, 0.1f, 0.12f));
    D(R->cylinder, S(0.665f, 0.09f, 0.625f) * Tr(0, 1.27f, -0.01f), Pt, C4(0.97f, 0.97f, 0.95f), 0.01f);
    D(R->cube, S(0.06f, 0.08f, 0.32f) * XMMatrixRotationX(0.5f) * XMMatrixRotationY(0.4f) * Tr(0.12f, 1.27f, -0.42f), Pt, C4(0.97f, 0.97f, 0.95f), 0.01f);
    D(R->cube, S(0.06f, 0.08f, 0.28f) * XMMatrixRotationX(0.7f) * XMMatrixRotationY(-0.3f) * Tr(0.02f, 1.24f, -0.42f), Pt, C4(0.97f, 0.97f, 0.95f), 0.01f);
    bool hurt = a.st == ASt::Hit || a.st == ASt::Down;
    bool fierce = a.st == ASt::Attack || a.st == ASt::Charging || a.st == ASt::ChargeAtk;
    for (int s = -1; s <= 1; s += 2)
    {
        if (hurt) D(R->cube, S(0.1f, 0.025f, 0.02f) * Tr(s * 0.12f, 1.2f, 0.29f), Pt, C4(0.1f, 0.08f, 0.1f), 0);
        else if (fierce) D(R->cube, S(0.1f, 0.04f, 0.02f) * XMMatrixRotationZ(s * -0.35f) * Tr(s * 0.12f, 1.21f, 0.29f), Pt, C4(0.1f, 0.08f, 0.1f), 0);
        else D(R->sphere, S(0.075f, 0.11f, 0.04f) * Tr(s * 0.12f, 1.2f, 0.28f), Pt, C4(0.1f, 0.08f, 0.1f), 0);
        D(R->sphere, S(0.09f, 0.05f, 0.03f) * Tr(s * 0.2f, 1.1f, 0.25f), Pt, C4(1, 0.6f, 0.62f, 1), 0);
    }

    // 刃の軌跡（攻撃中だけ記録し、0.14秒で消える）
    if (!sheetMode)
    {
        bool swinging = !a.broken && (a.st == ASt::Attack || a.st == ASt::ChargeAtk);
        if (swinging) trail.push_back({ toW(F3(0, 0, 0.7f), W), toW(F3(0, 0, kTipZ), W), time });
        trail.erase(std::remove_if(trail.begin(), trail.end(), [&](const TrailPt& p) { return time - p.t > 0.14f; }), trail.end());
        for (size_t i = 1; i < trail.size(); i++)
        {
            float age = time - trail[i].t;
            float al = 0.55f * Saturate(1 - age / 0.14f);
            R->DrawQuadWorld(trail[i - 1].a, trail[i - 1].b, trail[i].b, trail[i].a, C4(1.f, 0.82f, 0.55f, al), Blend::Add);
        }
    }
}
