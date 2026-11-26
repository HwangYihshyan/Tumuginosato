// ツムギノサト - トゥーンシェーダ（DirectX 11 / SM5.0）
//
// このファイルには4つの入口（エントリポイント）がある。Renderer.cpp の Init で1つずつコンパイルしている
//   VSMain    / PSMain    ：ふつうの描画（トゥーンの陰影・点光源・リム・ハイライト・フォグ）
//   VSOutline / PSOutline ：輪郭線（背面法。モデルを法線方向にふくらませ、裏面だけを黒く塗る）
//
// 定数バッファは C++ 側（Renderer.cpp の FrameCB / ObjectCB）と「同じ順番・同じ大きさ」で並べること。
// 1行は 16 バイト（float4）単位でそろうので、float4 / float4x4 / int4 だけを使って、ずれが出ないようにしている。

// ------------------------------------------------------------------
// フレームごとに1回だけ書きかえる値（カメラ・光・霧など）
// ------------------------------------------------------------------
cbuffer FrameCB : register(b0)
{
    float4x4 gView;        // ビュー行列（ワールド → カメラ）
    float4x4 gProj;        // 射影行列（カメラ → 画面）
    float4   gCamPos;      // カメラの位置（リム・ハイライト・フォグの距離に使う）
    float4   gLightDir;    // 平行光の向き（光が進む向き。面から光源へ向かう向きはこの逆）
    float4   gLightColor;  // 平行光の色
    float4   gAmbient;     // 環境光（影の中の最低限の明るさ）
    float4   gFogColor;
    float4   gFogParams;   // x=start y=end z=max w=height fog
    float4   gTime;
    float4   gPLPos[16];   // 点光源：xyz=pos w=range（届く距離）
    float4   gPLCol[16];   // 点光源の色
    int4     gPLCount;     // x = 使う点光源の数（カメラの注視点に近い順に最大16個）
    float4   gShadowColor; // 影の部分にかける色（1,1,1 なら色なし）。場面ごとに FrameSettings.shadowColor で決める
};

// ------------------------------------------------------------------
// 描くものごと（Draw 1回ごと）に書きかえる値
// ------------------------------------------------------------------
cbuffer ObjectCB : register(b1)
{
    float4x4 gWorld;       // モデル → ワールド
    float4x4 gWorldIT;     // 法線用（world の逆転置）。縦横の倍率がちがっても法線がゆがまない
    float4   gColor;       // 基本の色（テクスチャがあれば掛け合わせる）
    float4   gUV;          // xy=scale zw=offset
    float4   gEmissive;    // 発光（光に関係なく足す色。ダメージの点滅や光る目など）
    float4   gFlags;       // x=useTex y=unlit z=alphaClip w=outline width
    float4   gFlags2;      // x=rim y=worldUV z=fog amount w=specular（トゥーンのハイライト）
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
    float4 pos  : SV_POSITION;   // 画面上の位置
    float3 wpos : TEXCOORD0;     // ワールドの位置（点光源・フォグの距離に使う）
    float3 nrm  : TEXCOORD1;     // ワールドの法線
    float2 uv   : TEXCOORD2;
};

// ------------------------------------------------------------------
// 頂点シェーダ（ふつうの描画）
// ------------------------------------------------------------------
VSOut VSMain(VSIn i)
{
    VSOut o;
    float4 wp = mul(float4(i.pos, 1), gWorld);
    o.wpos = wp.xyz;
    o.pos  = mul(mul(wp, gView), gProj);
    // 法線は world ではなく「world の逆転置」で変換する。
    // world で変換すると、S(1, 0.5, 1) のように縦だけ縮めた球では法線が横に寝てしまい、
    // トゥーンの影の境目がずれる。逆転置なら倍率がちがっても面に垂直なまま。
    // w=0 にしているので、平行移動の成分は効かない（向きだけ変わる）。
    o.nrm  = normalize(mul(float4(i.nrm, 0), gWorldIT).xyz);
    // worldUV：床や地面のように、モデルの UV ではなくワールドの xz をそのまま UV にする（つなぎ目が出ない）
    float2 uv = (gFlags2.y > 0.5) ? wp.xz : i.uv;
    o.uv = uv * gUV.xy + gUV.zw;
    return o;
}

// 霧の濃さ（0〜1）。距離の霧と、低いところにたまる高さの霧の、濃いほうを使う
float FogFactor(float3 wpos)
{
    float dist = length(gCamPos.xyz - wpos);
    float f  = saturate((dist - gFogParams.x) / max(gFogParams.y - gFogParams.x, 0.001)) * gFogParams.z;
    float hf = saturate(1.0 - wpos.y / 4.0) * gFogParams.w * saturate(dist / 25.0);
    return saturate(max(f, hf)) * gFlags2.z;   // gFlags2.z：このオブジェクトが霧をどれだけ受けるか（UI の板などは 0）
}

