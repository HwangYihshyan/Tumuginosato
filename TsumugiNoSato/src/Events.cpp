// 場面転換・階のイベント（1/2/5/7/10階）・7階のQTE・ジオラマ部品のイベント
#include "Game.h"

// ------------------------------------------------------------------
// 場面転換：暗転 → 題字 → 明転
// ------------------------------------------------------------------
static float kOut = 0.8f, kHold = 1.3f, kIn = 0.7f;

void Game::StartTransition(const std::wstring& title, const std::wstring& sub, std::function<void()> mid, bool quick)
{
    if (transOn) return;
    transQuick = quick;
    kOut = quick ? 0.35f : 0.8f; kHold = quick ? 0.35f : 1.3f; kIn = quick ? 0.35f : 0.7f;
    transOn = true; transT = 0; transFired = false;
    transTitle = title; transSub = sub; transMid = mid;
}

void Game::UpdateTransition(float dt)
{
    if (!transOn) return;
    transT += dt;
    if (!transFired && transT >= kOut)
    {
        transFired = true;
        if (transMid) transMid();
    }
    if (transT >= kOut + kHold + kIn) transOn = false;
}

void Game::DrawTransition()
{
    if (!transOn) return;
    float W = (float)R->W, H = (float)R->H;
    float a = transT < kOut ? transT / kOut : transT < kOut + kHold ? 1.f : 1.f - (transT - kOut - kHold) / kIn;
    a = Saturate(a);
    R->FillRect(0, 0, W, H, RGBA(0.02f, 0.02f, 0.04f, a));
    float ta = transT < kOut * 0.6f ? 0.f : transT < kOut + kHold ? Saturate((transT - kOut * 0.6f) / 0.4f) : a;
    if (ta > 0.01f)
    {
        float wob = std::sin(time * 2.f) * 2.f;
        R->FillRect(W / 2 - 260, H / 2 + 26, 520 * Saturate((transT - kOut * 0.6f) / 0.6f), 3, RGBA(0.85f, 0.55f, 0.2f, ta));
        R->Text(transTitle, 0, H / 2 - 90 + wob, W, 90, 64, RGBA(1, 0.97f, 0.9f, ta), 1, true, true, 1);
        R->Text(transSub, 0, H / 2 + 40, W, 40, 24, RGBA(1, 0.85f, 0.6f, ta), 1, false, true, 1);
    }
}

// ------------------------------------------------------------------
// 階のイベント
// ------------------------------------------------------------------
static const QteStep kQte[4] = {
    { 0, L"Space", L"うつろの爪をかわせ！", 1.5f, 1 },
    { 1, L"左クリック 連打", L"まとわりつく霧の糸を切り払え！", 2.6f, 8 },
    { 2, L"E", L"ツムギの糸でうつろを縫い留めろ！", 1.4f, 1 },
    { 3, L"右クリック", L"とどめの一撃！", 1.5f, 1 },
};

