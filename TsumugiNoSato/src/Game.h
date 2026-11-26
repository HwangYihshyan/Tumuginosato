#pragma once
#include "Renderer.h"

// ------------------------------------------------------------------
// 入力
// ------------------------------------------------------------------
struct InputState
{
    bool key[256]{}, prevKey[256]{};
    bool lmb = false, rmb = false, plmb = false, prmb = false;
    float mouseX = 0, mouseY = 0, mdx = 0, mdy = 0, wheel = 0;
    bool mouseMoved = false;
    bool Down(int k) const { return key[k]; }
    bool Pressed(int k) const { return key[k] && !prevKey[k]; }
    bool LPressed() const { return lmb && !plmb; }
    bool RPressed() const { return rmb && !prmb; }
    void EndFrame()
    {
        memcpy(prevKey, key, sizeof(key));
        plmb = lmb; prmb = rmb;
        mdx = mdy = wheel = 0;
        mouseMoved = false;
    }
};

struct Controls
{
    float mx = 0, mz = 0;          // カメラ基準の移動
    float camYaw = 0;              // カメラ回転量
    bool atkDown = false, atkPressed = false, atkReleased = false;
    bool guard = false, guardPressed = false, dodge = false, lockon = false, interact = false, swap = false;
    bool dash = false;             // Shift
    int item = -1;
    bool up = false, down = false, left = false, right = false, confirm = false, menu = false, back = false;
    bool prevChar = false, nextChar = false, useA = false, useB = false;   // 武器画面：Z/C キャラ切り替え、R/F 研ぎ粉・繕い糸
    bool lockNext = false, camReset = false; float wheel = 0;            // ロックオンの切り替え（R）・カメラを背後へ（V）・ホイール
};

enum class SceneId { Title, Village, Dungeon, Sheet, Interior };

// ソラの武器（大糸切り鋏）の姿勢。体の座標系で表す
struct WeaponPose
{
    float yaw = 0, pitch = 0, roll = 0;   // 刃の向き（左右・上下・ひねり）
    XMFLOAT3 grip{ 0, 0, 0 };             // 握る位置（右手）
    float open = 0;                       // 刃の開き（片側の角度・ラジアン）
    float twist = 0;                      // 上半身のひねり
    bool twoHand = false;                 // 両手持ち
};

// ------------------------------------------------------------------
// 復興パート（ハタオリ村）
// ------------------------------------------------------------------
enum PartType
{
    P_HouseTouji, P_HouseKurobee, P_HouseRabbit, P_HouseVillager,
    P_Loom, P_Forge, P_Kiln, P_Field, P_Tree, P_Lantern, P_Path, P_Cloth, P_Well, P_Wheel,
    P_Shop,                        // 商人の店
    P_River, P_Bridge,             // 川（1マス）と橋（川の上にだけかけられる）
    P_Jizo, P_Bench, P_Flower, P_StoneLantern, P_Scarecrow,   // 住人との会話で教えてもらえるジオラマ
    P_COUNT
};
struct PartDef { const wchar_t* name; int w, h; int pts; const wchar_t* desc; };
extern const PartDef kParts[P_COUNT];

enum ReqKind { RK_NearCenter, RK_NearPart, RK_AdjRiver, RK_TreesAround };
struct ReqDef { PartType owner; ReqKind kind; PartType other; int dist; int count; int pts; const wchar_t* who; const wchar_t* text; };
extern const ReqDef kReqs[];
extern const int kReqCount;

// はめ込みパーツ：主要なジオラマ（家・工房・店など）の「はめ込み枠」に取り付ける小さなジオラマ
//   人物（S_Chief 以降）もはめ込みパーツ。はめ込むと住人として町に現れる
enum SubType
{
    S_Noren, S_Lamp, S_Pot, S_Barrel, S_Wood, S_Sign,
    S_Chief, S_Merchant, S_Weaver, S_Smith, S_Rabbits, S_Villager, S_Child, S_Kairi,
    S_COUNT
};
inline bool IsPerson(int s) { return s >= S_Chief && s < S_COUNT; }
struct SubDef { const wchar_t* name; int pts; const wchar_t* desc; };
extern const SubDef kSubs[S_COUNT];
constexpr int SUB_ID = 100;      // パレットでは 100+番号 をはめ込みパーツとして扱う
constexpr int SIL_ID = 200;      // サムネイル：はめ込みパーツの「形」（灰色の影絵）
constexpr int MAX_SLOT = 3;
// 各階の糸巻きから出るジオラマパーツ（その階に入ると、10個の糸巻きが全部出る。1つにパーツ1つ）
constexpr int MAX_FLOOR = 15;
constexpr int LOOT_N = 10;
struct FloorLootDef { int ids[LOOT_N]; };      // 部品の番号。はめ込みパーツは SUB_ID+番号
extern const FloorLootDef kFloorLoot[MAX_FLOOR + 1];

