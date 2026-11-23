#include "Renderer.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace
{
    struct FrameCB
    {
        XMFLOAT4X4 view, proj;
        XMFLOAT4 camPos, lightDir, lightColor, ambient, fogColor, fogParams, time;
        XMFLOAT4 plPos[16];
        XMFLOAT4 plCol[16];
        int plCount[4];
    };
    struct ObjectCB
    {
        XMFLOAT4X4 world;
        XMFLOAT4 color, uv, emissive, flags, flags2;
    };

    void Log(const char* s) { OutputDebugStringA(s); OutputDebugStringA("\n"); }
}

// ------------------------------------------------------------------
// MeshBuilder
// ------------------------------------------------------------------
uint32_t MeshBuilder::AddV(XMFLOAT3 p, XMFLOAT3 n, XMFLOAT2 uv)
{
    v.push_back({ p, n, uv });
    return (uint32_t)v.size() - 1;
}

void MeshBuilder::Tri(uint32_t a, uint32_t b, uint32_t c)
{
    XMVECTOR pa = XMLoadFloat3(&v[a].p), pb = XMLoadFloat3(&v[b].p), pc = XMLoadFloat3(&v[c].p);
    XMVECTOR cr = XMVector3Cross(pb - pa, pc - pa);
    XMVECTOR n = XMLoadFloat3(&v[a].n) + XMLoadFloat3(&v[b].n) + XMLoadFloat3(&v[c].n);
    if (XMVectorGetX(XMVector3Dot(cr, n)) < 0) std::swap(b, c);
    idx.push_back(a); idx.push_back(b); idx.push_back(c);
}

void MeshBuilder::Quad(XMFLOAT3 p0, XMFLOAT3 p1, XMFLOAT3 p2, XMFLOAT3 p3, XMFLOAT3 n,
                       XMFLOAT2 t0, XMFLOAT2 t1, XMFLOAT2 t2, XMFLOAT2 t3)
{
    uint32_t a = AddV(p0, n, t0), b = AddV(p1, n, t1), c = AddV(p2, n, t2), d = AddV(p3, n, t3);
    Tri(a, b, c);
    Tri(a, c, d);
}

