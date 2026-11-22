#pragma once
#include "Common.h"

struct Vertex { XMFLOAT3 p; XMFLOAT3 n; XMFLOAT2 uv; };
struct Mesh { ComPtr<ID3D11Buffer> vb, ib; UINT count = 0; };
struct Texture { ComPtr<ID3D11ShaderResourceView> srv; int w = 0, h = 0; };

struct MeshBuilder
{
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    uint32_t AddV(XMFLOAT3 p, XMFLOAT3 n, XMFLOAT2 uv);
    void Tri(uint32_t a, uint32_t b, uint32_t c);     // 法線を見て巻き順を自動補正
    void Quad(XMFLOAT3 p0, XMFLOAT3 p1, XMFLOAT3 p2, XMFLOAT3 p3, XMFLOAT3 n,
              XMFLOAT2 t0, XMFLOAT2 t1, XMFLOAT2 t2, XMFLOAT2 t3);
    // 軸平行の箱。UV = 座標*s + off
    void Box(XMFLOAT3 mn, XMFLOAT3 mx, float s, float off = 0.f, bool top = true, bool bottom = false);
    void FloorQuad(float x0, float z0, float x1, float z1, float y, float s);
};

enum class Blend { Opaque, Alpha, Add };

struct DrawParams
{
    XMFLOAT4 color{ 1, 1, 1, 1 };
    Texture* tex = nullptr;
    XMFLOAT4 uv{ 1, 1, 0, 0 };
    XMFLOAT3 emissive{ 0, 0, 0 };
    bool unlit = false;
    bool alphaClip = false;
    float outline = 0.f;
    float rim = 0.25f;
    bool worldUV = false;
    float fog = 1.f;
    Blend blend = Blend::Opaque;
    bool depthWrite = true;
    bool depthTest = true;
    bool cullNone = false;
};

struct PointLight { XMFLOAT3 pos; float range; XMFLOAT3 color; };

struct FrameSettings
{
    XMFLOAT3 camPos{ 0, 5, -10 };
    XMFLOAT3 camTarget{ 0, 0, 0 };
    float fovY = 0.95f;
    float nearZ = 0.1f, farZ = 500.f;
    XMFLOAT3 lightDir{ 0.4f, -1.f, 0.3f };
    XMFLOAT3 lightColor{ 1, 1, 1 };
    XMFLOAT3 ambient{ 0.4f, 0.4f, 0.45f };
    XMFLOAT3 fogColor{ 0.8f, 0.8f, 0.85f };
    float fogStart = 20, fogEnd = 80, fogMax = 1, heightFog = 0;
    XMFLOAT3 clear{ 0.5f, 0.6f, 0.7f };
    float time = 0;
    std::vector<PointLight> lights;
};

class Renderer
{
public:
    bool Init(HWND hwnd, int w, int h, const std::wstring& shaderPath);
    Texture* LoadTexture(const std::wstring& path);
    Mesh CreateMesh(const MeshBuilder& mb);

    void BeginFrame(const FrameSettings& fs);
    void Draw(const Mesh& m, const XMMATRIX& world, const DrawParams& p);
    void DrawBillboard(Texture* t, XMFLOAT3 pos, float w, float h, XMFLOAT4 color, Blend b,
                       XMFLOAT4 uv = { 1, 1, 0, 0 }, float rot = 0.f, bool depthTest = true, float fog = 1.f);
    void DrawScreenOverlay(Texture* t, XMFLOAT4 color, Blend b);

    // 2D（Direct2D / DirectWrite）
    void Begin2D();
    void End2D();
    void FillRect(float x, float y, float w, float h, D2D1_COLOR_F c);
    void FillRound(float x, float y, float w, float h, float r, D2D1_COLOR_F c);
    void StrokeRound(float x, float y, float w, float h, float r, D2D1_COLOR_F c, float width = 1.5f);
    void StrokeRect(float x, float y, float w, float h, D2D1_COLOR_F c, float width = 1.f);
    void FillCircle(float cx, float cy, float r, D2D1_COLOR_F c);
    void StrokeCircle(float cx, float cy, float r, D2D1_COLOR_F c, float width = 1.5f);
    void Line(float x0, float y0, float x1, float y1, D2D1_COLOR_F c, float width = 1.5f);
    void Text(const std::wstring& s, float x, float y, float w, float h, float size, D2D1_COLOR_F c,
              int align = 0, bool bold = false, bool shadow = true, int family = 0);

    void Present(bool vsync);
    bool SaveScreenshot(const std::wstring& path);
    bool Project(XMFLOAT3 world, float& sx, float& sy) const;
    void ScreenRay(float sx, float sy, XMFLOAT3& origin, XMFLOAT3& dir) const;

    int W = 0, H = 0;
    Mesh cube, sphere, cylinder, cone, plane, quad, octa, prism;
    Texture* white = nullptr;
    XMMATRIX view{}, proj{};
    XMFLOAT3 camPos{}, camFwd{};

private:
    void SetStates(const DrawParams& p);
    IDWriteTextFormat* Font(float size, bool bold, int family);

    ComPtr<ID3D11Device> dev;
    ComPtr<ID3D11DeviceContext> ctx;
    ComPtr<IDXGISwapChain1> swap;
    ComPtr<ID3D11Texture2D> backBuf;
    ComPtr<ID3D11RenderTargetView> backRTV;
    ComPtr<ID3D11Texture2D> msaaTex;
    ComPtr<ID3D11RenderTargetView> msaaRTV;
    ComPtr<ID3D11DepthStencilView> dsv;
    UINT msaa = 1;

    ComPtr<ID3D11VertexShader> vs, vsOutline;
    ComPtr<ID3D11PixelShader> ps, psOutline;
    ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11Buffer> frameCB, objCB;
    ComPtr<ID3D11RasterizerState> rsBack, rsFront, rsNone;
    ComPtr<ID3D11BlendState> bsOpaque, bsAlpha, bsAdd;
    ComPtr<ID3D11DepthStencilState> dsDefault, dsNoWrite, dsOff, dsOffNoWrite;
    ComPtr<ID3D11SamplerState> sampler;

    ComPtr<ID2D1Factory1> d2dFactory;
    ComPtr<ID2D1Device> d2dDev;
    ComPtr<ID2D1DeviceContext> d2d;
    ComPtr<ID2D1Bitmap1> d2dTarget;
    ComPtr<ID2D1SolidColorBrush> brush;
    ComPtr<IDWriteFactory> dwrite;
    std::map<int, ComPtr<IDWriteTextFormat>> fonts;

    ComPtr<IWICImagingFactory> wic;
    std::map<std::wstring, std::unique_ptr<Texture>> texCache;

    XMFLOAT3 camRight{}, camUp{};
    float fovY = 0.95f;
};

inline D2D1_COLOR_F RGBA(float r, float g, float b, float a = 1.f) { return D2D1::ColorF(r, g, b, a); }
