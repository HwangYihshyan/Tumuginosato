// 住人との会話：話題を選ぶ
//   物語のこと（ストーリーのヒント）／ジオラマのこと（はめ込みの解放と、ジオラマパーツを教えてもらえる）／
//   村のこと（思い出話で復興率が上がる）／ちょっとしたコツ（ゲームの TIPS）
#include "Game.h"

namespace
{
    const wchar_t* kTopics[5] = { L"物語のこと", L"ジオラマのこと", L"村のこと", L"ちょっとしたコツ", L"さようなら" };
    const wchar_t* kTopicDesc[5] = {
        L"霧や根の洞の話を聞く（ストーリーのヒント）",
        L"はめ込みを手伝ってもらう・ジオラマパーツを教えてもらう",
        L"村の思い出話を聞く（はじめて聞くと復興率が上がる）",
        L"ゲームのコツを教えてもらう",
        L"話を終える",
    };
    const wchar_t* kTips[12] = {
        L"Q で敵をロックオンできる。R かホイールで次の敵に切り替えられるよ。",
        L"Shift でダッシュ。Space の回避は無敵時間があるから、敵の攻撃に合わせよう。",
        L"右上の地図：黄緑はジオラマの糸巻き、黄色はふつうの宝箱、オレンジは武器の宝箱、赤は敵、青は泉と出口だ。",
        L"敵の弱点の属性で攻めると1.5倍効く。火の鋏は根っこ人形や蜘蛛に強いぞ。",
        L"根の鎧武者は正面からの攻撃を受け流す。後ろや横に回りこむんだ。",
        L"最初の武器は壊れても消えない。繕い糸か、鍛冶屋で直せるよ。",
        L"14階のリミテッド層は武器がすぐ傷む。研ぎ粉をたくさん持っていこう。",
        L"はめ込みパーツは配置メニューで持ち上げて、左のカードの同じ形の枠へ運ぶんだ。",
        L"人物のジオラマは、はめ込みの解放を待たずにはめられる。",
        L"井戸は川から2マス以内、水車と橋は川の上にしか置けない。",
        L"建物が完成したら、町でそばへ行ってごらん。何かが起きるよ。",
        L"Esc のメニューの「操作方法」で、場面ごとの操作を見られるよ。",
    };
    // 話し手ごとの、物語の前置き
    std::wstring StoryOpen(int id)
    {
        switch (id)
        {
        case S_Chief: return L"わしが若いころにも、一度だけ霧が村を呑みかけたことがある。";
        case S_Merchant: return L"商いで遠くまで行くとね、いろんな噂が入ってくるのさ。";
        case S_Weaver: return L"機を織っていると、ときどき糸が昔のことを話してくれるの。";
        case S_Smith: return L"炉の火を見ていると、洞の奥から唸り声が聞こえる気がする。";
        case S_Rabbits: return L"畑の土の中にね、ときどき白い糸が埋まってるの。";
        case 20: return L"オレは洞の土を掘ってきた。奥へ行くほど、土が冷てえ。";
        case 21: return L"海の向こうのミナモでも、霧の話はよく聞いたよ。";
        default: return L"このあいだ、霧の中で不思議なものを見たんだ。";
        }
    }
    // 会話で教えてもらえるジオラマパーツ（話し手ごとにちがう）
    int GiftOf(int id)
    {
        switch (id)
        {
        case S_Chief: return P_Jizo;
        case S_Merchant: return P_Bench;
        case S_Weaver: return P_Flower;
        case S_Smith: return P_StoneLantern;
        case S_Rabbits: return P_Scarecrow;
        case S_Villager: return P_Flower;
        case S_Child: return P_Scarecrow;
        case 20: return P_StoneLantern;
        case 21: return P_Bench;
        default: return -1;
        }
    }
}

// 話し手の番号（人物のはめ込みパーツの番号。クロベエ=20、カイリ=21）
int Game::SpeakerId(const Walker& w) const
{
    if (w.person >= 0) return w.person;
    if (w.kind == 2) return 20;
    if (w.kind == 5) return 21;
    if (w.kind == 1) return S_Chief;
    if (w.kind == 4) return S_Merchant;
    return 22;
}

