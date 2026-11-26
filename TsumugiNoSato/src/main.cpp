// ツムギノサト ― 霧海の記憶 ―（垂直スライス）
// C++17 / DirectX 11
#include "Game.h"
#include "Recorder.h"
#include <shellapi.h>
#include <filesystem>

static InputState g_in;
static bool g_active = true;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_ACTIVATE: g_active = LOWORD(wp) != WA_INACTIVE; break;
    case WM_KEYDOWN: case WM_SYSKEYDOWN:
        if (wp < 256) g_in.key[wp] = true;
        if (wp == VK_TAB || wp == VK_MENU) return 0;
        break;
    case WM_KEYUP: case WM_SYSKEYUP:
        if (wp < 256) g_in.key[wp] = false;
        break;
    case WM_LBUTTONDOWN: g_in.lmb = true; SetCapture(hwnd); break;
    case WM_LBUTTONUP: g_in.lmb = false; ReleaseCapture(); break;
    case WM_RBUTTONDOWN: g_in.rmb = true; break;
    case WM_RBUTTONUP: g_in.rmb = false; break;
    case WM_MOUSEMOVE:
        g_in.mouseX = (float)(short)LOWORD(lp);
        g_in.mouseY = (float)(short)HIWORD(lp);
        g_in.mouseMoved = true;
        break;
    case WM_MOUSEWHEEL: g_in.wheel += (float)GET_WHEEL_DELTA_WPARAM(wp); break;
    case WM_INPUT:
    {
        RAWINPUT ri; UINT sz = sizeof(ri);
        if (GetRawInputData((HRAWINPUT)lp, RID_INPUT, &ri, &sz, sizeof(RAWINPUTHEADER)) != (UINT)-1 && ri.header.dwType == RIM_TYPEMOUSE)
        {
            g_in.mdx += (float)ri.data.mouse.lLastX;
            g_in.mdy += (float)ri.data.mouse.lLastY;
        }
        break;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static std::wstring FindAssets()
{
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::filesystem::path p(buf);
    p = p.parent_path();
    for (int i = 0; i < 5; i++)
    {
        if (std::filesystem::exists(p / L"Assets" / L"Shaders" / L"Toon.hlsl")) return (p / L"Assets").wstring() + L"\\";
        p = p.parent_path();
    }
    return L"Assets\\";
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    bool autoshot = false;
    std::wstring shotDir = L"Screenshots";
    std::wstring moviePath;    // --movie 出力.mp4：紹介動画の台本を流して、MP4 に書き出す
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; i++)
    {
        if (std::wstring(argv[i]) == L"--autoshot")
        {
            autoshot = true;
            if (i + 1 < argc) shotDir = argv[++i];
        }
        else if (std::wstring(argv[i]) == L"--movie" && i + 1 < argc)
        {
            autoshot = true;   // 自動操作のしくみ（台本・固定の時間の進み）は撮影デモと同じ
            moviePath = std::filesystem::absolute(argv[++i]).wstring();
            shotDir = (std::filesystem::path(moviePath).parent_path() / L"movie_work").wstring();   // デモ用のセーブなどの置き場
        }
    }
    LocalFree(argv);

    const int W = 1280, H = 720;
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"TsumugiNoSato";
    RegisterClassExW(&wc);
    RECT rc{ 0, 0, W, H };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rc, style, FALSE);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"ツムギノサト ― 霧海の記憶 ―", style, CW_USEDEFAULT, CW_USEDEFAULT,
                                rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInst, nullptr);
    ShowWindow(hwnd, SW_SHOW);

    RAWINPUTDEVICE rid{ 0x01, 0x02, 0, hwnd };
    RegisterRawInputDevices(&rid, 1, sizeof(rid));

    std::wstring assets = FindAssets();
    int rc2 = 0;
    {   // COM オブジェクトを CoUninitialize より先に解放するためのスコープ
    Renderer renderer;
    if (!renderer.Init(hwnd, W, H, assets + L"Shaders\\Toon.hlsl"))
    {
        MessageBoxW(hwnd, L"DirectX 11 の初期化に失敗しました", L"ツムギノサト", MB_OK | MB_ICONERROR);
        return 1;
    }
    Game game;
    game.autoshot = autoshot;
    game.movie = !moviePath.empty();
    game.shotDir = shotDir;
    if (autoshot) std::filesystem::create_directories(shotDir);
    if (!game.Init(&renderer, assets)) return 1;
    // 紹介動画：ゲームは 60 コマ/秒で進め、2 コマに 1 枚を 30fps の MP4 に書き込む
    VideoRecorder recorder;
    std::vector<uint8_t> framePx;
    int frameNo = 0;
    if (game.movie && !recorder.Open(moviePath, W, H, 30, 8000000))
    {
        MessageBoxW(hwnd, L"動画ファイルを作れませんでした", L"ツムギノサト", MB_OK | MB_ICONERROR);
        return 1;
    }

    LARGE_INTEGER freq, prev, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);
    bool cursorHidden = false;
    MSG msg{};
    while (!game.quit)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) game.quit = true;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (game.quit) break;
        QueryPerformanceCounter(&now);
        float dt = (float)(now.QuadPart - prev.QuadPart) / freq.QuadPart;
        prev = now;
        dt = std::min(dt, 0.05f);
        if (autoshot) dt = 1.f / 60.f;

        game.Update(dt, g_in);
        game.Render();
        if (!game.pendingShot.empty())
        {
            renderer.SaveScreenshot(game.pendingShot);
            game.pendingShot.clear();
        }
        if (recorder.IsOpen() && (frameNo++ % 2) == 0)
        {
            int pitch = 0;
            if (renderer.CaptureFrame(framePx, pitch)) recorder.AddFrame(framePx.data(), pitch);
        }
        renderer.Present(!autoshot);
        g_in.EndFrame();

        // ダンジョンではマウスをウィンドウ内に固定してカメラ操作
        bool lock = game.wantMouseLock && g_active;
        if (lock)
        {
            RECT cr; GetClientRect(hwnd, &cr);
            POINT c{ (cr.right) / 2, (cr.bottom) / 2 };
            ClientToScreen(hwnd, &c);
            SetCursorPos(c.x, c.y);
        }
        if (lock != cursorHidden) { ShowCursor(lock ? FALSE : TRUE); cursorHidden = lock; }
    }
    if (cursorHidden) ShowCursor(TRUE);
    recorder.Close();   // MP4 を書き終える
    }
    CoUninitialize();
    return rc2;
}
