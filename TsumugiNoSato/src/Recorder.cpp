// 動画の書き出し（Media Foundation の Sink Writer で MP4 / H.264）
#include "Recorder.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

bool VideoRecorder::Open(const std::wstring& path, int w, int h, int f, UINT32 bitrate)
{
    Close();
    if (FAILED(MFStartup(MF_VERSION))) return false;
    started = true;
    W = w; H = h; fps = f; frame = 0;
    nv12.resize((size_t)W * H * 3 / 2);
    DeleteFileW(path.c_str());
    if (FAILED(MFCreateSinkWriterFromURL(path.c_str(), nullptr, nullptr, &writer))) { writer.Reset(); return false; }

    // 出力：H.264
    ComPtr<IMFMediaType> out;
    MFCreateMediaType(&out);
    out->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    out->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    out->SetUINT32(MF_MT_AVG_BITRATE, bitrate);
    out->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(out.Get(), MF_MT_FRAME_SIZE, W, H);
    MFSetAttributeRatio(out.Get(), MF_MT_FRAME_RATE, fps, 1);
    MFSetAttributeRatio(out.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (FAILED(writer->AddStream(out.Get(), &stream))) { writer.Reset(); return false; }

    // 入力：NV12（H.264 の標準のエンコーダがそのまま受け取れる形。RGB からの変換はこちらで行う）
    ComPtr<IMFMediaType> in;
    MFCreateMediaType(&in);
    in->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    in->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    in->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(in.Get(), MF_MT_FRAME_SIZE, W, H);
    MFSetAttributeRatio(in.Get(), MF_MT_FRAME_RATE, fps, 1);
    MFSetAttributeRatio(in.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    in->SetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32)W);
    if (FAILED(writer->SetInputMediaType(stream, in.Get(), nullptr))) { writer.Reset(); return false; }
    if (FAILED(writer->BeginWriting())) { writer.Reset(); return false; }
    return true;
}

bool VideoRecorder::AddFrame(const uint8_t* bgra, int pitch)
{
    if (!writer) return false;
    // BGRA → NV12（BT.709・テレビの範囲 16〜235）。Y は全画素、UV は 2×2 画素ごとに1組
    uint8_t* Y = nv12.data();
    uint8_t* UV = nv12.data() + (size_t)W * H;
    for (int y = 0; y < H; y++)
    {
        const uint8_t* row = bgra + (size_t)y * pitch;
        uint8_t* yr = Y + (size_t)y * W;
        for (int x = 0; x < W; x++)
        {
            int b = row[x * 4], g = row[x * 4 + 1], r = row[x * 4 + 2];
            yr[x] = (uint8_t)std::clamp((47 * r + 157 * g + 16 * b + 4096 + 128) >> 8, 0, 255);
        }
    }
    for (int y = 0; y < H; y += 2)
    {
        const uint8_t* r0 = bgra + (size_t)y * pitch;
        const uint8_t* r1 = bgra + (size_t)(y + 1) * pitch;
        uint8_t* uvr = UV + (size_t)(y / 2) * W;
        for (int x = 0; x < W; x += 2)
        {
            int b = (r0[x * 4] + r0[x * 4 + 4] + r1[x * 4] + r1[x * 4 + 4]) >> 2;
            int g = (r0[x * 4 + 1] + r0[x * 4 + 5] + r1[x * 4 + 1] + r1[x * 4 + 5]) >> 2;
            int r = (r0[x * 4 + 2] + r0[x * 4 + 6] + r1[x * 4 + 2] + r1[x * 4 + 6]) >> 2;
            uvr[x]     = (uint8_t)std::clamp((-26 * r - 87 * g + 112 * b + 32768 + 128) >> 8, 0, 255);
            uvr[x + 1] = (uint8_t)std::clamp((112 * r - 102 * g - 10 * b + 32768 + 128) >> 8, 0, 255);
        }
    }

    ComPtr<IMFMediaBuffer> buf;
    if (FAILED(MFCreateMemoryBuffer((DWORD)nv12.size(), &buf))) return false;
    BYTE* dst = nullptr;
    if (FAILED(buf->Lock(&dst, nullptr, nullptr))) return false;
    memcpy(dst, nv12.data(), nv12.size());
    buf->Unlock();
    buf->SetCurrentLength((DWORD)nv12.size());

    ComPtr<IMFSample> sample;
    MFCreateSample(&sample);
    sample->AddBuffer(buf.Get());
    const LONGLONG dur = 10000000LL / fps;   // 100ns 単位
    sample->SetSampleTime(frame * dur);
    sample->SetSampleDuration(dur);
    frame++;
    return SUCCEEDED(writer->WriteSample(stream, sample.Get()));
}

void VideoRecorder::Close()
{
    if (writer) { writer->Finalize(); writer.Reset(); }
    if (started) { MFShutdown(); started = false; }
}