void Game::UpdateTalkMenu(const Controls& c)
{
    if (talkWalker < 0 || talkWalker >= (int)walkers.size()) { talkMenu = false; talkWalker = -1; return; }
    if (c.up) talkSel = (talkSel + 4) % 5;
    if (c.down) talkSel = (talkSel + 1) % 5;
    if (c.menu || c.back) { talkMenu = false; talkWalker = -1; return; }
    if (c.confirm || c.interact)
    {
        talkMenu = false;
        if (talkSel != 4) TalkTopic(talkSel);
        talkWalker = -1;
    }
}

void Game::TalkTopic(int topic)
{
    Walker& w = walkers[talkWalker];
    std::wstring name = WalkerName(w);
    int id = SpeakerId(w);
    switch (topic)
    {
    case 0:
    {
        // 物語のヒント：どこまで潜ったかで変わる
        Say(name, StoryOpen(id));
        if (deepest < 2) Say(name, L"根の洞の霧の奥で、鈴の音を聞いた者がおる。白い着物の娘の姿を見たとも…。");
        else if (deepest < 5) Say(name, L"5階には、村の布を食い荒らした大きな蜘蛛が巣くっておるらしい。子分を呼ぶから気をつけるんじゃ。");
        else if (deepest < 12) Say(name, L"深いところでは、霧が人の形になる「うつろ」を見たという話がある。12階あたりじゃ。");
        else if (!Done(EV_F15)) Say(name, L"いちばん奥の15階に、うつろの本体がいる。四本の柱に守られておるそうだ。");
        else Say(name, L"うつろは鎮まった。じゃが、霧の向こうの海にも、まだ呑まれた村があるかもしれん…。");
        break;
    }
    case 1:
    {
        // ジオラマのこと：はめ込みの解放と、「その人がはめ込まれている建物」のはめ込みパーツを教えてくれる
        //   （まだ糸巻きから出ていないぶんを先にもらえる。その糸巻きは洞から消えるので、数はふえない）
        bool unlocked = UnlockByTalk(name);
        int sub = -1, fl = -1, k = -1;
        if (id < 32 && !((talkGift >> id) & 1u) && FindGift(w, sub, fl, k))
        {
            talkGift |= 1u << id;
            floorGot[fl] |= (unsigned short)(1u << k);
            if (LootLeft(fl) == 0) floorLoot |= 1u << fl;
            subInv[sub]++;
            std::wstring home = kParts[placed[w.home].t].name;
            Say(name, home + L"に足りないのは「" + kSubs[sub].name + L"」じゃな。昔のことを話すうちに、ほれ、糸がほどけて形になったぞ。");
            Say(L"ツムギ", std::wstring(L"はめ込みパーツ「") + kSubs[sub].name + L"」をもらった！ 配置メニューで" + home + L"のカードの同じ形の枠へ運ぼう。");
            ToastMsg(std::wstring(L"はめ込みパーツをもらった：") + kSubs[sub].name + L"（" + home + L"の）", RGBA(0.6f, 0.95f, 1));
            SaveGame(false);
        }
        else if (!unlocked)
        {
            // もうもらった／足りないパーツがない：次に行くと良い階を教えてくれる
            int nf = -1;
            for (int f = 1; f <= MAX_FLOOR && nf < 0; f++) if (LootLeft(f) > 0 && f != 12) nf = f;
            if (nf > 0) Say(name, std::to_wstring(nf) + L"階の糸巻きには、まだ村のパーツが眠っておるはずじゃ（あと " + std::to_wstring(LootLeft(nf)) + L" 個）。");
            else Say(name, L"村のパーツはもう、ほとんど集まったようじゃな。あとは、はめ込みを仕上げるだけじゃ。");
        }
        break;
    }
    case 2:
    {
        // 村のこと：はじめて聞く思い出話で、復興率が少し上がる
        if (id < 32 && !((talkBoost >> id) & 1u))
        {
            talkBoost |= 1u << id;
            Say(name, L"霧の前は、毎年秋に機祭りをやっておってな。村じゅうに反物を干して、夜通し灯をともしたんじゃ。");
            Say(L"ツムギ", L"思い出話を聞いたら、村の糸が少しよみがえった気がする！");
            ToastMsg(L"村の思い出話で復興率が上がった", RGBA(1, 0.85f, 0.4f));
            SaveGame(false);
        }
        else
        {
            // 2回目からは、かなっていない要望を教えてくれる
            int r = -1;
            for (int k = 0; k < kReqCount && r < 0; k++)
            {
                bool present = false;
                for (auto& p : placed) if (p.t == kReqs[k].owner) present = true;
                if (present && !ReqSatisfied(k)) r = k;
            }
            if (r >= 0) Say(name, std::wstring(kReqs[r].who) + L"は「" + kReqs[r].text + L"」がいいと言っておったぞ。");
            else Say(name, L"村はずいぶん元どおりになった。みんな、ソラに感謝しておるよ。");
        }
        break;
    }
    case 3:
        Say(name, kTips[(tipIdx++ + id) % 12]);
        break;
    default: break;
    }
}