// open：はめ込みが解放されたか（住人と話すと解放）　evDone：完成のイベントを見たか
struct Placed { PartType t; int x, z, rot; float appearT; int sub[MAX_SLOT] = { -1, -1, -1 }; bool open = false; bool evDone = false; };
// kind: 0=人間 1=フクロウ 2=アナグマ 3=ウサギ 4=タヌキ商人 5=カワウソ
struct Walker { int kind; XMFLOAT3 pos; XMFLOAT3 target; float yaw; float waitT; float phase; XMFLOAT4 color; int home = -1; int person = -1; float stuck = 0; };

constexpr int VG = 20;           // 20x20 マス
constexpr int RIVER_X = 14;      // 14・15 列は川。16〜19 列は「向こう岸」（橋をかけると渡れる）
inline bool IsRiverCol(int cx) { return cx >= RIVER_X && cx < RIVER_X + 2; }
const XMFLOAT3 kEntrancePos{ 0.f, 0.f, 25.f };   // 根の洞の入口（村の北）

// ------------------------------------------------------------------
// ダンジョン（根の洞）
// ------------------------------------------------------------------
constexpr int DN = 44;           // 44x44 マス（1マス=2m）。広いマップに部屋がたくさん
constexpr float CELL = 2.f;
constexpr float WALL_H = 3.2f;

struct Room { int x, z, w, h; };
struct Deco { int kind; XMFLOAT3 pos; float scale; float rot; float tilt; };

// 操作キャラ（0=ソラ 1=クロベエ 2=カイリ）
struct CharDef
{
    const wchar_t* name; const wchar_t* spare;
    float maxHp, speed;
    float reach, arc[3], dmg[3], dur[3];
    float chargeReach, chargeArc, chargeDmg;
    float hitFrac;                // 攻撃の当たり判定が出るタイミング（攻撃時間に対する割合）
    bool badger;
    bool ranged;
};

// クロベエの投げ（ひも通しを投げ、紐ごと扇形に薙ぎ払ってから引き戻す）
//   kFanArc：扇の角度（度）　kFanReach：届く距離　kFanDir：薙ぐ向き（+1=左→右 / -1=右→左）
constexpr float kFanArc[3] = { 120.f, 120.f, 160.f };
constexpr float kFanReach[3] = { 4.2f, 4.2f, 5.0f };
constexpr float kFanDir[3] = { 1.f, -1.f, 1.f };

// 武器
struct WeaponDef { const wchar_t* name; int owner; float atk; int maxDur; int element; const wchar_t* desc; };  // element 0=なし 1=火 2=水
enum { W_SORA0, W_SORA_FIRE, W_SORA_WATER, W_KURO0, W_KURO_FIRE, W_KAIRI0, W_KAIRI_WATER, W_COUNT };
extern const WeaponDef kWeapons[W_COUNT];
struct OwnedWeapon { int def; float dur; bool broken; int lvl; float exp; int att[2]; };

// アタッチメント（武器のスロットにはめる）
enum { AT_STONE, AT_FIRE, AT_FEATHER, AT_STEEL, AT_COUNT };
struct AttachDef { const wchar_t* name; const wchar_t* desc; };
extern const AttachDef kAttach[AT_COUNT];

enum class ASt { Normal, Attack, Charging, ChargeAtk, Dodge, Guard, Hit, Down };
struct Actor
{
    int def = 0;
    XMFLOAT3 pos{}, kb{};
    float yaw = 0;
    float hp = 100, maxHp = 100;
    ASt st = ASt::Normal;
    float stT = 0;
    int combo = 0;
    bool queued = false, hitDone = false;
    float invuln = 0, flash = 0, walkT = 0, speedNow = 0;
    float holdT = 0;
    float chargeLv = 0;      // 溜めの強さ（0〜1）
    XMFLOAT3 dodgeDir{};
    float glideT = 0;        // 回避のあと、無敵のあいだ（1秒）すべり続ける残り時間
    // 装備中の武器（weapons[weapon] の写し）
    int weapon = 0;
    float dur = 60, maxDur = 60;
    bool broken = false;
    int lvl = 1; float exp = 0;
    float atkMul = 1, spdMul = 1, wearSave = 0;   // アタッチメントの効果
    float aiCd = 0, dustT = 0;
    bool dashing = false;
};

