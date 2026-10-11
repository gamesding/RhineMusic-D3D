cbuffer SmaaMetrics : register(b3) { float4 metrics; };
#define SMAA_RT_METRICS metrics
#define SMAA_HLSL_4_1 1
#define SMAA_PRESET_HIGH 1
#include "../vendor/smaa/SMAA.hlsl"
Texture2D inputColor : register(t0);
Texture2D edges : register(t1);
Texture2D area : register(t2);
Texture2D search : register(t3);
Texture2D blendWeights : register(t4);
struct EdgeVertex {float4 position:SV_Position; float2 uv:TEXCOORD0;float4 offsets[3]:TEXCOORD1;};
struct WeightVertex {float4 position:SV_Position; float2 uv:TEXCOORD0;float2 pixel:TEXCOORD1;float4 offsets[3]:TEXCOORD2;};
struct BlendVertex {float4 position:SV_Position;float2 uv:TEXCOORD0;float4 offset:TEXCOORD1;};
float2 screenUV(uint id){return float2((id<<1)&2,id&2);}
EdgeVertex EdgeVS(uint id:SV_VertexID){EdgeVertex v;v.uv=screenUV(id);v.position=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);SMAAEdgeDetectionVS(v.uv,v.offsets);return v;}
WeightVertex WeightVS(uint id:SV_VertexID){WeightVertex v;v.uv=screenUV(id);v.position=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);SMAABlendingWeightCalculationVS(v.uv,v.pixel,v.offsets);return v;}
BlendVertex BlendVS(uint id:SV_VertexID){BlendVertex v;v.uv=screenUV(id);v.position=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);SMAANeighborhoodBlendingVS(v.uv,v.offset);return v;}
float2 EdgePS(EdgeVertex v):SV_Target{return SMAAColorEdgeDetectionPS(v.uv,v.offsets,inputColor);}
float4 WeightPS(WeightVertex v):SV_Target{return SMAABlendingWeightCalculationPS(v.uv,v.pixel,v.offsets,edges,area,search,0);}
float4 BlendPS(BlendVertex v):SV_Target{return SMAANeighborhoodBlendingPS(v.uv,v.offset,inputColor,blendWeights);}
