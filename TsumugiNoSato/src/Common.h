#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wrl/client.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <DirectXMath.h>

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <random>
#include <deque>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

constexpr float PI = 3.14159265f;

inline float Clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline float Saturate(float v) { return Clampf(v, 0.f, 1.f); }
inline float Lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float SmoothStep(float a, float b, float x) { float t = Saturate((x - a) / (b - a)); return t * t * (3 - 2 * t); }

inline XMFLOAT3 F3(float x, float y, float z) { return { x, y, z }; }
inline XMFLOAT4 C4(float r, float g, float b, float a = 1.f) { return { r, g, b, a }; }
inline XMFLOAT3 Add(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline XMFLOAT3 Sub(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline XMFLOAT3 Mul(const XMFLOAT3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }
inline XMFLOAT3 Lerp3(const XMFLOAT3& a, const XMFLOAT3& b, float t) { return { Lerpf(a.x, b.x, t), Lerpf(a.y, b.y, t), Lerpf(a.z, b.z, t) }; }
inline XMFLOAT4 Lerp4(const XMFLOAT4& a, const XMFLOAT4& b, float t) { return { Lerpf(a.x, b.x, t), Lerpf(a.y, b.y, t), Lerpf(a.z, b.z, t), Lerpf(a.w, b.w, t) }; }
inline float Len(const XMFLOAT3& a) { return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z); }
inline float LenXZ(const XMFLOAT3& a) { return std::sqrt(a.x * a.x + a.z * a.z); }
inline float DistXZ(const XMFLOAT3& a, const XMFLOAT3& b) { return LenXZ(Sub(a, b)); }
inline XMFLOAT3 NormXZ(const XMFLOAT3& a) { float l = LenXZ(a); return l > 1e-5f ? XMFLOAT3{ a.x / l, 0, a.z / l } : XMFLOAT3{ 0, 0, 0 }; }
inline float DotXZ(const XMFLOAT3& a, const XMFLOAT3& b) { return a.x * b.x + a.z * b.z; }
inline XMFLOAT3 Fwd(float yaw) { return { std::sin(yaw), 0, std::cos(yaw) }; }
inline float YawOf(const XMFLOAT3& d) { return std::atan2(d.x, d.z); }
inline float WrapAngle(float a) { while (a > PI) a -= 2 * PI; while (a < -PI) a += 2 * PI; return a; }
inline float ApproachAngle(float cur, float target, float step)
{
    float d = WrapAngle(target - cur);
    if (std::fabs(d) <= step) return target;
    return WrapAngle(cur + (d > 0 ? step : -step));
}
inline float Approach(float cur, float target, float step)
{
    if (cur < target) return std::min(cur + step, target);
    return std::max(cur - step, target);
}
