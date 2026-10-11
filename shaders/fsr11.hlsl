// GPUOpen FSR 1 EASU/RCAS: compatibility for DX11 feature level 11.0.
// HDR tone mapping precedes spatial reconstruction; the native UI follows it.
#define A_GPU 1
#define A_HLSL 1
#define FSR_EASU_F 1
#define FSR_RCAS_F 1
#include "../vendor/fsr1/ffx-fsr/ffx_a.h"
Texture2D<float4> source : register(t2);
SamplerState inputSampler : register(s0);
cbuffer FsrConstants : register(b2){uint4 con0,con1,con2,con3,rcas;};
AF4 FsrEasuRF(AF2 p){return source.GatherRed(inputSampler,p);}
AF4 FsrEasuGF(AF2 p){return source.GatherGreen(inputSampler,p);}
AF4 FsrEasuBF(AF2 p){return source.GatherBlue(inputSampler,p);}
AF4 FsrRcasLoadF(ASU2 p){uint w,h;source.GetDimensions(w,h);return source.Load(int3(clamp(p,int2(0,0),int2(w,h)-1),0));}
void FsrRcasInputF(inout AF1 r,inout AF1 g,inout AF1 b){}
#include "../vendor/fsr1/ffx-fsr/ffx_fsr1.h"
float4 EasuPS(float4 position:SV_Position):SV_Target{float3 value;FsrEasuF(value,uint2(position.xy),con0,con1,con2,con3);return float4(value,1);}
float4 RcasPS(float4 position:SV_Position):SV_Target{float3 value;FsrRcasF(value.r,value.g,value.b,uint2(position.xy),rcas);return float4(value,1);}