void MeshBuilder::Box(XMFLOAT3 mn, XMFLOAT3 mx, float s, float o, bool top, bool bottom)
{
    float x0 = mn.x, y0 = mn.y, z0 = mn.z, x1 = mx.x, y1 = mx.y, z1 = mx.z;
    auto U = [&](float a) { return a * s + o; };
    auto V = [&](float a) { return -a * s + o; };
    // -Z
    Quad({ x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, { x1, y0, z0 }, { 0, 0, -1 },
         { U(x0), V(y0) }, { U(x0), V(y1) }, { U(x1), V(y1) }, { U(x1), V(y0) });
    // +Z
    Quad({ x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, { x0, y0, z1 }, { 0, 0, 1 },
         { U(-x1), V(y0) }, { U(-x1), V(y1) }, { U(-x0), V(y1) }, { U(-x0), V(y0) });
    // -X
    Quad({ x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, { x0, y0, z0 }, { -1, 0, 0 },
         { U(-z1), V(y0) }, { U(-z1), V(y1) }, { U(-z0), V(y1) }, { U(-z0), V(y0) });
    // +X
    Quad({ x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, { x1, y0, z1 }, { 1, 0, 0 },
         { U(z0), V(y0) }, { U(z0), V(y1) }, { U(z1), V(y1) }, { U(z1), V(y0) });
    if (top)
        Quad({ x0, y1, z0 }, { x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { 0, 1, 0 },
             { U(x0), V(z0) }, { U(x0), V(z1) }, { U(x1), V(z1) }, { U(x1), V(z0) });
    if (bottom)
        Quad({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, { 0, -1, 0 },
             { U(x0), V(z0) }, { U(x1), V(z0) }, { U(x1), V(z1) }, { U(x0), V(z1) });
}

void MeshBuilder::FloorQuad(float x0, float z0, float x1, float z1, float y, float s)
{
    Quad({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, { 0, 1, 0 },
         { x0 * s, z0 * s }, { x0 * s, z1 * s }, { x1 * s, z1 * s }, { x1 * s, z0 * s });
}

static MeshBuilder MakeSphere(int sl, int st)
{
    MeshBuilder mb;
    for (int i = 0; i <= st; i++)
    {
        float th = PI * i / st;
        for (int j = 0; j <= sl; j++)
        {
            float ph = 2 * PI * j / sl;
            XMFLOAT3 n{ std::sin(th) * std::cos(ph), std::cos(th), std::sin(th) * std::sin(ph) };
            mb.AddV({ n.x * 0.5f, n.y * 0.5f, n.z * 0.5f }, n, { (float)j / sl, (float)i / st });
        }
    }
    for (int i = 0; i < st; i++)
        for (int j = 0; j < sl; j++)
        {
            uint32_t a = i * (sl + 1) + j, b = a + sl + 1;
            if (i != 0) mb.Tri(a, b, a + 1);
            if (i != st - 1) mb.Tri(a + 1, b, b + 1);
        }
    return mb;
}

static MeshBuilder MakeCylinder(int sl)
{
    MeshBuilder mb;
    for (int j = 0; j <= sl; j++)
    {
        float ph = 2 * PI * j / sl;
        XMFLOAT3 n{ std::cos(ph), 0, std::sin(ph) };
        mb.AddV({ n.x * 0.5f, 0, n.z * 0.5f }, n, { (float)j / sl, 1 });
        mb.AddV({ n.x * 0.5f, 1, n.z * 0.5f }, n, { (float)j / sl, 0 });
    }
    for (int j = 0; j < sl; j++)
    {
        uint32_t a = j * 2;
        mb.Tri(a, a + 1, a + 3);
        mb.Tri(a, a + 3, a + 2);
    }
    for (int cap = 0; cap < 2; cap++)
    {
        float y = cap ? 1.f : 0.f;
        XMFLOAT3 n{ 0, cap ? 1.f : -1.f, 0 };
        uint32_t c = mb.AddV({ 0, y, 0 }, n, { 0.5f, 0.5f });
        uint32_t first = (uint32_t)mb.v.size();
        for (int j = 0; j <= sl; j++)
        {
            float ph = 2 * PI * j / sl;
            mb.AddV({ std::cos(ph) * 0.5f, y, std::sin(ph) * 0.5f }, n, { 0.5f + std::cos(ph) * 0.5f, 0.5f + std::sin(ph) * 0.5f });
        }
        for (int j = 0; j < sl; j++) mb.Tri(c, first + j, first + j + 1);
    }
    return mb;
}

static MeshBuilder MakeCone(int sl)
{
    MeshBuilder mb;
    for (int j = 0; j < sl; j++)
    {
        float p0 = 2 * PI * j / sl, p1 = 2 * PI * (j + 1) / sl, pm = (p0 + p1) * 0.5f;
        XMFLOAT3 n0{ std::cos(p0), 0.5f, std::sin(p0) }, n1{ std::cos(p1), 0.5f, std::sin(p1) }, nm{ std::cos(pm), 0.5f, std::sin(pm) };
        uint32_t a = mb.AddV({ std::cos(p0) * 0.5f, 0, std::sin(p0) * 0.5f }, n0, { (float)j / sl, 1 });
        uint32_t b = mb.AddV({ std::cos(p1) * 0.5f, 0, std::sin(p1) * 0.5f }, n1, { (float)(j + 1) / sl, 1 });
        uint32_t c = mb.AddV({ 0, 1, 0 }, nm, { (j + 0.5f) / sl, 0 });
        mb.Tri(a, b, c);
    }
    uint32_t c = mb.AddV({ 0, 0, 0 }, { 0, -1, 0 }, { 0.5f, 0.5f });
    uint32_t first = (uint32_t)mb.v.size();
    for (int j = 0; j <= sl; j++)
    {
        float ph = 2 * PI * j / sl;
        mb.AddV({ std::cos(ph) * 0.5f, 0, std::sin(ph) * 0.5f }, { 0, -1, 0 }, { 0.5f, 0.5f });
    }
    for (int j = 0; j < sl; j++) mb.Tri(c, first + j, first + j + 1);
    return mb;
}

static MeshBuilder MakeOcta()
{
    MeshBuilder mb;
    XMFLOAT3 top{ 0, 0.5f, 0 }, bot{ 0, -0.5f, 0 };
    XMFLOAT3 ring[4] = { { 0.5f, 0, 0 }, { 0, 0, 0.5f }, { -0.5f, 0, 0 }, { 0, 0, -0.5f } };
    for (int i = 0; i < 4; i++)
    {
        XMFLOAT3 a = ring[i], b = ring[(i + 1) % 4];
        for (int k = 0; k < 2; k++)
        {
            XMFLOAT3 t = k ? bot : top;
            XMFLOAT3 n{ a.x + b.x + t.x, a.y + b.y + t.y, a.z + b.z + t.z };
            XMVECTOR nv = XMVector3Normalize(XMLoadFloat3(&n));
            XMStoreFloat3(&n, nv);
            uint32_t i0 = mb.AddV(a, n, { 0, 1 }), i1 = mb.AddV(b, n, { 1, 1 }), i2 = mb.AddV(t, n, { 0.5f, 0 });
            mb.Tri(i0, i1, i2);
        }
    }
    return mb;
}

static MeshBuilder MakePrism()
{
    MeshBuilder mb;
    XMFLOAT3 A{ -0.5f, 0, -0.5f }, B{ 0.5f, 0, -0.5f }, C{ 0.5f, 0, 0.5f }, D{ -0.5f, 0, 0.5f }, R1{ -0.5f, 1, 0 }, R2{ 0.5f, 1, 0 };
    XMFLOAT3 nf{ 0, 0.4472f, -0.8944f }, nb{ 0, 0.4472f, 0.8944f };
    mb.Quad(A, R1, R2, B, nf, { 0, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 });
    mb.Quad(C, R2, R1, D, nb, { 0, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 });
    uint32_t a = mb.AddV(A, { -1, 0, 0 }, { 0, 1 }), d = mb.AddV(D, { -1, 0, 0 }, { 1, 1 }), r = mb.AddV(R1, { -1, 0, 0 }, { 0.5f, 0 });
    mb.Tri(a, d, r);
    uint32_t b = mb.AddV(B, { 1, 0, 0 }, { 0, 1 }), c = mb.AddV(C, { 1, 0, 0 }, { 1, 1 }), r2 = mb.AddV(R2, { 1, 0, 0 }, { 0.5f, 0 });
    mb.Tri(b, c, r2);
    // 裏面（軒下）
    mb.Quad(A, B, C, D, { 0, -1, 0 }, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 });
    return mb;
}

// ------------------------------------------------------------------
// Renderer
// ------------------------------------------------------------------
bool Renderer::Init(HWND hwnd, int w, int h, const std::wstring& shaderPath)
{
    W = w; H = h;
    HRESULT hr;
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL fls[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, fls, 2, D3D11_SDK_VERSION, &dev, nullptr, &ctx);
    if (FAILED(hr))
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, fls, 2, D3D11_SDK_VERSION, &dev, nullptr, &ctx);
    if (FAILED(hr)) return false;

    ComPtr<IDXGIDevice> dxgiDev; dev.As(&dxgiDev);
    ComPtr<IDXGIAdapter> adapter; dxgiDev->GetAdapter(&adapter);
    ComPtr<IDXGIFactory2> factory; adapter->GetParent(IID_PPV_ARGS(&factory));

    DXGI_SWAP_CHAIN_DESC1 sd{};
    sd.Width = w; sd.Height = h;
    sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    hr = factory->CreateSwapChainForHwnd(dev.Get(), hwnd, &sd, nullptr, nullptr, &swap);
    if (FAILED(hr)) return false;
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    swap->GetBuffer(0, IID_PPV_ARGS(&backBuf));
    dev->CreateRenderTargetView(backBuf.Get(), nullptr, &backRTV);

    UINT q = 0;
    dev->CheckMultisampleQualityLevels(DXGI_FORMAT_B8G8R8A8_UNORM, 4, &q);
    msaa = q > 0 ? 4 : 1;

    D3D11_TEXTURE2D_DESC td{};
    td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = msaa;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET;
    if (msaa > 1)
    {
        dev->CreateTexture2D(&td, nullptr, &msaaTex);
        dev->CreateRenderTargetView(msaaTex.Get(), nullptr, &msaaRTV);
    }
    td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ComPtr<ID3D11Texture2D> depthTex;
    dev->CreateTexture2D(&td, nullptr, &depthTex);
    dev->CreateDepthStencilView(depthTex.Get(), nullptr, &dsv);

    // ---- states
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_BACK; rd.DepthClipEnable = TRUE; rd.MultisampleEnable = TRUE; rd.AntialiasedLineEnable = TRUE;
    dev->CreateRasterizerState(&rd, &rsBack);
    rd.CullMode = D3D11_CULL_FRONT; dev->CreateRasterizerState(&rd, &rsFront);
    rd.CullMode = D3D11_CULL_NONE; dev->CreateRasterizerState(&rd, &rsNone);

    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    dev->CreateBlendState(&bd, &bsOpaque);
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    dev->CreateBlendState(&bd, &bsAlpha);
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    dev->CreateBlendState(&bd, &bsAdd);

    D3D11_DEPTH_STENCIL_DESC dd{};
    dd.DepthEnable = TRUE; dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    dev->CreateDepthStencilState(&dd, &dsDefault);
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dev->CreateDepthStencilState(&dd, &dsNoWrite);
    dd.DepthEnable = FALSE;
    dev->CreateDepthStencilState(&dd, &dsOffNoWrite);
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dev->CreateDepthStencilState(&dd, &dsOff);

    D3D11_SAMPLER_DESC smp{};
    smp.Filter = D3D11_FILTER_ANISOTROPIC; smp.MaxAnisotropy = 8;
    smp.AddressU = smp.AddressV = smp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    smp.MaxLOD = D3D11_FLOAT32_MAX;
    dev->CreateSamplerState(&smp, &sampler);

    // ---- shaders
    auto compile = [&](const char* entry, const char* target, ComPtr<ID3DBlob>& blob) -> bool {
        ComPtr<ID3DBlob> err;
        HRESULT r = D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &blob, &err);
        if (FAILED(r))
        {
            if (err) MessageBoxA(nullptr, (const char*)err->GetBufferPointer(), "Shader error", MB_OK);
            else MessageBoxW(nullptr, shaderPath.c_str(), L"Shader file not found", MB_OK);
            return false;
        }
        return true;
    };
    ComPtr<ID3DBlob> b1, b2, b3, b4;
    if (!compile("VSMain", "vs_5_0", b1) || !compile("PSMain", "ps_5_0", b2) ||
        !compile("VSOutline", "vs_5_0", b3) || !compile("PSOutline", "ps_5_0", b4)) return false;
    dev->CreateVertexShader(b1->GetBufferPointer(), b1->GetBufferSize(), nullptr, &vs);
    dev->CreatePixelShader(b2->GetBufferPointer(), b2->GetBufferSize(), nullptr, &ps);
    dev->CreateVertexShader(b3->GetBufferPointer(), b3->GetBufferSize(), nullptr, &vsOutline);
    dev->CreatePixelShader(b4->GetBufferPointer(), b4->GetBufferSize(), nullptr, &psOutline);
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    dev->CreateInputLayout(ied, 3, b1->GetBufferPointer(), b1->GetBufferSize(), &layout);

    D3D11_BUFFER_DESC cbd{};
    cbd.Usage = D3D11_USAGE_DYNAMIC; cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.ByteWidth = sizeof(FrameCB); dev->CreateBuffer(&cbd, nullptr, &frameCB);
    cbd.ByteWidth = sizeof(ObjectCB); dev->CreateBuffer(&cbd, nullptr, &objCB);
    // 軌跡などに使う動的な四角形
    {
        D3D11_BUFFER_DESC vd{};
        vd.Usage = D3D11_USAGE_DYNAMIC; vd.BindFlags = D3D11_BIND_VERTEX_BUFFER; vd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vd.ByteWidth = sizeof(Vertex) * 4;
        dev->CreateBuffer(&vd, nullptr, &dynVB);
        uint32_t ix[6] = { 0, 1, 2, 0, 2, 3 };
        D3D11_BUFFER_DESC id{}; id.Usage = D3D11_USAGE_IMMUTABLE; id.BindFlags = D3D11_BIND_INDEX_BUFFER; id.ByteWidth = sizeof(ix);
        D3D11_SUBRESOURCE_DATA isd{ ix, 0, 0 };
        dev->CreateBuffer(&id, &isd, &dynIB);
    }

    // ---- D2D / DWrite / WIC
    D2D1_FACTORY_OPTIONS fo{};
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &fo, (void**)d2dFactory.GetAddressOf());
    d2dFactory->CreateDevice(dxgiDev.Get(), &d2dDev);
    d2dDev->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2d);
    ComPtr<IDXGISurface> surf; swap->GetBuffer(0, IID_PPV_ARGS(&surf));
    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.f, 96.f);
    d2d->CreateBitmapFromDxgiSurface(surf.Get(), &bp, &d2dTarget);
    d2d->SetTarget(d2dTarget.Get());
    d2d->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    d2d->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &brush);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)dwrite.GetAddressOf());
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));

    // ---- primitives
    {
        MeshBuilder mb; mb.Box({ -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f }, 1.f, 0.5f, true, true); cube = CreateMesh(mb);
    }
    sphere = CreateMesh(MakeSphere(28, 18));
    cylinder = CreateMesh(MakeCylinder(24));
    cone = CreateMesh(MakeCone(24));
    octa = CreateMesh(MakeOcta());
    prism = CreateMesh(MakePrism());
    {
        MeshBuilder mb;
        mb.Quad({ -0.5f, 0, -0.5f }, { -0.5f, 0, 0.5f }, { 0.5f, 0, 0.5f }, { 0.5f, 0, -0.5f }, { 0, 1, 0 }, { 0, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 });
        plane = CreateMesh(mb);
    }
    {
        MeshBuilder mb;
        mb.Quad({ -0.5f, -0.5f, 0 }, { -0.5f, 0.5f, 0 }, { 0.5f, 0.5f, 0 }, { 0.5f, -0.5f, 0 }, { 0, 0, -1 }, { 0, 1 }, { 0, 0 }, { 1, 0 }, { 1, 1 });
        quad = CreateMesh(mb);
    }
    // 白テクスチャ
    {
        auto t = std::make_unique<Texture>();
        uint32_t px = 0xffffffff;
        D3D11_TEXTURE2D_DESC wd{}; wd.Width = wd.Height = 1; wd.MipLevels = wd.ArraySize = 1; wd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        wd.SampleDesc.Count = 1; wd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA sdd{ &px, 4, 0 };
        ComPtr<ID3D11Texture2D> tex; dev->CreateTexture2D(&wd, &sdd, &tex);
        dev->CreateShaderResourceView(tex.Get(), nullptr, &t->srv);
        t->w = t->h = 1;
        white = t.get();
        texCache[L"__white"] = std::move(t);
    }
    return true;
}