// Moth 糸喰い蛾・Shade 影法師・Knight 根の鎧武者（深い階の敵）、Utsuro うつろ（12階のQTE・15階のボス）
enum class EType { Rat, Doll, Spore, Spider, Guardian, RootArm, Moth, Shade, Knight, Utsuro, COUNT };
inline bool IsBossType(EType t) { return t == EType::Spider || t == EType::Guardian || t == EType::RootArm || t == EType::Utsuro; }
enum class ESt { Idle, Notice, Chase, Windup, Attack, Recover, Hit, Dead };
const wchar_t* EnemyName(EType t);
std::wstring FloorSubtitle(int floor);
struct Enemy
{
    EType t = EType::Rat;
    XMFLOAT3 pos{}, kb{}, dashDir{}, wander{};
    float yaw = 0, hp = 10, maxHp = 10, radius = 0.45f;
    ESt st = ESt::Idle;
    float stT = 0, flash = 0, wanderT = 0, atkCd = 0, deadT = 0, bob = 0;
    bool token = false, keyHolder = false, hitDone = false;
    int pattern = 0;
    XMFLOAT3 target{};
};

struct Projectile
{
    XMFLOAT3 pos, vel; float life, dmg, radius; int kind;
    bool friendly = false, pierce = false; int owner = 0; uint64_t hitMask = 0;
};
enum class PickKind { Crystal, Key, Herb, Money };
struct Pickup { PickKind k; XMFLOAT3 pos; float t; bool taken; float vy; int value = 0; };
// kind: 0=通常の宝箱 1=武器の宝箱（大） 2=ジオラマパーツの糸巻き
struct Chest { XMFLOAT3 pos; float yaw; bool opened; float openT; int kind = 0; int loot = -1; bool key = false; };   // loot：糸巻きのパーツの番号　key：鍵入り（5階のボスの宝箱）
struct Spring { XMFLOAT3 pos; bool used; };
struct Marker { XMFLOAT3 pos; float t, delay, radius, dmg; };   // 地面の予告円（ボスの根の突き上げ）
struct Pillar { XMFLOAT3 pos; float hp, maxHp; float flash; bool alive; float grow = 1.f; };   // 15階：うつろを守る糸の柱（壊せる地形）。grow：紡ぎなおされるときの伸び
struct FxInst { XMFLOAT3 pos; float t, dur, size; int kind; XMFLOAT4 color; float rot; };
struct DmgNum { XMFLOAT3 pos; float t; int val; D2D1_COLOR_F col; float size; };
struct Toast { std::wstring text; float t; D2D1_COLOR_F col; };
struct DialogLine { std::wstring who, text; };

struct ShotStep { float wait; std::function<void()> act; std::wstring shot; };

// QTE（7階のイベント）
struct QteStep { int key; const wchar_t* label; const wchar_t* text; float window; int mash; };

// ------------------------------------------------------------------
class Game
{
public:
    bool Init(Renderer* r, const std::wstring& assetDir);
    void Update(float dt, InputState& in);
    void Render();