void Game::DrawTalkMenu()
{
    if (!talkMenu || talkWalker < 0 || talkWalker >= (int)walkers.size()) return;
    float W = (float)R->W, H = (float)R->H;
    const Walker& w = walkers[talkWalker];
    int id = SpeakerId(w);
    float bw = 460, bh = 68 + 5 * 52.f + 20, bx = W - bw - 40, by = H - bh - 60;
    R->FillRound(bx, by, bw, bh, 14, RGBA(0.06f, 0.09f, 0.17f, 0.95f));
    R->StrokeRound(bx + 5, by + 5, bw - 10, bh - 10, 10, RGBA(0.85f, 0.6f, 0.25f), 2);
    R->Text(WalkerName(w) + L"　― 何を話す？", bx + 22, by + 16, bw - 44, 30, 20, RGBA(1, 0.92f, 0.75f), 0, true);
    for (int i = 0; i < 5; i++)
    {
        float y = by + 60 + i * 52.f;
        bool sel = talkSel == i;
        R->FillRound(bx + 18, y, bw - 36, 46, 10, sel ? RGBA(0.98f, 0.94f, 0.85f) : RGBA(1, 1, 1, 0.06f));
        R->Text(kTopics[i], bx + 36, y + 4, 200, 24, 17, sel ? RGBA(0.12f, 0.15f, 0.25f) : RGBA(1, 1, 1), 0, true, !sel);
        R->Text(kTopicDesc[i], bx + 36, y + 25, bw - 80, 20, 12, sel ? RGBA(0.3f, 0.35f, 0.45f) : RGBA(1, 1, 1, 0.65f), 0, false, !sel);
        // まだ聞いていない話題には印
        int gs, gf, gk;
        bool fresh = (i == 1 && !((talkGift >> id) & 1u) && FindGift(w, gs, gf, gk)) || (i == 2 && !((talkBoost >> id) & 1u));
        if (fresh) R->Text(L"NEW", bx + bw - 90, y + 12, 60, 22, 14, RGBA(1, 0.55f, 0.2f), 2, true);
        if (sel) R->Text(L"☞", bx - 14, y + 6, 30, 34, 24, RGBA(1, 1, 1), 1, true);
    }
}

// その人がはめ込まれている建物（住人の家・仕事場）で、まだはまっていない枠のパーツのうち、
// 手元になく、まだ糸巻きから出ていないもの（見つかれば、その糸巻きの階 fl と番号 k）
bool Game::FindGift(const Walker& w, int& sub, int& fl, int& k) const
{
    if (w.home < 0 || w.home >= (int)placed.size()) return false;
    const Placed& p = placed[w.home];
    for (int s = 0; s < SlotCount(p.t); s++)
    {
        int need = SlotNeed(p.t, s);
        if (p.sub[s] >= 0 || need < 0 || IsPerson(need) || subInv[need] > 0) continue;
        for (int f = 1; f <= MAX_FLOOR; f++)
            for (int i = 0; i < LOOT_N; i++)
            {
                int id = kFloorLoot[f].ids[i];
                if (id < 0) break;
                if (id == SUB_ID + need && !((floorGot[f] >> i) & 1)) { sub = need; fl = f; k = i; return true; }
            }
    }
    return false;
}