Texture* Renderer::LoadTexture(const std::wstring& path)
{
    auto it = texCache.find(path);
    if (it != texCache.end()) return it->second.get();

    ComPtr<IWICBitmapDecoder> dec;
    if (FAILED(wic->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &dec)))
    {
        OutputDebugStringW((L"Texture load failed: " + path + L"\n").c_str());
        return white;
    }
    ComPtr<IWICBitmapFrameDecode> frame; dec->GetFrame(0, &frame);
    UINT w, h; frame->GetSize(&w, &h);
    ComPtr<IWICBitmapSource> src = frame;
    // 大きすぎるものは縮小（読み込みの高速化）
    UINT maxDim = 1024;
    if (w > maxDim || h > maxDim)
    {
        float s = (float)maxDim / std::max(w, h);
        UINT nw = std::max(1u, (UINT)(w * s)), nh = std::max(1u, (UINT)(h * s));
        ComPtr<IWICBitmapScaler> sc; wic->CreateBitmapScaler(&sc);
        sc->Initialize(frame.Get(), nw, nh, WICBitmapInterpolationModeFant);
        src = sc; w = nw; h = nh;
    }
    ComPtr<IWICFormatConverter> conv; wic->CreateFormatConverter(&conv);
    conv->Initialize(src.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
    std::vector<uint8_t> px(w * h * 4);
    conv->CopyPixels(nullptr, w * 4, (UINT)px.size(), px.data());

    D3D11_TEXTURE2D_DESC td{};
    td.Width = w; td.Height = h; td.MipLevels = 0; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &tex))) return white;
    ctx->UpdateSubresource(tex.Get(), 0, nullptr, px.data(), w * 4, 0);
    auto t = std::make_unique<Texture>();
    dev->CreateShaderResourceView(tex.Get(), nullptr, &t->srv);
    ctx->GenerateMips(t->srv.Get());
    t->w = w; t->h = h;
    Texture* raw = t.get();
    texCache[path] = std::move(t);
    return raw;
}