    bool autoshot = false;
    bool movie = false;         // --movie：紹介動画の台本を流す（autoshot と同じしくみで自動操作し、main.cpp が毎フレーム録画する）
    std::wstring shotDir;
    std::wstring pendingShot;   // 描画後に保存するファイル名
    bool quit = false;
    bool wantMouseLock = false;

private:
    Renderer* R = nullptr;
    std::wstring assets;
    std::map<std::string, Texture*> T;
    std::mt19937 rng{ 1234 };
    float RandF(float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); }
    int RandI(int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); }

    SceneId scene = SceneId::Title;
    float time = 0, sceneT = 0;
    float hitstop = 0, shake = 0;

    // ---- 共通UI
    std::vector<Toast> toasts;
    std::deque<DialogLine> dialog;
    void Say(const std::wstring& who, const std::wstring& text) { dialog.push_back({ who, text }); }
    void ToastMsg(const std::wstring& s, D2D1_COLOR_F c = RGBA(1, 1, 1)) { toasts.push_back({ s, 0, c }); }
    Controls ReadControls(InputState& in, float dt);

    // ---- 場面転換（フェード＋タイトル）
    bool transOn = false; float transT = 0; bool transFired = false; bool transQuick = false;
    std::function<void()> transMid;
    std::wstring transTitle, transSub;
    bool entering = false;            // 入口へ歩いて入っていく演出中
    void StartTransition(const std::wstring& title, const std::wstring& sub, std::function<void()> mid, bool quick = false);
    void UpdateTransition(float dt);
    void DrawTransition();

    // ---- 進行状況
    int inventory[P_COUNT]{};
    std::vector<PartType> storyQueue;
    static constexpr int ITEM_N = 5;
    int items[ITEM_N] = { 2, 1, 1, 1, 1 };   // 薬草 灯油 研ぎ粉 帰り糸 繕い糸
    int money = 100;                 // 文
    int threads = 0;
    int deepest = 0;
    bool knot = false;               // 結び目（5階クリア）
    int crystalsThisRun = 0;         // この探索で手に入れたジオラマパーツ数
    int totalDives = 0;
    unsigned eventsDone = 0;         // 一度きりのイベント
    bool chapterClear = false;
    bool joined[3] = { true, false, false };
    std::vector<OwnedWeapon> weapons;
    int attInv[AT_COUNT]{};
    int subInv[S_COUNT]{};           // 手持ちのはめ込みパーツ
    unsigned floorLoot = 0;          // ジオラマパーツを全部回収した階（ビット）
    unsigned short floorGot[MAX_FLOOR + 1]{};  // 階ごとに、どの糸巻き（0〜9番）を開けたか（ビット）
    enum EventBit
    {
        EV_OPENING = 1u << 0, EV_F1 = 1u << 1, EV_F2 = 1u << 2, EV_F5 = 1u << 3, EV_F7 = 1u << 4, EV_F10 = 1u << 5,
        EV_TOUJI = 1u << 8, EV_KUROBEE = 1u << 9, EV_KAIRI = 1u << 10, EV_LOOM = 1u << 11, EV_LANTERN = 1u << 12,
        EV_FIRSTRETURN = 1u << 13, EV_RECOVERED_ALL = 1u << 14, EV_TREE = 1u << 15, EV_UNLOCK_HINT = 1u << 16,
        EV_F3 = 1u << 17, EV_F12 = 1u << 18, EV_F14 = 1u << 19, EV_F15 = 1u << 20,
        EV_BOSS5 = 1u << 21,   // 5階のほつれ蜘蛛を倒した（そのあと5階は何もない層になる）
    };
    bool Done(unsigned e) const { return (eventsDone & e) != 0; }
    // ボスの階をクリアしたか（5階＝ほつれ蜘蛛。前のセーブでは「結び目」があれば倒している／15階＝うつろ）
    bool BossCleared(int floor) const { return floor == 5 ? (Done(EV_BOSS5) || knot) : floor == 15 ? Done(EV_F15) : false; }

    // ---- タイトル
    void UpdateTitle(float dt, InputState& in);
    void DrawTitleUI();

    // ---- 村
    std::vector<Placed> placed;
    std::vector<Walker> walkers;
    int cursorX = 6, cursorZ = 6, cursorRot = 0, palSel = 0;
    float vYaw = 0, vDist = 28, vPitch = 0.95f;
    XMFLOAT3 vFocus{ 0, 0, 0 };
    bool vWalkCam = false;            // true = 町歩き／false = 配置モード
    float rateShown = 0;
    void EnterVillage(bool fromDungeon);
    void UpdateVillage(float dt, InputState& in);
    void RenderVillage(bool title);
    void DrawVillageUI();
    void DrawPart(PartType t, XMFLOAT3 c, int rot, float appear, float seed);
    void DrawGround(bool village, float rate);
    void DrawEntrance();
    XMFLOAT3 CellCenter(int x, int z, int w = 1, int h = 1) const;
    bool CanPlace(PartType t, int x, int z, int rot, int ignore = -1) const;
    void PartSize(PartType t, int rot, int& w, int& h) const;
    bool ReqSatisfied(int r) const;
    float RestoreRate() const;
    void RebuildWalkers();
    std::vector<int> PaletteList() const;
    PartType NextCrystalPart();
    int Gap(int ax, int az, int aw, int ah, int bx, int bz, int bw, int bh) const;
    void EnsureStart();
    void OnPartPlaced(PartType t, XMFLOAT3 where);
    bool DoorOf(const Placed& p, XMFLOAT3& outside, int& pattern) const;
    std::wstring TownGoal() const;
    int SlotCount(PartType t) const;
    int SlotNeed(PartType t, int s) const;   // その枠にはめるパーツ
    XMFLOAT3 SlotPos(const Placed& p, int s) const;
    void DrawSub(int s, XMFLOAT3 pos, float yaw, float appear);
    int PartAtCursor() const;
    bool AnyParts() const;
    void WalkFromCursor();
    // はめ込み：住人と話して解放（人物は最初からはめられる）→ 配置メニューで右のパーツを、左のカードの同じ形の枠へ運んではめる
    int carrySub = -1;                     // 配置メニューで運んでいるはめ込みパーツ（-1 = なし）
    bool silhouette = false;               // 影絵で描く（はめ込み枠の形のサムネイル用）
    int partMask = 0;                      // 川・橋を描くときの「となりにつながる」印
    int NeighborMask(const Placed& p) const;   // 川・橋のとなりのつながり
    int geoRowShown = -1;                  // 配置メニュー：前のフレームで見せていた行（選んだときだけスクロール）
    int carryCard = 0, carrySlot = 0;      // キーボードで運ぶ先（置いた建物のカードの順番・枠）
    bool carryByMouse = false; float uiMouseX = 0, uiMouseY = 0;
    bool moving = false; Placed movingPart{ P_Tree, 0, 0, 0, 0.f }; int moveFromX = 0, moveFromZ = 0;   // 置いたジオラマを動かしている
    std::vector<Placed> stored;     // はめ込んだまま片付けた建物（はめたパーツは外れず、もう一度置くとそのまま戻る）
    int StoredIndex(PartType t) const { for (int i = 0; i < (int)stored.size(); i++) if (stored[i].t == t) return i; return -1; }
    void CancelMove();
    float flyT = -1; int flyPart = -1, flySlot = -1, flySub = -1; float flyX = 0, flyY = 0;   // はめ込む演出
    bool TryFit(int placedIdx, int slot, int sub);   // 枠にはめる（形が合い、解放済みなら）
    bool SlotScreen(int placedIdx, int slot, float& cx, float& cy) const;   // 左のカードの枠の画面位置
    bool UnlockByTalk(const std::wstring& who);
    // 会話の話題（物語・ジオラマ・村・コツ）
    bool talkMenu = false; int talkSel = 0, talkWalker = -1, tipIdx = 0;
    unsigned talkGift = 0, talkBoost = 0;   // 話し手ごとに「ジオラマを教えてもらった」「思い出話を聞いた」
    int SpeakerId(const Walker& w) const;
    bool FindGift(const Walker& w, int& sub, int& fl, int& k) const;
    void UpdateTalkMenu(const Controls& c);
    void TalkTopic(int topic);
    void DrawTalkMenu();
    bool IsComplete(const Placed& p) const;
    bool VillageComplete() const;
    void OnPartCompleted(int idx);
    void CheckTownEvents();
    bool RiverCell(int cx, int cz) const;  // 川のマス（もとの川・置いた川）
    bool BridgeCell(int cx, int cz) const;
    bool BridgeEndCell(int cx, int cz) const;   // 橋の端（通り道）
    void DrawVillageBorder();
    float BridgeHeight(float x, float z) const;   // 橋の上を歩くときの高さ（0=橋の外）
    XMFLOAT3 NearWalkable(XMFLOAT3 p) const;      // いちばん近い歩ける場所
    // 配置メニューのキーボード操作（0=メニュー 1=マップ）
    int geoFocus = 0, geoCol = 0, geoRow = 0;
    void GeoLists(const std::vector<int>& pal, std::vector<std::pair<int, int>>& left, std::vector<int>& right) const;
    // 村のメニュー（Esc）
    bool townMenu = false; int townSel = 0; bool townWeapon = false; int townPage = 0;
    int townAsk = 0, askSel = 0;   // 1=セーブのあと「続けますか」 2=終了の前に「セーブしますか」
    void UpdateTownMenu(const Controls& c);
    void DrawTownMenu();
    void DrawTownAsk();
    void DrawControlsPage(int page, float x, float y, float w, float h, bool onPaper = false);   // onPaper：和紙の上（墨の文字）
    // 根の洞の入口：入る階を選ぶメニュー
    bool floorMenu = false; int floorSel = 0;
    void UpdateFloorMenu(const Controls& c);
    void DrawFloorMenu();
    void StartDive(int floor);
    void TakeLoot(int floor, int k);  // その階の k 番目のパーツを手に入れる
    int LootLeft(int floor) const;    // その階でまだ開けていない糸巻きの数
    int LootTotal(int floor) const;

    // ---- 建物の中（ジオラマの部屋・3種類）
    int roomPattern = 0;              // 0=民家 1=工房 2=商店
    int roomPart = -1;                // どの建物から入ったか
    XMFLOAT3 roomReturn{};
    float roomReturnYaw = 0;
    std::vector<XMFLOAT4> roomSolids; // x0,z0,x1,z1
    Walker roomNpc{};
    bool shopOpen = false; int shopSel = 0;
    void EnterInterior(int partIdx);
    void LeaveInterior();
    void UpdateInterior(float dt, InputState& in);
    void RenderInterior();
    void DrawInteriorUI();
    void DrawShopUI();

    // ---- ダンジョン
    uint8_t cell[DN][DN]{};
    bool seen[DN][DN]{};
    std::vector<Room> rooms;
    Mesh floorMesh, wallMesh;
    std::vector<Deco> decos;
    int floorNo = 1;
    int exitX = 0, exitZ = 0;
    XMFLOAT3 exitPos{}, exitFace{};
    bool hasKey = false;
    bool exitMenu = false; int exitSel = 0;
    Actor party[3];
    int active = 0;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projs;
    std::vector<Pickup> pickups;
    std::vector<Chest> chests;
    std::vector<Spring> springs;
    std::vector<Marker> markers;
    std::vector<FxInst> fx;
    std::vector<DmgNum> nums;
    int tokens = 0;
    int lockIdx = -1;
    float lantern = 100;
    float camYaw = 0, camDist = 7.f, camPitch = 0.5f;
    XMFLOAT3 camPosNow{}, camTargetNow{};
    bool autopilot = false;
    float autoAtkT = 0;
    float downT = 0;
    float arriveT = 9;               // 階に降り立つ演出
    std::wstring interactHint;
    int objective = 0;               // 0=鍵持ち→出口 1=ジオラマパーツ 2=宝箱
    // メニュー
    bool menuOpen = false; int menuTab = 0, menuSel = 0;
    int menuChar = 0;                // 武器画面で見ているキャラ
    void RenderWeaponShowcase();
    void DrawWeaponScreen();
    void UseItemOn(int i, Actor& a);
    // 配置／組み立てメニュー（サムネイルは3Dの模型をアトラスに描いて貼る）
    bool thumbsReady = false;
    void BuildThumbs();
    void DrawThumb(int id, float x, float y, float w, float h, float op = 1.f);
    struct GeoRect { float x, y, w, h; int kind, ref; };   // kind 0=パレット(ref=番号) 1=置いた建物(ref=placed)
    std::vector<GeoRect> GeoLayout(const std::vector<int>& pal) const;
    int geoScroll = 0;
    bool PartHasEvent(PartType t) const;
    bool IsMainPart(int id) const;
    void DrawGeoMenu();
    // 階のイベント
    float eventT = 0; int eventState = 0;
    XMFLOAT3 nagiPos{}; float nagiAlpha = 0;
    // QTE
    bool qteOn = false; int qteStep = 0; float qteT = 0; int qteMash = 0; float qteFailT = 0; bool autoQTE = false;
    float rootAnim = 0;
    // 操作キャラの制限（3階＝ソラ、7階＝クロベエ、14階＝カイリ。-1＝なし）と、リミテッド層（14階：武器の消耗3倍）
    int forceChar = -1; float wearMul = 1.f;
    XMFLOAT3 eventPos{};                  // 12階：QTE が起きる場所（マップのまん中の部屋）
    int RequiredChar(int floor) const { return floor == 3 ? 0 : floor == 7 ? 1 : floor == 14 ? 2 : -1; }
    // 回避：動き（硬直）はもとの長さのまま、無敵の持続時間だけ1秒のばす
    //   動き：ソラ0.4秒 クロベエ0.6秒 カイリ0.45秒　無敵：それぞれ＋1秒（1.4／1.6／1.45秒）
    static float DodgeBase(int def) { return def == 1 ? 0.6f : def == 2 ? 0.45f : 0.4f; }
    static float DodgeDur(int def) { return DodgeBase(def); }
    static float DodgeInvuln(int def) { return DodgeBase(def) + 1.0f; }
    // 回避のあと1秒すべる速さ（はじめの速さ。1秒で0まで落ちる → 足される距離はこの半分）
    static float DodgeGlideV(int def) { return def == 1 ? 5.5f : def == 2 ? 6.5f : 6.0f; }
    std::wstring FloorBlock(int floor) const;   // 入れない理由（入れるなら空）
    // 15階：うつろの戦い（糸の柱を壊すと守りが消える。突進を柱にぶつけると気絶する）
    std::vector<Pillar> pillars;
    float summonT = 0;
    std::vector<std::pair<EType, XMFLOAT3>> pendingSpawns;   // ボスが呼び出す敵（敵の更新が終わってから出す）
    bool PillarSolid(float x, float z) const;
    void UpdateUtsuro(Enemy& e, float dt);
    float immuneMsgT = 0, shieldDownT = 0, shieldDownHp = 0;   // うつろ：効かないお知らせ・守りが消えている時間と、そのときの体力
    void BreakPillar(Pillar& p, bool byCharge);
    void DamagePillars(XMFLOAT3 from, float yaw, float reach, float arcDeg, float dmg);   // 主人公の攻撃が柱に当たる
    void DrawPillarBars();
    void DrawUtsuro(const Enemy& e);
    void DrawPillars();
    bool DrawEnemy2(const Enemy& e);   // 深い階の敵の描画（描いたら true）
    void SpawnLootSpools(int floor);      // その階の残りの糸巻きを出す

    void EnterDungeon(int floor, uint32_t seed);
    void GoToFloor(int floor);
    void GenerateFloor(int floor, uint32_t seed);
    void BuildDungeonMeshes();
    void UpdateDungeon(float dt, const Controls& c);
    void RenderDungeon();
    void DrawDungeonUI();
    void DrawMenuUI();
    void UpdateMenu(const Controls& c);
    void LeaveDungeon(bool dead);
    void UpdateFloorEvents(float dt, const Controls& c);
    void UpdateQTE(float dt, const Controls& c);
    void DrawQTEUI();
    bool ObjectiveTarget(XMFLOAT3& out) const;
    void OpenChest(Chest& ch);

    const CharDef& Def(const Actor& a) const;
    void UpdateActor(Actor& a, const Controls* c, float dt, bool controlled);
    void UpdateFollower(Actor& a, float dt, int slot);
    void ActorAttackHits(Actor& a, float reach, float arcDeg, float dmg, bool heavy, int wear);
    void AreaHit(Actor& a, XMFLOAT3 center, float radius, float dmg, bool pull, int wear);
    void DamageActor(Actor& a, float dmg, XMFLOAT3 from, float kb);
    void WeaponWear(Actor& a, int n);
    float AttackMul(const Actor& a) const;
    void UpdateEnemies(float dt);
    void HitEnemy(Enemy& e, float dmg, XMFLOAT3 from, float kb, bool heavy, Actor* by);
    void KillEnemy(Enemy& e, Actor* by);
    void SpawnEnemy(EType t, XMFLOAT3 pos, bool deep);
    void MoveCircle(XMFLOAT3& p, float r, float dx, float dz) const;
    bool PropSolid(float x, float z, float r) const;
    bool Solid(float x, float z) const;
    bool SolidCell(int cx, int cz) const { return cx < 0 || cz < 0 || cx >= DN || cz >= DN || cell[cx][cz] == 0; }
    int RoomAt(XMFLOAT3 p) const;
    XMFLOAT3 RandomFloorPos(const Room& r, float margin);
    XMFLOAT3 PathNext(XMFLOAT3 from, XMFLOAT3 to);
    Controls AutoControls(float dt);
    void SpawnFx(int kind, XMFLOAT3 pos, float size, XMFLOAT4 col, float dur = 0.55f);
    void AddNum(XMFLOAT3 pos, int v, D2D1_COLOR_F c, float size = 26);
    void AddNum2(XMFLOAT3 pos, int v, D2D1_COLOR_F c, float size);
    void UseItem(int i);

    // ---- 仲間・武器
    void SwapTo(int idx);
    int NextMember(int from) const;
    int NextMember2(int from) const;
    void Recruit(int idx, XMFLOAT3 nearPos);
    void InitWeapons();
    void EquipWeapon(Actor& a, int ownedIdx);
    void SyncWeapon(const Actor& a);
    void RecalcWeapon(Actor& a);
    int GiveWeapon(int def);
    bool IsStarter(int def) const { return def == W_SORA0 || def == W_KURO0 || def == W_KAIRI0; }
    void BreakWeapon(Actor& a);
    bool WeaponsOut() const { return scene == SceneId::Dungeon || scene == SceneId::Sheet; }

    // ---- キャラ描画
    void DrawSora(const Actor& a, float alpha);
    void DrawKurobee(const Actor& a, float alpha);
    void DrawKairi(const Actor& a, float alpha);
    void DrawActor(const Actor& a);
    void DrawNagi(XMFLOAT3 pos, float yaw, float alpha);
    void DrawTanuki(XMFLOAT3 pos, float yaw, float phase);
    void DrawTsumugi(XMFLOAT3 pos);
    void DrawEnemy(const Enemy& e);
    void DrawWalker(const Walker& w);
    void DrawChest(const Chest& ch);
    void DrawBigEnemy(const Enemy& e);
    void DrawFx();
    void TickFx(float dt);
    void DrawShadow(XMFLOAT3 pos, float size, float alpha = 0.45f);
    void Seg(XMFLOAT3 a, XMFLOAT3 b, float r, DrawParams p);
    XMFLOAT3 KurobeeNeedle(const Actor& a, XMFLOAT3* hand = nullptr) const;
    XMFLOAT3 tint{ 0, 0, 0 };
    std::vector<XMFLOAT4> forest;   // x,z = 位置, y = 大きさ, w = 種類
    Mesh tube;                      // 遠景用の筒（ふたなし）
    int autoCount = 0;

    // ---- 大糸切り鋏
    Mesh bladeA, bladeB, edgeA, edgeB;
    struct TrailPt { XMFLOAT3 a, b; float t; };
    std::vector<TrailPt> trail;
    bool sheetMode = false;
    int sheetPage = 0;
    void BuildWeaponMeshes();
    void DrawScissors(XMMATRIX world, float open, XMFLOAT3 em, int element = 0);
    WeaponPose SoraPose(const Actor& a) const;
    XMMATRIX SoraBody(const Actor& a) const;
    XMMATRIX SoraWeaponWorld(const Actor& a, WeaponPose* out = nullptr) const;
    void RenderSheet();
    void DrawSheetUI();

    // ---- 右クリック技・町歩き・セーブ
    float autoSpecial = 0;      // 自動操作：右クリックを押し続ける残り時間
    bool autoGuard = false;     // 自動操作：右クリックを押しっぱなし
    bool autoDash = false;
    bool autoWalkOn = false; XMFLOAT3 autoWalk{}; XMFLOAT3 testStart{};   // 撮影用：町で目的地へ歩かせる
    int titleSel = 0;
    // 設定（タイトル画面）
    bool settingsOpen = false; int setSel = 0;
    float setCamSens = 1.f; bool setShake = true, setDmgNums = true, setBigMap = false;
    std::wstring SettingsPath() const;
    void SaveSettings();
    void LoadSettings();
    void DrawSettings();
    std::wstring talkHint;
    void UpdateFollowCam(float dt);
    bool VillageSolid(float x, float z) const;
    void EnterTownWalk(bool atEntrance = false);
    void UpdateTown(float dt, InputState& in);
    void UpdateWalkersAndFollowers(float dt);
    std::wstring WalkerName(const Walker& w) const;
    std::wstring WalkerLine(const Walker& w, int idx) const;
    std::wstring SavePath() const;
    bool HasSave() const;
    bool SaveGame(bool showToast);
    bool LoadGame();
    void NewGame();
    void SetupCombat();

    // ---- 自動撮影
    std::vector<ShotStep> steps;
    InputState fakeIn;              // 撮影デモで台本が押すキー
    int moveTest[8]{};              // 撮影デモ：移動・片付けの確認
    std::vector<XMFLOAT3> testPos; float testHp = 0;   // 撮影デモ：会話中に敵が止まるかの確認
    float dodgeDist = 0, glideDist = 0;   // 撮影デモ：回避で進んだ距離
    size_t stepIdx = 0;
    float stepT = 0;
    void BuildShotScript();
    void UpdateDirector(float dt);
    // ---- 紹介動画（--movie）
    //   台本は steps（撮影デモと同じ）。字幕と、最初と最後の題字カードは DrawMovieOverlay で画面に重ねる
    void BuildMovieScript();
    void DrawMovieOverlay();
    int movieCard = 0; float movieCardT = 0;          // 題字カード：0=なし 1=始まり 2=終わり、出てからの時間
    std::wstring movieNo, movieTitle, movieSub;       // 字幕：番号・見出し・説明
    float movieCapT = 99, movieCapDur = 0;            // 字幕が出てからの時間・出しておく長さ
    std::function<void(float)> movieTick;             // 毎フレーム呼ぶ（カメラをゆっくり回すなど）
    void TeleportPartyNear(XMFLOAT3 p, XMFLOAT3 lookAt);

    // ---- HUD 部品
    void Bar(float x, float y, float w, float h, float v, D2D1_COLOR_F fill, D2D1_COLOR_F back);
    void DrawDialog();
    void DrawToasts();
    void DrawFaceIcon(int def, float cx, float cy, float r);
    void DrawInkHint(const std::wstring& s, float cx, float y, float w);   // 操作のヒント（墨の帯）
    // UI の共通の形（和紙と墨）：墨の板（大きい板は内側に縫い目）／選べる行（選ぶと和紙の札）
    void InkPanel(float x, float y, float w, float h, float a = 0.9f);
    void InkRow(float x, float y, float w, float h, bool sel);
    void InkTab(float x, float y, float w, float h, bool on);   // タブ：選ぶと朱の平行四辺形
    // 斜めのゲージ（アニメ風の HUD）。trail：減ったぶんを遅れて追いかける白い帯（負なら出さない）
    void SlantBar(float x, float y, float w, float h, float v, D2D1_COLOR_F fill, D2D1_COLOR_F back, float trail = -1.f);
    float hpTrail = 1.f, hpTrailTime = 0.f;   // HP の白い帯の位置と、前に描いた時刻
};
