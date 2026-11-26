#pragma once
// 動画の書き出し（Windows の Media Foundation で MP4 / H.264 にする）
//   紹介動画を作る --movie モードで使う。ゲームの画面（BGRA）を1枚ずつ渡すと、NV12 に変換して H.264 で書き込む
#include "Common.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>   // IMFSinkWriter（ComPtr のメンバーに使うので、型の中身が必要。mfidl.h を先に読む）

class VideoRecorder
{
public:
    // path：書き出す .mp4　w, h：画面の大きさ（偶数）　fps：1秒あたりの枚数　bitrate：ビット毎秒
    bool Open(const std::wstring& path, int w, int h, int fps, UINT32 bitrate = 12000000);
    // bgra：上から下へ並んだ BGRA の画素。pitch：1行のバイト数
    bool AddFrame(const uint8_t* bgra, int pitch);
    void Close();   // 書き終わり（これを呼ばないと MP4 が壊れる）
    bool IsOpen() const { return writer != nullptr; }
    ~VideoRecorder() { Close(); }

private:
    ComPtr<IMFSinkWriter> writer;
    DWORD stream = 0;
    int W = 0, H = 0, fps = 30;
    LONGLONG frame = 0;
    std::vector<uint8_t> nv12;   // 変換先（Y 面 W×H ＋ UV 面 W×H/2）
    bool started = false;
};
