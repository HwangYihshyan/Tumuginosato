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
    bool guard = false, dodge = false, lockon = false, interact = false, swap = false;
    int item = -1;
    bool up = false, down = false, confirm = false;
};

enum class SceneId { Title, Village, Dungeon };

// ------------------------------------------------------------------
// 復興パート（ハタオリ村）
// ------------------------------------------------------------------
enum PartType
{
    P_HouseTouji, P_HouseKurobee, P_HouseRabbit, P_HouseVillager,
    P_Loom, P_Forge, P_Kiln, P_Field, P_Tree, P_Lantern, P_Path, P_Cloth, P_Well, P_Wheel,
    P_COUNT
};
struct PartDef { const wchar_t* name; int w, h; int pts; const wchar_t* desc; };
extern const PartDef kParts[P_COUNT];

enum ReqKind { RK_NearCenter, RK_NearPart, RK_AdjRiver, RK_TreesAround };
struct ReqDef { PartType owner; ReqKind kind; PartType other; int dist; int count; int pts; const wchar_t* who; const wchar_t* text; };
extern const ReqDef kReqs[];
extern const int kReqCount;

struct Placed { PartType t; int x, z, rot; float appearT; };
struct Walker { int kind; XMFLOAT3 pos; XMFLOAT3 target; float yaw; float waitT; float phase; XMFLOAT4 color; };

constexpr int VG = 20;           // 20x20 マス
constexpr int RIVER_X = 18;      // 18,19 列は川

// ------------------------------------------------------------------
// ダンジョン（根の洞）
// ------------------------------------------------------------------
constexpr int DN = 30;           // 30x30 マス（1マス=2m）
constexpr float CELL = 2.f;
constexpr float WALL_H = 3.2f;

struct Room { int x, z, w, h; };
struct Deco { int kind; XMFLOAT3 pos; float scale; float rot; float tilt; };

struct CharDef
{
    const wchar_t* name; const wchar_t* weapon; const wchar_t* spare;
    float maxHp, speed;
    float reach, arcDeg, dmg[3], dur[3];
    float chargeReach, chargeArc, chargeDmg;
    int maxDur;
    bool badger;
};

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
    XMFLOAT3 dodgeDir{};
    float dur = 60, maxDur = 60;
    bool broken = false;
    int lvl = 1; float exp = 0;
    float aiCd = 0;
};

enum class EType { Rat, Doll, Spore, Spider };
enum class ESt { Idle, Notice, Chase, Windup, Attack, Recover, Hit, Dead };
struct Enemy
{
    EType t = EType::Rat;
    XMFLOAT3 pos{}, kb{}, dashDir{}, wander{};
    float yaw = 0, hp = 10, maxHp = 10, radius = 0.45f;
    ESt st = ESt::Idle;
    float stT = 0, flash = 0, wanderT = 0, atkCd = 0, deadT = 0, bob = 0;
    bool token = false, keyHolder = false, hitDone = false;
    int pattern = 0;
};

struct Projectile { XMFLOAT3 pos, vel; float life, dmg, radius; int kind; };
enum class PickKind { Crystal, Key, Herb };
struct Pickup { PickKind k; XMFLOAT3 pos; float t; bool taken; float vy; };
struct Chest { XMFLOAT3 pos; float yaw; bool opened; float openT; };
struct Spring { XMFLOAT3 pos; bool used; };
struct FxInst { XMFLOAT3 pos; float t, dur, size; int kind; XMFLOAT4 color; float rot; };
struct DmgNum { XMFLOAT3 pos; float t; int val; D2D1_COLOR_F col; float size; };
struct Toast { std::wstring text; float t; D2D1_COLOR_F col; };
struct DialogLine { std::wstring who, text; };

struct ShotStep { float wait; std::function<void()> act; std::wstring shot; };

// ------------------------------------------------------------------
class Game
{
public:
    bool Init(Renderer* r, const std::wstring& assetDir);
    void Update(float dt, InputState& in);
    void Render();

    bool autoshot = false;
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

    // ---- 進行状況
    int inventory[P_COUNT]{};
    std::vector<PartType> storyQueue;
    int items[4] = { 2, 1, 1, 1 };   // 薬草 灯油 研ぎ粉 帰り糸
    int threads = 0;
    int deepest = 0;
    bool knot = false;               // 結び目（5階クリア）
    int crystalsThisRun = 0;
    int totalDives = 0;

    // ---- タイトル
    void UpdateTitle(float dt, InputState& in);
    void DrawTitleUI();