Mesh Renderer::CreateMesh(const MeshBuilder& mb)
{
    Mesh m;
    if (mb.v.empty()) return m;
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.ByteWidth = (UINT)(mb.v.size() * sizeof(Vertex));
    D3D11_SUBRESOURCE_DATA sd{ mb.v.data(), 0, 0 };
    dev->CreateBuffer(&bd, &sd, &m.vb);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.ByteWidth = (UINT)(mb.idx.size() * sizeof(uint32_t));
    sd.pSysMem = mb.idx.data();
    dev->CreateBuffer(&bd, &sd, &m.ib);
    m.count = (UINT)mb.idx.size();
    return m;
}

void Renderer::BeginFrame(const FrameSettings& fs)
{
    ID3D11RenderTargetView* rtv = msaa > 1 ? msaaRTV.Get() : backRTV.Get();
    ctx->OMSetRenderTargets(1, &rtv, dsv.Get());
    float cc[4] = { fs.clear.x, fs.clear.y, fs.clear.z, 1 };
    ctx->ClearRenderTargetView(rtv, cc);
    ctx->ClearDepthStencilView(dsv.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
    D3D11_VIEWPORT vp{ 0, 0, (float)W, (float)H, 0, 1 };
    ctx->RSSetViewports(1, &vp);
    SetupCamera(fs, (float)W / H);
}

void Renderer::SetupCamera(const FrameSettings& fs, float aspect)
{
    fovY = fs.fovY;
    camPos = fs.camPos;
    XMVECTOR eye = XMLoadFloat3(&fs.camPos), at = XMLoadFloat3(&fs.camTarget);
    view = XMMatrixLookAtLH(eye, at, XMVectorSet(0, 1, 0, 0));
    proj = XMMatrixPerspectiveFovLH(fs.fovY, aspect, fs.nearZ, fs.farZ);
    XMMATRIX inv = XMMatrixInverse(nullptr, view);
    XMStoreFloat3(&camRight, inv.r[0]);
    XMStoreFloat3(&camUp, inv.r[1]);
    XMStoreFloat3(&camFwd, inv.r[2]);

    FrameCB cb{};
    XMStoreFloat4x4(&cb.view, XMMatrixTranspose(view));
    XMStoreFloat4x4(&cb.proj, XMMatrixTranspose(proj));
    cb.camPos = { fs.camPos.x, fs.camPos.y, fs.camPos.z, 1 };
    XMFLOAT3 ld = fs.lightDir;
    XMStoreFloat3(&ld, XMVector3Normalize(XMLoadFloat3(&ld)));
    cb.lightDir = { ld.x, ld.y, ld.z, 0 };
    cb.lightColor = { fs.lightColor.x, fs.lightColor.y, fs.lightColor.z, 1 };
    cb.ambient = { fs.ambient.x, fs.ambient.y, fs.ambient.z, 1 };
    cb.fogColor = { fs.fogColor.x, fs.fogColor.y, fs.fogColor.z, 1 };
    cb.fogParams = { fs.fogStart, fs.fogEnd, fs.fogMax, fs.heightFog };
    cb.time = { fs.time, 0, 0, 0 };
    // カメラ注視点に近いライトを優先
    std::vector<PointLight> ls = fs.lights;
    XMFLOAT3 ref = fs.camTarget;
    std::sort(ls.begin(), ls.end(), [&](const PointLight& a, const PointLight& b) {
        return Len(Sub(a.pos, ref)) < Len(Sub(b.pos, ref));
    });
    int n = std::min<int>(16, (int)ls.size());
    for (int i = 0; i < n; i++)
    {
        cb.plPos[i] = { ls[i].pos.x, ls[i].pos.y, ls[i].pos.z, ls[i].range };
        cb.plCol[i] = { ls[i].color.x, ls[i].color.y, ls[i].color.z, 1 };
    }
    cb.plCount[0] = n;
    D3D11_MAPPED_SUBRESOURCE ms;
    ctx->Map(frameCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    memcpy(ms.pData, &cb, sizeof(cb));
    ctx->Unmap(frameCB.Get(), 0);

    ctx->IASetInputLayout(layout.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11Buffer* cbs[2] = { frameCB.Get(), objCB.Get() };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetSamplers(0, 1, sampler.GetAddressOf());
}

void Renderer::SetStates(const DrawParams& p)
{
    float bf[4] = { 0, 0, 0, 0 };
    ID3D11BlendState* bs = p.blend == Blend::Alpha ? bsAlpha.Get() : p.blend == Blend::Add ? bsAdd.Get() : bsOpaque.Get();
    ctx->OMSetBlendState(bs, bf, 0xffffffff);
    ID3D11DepthStencilState* ds;
    if (p.depthTest) ds = p.depthWrite ? dsDefault.Get() : dsNoWrite.Get();
    else ds = p.depthWrite ? dsOff.Get() : dsOffNoWrite.Get();
    ctx->OMSetDepthStencilState(ds, 0);
    ctx->RSSetState(p.cullNone ? rsNone.Get() : rsBack.Get());
}

void Renderer::Draw(const Mesh& m, const XMMATRIX& world, const DrawParams& p)
{
    if (!m.vb) return;
    ObjectCB cb{};
    XMStoreFloat4x4(&cb.world, XMMatrixTranspose(world));
    cb.color = p.color;
    cb.uv = p.uv;
    cb.emissive = { p.emissive.x, p.emissive.y, p.emissive.z, 0 };
    cb.flags = { p.tex ? 1.f : 0.f, p.unlit ? 1.f : 0.f, p.alphaClip ? 1.f : 0.f, p.outline };
    cb.flags2 = { p.rim, p.worldUV ? 1.f : 0.f, p.fog, 0 };
    D3D11_MAPPED_SUBRESOURCE ms;
    ctx->Map(objCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    memcpy(ms.pData, &cb, sizeof(cb));
    ctx->Unmap(objCB.Get(), 0);

    SetStates(p);
    UINT stride = sizeof(Vertex), off = 0;
    ctx->IASetVertexBuffers(0, 1, m.vb.GetAddressOf(), &stride, &off);
    ctx->IASetIndexBuffer(m.ib.Get(), DXGI_FORMAT_R32_UINT, 0);
    ID3D11ShaderResourceView* srv = p.tex ? p.tex->srv.Get() : white->srv.Get();
    ctx->PSSetShaderResources(0, 1, &srv);
    ctx->VSSetShader(vs.Get(), nullptr, 0);
    ctx->PSSetShader(ps.Get(), nullptr, 0);
    ctx->DrawIndexed(m.count, 0, 0);

    if (p.outline > 0 && p.blend == Blend::Opaque)
    {
        ctx->RSSetState(rsFront.Get());
        ctx->VSSetShader(vsOutline.Get(), nullptr, 0);
        ctx->PSSetShader(psOutline.Get(), nullptr, 0);
        ctx->DrawIndexed(m.count, 0, 0);
    }
}

void Renderer::DrawBillboard(Texture* t, XMFLOAT3 pos, float w, float h, XMFLOAT4 color, Blend b, XMFLOAT4 uv, float rot, bool depthTest, float fog)
{
    XMMATRIX camRot(XMLoadFloat3(&camRight), XMLoadFloat3(&camUp), XMLoadFloat3(&camFwd), XMVectorSet(0, 0, 0, 1));
    XMMATRIX world = XMMatrixScaling(w, h, 1) * XMMatrixRotationZ(rot) * camRot * XMMatrixTranslation(pos.x, pos.y, pos.z);
    DrawParams p;
    p.tex = t; p.color = color; p.blend = b; p.uv = uv; p.unlit = true; p.depthWrite = false; p.depthTest = depthTest;
    p.cullNone = true; p.fog = fog;
    Draw(quad, world, p);
}

void Renderer::DrawScreenOverlay(Texture* t, XMFLOAT4 color, Blend b)
{
    float d = 0.5f;
    float hh = 2 * d * std::tan(fovY * 0.5f) * 1.02f;
    float ww = hh * (float)W / H;
    XMFLOAT3 pos = Add(camPos, Mul(camFwd, d));
    DrawBillboard(t, pos, ww, hh, color, b, { 1, 1, 0, 0 }, 0, false, 0.f);
}

void Renderer::DrawQuadWorld(XMFLOAT3 p0, XMFLOAT3 p1, XMFLOAT3 p2, XMFLOAT3 p3, XMFLOAT4 color, Blend b, Texture* t)
{
    D3D11_MAPPED_SUBRESOURCE ms;
    if (FAILED(ctx->Map(dynVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return;
    Vertex* v = (Vertex*)ms.pData;
    XMFLOAT3 n{ 0, 1, 0 };
    v[0] = { p0, n, { 0, 0 } }; v[1] = { p1, n, { 1, 0 } }; v[2] = { p2, n, { 1, 1 } }; v[3] = { p3, n, { 0, 1 } };
    ctx->Unmap(dynVB.Get(), 0);
    Mesh m; m.vb = dynVB; m.ib = dynIB; m.count = 6;
    DrawParams p;
    p.color = color; p.blend = b; p.tex = t; p.unlit = true; p.depthWrite = false; p.cullNone = true; p.fog = 0.5f;
    Draw(m, XMMatrixIdentity(), p);
}

bool Renderer::Project(XMFLOAT3 world, float& sx, float& sy) const
{
    XMVECTOR p = XMVector4Transform(XMVectorSet(world.x, world.y, world.z, 1), view * proj);
    float w = XMVectorGetW(p);
    if (w <= 0.05f) return false;
    sx = (XMVectorGetX(p) / w * 0.5f + 0.5f) * W;
    sy = (-XMVectorGetY(p) / w * 0.5f + 0.5f) * H;
    return true;
}

void Renderer::ScreenRay(float sx, float sy, XMFLOAT3& origin, XMFLOAT3& dir) const
{
    XMVECTOR n = XMVector3Unproject(XMVectorSet(sx, sy, 0, 0), 0, 0, (float)W, (float)H, 0, 1, proj, view, XMMatrixIdentity());
    XMVECTOR f = XMVector3Unproject(XMVectorSet(sx, sy, 1, 0), 0, 0, (float)W, (float)H, 0, 1, proj, view, XMMatrixIdentity());
    XMStoreFloat3(&origin, n);
    XMStoreFloat3(&dir, XMVector3Normalize(f - n));
}

// ---- 2D -----------------------------------------------------------
void Renderer::Begin2D()
{
    if (msaa > 1) ctx->ResolveSubresource(backBuf.Get(), 0, msaaTex.Get(), 0, DXGI_FORMAT_B8G8R8A8_UNORM);
    d2d->BeginDraw();
}
void Renderer::End2D() { d2d->EndDraw(); }

void Renderer::FillRect(float x, float y, float w, float h, D2D1_COLOR_F c)
{
    brush->SetColor(c);
    d2d->FillRectangle(D2D1::RectF(x, y, x + w, y + h), brush.Get());
}
void Renderer::StrokeRect(float x, float y, float w, float h, D2D1_COLOR_F c, float width)
{
    brush->SetColor(c);
    d2d->DrawRectangle(D2D1::RectF(x, y, x + w, y + h), brush.Get(), width);
}
void Renderer::FillRound(float x, float y, float w, float h, float r, D2D1_COLOR_F c)
{
    brush->SetColor(c);
    d2d->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), r, r), brush.Get());
}
void Renderer::StrokeRound(float x, float y, float w, float h, float r, D2D1_COLOR_F c, float width)
{
    brush->SetColor(c);
    d2d->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), r, r), brush.Get(), width);
}
void Renderer::FillCircle(float cx, float cy, float r, D2D1_COLOR_F c)
{
    brush->SetColor(c);
    d2d->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), brush.Get());
}
void Renderer::StrokeCircle(float cx, float cy, float r, D2D1_COLOR_F c, float width)
{
    brush->SetColor(c);
    d2d->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), brush.Get(), width);
}
void Renderer::Line(float x0, float y0, float x1, float y1, D2D1_COLOR_F c, float width)
{
    brush->SetColor(c);
    d2d->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush.Get(), width);
}