void Game::UpdateFloorEvents(float dt, const Controls& c)
{
    if (transOn || arriveT < 0.75f) return;
    eventT += dt;
    Actor& A = party[active];
    // 霧の巫女ナギ：出口へ続く道の少し先に現れる（カメラの向きではなく道の目の前）
    auto placeNagi = [&]() {
        XMFLOAT3 goal = Sub(exitPos, Mul(exitFace, 1.5f));
        XMFLOAT3 p = A.pos;
        float walked = 0;
        for (int i = 0; i < 40 && walked < 5.5f; i++)
        {
            XMFLOAT3 n = PathNext(p, goal);
            XMFLOAT3 d = Sub(n, p);
            float l = LenXZ(d);
            if (l < 0.05f) break;
            float st = std::min(l, 0.5f);
            p = Add(p, Mul(d, st / l));
            walked += st;
        }
        if (Solid(p.x, p.z) || walked < 1.5f) p = Add(A.pos, Mul(Fwd(A.yaw), 3.f));
        p.y = 0;
        nagiPos = p; nagiAlpha = 0;
        camYaw = YawOf(Sub(p, A.pos));
        A.yaw = camYaw;
    };
    auto nagiStep = [&]() {
        if (eventState == 1)
        {
            nagiAlpha = std::min(1.f, nagiAlpha + dt * 2.f);
            if (dialog.empty())
            {
                eventState = 2;
                SpawnFx(3, Add(nagiPos, F3(0, 0.8f, 0)), 3.5f, C4(0.9f, 0.9f, 1.f, 0.9f), 0.9f);
                SpawnFx(2, Add(nagiPos, F3(0, 1.2f, 0)), 3.f, C4(0.8f, 0.8f, 1.f), 0.7f);
                ToastMsg(L"霧の巫女は鈴の音とともに消えた…", RGBA(0.8f, 0.85f, 1));
            }
        }
        if (eventState == 2) nagiAlpha = std::max(0.f, nagiAlpha - dt * 1.5f);
    };
    switch (floorNo)
    {
    case 1:
        // チュートリアルと、ナギとの最初の出会い
        if (!Done(EV_F1) && eventState == 0)
        {
            eventsDone |= EV_F1; eventState = 1;
            placeNagi();
            Say(L"ツムギ", L"ここが根の洞…。霧に呑まれた村のものは、糸巻きの形になってここに散らばってるの。");
            Say(L"ツムギ", L"金色に光る「糸巻きの宝箱」をほどけば、ジオラマパーツが手に入るよ！ 鍵持ちを倒せば次の階へ進める。糸巻きは各階に10個あるよ！");
            Say(L"ナギ", L"……また、だれかが来た。");
            Say(L"ツムギ", L"今の…女の子？ 鈴の音がした…。Esc でメニュー、Q で敵をロックオンできるよ。");
        }
        nagiStep();
        break;
    case 2:
        if (!Done(EV_F2) && eventState == 0)
        {
            eventsDone |= EV_F2; eventState = 1;
            placeNagi();
            Say(L"ナギ", L"……あなたが、村を紡ぎなおしている子？");
            Say(L"ナギ", L"霧は、みんなが忘れたかったものまで呑み込んだの。全部を思い出すことが、しあわせとは限らない。");
            Say(L"ツムギ", L"待って！ あなたは誰なの！？");
        }
        nagiStep();
        break;
    case 3:
        if (!Done(EV_F3) && eventState == 0)
        {
            eventsDone |= EV_F3; eventState = 1;
            Say(L"ツムギ", L"この階は糸がとても細い…。ここを通れるのはソラだけみたい。");
            Say(L"ソラ", L"みんなは外で待ってて。ひとりで行ってくる。");
        }
        break;
    case 5:
        if (eventState == 0)
        {
            eventState = 1;
            if (!Done(EV_F5))
            {
                eventsDone |= EV_F5;
                Say(L"ツムギ", L"糸が…ほどけてる？ 気をつけて、なにか大きいのがいる！");
                Say(L"ソラ", L"……ほつれ蜘蛛。村の布を食い荒らした化け物だ。子分を呼ぶ前に、早く倒さないと！");
            }
        }
        break;
    case 7:
        if (!Done(EV_F7) && eventState == 0)
        {
            eventsDone |= EV_F7; eventState = 1;
            Say(L"クロベエ", L"ここは土が固く締まってやがる。掘って進めるのはオレだけだ。");
            Say(L"クロベエ", L"坊とツムギは後ろで待ってな。アナグマの本領、見せてやるよ！");
        }
        break;
    case 12:
        // マップのまん中に行くと、うつろが現れる → QTE → 倒したような演出 → 「カイリ」の糸巻き
        if (eventState == 0 && !Done(EV_F12) && DistXZ(A.pos, eventPos) < 4.5f)
        {
            eventState = 1; rootAnim = 0;
            XMFLOAT3 up = Add(eventPos, Mul(NormXZ(Sub(eventPos, A.pos)), 2.5f));
            SpawnEnemy(EType::Utsuro, up, true);
            Enemy& u = enemies.back();
            u.yaw = YawOf(Sub(A.pos, up)); u.st = ESt::Idle; u.hp = u.maxHp = 400;
            camYaw = YawOf(Sub(up, A.pos)); A.yaw = camYaw;
            SpawnFx(3, Add(up, F3(0, 1.f, 0)), 6.f, C4(0.6f, 0.6f, 0.7f, 0.9f), 1.0f);
            Say(L"？？？", L"……カエセ……ワスレタ……モノヲ……");
            Say(L"ツムギ", L"霧の塊…「うつろ」！ 村の忘れられた記憶が形になってる！");
            Say(L"ツムギ", L"合図に合わせて動いて！ わたしの糸で縫い留めるから、そのすきに！");
        }
        if (eventState == 1)
        {
            rootAnim = std::min(1.f, rootAnim + dt);
            if (dialog.empty()) { eventState = 2; qteOn = true; qteStep = 0; qteT = 0; qteMash = 0; qteFailT = 0; }
        }
        if (qteOn) UpdateQTE(dt, c);
        break;
    case 14:
        if (!Done(EV_F14) && eventState == 0)
        {
            eventsDone |= EV_F14; eventState = 1;
            Say(L"カイリ", L"うわっ、ここの霧は刃を錆びさせる…！ 武器がすぐにだめになっちまう。");
            Say(L"ツムギ", L"リミテッド層だ…武器の消耗が3倍だよ。研ぎ粉と繕い糸を大事に使って！");
            Say(L"カイリ", L"水の上を渡れるのはあたしだけ。ここはまかせな！");
        }
        break;
    case 15:
    {
        Enemy* u = nullptr;
        for (auto& e : enemies) if (e.t == EType::Utsuro) u = &e;
        if (eventState == 0)
        {
            eventState = 1;
            if (!Done(EV_F15))
            {
                Say(L"ツムギ", L"ここが…いちばん奥。あれが、うつろの本体！");
                Say(L"ナギ", L"四本の糸の柱が、うつろを守っている。柱がある限り、刃はほとんど届かない。");
                Say(L"ツムギ", L"うつろの突進を柱にぶつけさせて！ 柱が壊れて、うつろが目を回すよ！");
            }
        }
        if (eventState == 1 && u && u->st == ESt::Dead)
        {
            eventState = 2; eventT = 0;
            eventsDone |= EV_F15;
            chapterClear = true;
            Say(L"ツムギ", L"うつろが…ほどけていく。忘れられた記憶が、村へ帰っていくよ。");
            Say(L"ナギ", L"……ありがとう。わたしも、思い出せた気がする。");
            Say(L"ソラ", L"戻ろう、ツムギ。一日遅れだけど、機祭りをやるんだ。");
        }
        if (eventState == 2 && dialog.empty() && eventT > 0.5f)
        {
            eventState = 3;
            StartTransition(L"第1章　完", L"― 一日遅れの機祭り ―", [this] {
                EnterVillage(true);
                ToastMsg(L"第1章 クリア！ 村で機祭りが開かれた", RGBA(1, 0.85f, 0.4f));
            });
        }
        break;
    }
    default: break;
    }
}