    // ---- 村
    std::vector<Placed> placed;
    std::vector<Walker> walkers;
    int cursorX = 6, cursorZ = 6, cursorRot = 0, palSel = 0;
    float vYaw = 0, vDist = 28, vPitch = 0.95f;
    XMFLOAT3 vFocus{ 0, 0, 0 };
    bool vWalkCam = false;
    float rateShown = 0;
    void EnterVillage(bool fromDungeon);
    void UpdateVillage(float dt, InputState& in);
    void RenderVillage(bool title);
    void DrawVillageUI();
    void DrawPart(PartType t, XMFLOAT3 c, int rot, float appear, float seed);
    void DrawGround(bool village, float rate);
    XMFLOAT3 CellCenter(int x, int z, int w = 1, int h = 1) const;
    bool CanPlace(PartType t, int x, int z, int rot, int ignore = -1) const;
    void PartSize(PartType t, int rot, int& w, int& h) const;
    bool ReqSatisfied(int r) const;
    float RestoreRate() const;
    void RebuildWalkers();
    std::vector<int> PaletteList() const;
    PartType NextCrystalPart();
    int Gap(int ax, int az, int aw, int ah, int bx, int bz, int bw, int bh) const;

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
    Actor party[2];
    int active = 0;
    int partySize = 2;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projs;
    std::vector<Pickup> pickups;
    std::vector<Chest> chests;
    std::vector<Spring> springs;
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
    float bossIntroT = 0;
    std::wstring interactHint;

    void EnterDungeon(int floor, uint32_t seed);
    void GenerateFloor(int floor, uint32_t seed);
    void BuildDungeonMeshes();
    void UpdateDungeon(float dt, const Controls& c);
    void RenderDungeon();
    void DrawDungeonUI();
    void LeaveDungeon(bool dead);

    const CharDef& Def(const Actor& a) const;
    void UpdateActor(Actor& a, const Controls* c, float dt, bool controlled);
    void UpdateFollower(Actor& a, float dt);
    void ActorAttackHits(Actor& a, float reach, float arcDeg, float dmg, bool heavy, int wear);
    void DamageActor(Actor& a, float dmg, XMFLOAT3 from, float kb);
    void WeaponWear(Actor& a, int n);
    void UpdateEnemies(float dt);
    void HitEnemy(Enemy& e, float dmg, XMFLOAT3 from, float kb, bool heavy, Actor* by);
    void KillEnemy(Enemy& e, Actor* by);
    void SpawnEnemy(EType t, XMFLOAT3 pos, bool deep);
    void MoveCircle(XMFLOAT3& p, float r, float dx, float dz) const;
    bool Solid(float x, float z) const;
    bool SolidCell(int cx, int cz) const { return cx < 0 || cz < 0 || cx >= DN || cz >= DN || cell[cx][cz] == 0; }
    int RoomAt(XMFLOAT3 p) const;
    XMFLOAT3 RandomFloorPos(const Room& r, float margin);
    XMFLOAT3 PathNext(XMFLOAT3 from, XMFLOAT3 to);
    Controls AutoControls(float dt);
    void SpawnFx(int kind, XMFLOAT3 pos, float size, XMFLOAT4 col, float dur = 0.55f);
    void AddNum(XMFLOAT3 pos, int v, D2D1_COLOR_F c, float size = 26);
    void UseItem(int i);

    // ---- キャラ描画
    void DrawSora(const Actor& a, float alpha);
    void DrawKurobee(const Actor& a, float alpha);
    void DrawTsumugi(XMFLOAT3 pos);
    void DrawEnemy(const Enemy& e);
    void DrawWalker(const Walker& w);
    void DrawShadow(XMFLOAT3 pos, float size, float alpha = 0.45f);
    void Seg(XMFLOAT3 a, XMFLOAT3 b, float r, DrawParams p);
    XMFLOAT3 tint{ 0, 0, 0 };
    std::vector<XMFLOAT4> forest;   // x,z = 位置, y = 大きさ, w = 種類
    Mesh tube;                      // 遠景用の筒（ふたなし）
    float autoHold = 0;
    int autoCount = 0;

    // ---- 自動撮影
    std::vector<ShotStep> steps;
    size_t stepIdx = 0;
    float stepT = 0;
    void BuildShotScript();
    void UpdateDirector(float dt);
    void TeleportPartyNear(XMFLOAT3 p, XMFLOAT3 lookAt);

    // ---- HUD 部品
    void Bar(float x, float y, float w, float h, float v, D2D1_COLOR_F fill, D2D1_COLOR_F back);
    void DrawDialog();
    void DrawToasts();
};