IDWriteTextFormat* Renderer::Font(float size, bool bold, int family)
{
    int key = (int)(size * 10) + (bold ? 100000 : 0) + family * 1000000;
    auto it = fonts.find(key);
    if (it != fonts.end()) return it->second.Get();
    ComPtr<IDWriteTextFormat> f;
    const wchar_t* fam = family == 1 ? L"Yu Mincho" : L"Yu Gothic UI";
    dwrite->CreateTextFormat(fam, nullptr, bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_SEMI_BOLD,
                             DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"ja-jp", &f);
    fonts[key] = f;
    return f.Get();
}

void Renderer::Text(const std::wstring& s, float x, float y, float w, float h, float size, D2D1_COLOR_F c, int align, bool bold, bool shadow, int family)
{
    IDWriteTextFormat* f = Font(size, bold, family);
    if (!f) return;
    f->SetTextAlignment(align == 0 ? DWRITE_TEXT_ALIGNMENT_LEADING : align == 1 ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_TRAILING);
    if (shadow)
    {
        brush->SetColor(D2D1::ColorF(0, 0, 0, 0.65f * c.a));
        d2d->DrawText(s.c_str(), (UINT32)s.size(), f, D2D1::RectF(x + 1.5f, y + 1.5f, x + w + 1.5f, y + h + 1.5f), brush.Get());
    }
    brush->SetColor(c);
    d2d->DrawText(s.c_str(), (UINT32)s.size(), f, D2D1::RectF(x, y, x + w, y + h), brush.Get());
}

