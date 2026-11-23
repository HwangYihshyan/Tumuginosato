// 仲間（ソラ・クロベエ・カイリ）と武器・アタッチメント
#include "Game.h"

void Game::InitWeapons()
{
    weapons.clear();
    const int first[3] = { W_SORA0, W_KURO0, W_KAIRI0 };
    for (int i = 0; i < 3; i++)
    {
        party[i].def = i;
        party[i].maxHp = party[i].hp = Def(party[i]).maxHp;
        int idx = GiveWeapon(first[i]);
        party[i].weapon = idx;
        party[i].dur = party[i].maxDur = (float)kWeapons[first[i]].maxDur;
        party[i].broken = false; party[i].lvl = 1; party[i].exp = 0;
        RecalcWeapon(party[i]);
    }
}

int Game::GiveWeapon(int def)
{
    weapons.push_back({ def, (float)kWeapons[def].maxDur, false, 1, 0.f, { -1, -1 } });
    return (int)weapons.size() - 1;
}

void Game::SyncWeapon(const Actor& a)
{
    if (a.weapon < 0 || a.weapon >= (int)weapons.size()) return;
    OwnedWeapon& w = weapons[a.weapon];
    w.dur = a.dur; w.broken = a.broken; w.lvl = a.lvl; w.exp = a.exp;
}

void Game::EquipWeapon(Actor& a, int idx)
{
    if (idx < 0 || idx >= (int)weapons.size()) return;
    SyncWeapon(a);
    a.weapon = idx;
    const OwnedWeapon& w = weapons[idx];
    a.dur = w.dur; a.maxDur = (float)kWeapons[w.def].maxDur; a.broken = w.broken; a.lvl = w.lvl; a.exp = w.exp;
    RecalcWeapon(a);
}

void Game::RecalcWeapon(Actor& a)
{
    a.atkMul = 1; a.spdMul = 1; a.wearSave = 0;
    if (a.weapon < 0 || a.weapon >= (int)weapons.size()) return;
    for (int s = 0; s < 2; s++)
    {
        switch (weapons[a.weapon].att[s])
        {
        case AT_STONE: a.atkMul += 0.15f; break;
        case AT_FIRE: a.atkMul += 0.10f; break;
        case AT_FEATHER: a.spdMul += 0.12f; break;
        case AT_STEEL: a.wearSave += 0.4f; break;
        default: break;
        }
    }
}

// 武器が壊れたとき
//   最初の武器：消えずに「壊れた」状態で残る（予備の道具で戦う）。繕い糸で直せる
//   それ以外：武器は消える。はめていたアタッチメントは手元に戻り、最初の武器に持ち替える
void Game::BreakWeapon(Actor& a)
{
    a.dur = 0;
    threads++;
    SpawnFx(1, Add(a.pos, F3(0, 1, 0)), 3.f, C4(1, 0.8f, 0.5f), 0.6f);
    if (a.weapon < 0 || a.weapon >= (int)weapons.size()) { a.broken = true; return; }
    int idx = a.weapon;
    int def = weapons[idx].def;
    if (IsStarter(def))
    {
        a.broken = true;
        SyncWeapon(a);
        ToastMsg(std::wstring(kWeapons[def].name) + L"が壊れた！（繕い糸で直せる）", RGBA(1, 0.55f, 0.45f));
        return;
    }
    int back = 0;
    for (int s = 0; s < 2; s++)
        if (weapons[idx].att[s] >= 0) { attInv[weapons[idx].att[s]]++; back++; }
    weapons.erase(weapons.begin() + idx);
    for (auto& m : party)
        if (&m != &a && m.weapon > idx) m.weapon--;
    // 最初の武器に持ち替える
    a.weapon = -1;
    int starter = -1;
    for (int i = 0; i < (int)weapons.size(); i++)
        if (IsStarter(weapons[i].def) && kWeapons[weapons[i].def].owner == a.def) starter = i;
    if (starter < 0) starter = GiveWeapon(a.def == 0 ? W_SORA0 : a.def == 1 ? W_KURO0 : W_KAIRI0);
    EquipWeapon(a, starter);
    std::wstring msg = std::wstring(kWeapons[def].name) + L"が壊れて消えた…";
    if (back > 0) msg += L"（アタッチメントは手元に戻った）";
    ToastMsg(msg, RGBA(1, 0.55f, 0.45f));
    ToastMsg(std::wstring(kWeapons[weapons[a.weapon].def].name) + L" に持ち替えた", RGBA(1, 0.85f, 0.7f));
}

float Game::AttackMul(const Actor& a) const
{
    float m = (a.broken ? 0.45f : 1.f) * (1.f + 0.06f * (a.lvl - 1)) * a.atkMul;
    if (a.weapon >= 0 && a.weapon < (int)weapons.size() && !a.broken) m *= kWeapons[weapons[a.weapon].def].atk;
    return m;
}

int Game::NextMember(int from) const
{
    if (forceChar >= 0 && scene == SceneId::Dungeon) return -1;   // 操作キャラの決まった階は交代できない
    return NextMember2(from);
}
int Game::NextMember2(int from) const
{
    for (int k = 1; k <= 2; k++)
    {
        int i = (from + k) % 3;
        if (joined[i] && party[i].st != ASt::Down && party[i].hp > 0) return i;
    }
    return -1;
}

// 交代：仲間は画面に出ず、操作キャラと入れ替わる
void Game::SwapTo(int idx)
{
    if (idx == active || idx < 0 || !joined[idx]) return;
    if (forceChar >= 0 && scene == SceneId::Dungeon && idx != forceChar) { ToastMsg(std::wstring(L"この階は") + Def(party[forceChar]).name + L"しか戦えない", RGBA(1, 0.8f, 0.7f)); return; }
    Actor& o = party[active];
    Actor& n = party[idx];
    n.pos = o.pos; n.pos.y = 0; n.yaw = o.yaw; n.st = ASt::Normal; n.stT = 0; n.kb = {}; n.speedNow = 0; n.invuln = 0.5f;
    active = idx;
    trail.clear();
    SpawnFx(3, Add(n.pos, F3(0, 0.8f, 0)), 2.6f, C4(1.f, 1.f, 1.f, 0.85f), 0.5f);
    SpawnFx(2, Add(n.pos, F3(0, 1.0f, 0)), 3.f, C4(1.f, 0.9f, 0.6f), 0.4f);
    ToastMsg(std::wstring(Def(n).name) + L" に交代！", RGBA(1, 0.92f, 0.7f));
}

void Game::Recruit(int idx, XMFLOAT3 nearPos)
{
    if (joined[idx]) return;
    joined[idx] = true;
    party[idx].hp = party[idx].maxHp;
    party[idx].st = ASt::Normal;
    party[idx].pos = nearPos;
    ToastMsg(std::wstring(Def(party[idx]).name) + L" が仲間になった！", RGBA(1, 0.85f, 0.4f));
}

void Game::DrawActor(const Actor& a)
{
    if (a.def == 0) DrawSora(a, 1);
    else if (a.def == 1) DrawKurobee(a, 1);
    else DrawKairi(a, 1);
}