void Game::UpdateQTE(float dt, const Controls& c)
{
    const QteStep& s = kQte[qteStep];
    Enemy* root = nullptr;
    for (auto& e : enemies) if ((e.t == EType::RootArm || e.t == EType::Utsuro) && e.st != ESt::Dead) root = &e;
    if (qteFailT > 0)
    {
        qteFailT -= dt;
        if (qteFailT <= 0) { qteT = 0; qteMash = 0; }
        return;
    }
    qteT += dt;
    bool pressed = false;
    if (autoQTE)
    {
        float interval = s.mash > 1 ? 0.07f : 0.4f;
        if (qteT > 0.35f && std::fmod(qteT, interval) < dt) pressed = true;
    }
    else
    {
        if (s.key == 0) pressed = c.dodge;
        else if (s.key == 1) pressed = c.atkPressed;
        else if (s.key == 2) pressed = c.interact;
        else pressed = c.guardPressed;
    }
    if (pressed)
    {
        qteMash++;
        if (root) SpawnFx(0, Add(root->pos, F3(RandF(-1, 1), RandF(1.5f, 4.f), -1.5f)), 1.8f, C4(1, 0.85f, 0.6f), 0.3f);
    }
    if (qteMash >= s.mash)
    {
        // 成功
        if (root)
        {
            root->hp -= root->maxHp / 4.f;
            root->flash = 1;
            SpawnFx(1, Add(root->pos, F3(0, 3.f, -1.f)), 6.f, C4(1, 0.8f, 0.5f), 0.5f);
            AddNum(Add(root->pos, F3(0, 5.f, 0)), (int)(root->maxHp / 4), RGBA(1, 0.7f, 0.25f), 40);
        }
        hitstop = 0.08f; shake = 0.35f;
        ToastMsg(L"成功！", RGBA(0.7f, 1, 0.7f));
        qteStep++; qteT = 0; qteMash = 0;
        if (qteStep >= 4)
        {
            qteOn = false;
            if (root) { root->keyHolder = false; KillEnemy(*root, &party[active]); }
            eventsDone |= EV_F12;
            hasKey = true;                       // QTE の階は、クリアすると出口が開く
            // うつろを倒したような演出：霧がはじけて消え、まん中に「カイリ」の糸巻きが残る
            SpawnFx(1, Add(eventPos, F3(0, 2.f, 0)), 9.f, C4(0.85f, 0.85f, 1.f), 0.9f);
            SpawnFx(3, Add(eventPos, F3(0, 1.f, 0)), 8.f, C4(0.6f, 0.6f, 0.7f, 0.9f), 1.2f);
            shake = 0.5f; hitstop = 0.15f;
            SpawnLootSpools(12);
            ToastMsg(L"うつろの影を切り払った！ 霧の中から糸巻きが現れた", RGBA(1, 0.85f, 0.4f));
            Say(L"ツムギ", L"消えた…でも、まだ奥に本体がいる気がする。");
            Say(L"ツムギ", L"見て、糸巻き！ 中から…カワウソの声がする！ 持ち帰って水車にはめてみよう！");
        }
        return;
    }
    if (qteT > s.window)
    {
        // 失敗：根に打たれて、同じ合図からやり直し
        Actor& A = party[active];
        A.invuln = 0;
        DamageActor(A, 12, root ? root->pos : A.pos, 6);
        ToastMsg(L"失敗…もう一度！", RGBA(1, 0.55f, 0.5f));
        qteFailT = 1.0f;
    }
}