// ------------------------------------------------------------------
// サムネイル用アトラス（MSAA で描いて解決し、D2D のビットマップとして UI に貼る）
// ------------------------------------------------------------------
bool Renderer::CreateAtlas(int w, int h)
{
    D3D11_TEXTURE2D_DESC td{};
    td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &atlasTex))) return false;
    td.SampleDesc.Count = msaa;
    td.BindFlags = D3D11_BIND_RENDER_TARGET;
    if (msaa > 1) { if (FAILED(dev->CreateTexture2D(&td, nullptr, &atlasMs))) return false; }
    if (FAILED(dev->CreateRenderTargetView(msaa > 1 ? atlasMs.Get() : atlasTex.Get(), nullptr, &atlasRTV))) return false;
    td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ComPtr<ID3D11Texture2D> depth;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &depth))) return false;
    if (FAILED(dev->CreateDepthStencilView(depth.Get(), nullptr, &atlasDSV))) return false;
    ComPtr<IDXGISurface> surf;
    if (FAILED(atlasTex.As(&surf))) return false;
    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.f, 96.f);
    if (FAILED(d2d->CreateBitmapFromDxgiSurface(surf.Get(), &bp, &atlasBmp))) return false;
    atlasW = w; atlasH = h;
    return true;
}

void Renderer::BeginAtlas()
{
    if (!atlasRTV) return;
    float cc[4] = { 0, 0, 0, 0 };
    ctx->OMSetRenderTargets(1, atlasRTV.GetAddressOf(), atlasDSV.Get());
    ctx->ClearRenderTargetView(atlasRTV.Get(), cc);
}

