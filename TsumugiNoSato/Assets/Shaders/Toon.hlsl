// ツムギノサト - トゥーンシェーダ（DirectX 11 / SM5.0）

cbuffer FrameCB : register(b0)
{
    float4x4 gView;
    float4x4 gProj;
    float4   gCamPos;
    float4   gLightDir;
    float4   gLightColor;
    float4   gAmbient;
    float4   gFogColor;
    float4   gFogParams;   // x=start y=end z=max w=height fog
    float4   gTime;
    float4   gPLPos[16];   // xyz=pos w=range
    float4   gPLCol[16];
    int4     gPLCount;
};

cbuffer ObjectCB : register(b1)
{
    float4x4 gWorld;
    float4   gColor;
    float4   gUV;          // xy=scale zw=offset
    float4   gEmissive;
    float4   gFlags;       // x=useTex y=unlit z=alphaClip w=outline width
    float4   gFlags2;      // x=rim y=worldUV z=fog amount
};

Texture2D    gTex  : register(t0);
SamplerState gSamp : register(s0);

struct VSIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv  : TEXCOORD0;
};

struct VSOut
{
    float4 pos  : SV_POSITION;
    float3 wpos : TEXCOORD0;
    float3 nrm  : TEXCOORD1;
    float2 uv   : TEXCOORD2;
};

VSOut VSMain(VSIn i)
{
    VSOut o;
    float4 wp = mul(float4(i.pos, 1), gWorld);
    o.wpos = wp.xyz;
    o.pos  = mul(mul(wp, gView), gProj);
    o.nrm  = normalize(mul(float4(i.nrm, 0), gWorld).xyz);
    float2 uv = (gFlags2.y > 0.5) ? wp.xz : i.uv;
    o.uv = uv * gUV.xy + gUV.zw;
    return o;
}

float FogFactor(float3 wpos)
{
    float dist = length(gCamPos.xyz - wpos);
    float f  = saturate((dist - gFogParams.x) / max(gFogParams.y - gFogParams.x, 0.001)) * gFogParams.z;
    float hf = saturate(1.0 - wpos.y / 4.0) * gFogParams.w * saturate(dist / 25.0);
    return saturate(max(f, hf)) * gFlags2.z;
}

float4 PSMain(VSOut i) : SV_TARGET
{
    float4 base = gColor;
    if (gFlags.x > 0.5) base *= gTex.Sample(gSamp, i.uv);
    if (gFlags.z > 0.5) clip(base.a - 0.45);

    float3 col;
    if (gFlags.y > 0.5)
    {
        col = base.rgb;
    }
    else
    {
        float3 n = normalize(i.nrm);
        float3 L = normalize(-gLightDir.xyz);
        float ndl = dot(n, L);
        // 3段階のトゥーン陰影
        float band = smoothstep(-0.05, 0.05, ndl) * 0.55 + smoothstep(0.45, 0.52, ndl) * 0.45;
        float3 light = gAmbient.rgb + gLightColor.rgb * band;

        [loop]
        for (int k = 0; k < gPLCount.x; k++)
        {
            float3 d = gPLPos[k].xyz - i.wpos;
            float dist = length(d);
            float att = saturate(1.0 - dist / gPLPos[k].w);
            att *= att;
            float pn = saturate(dot(n, d / max(dist, 0.001)) * 0.65 + 0.35);
            light += gPLCol[k].rgb * att * pn;
        }

        float3 V = normalize(gCamPos.xyz - i.wpos);
        float rim = pow(1.0 - saturate(dot(n, V)), 3.0) * gFlags2.x;
        col = base.rgb * light + rim * (gLightColor.rgb * 0.6 + 0.2);
    }
    col += gEmissive.rgb;
    col = lerp(col, gFogColor.rgb, FogFactor(i.wpos));
    return float4(col, base.a);
}

// ---- 輪郭線（背面法） ----
VSOut VSOutline(VSIn i)
{
    VSOut o;
    float4 wp = mul(float4(i.pos, 1), gWorld);
    float3 wn = normalize(mul(float4(i.nrm, 0), gWorld).xyz);
    float d = length(gCamPos.xyz - wp.xyz);
    wp.xyz += wn * gFlags.w * (0.7 + d * 0.035);
    o.wpos = wp.xyz;
    o.pos  = mul(mul(wp, gView), gProj);
    o.nrm  = wn;
    o.uv   = i.uv;
    return o;
}

float4 PSOutline(VSOut i) : SV_TARGET
{
    float3 c = float3(0.13, 0.10, 0.12);
    c = lerp(c, gFogColor.rgb, FogFactor(i.wpos));
    return float4(c, 1);
}