// ------------------------------------------------------------------
// ピクセルシェーダ（ふつうの描画）
// ------------------------------------------------------------------
float4 PSMain(VSOut i) : SV_TARGET
{
    float4 base = gColor;
    if (gFlags.x > 0.5) base *= gTex.Sample(gSamp, i.uv);
    if (gFlags.z > 0.5) clip(base.a - 0.45);   // alphaClip：半分より透明なところは描かない（葉っぱなど）

    float3 col;
    if (gFlags.y > 0.5)
    {
        col = base.rgb;   // unlit：光を計算しない（エフェクト・ビルボード・光る部分）
    }
    else
    {
        float3 n = normalize(i.nrm);
        float3 L = normalize(-gLightDir.xyz);   // 面から光源へ向かう向き
        float ndl = dot(n, L);                  // 1=光の正面 0=真横 マイナス=裏側

        // ---- 3段階のトゥーン陰影 ----
        // ふつうのライティング（ndl をそのまま使う）だと明るさがなめらかに変わるが、
        // トゥーンでは smoothstep でほぼ段差にして「影・中間・明るい」の3段に分ける。
        //   ndl が 0 をこえると +0.55（影 → 中間）、0.5 をこえるとさらに +0.45（中間 → 明るい）
        // smoothstep の幅（-0.02〜0.02、0.47〜0.50）をせまくするほど、境目がくっきりする
        float band = smoothstep(-0.02, 0.02, ndl) * 0.55 + smoothstep(0.47, 0.50, ndl) * 0.45;
        float3 light = gAmbient.rgb + gLightColor.rgb * band;

        // ---- 影の色 ----
        // band=0（影）のときは gShadowColor を掛け、band=1（明るい）のときは 1 を掛ける（＝変わらない）。
        // 影を「ただ暗く」ではなく、青紫などの色に寄せるとアニメの塗りらしくなる
        light *= lerp(gShadowColor.rgb, 1.0, band);

        // ---- 点光源（灯・宝箱・泉など）----
        // 距離で弱まる（届く距離 w で 0）。2乗して、近くだけ明るく、遠くはすっと消えるようにする。
        // 裏側も 0.35 は照らす（*0.65+0.35）ので、光源のまわりがふんわり明るくなる
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

        // ---- リムライト ----
        // 視線と面が直角に近い（＝輪郭の近く）ほど明るくする。3乗してふちだけに細く出す
        float3 V = normalize(gCamPos.xyz - i.wpos);
        float rim = pow(1.0 - saturate(dot(n, V)), 3.0) * gFlags2.x;

        // ---- トゥーンのハイライト（金属の刃・針など。DrawParams.spec で強さを決める）----
        // H = 光の向きと視線のまん中の向き（ハーフベクトル）。法線が H に近いところが光って見える（Blinn-Phong）。
        // ふつうは pow でなめらかにするが、ここでは smoothstep で「くっきりした光の点」にする。
        //   0.90〜0.915 の数字を下げると点が大きく、上げると小さくなる
        // step(0, ndl)：光の当たっていない裏側には出さない
        float3 H = normalize(L + V);
        float spec = smoothstep(0.90, 0.915, dot(n, H)) * step(0.0, ndl) * gFlags2.w;

        col = base.rgb * light + rim * (gLightColor.rgb * 0.6 + 0.2) + spec * (gLightColor.rgb + 0.35);
    }
    col += gEmissive.rgb;
    col = lerp(col, gFogColor.rgb, FogFactor(i.wpos));
    return float4(col, base.a);
}

// ------------------------------------------------------------------
// 輪郭線（背面法）
//   Renderer::Draw で、ふつうに描いたあと「表面を消して裏面だけ描く」設定にしてもう一度描く。
//   頂点を法線方向に押し出して少し大きくした裏面が、モデルのふちからはみ出して黒い線に見える
// ------------------------------------------------------------------
VSOut VSOutline(VSIn i)
{
    VSOut o;
    float4 wp = mul(float4(i.pos, 1), gWorld);
    float3 wn = normalize(mul(float4(i.nrm, 0), gWorldIT).xyz);
    // 押し出す量 = DrawParams.outline ×（0.9 + 距離 × 0.035）
    // 遠いものは画面上で小さくなるので、距離に応じて少し太くして、線の見た目の太さをそろえる
    float d = length(gCamPos.xyz - wp.xyz);
    wp.xyz += wn * gFlags.w * (0.9 + d * 0.035);
    o.wpos = wp.xyz;
    o.pos  = mul(mul(wp, gView), gProj);
    o.nrm  = wn;
    o.uv   = i.uv;
    return o;
}

float4 PSOutline(VSOut i) : SV_TARGET
{
    float3 c = float3(0.07, 0.05, 0.07);                       // 墨のような濃い色（真っ黒より少し赤紫）
    c = lerp(c, gFogColor.rgb, FogFactor(i.wpos) * 0.75);      // 霧の中でも輪郭が消えすぎないよう、霧は 75% まで
    return float4(c, 1);
}