void Renderer::AtlasCell(const FrameSettings& fs, float x, float y, float w, float h)
{
    if (!atlasRTV) return;
    ctx->ClearDepthStencilView(atlasDSV.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
    D3D11_VIEWPORT vp{ x, y, w, h, 0, 1 };
    ctx->RSSetViewports(1, &vp);
    SetupCamera(fs, w / h);
}

void Renderer::EndAtlas()
{
    if (!atlasRTV) return;
    if (msaa > 1) ctx->ResolveSubresource(atlasTex.Get(), 0, atlasMs.Get(), 0, DXGI_FORMAT_B8G8R8A8_UNORM);
    ID3D11RenderTargetView* rtv = msaa > 1 ? msaaRTV.Get() : backRTV.Get();
    ctx->OMSetRenderTargets(1, &rtv, dsv.Get());
    D3D11_VIEWPORT vp{ 0, 0, (float)W, (float)H, 0, 1 };
    ctx->RSSetViewports(1, &vp);
}

void Renderer::DrawAtlas(float dx, float dy, float dw, float dh, float sx, float sy, float sw, float sh, float opacity)
{
    if (!atlasBmp) return;
    D2D1_RECT_F d = D2D1::RectF(dx, dy, dx + dw, dy + dh), s = D2D1::RectF(sx, sy, sx + sw, sy + sh);
    d2d->DrawBitmap(atlasBmp.Get(), &d, opacity, D2D1_INTERPOLATION_MODE_LINEAR, &s);
}

void Renderer::Present(bool vsync)
{
    swap->Present(vsync ? 1 : 0, 0);
}

bool Renderer::SaveScreenshot(const std::wstring& path)
{
    D3D11_TEXTURE2D_DESC d; backBuf->GetDesc(&d);
    d.Usage = D3D11_USAGE_STAGING; d.BindFlags = 0; d.CPUAccessFlags = D3D11_CPU_ACCESS_READ; d.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> st;
    if (FAILED(dev->CreateTexture2D(&d, nullptr, &st))) return false;
    ctx->CopyResource(st.Get(), backBuf.Get());
    D3D11_MAPPED_SUBRESOURCE ms;
    if (FAILED(ctx->Map(st.Get(), 0, D3D11_MAP_READ, 0, &ms))) return false;
    std::vector<uint8_t> px(d.Width * d.Height * 4);
    for (UINT y = 0; y < d.Height; y++)
    {
        memcpy(&px[y * d.Width * 4], (uint8_t*)ms.pData + y * ms.RowPitch, d.Width * 4);
        for (UINT x = 0; x < d.Width; x++) px[(y * d.Width + x) * 4 + 3] = 255;
    }
    ctx->Unmap(st.Get(), 0);

    ComPtr<IWICStream> stream; wic->CreateStream(&stream);
    if (FAILED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE))) return false;
    ComPtr<IWICBitmapEncoder> enc; wic->CreateEncoder(GUID_ContainerFormatPng, nullptr, &enc);
    enc->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    ComPtr<IWICBitmapFrameEncode> fr; enc->CreateNewFrame(&fr, nullptr);
    fr->Initialize(nullptr);
    fr->SetSize(d.Width, d.Height);
    WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGRA;
    fr->SetPixelFormat(&fmt);
    fr->WritePixels(d.Height, d.Width * 4, (UINT)px.size(), px.data());
    fr->Commit();
    enc->Commit();
    return true;
}