void Game::DrawQTEUI()
{
    float W = (float)R->W, H = (float)R->H;
    if (qteStep >= 4) return;
    const QteStep& s = kQte[qteStep];
    float cx = W / 2, cy = H - 220;
    float left = Saturate(1 - qteT / s.window);
    bool fail = qteFailT > 0;
    R->FillRound(cx - 300, cy - 120, 600, 46, 23, RGBA(0.05f, 0.07f, 0.13f, 0.8f));
    R->Text(fail ? L"失敗…霧の爪に打たれた！" : s.text, cx - 300, cy - 112, 600, 34, 22, fail ? RGBA(1, 0.5f, 0.45f) : RGBA(1, 0.92f, 0.7f), 1, true);
    R->FillCircle(cx, cy, 58, RGBA(0.06f, 0.09f, 0.17f, 0.9f));
    R->StrokeCircle(cx, cy, 58, RGBA(1, 1, 1, 0.3f), 3);
    // 残り時間の輪
    float rr = 58 + 40 * left;
    R->StrokeCircle(cx, cy, rr, fail ? RGBA(1, 0.3f, 0.25f, 0.8f) : RGBA(1, 0.75f, 0.3f, 0.9f), 5);
    R->Text(s.label, cx - 120, cy - 18, 240, 40, s.mash > 1 ? 18.f : 28.f, RGBA(1, 1, 1), 1, true);
    if (s.mash > 1)
    {
        Bar(cx - 120, cy + 72, 240, 14, (float)qteMash / s.mash, RGBA(1, 0.7f, 0.3f), RGBA(0.15f, 0.15f, 0.2f, 0.9f));
    }
    for (int i = 0; i < 4; i++)
        R->FillCircle(cx - 45 + i * 30, cy + 100, 8, i < qteStep ? RGBA(0.5f, 0.9f, 0.5f) : i == qteStep ? RGBA(1, 0.8f, 0.4f) : RGBA(1, 1, 1, 0.25f));
}

// ------------------------------------------------------------------
// ジオラマ部品を置いたときのイベント（仲間が加わるのもここ）
// ------------------------------------------------------------------
void Game::OnPartPlaced(PartType t, XMFLOAT3 where)
{
    // 置いたときの短いイベント。建物のイベントと仲間の加入は「完成してそばに行ったとき」（OnPartCompleted）
    switch (t)
    {
    case P_Lantern:
        if (Done(EV_LANTERN)) break;
        eventsDone |= EV_LANTERN;
        Say(L"ツムギ", L"最初の灯がともった…。霧の中でも、村の場所がわかるね。");
        break;
    case P_Bridge:
        Say(L"ツムギ", L"橋がかかった！ 川の上を渡れるよ。");
        break;
    default:
        if (SlotCount(t) > 0 && !Done(EV_UNLOCK_HINT))
        {
            eventsDone |= EV_UNLOCK_HINT;
            Say(L"ツムギ", L"建物が戻ったね！ はめ込み枠をうめると完成して、機能やイベントが解放されるよ。");
            Say(L"ツムギ", L"はめ込みは、住人と話すと手伝ってもらえるようになる。そのあと建物の前で F を押して、自分ではめ込もう。");
        }
        break;
    }
    SpawnFx(2, Add(where, F3(0, 2.f, 0)), 5.f, C4(1, 0.9f, 0.6f), 0.8f);
}
