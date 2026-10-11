// Actual DXR triangle traversal. Spectral paths refract at the original shell
// interfaces and deposit flux on the floor; color is never painted by screen UV.
cbuffer Frame : register(b0){
 row_major float4x4 viewProjection,previousViewProjection,inverseViewProjection,lightViewProjection;
 float4 cameraNight,selectedDetail,dimensionsTime,focusQuality,glassRectangle,glassPointer,reflectionSettings,spotPosition,spotAim,resourceQuality;
 float4 rayDimensions; // reflection width/height, frame, photon grid side
 float4 causticRegion; // center X/Z, world extent, floor geometry range index
};
struct Vertex{float3 position,normal;float2 uv;};
struct Instance{row_major float4x4 world,previous;float4 uv,data;};
struct Range{uint vertex,index,count,material;float4 baseColor,finish;};
RaytracingAccelerationStructure scene : register(t0);
Texture2D<float> depth : register(t1);
Texture2D<float4> normals : register(t2);
Texture2D<float4> opaque : register(t3);
StructuredBuffer<Vertex> vertices : register(t4);
StructuredBuffer<uint> indices : register(t5);
StructuredBuffer<Range> ranges : register(t6);
StructuredBuffer<Instance> instances : register(t7);
Texture2D<float4> atlas : register(t8);
Texture2DArray<float4> covers : register(t9);
Texture2D<float2> motion : register(t10);
Texture2D<float4> previousReflection : register(t11);
TextureCube<float4> studioEnvironment : register(t12);
SamplerState linearClamp : register(s0);
RWTexture2D<float4> reflection : register(u0);
RWTexture2D<uint> photons : register(u1);
RWTexture2D<float4> caustics : register(u2);
RWTexture2D<float4> color : register(u3);
float3 linearRGB(float3 c){return float3(c.r<=.04045?c.r/12.92:pow((c.r+.055)/1.055,2.4),c.g<=.04045?c.g/12.92:pow((c.g+.055)/1.055,2.4),c.b<=.04045?c.b/12.92:pow((c.b+.055)/1.055,2.4));}
float3 environment(float3 direction,float rough){return studioEnvironment.SampleLevel(linearClamp,direction,rough*7).rgb*(resourceQuality.z>.5||resourceQuality.w>.5?lerp(.48,.68,cameraNight.w):lerp(.25,.16,cameraNight.w));}
float3 world(float2 uv,float z){float4 p=mul(float4(uv*float2(2,-2)+float2(-1,1),z,1),inverseViewProjection);return p.xyz/p.w;}
#include "disc.hlsli"
struct Hit{float3 position,normal;float2 uv;uint instance,material,geometry;bool front;};
bool trace(float3 origin,float3 direction,out Hit hit){
 RayDesc ray;ray.Origin=origin;ray.Direction=direction;ray.TMin=.0008;ray.TMax=200;
 RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;query.TraceRayInline(scene,RAY_FLAG_NONE,0xff,ray);while(query.Proceed()){
  if(query.CandidateType()!=CANDIDATE_NON_OPAQUE_TRIANGLE)continue;uint raw=query.CandidateInstanceID(),index=raw&0xffff,geometry=raw==0xffff?(uint)causticRegion.w:raw>>16?(raw>>16)-1:query.CandidateGeometryIndex();Range range=ranges[geometry];bool keep=true;
  if(range.material==3&&index!=0xffff&&instances[index].data.x<-.5){uint base=range.index+query.CandidatePrimitiveIndex()*3;float2 bary=query.CandidateTriangleBarycentrics(),uv=vertices[range.vertex+indices[base]].uv*(1-bary.x-bary.y)+vertices[range.vertex+indices[base+1]].uv*bary.x+vertices[range.vertex+indices[base+2]].uv*bary.y;float radius=length(uv-.5);keep=radius<=.46&&radius>=.060;}
  if(keep)query.CommitNonOpaqueTriangleHit();
 }
 if(query.CommittedStatus()!=COMMITTED_TRIANGLE_HIT){hit=(Hit)0;return false;}
 uint rawInstance=query.CommittedInstanceID(),instance=rawInstance&0xffff,geometry=rawInstance==0xffff?(uint)causticRegion.w:rawInstance>>16?(rawInstance>>16)-1:query.CommittedGeometryIndex();Range range=ranges[geometry];uint base=range.index+query.CommittedPrimitiveIndex()*3;Vertex a=vertices[range.vertex+indices[base]],b=vertices[range.vertex+indices[base+1]],c=vertices[range.vertex+indices[base+2]];float2 bary=query.CommittedTriangleBarycentrics();float3 n=a.normal*(1-bary.x-bary.y)+b.normal*bary.x+c.normal*bary.y;hit.position=origin+direction*query.CommittedRayT();hit.uv=a.uv*(1-bary.x-bary.y)+b.uv*bary.x+c.uv*bary.y;hit.normal=instance==0xffff?normalize(n):normalize(mul(float4(n,0),instances[instance].world).xyz);hit.instance=instance;hit.material=range.material;hit.geometry=geometry;hit.front=dot(direction,hit.normal)<0;return true;
}
float3 diffuse(float3 p,float3 N){float night=cameraNight.w;float3 hemi=lerp(linearRGB(float3(.706,.631,.549)),linearRGB(float3(1,.98,.961)),N.y*.5+.5)*lerp(.32,.21,night);float3 key=lerp(linearRGB(float3(1,.969,.929)),linearRGB(float3(.898,.941,1)),night)*lerp(.45,.30,night)*saturate(dot(N,normalize(float3(-6,14,-5))));float3 light=spotPosition.xyz-p;float d2=max(.01,dot(light,light));light*=rsqrt(d2);float cone=smoothstep(cos(.32),cos(.016),dot(light,normalize(spotPosition.xyz-spotAim.xyz)));return (hemi+key+.045*saturate(dot(N,normalize(float3(7,8,-10))))+lerp(linearRGB(float3(1,.89,.698)),linearRGB(float3(.859,.914,1)),night)*spotPosition.w/d2*cone*saturate(dot(N,light)))/3.14159265;}
float3 surfaceColor(Hit hit){
 float3 N=hit.normal;float night=cameraNight.w;if(hit.material==4)return lerp(linearRGB(float3(.847,.788,.725)),linearRGB(float3(.043,.094,.157)),night)*diffuse(hit.position,N);
 if(hit.material>=5){Range range=ranges[hit.geometry];float3 V=normalize(cameraNight.xyz-hit.position);return range.baseColor.rgb*diffuse(hit.position,N)*(1-range.finish.y)+environment(reflect(-V,N),range.finish.x)*lerp(.04,range.baseColor.rgb,range.finish.y);}
 Instance instance=instances[hit.instance];if(hit.material==3&&instance.data.x<-.5){float2 uv=discUv(hit.uv,instance.data.x);float3 base=instance.data.z>=0?covers.SampleLevel(linearClamp,float3(uv,instance.data.z),0).rgb:atlas.SampleLevel(linearClamp,instance.uv.xy+uv*instance.uv.zw,0).rgb;float3 V=normalize(cameraNight.xyz-hit.position);if(dot(N,V)<0)N=-N;return linearRGB(base)*lerp(.34,.25,night)+environment(reflect(-V,N),.12)*.68+discSpectrum(hit.uv,N,V,normalize(float3(-6,14,9)),true);}if(hit.material==3){float3 c=instance.data.z>=0?covers.SampleLevel(linearClamp,float3(hit.uv,instance.data.z),0).rgb:atlas.SampleLevel(linearClamp,instance.uv.xy+hit.uv*instance.uv.zw,0).rgb;float3 origin=mul(float4(0,0,0,1),instance.world).xyz;float3 delta=(origin-selectedDetail.xyz)/float3(6,3.35,5.5);return linearRGB(c)*min(1,diffuse(hit.position,N)+studioEnvironment.SampleLevel(linearClamp,N,7).rgb*(resourceQuality.z>.5?lerp(.48,.68,night):lerp(.25,.16,night)))*lerp(lerp(.5,.12,night),1,exp(-dot(delta,delta)));}
 float3 local=mul(float4(hit.position,1),instance.world).xyz;return environment(reflect(normalize(hit.position-cameraNight.xyz),N),.4)*.65;
}
uint random(uint value){value^=value>>16;value*=0x7feb352d;value^=value>>15;value*=0x846ca68b;return value^(value>>16);}
float noise(uint value){return (random(value)&0xffffff)/16777216.0;}
float3 rayColor(float3 p,float3 direction,float ior){float3 throughput=1,result=0;[loop]for(int bounce=0;bounce<6;++bounce){Hit hit;if(!trace(p,direction,hit))return result+throughput*environment(direction,.4);if(hit.material>=3)return result+throughput*surfaceColor(hit);float3 N=hit.front?hit.normal:-hit.normal;float f=.035+.965*pow(1-saturate(dot(-direction,N)),5);result+=throughput*environment(reflect(direction,N),.4)*f;float3 transmitted=refract(direction,N,hit.front?1/ior:ior);if(dot(transmitted,transmitted)<.001)direction=reflect(direction,N);else{direction=normalize(transmitted);throughput*=1-f;}p=hit.position+direction*.0016;}return result+throughput*environment(direction,.4);}
float3 spectralReflection(float3 p,float3 direction){
 // Three primary reflectance bands share the same rough direction. Each band
 // traces glass interfaces with its own Cauchy index instead of shifting UVs.
 float3 wavelengths=float3(.650,.535,.450);float3 ior=1.437+.0044/(wavelengths*wavelengths);
 return float3(rayColor(p,direction,ior.r).r,rayColor(p,direction,ior.g).g,rayColor(p,direction,ior.b).b);
}
[numthreads(8,8,1)]void ReflectionCS(uint3 id:SV_DispatchThreadID){
 if(any(id.xy>=(uint2)rayDimensions.xy))return;float2 uv=(id.xy+.5)/rayDimensions.xy;float4 normal=normals.SampleLevel(linearClamp,uv,0);float z=depth.SampleLevel(linearClamp,uv,0);if(z>=.999999||(normal.w>=2.5&&normal.w!=6)){reflection[id.xy]=float4(0,0,0,-1);return;}
 float3 p=world(uv,z),N=normalize(normal.xyz*2-1),V=normalize(cameraNight.xyz-p);if(dot(N,V)<0)N=-N;float3 R=reflect(-V,N);
 // Polished CD tracks already have an analytic diffraction lobe. A jittered
 // glass cone adds unrelated white sparkle to this smooth metallic surface.
 // Trace its actual mirror direction; keep the original rough glass sampling.
 if(normal.w!=6){float rough=lerp(.44,.30,cameraNight.w);uint seed=(uint)rayDimensions.z*1664525;float u=noise(id.x+id.y*(uint)rayDimensions.x+seed),v=noise(id.x*7193+id.y*881+seed);float3 tangent=normalize(cross(abs(R.y)<.98?float3(0,1,0):float3(1,0,0),R)),bitangent=cross(R,tangent);R=normalize(R+(tangent*cos(u*6.2831853)+bitangent*sin(u*6.2831853))*sqrt(v)*rough*.16);}
 float3 traced=spectralReflection(p+N*.004,R);
 float2 previousUv=uv+motion.SampleLevel(linearClamp,uv,0)/dimensionsTime.xy;float4 history=previousReflection.SampleLevel(linearClamp,previousUv,0);float4 previousClip=mul(float4(p,1),previousViewProjection);uint material=(uint)round(normal.w),previousMaterial=(uint)max(0,floor(history.a/2));float previousDepth=history.a-previousMaterial*2;
 if(reflectionSettings.w==0&&all(previousUv>0)&&all(previousUv<1)&&history.a>=0&&material==previousMaterial&&abs(previousDepth-previousClip.z/previousClip.w)<.0008){float3 lower=max(0,traced-float3(.12,.12,.12)),upper=traced+float3(.12,.12,.12);traced=lerp(traced,clamp(history.rgb,lower,upper),.86);}
 reflection[id.xy]=float4(traced,z+material*2);
}
[numthreads(8,8,1)]void ClearPhotonsCS(uint3 id:SV_DispatchThreadID){if(id.x<512&&id.y<1536)photons[id.xy]=0;}
float3 spectralRGB(uint index){const float3 samples[6]={float3(.25,0,1),float3(0,.7,1),float3(0,1,.1),float3(1,.8,0),float3(1,.1,0),float3(.65,0,0)};return samples[index];}
[numthreads(8,8,1)]void PhotonCS(uint3 id:SV_DispatchThreadID){uint side=(uint)rayDimensions.w;if(any(id.xy>=side))return;uint index=id.x+id.y*side,spectral=index%6;float wavelength=(440+48*spectral)*.001,ior=1.437+.0044/(wavelength*wavelength);float2 uv=(float2(id.xy)+float2(noise(index+97),noise(index+269)))/side;uint count,stride;instances.GetDimensions(count,stride); // selected is the last CPU instance, encoded below
 uint selected=(uint)reflectionSettings.z;Instance box=instances[selected];float3 target=mul(float4(lerp(-2.225,2.225,uv.x),lerp(.175,3.525,uv.y),0,1),box.world).xyz;float3 sun=normalize(float3(-6,14,-5)),p=target+sun*40,direction=-sun;float energy=1;bool glass=false;
 [loop]for(int bounce=0;bounce<8;++bounce){Hit hit;if(!trace(p,direction,hit))return;if(hit.material==4){if(!glass)return;float2 pixel=((hit.position.xz-causticRegion.xy)/causticRegion.z+.5)*512-.5;if(any(pixel<0)||any(pixel>=511))return;uint2 q=(uint2)pixel;float2 fraction=frac(pixel);float3 flux=spectralRGB(spectral)*energy*lerp(.45,.30,cameraNight.w)*14.9075*(512*512)/(causticRegion.z*causticRegion.z*side*side)*2.0;[unroll]for(uint y=0;y<2;++y)[unroll]for(uint x=0;x<2;++x){float weight=(x?fraction.x:1-fraction.x)*(y?fraction.y:1-fraction.y);[unroll]for(uint c=0;c<3;++c)InterlockedAdd(photons[uint2(q.x+x,q.y+y+c*512)],(uint)round(min(8,flux[c])*weight*4096));}return;}if(hit.material==3&&instances[hit.instance].data.x<-.5){
     float3 N=hit.front?hit.normal:-hit.normal,T=normalize(cross(abs(N.y)<.95?float3(0,1,0):float3(1,0,0),N)),B=cross(N,T),radial=normalize(T*(hit.uv.x-.5)-B*(hit.uv.y-.5));
     float order=(index&1)?1:-1;float3 tangent=direction-N*dot(direction,N)+radial*(order*wavelength/1.6);float energySquared=dot(tangent,tangent);if(energySquared>=1)return;direction=normalize(tangent+N*sqrt(1-energySquared));energy*=.52;glass=true;p=hit.position+direction*.0016;continue;
 }if(hit.material>=3)return;
     if(hit.material==1&&instances[hit.instance].data.x>.5)return;glass=true;float3 N=hit.front?hit.normal:-hit.normal;float f=.035+.965*pow(1-saturate(dot(-direction,N)),5);float3 transmitted=refract(direction,N,hit.front?1/ior:ior);if(dot(transmitted,transmitted)<.001){direction=reflect(direction,N);energy*=.96;}else{direction=normalize(transmitted);energy*=1-f;}energy*=hit.material==0?.98:hit.material==1?.95:.94;p=hit.position+direction*.0016;
 }
}
[numthreads(8,8,1)]void ResolveCausticCS(uint3 id:SV_DispatchThreadID){if(any(id.xy>=512))return;float3 value=0;float weight=0;[unroll]for(int y=-2;y<=2;++y)[unroll]for(int x=-2;x<=2;++x){int2 q=clamp(int2(id.xy)+int2(x,y),0,511);float w=exp(-(x*x+y*y)*.7);[unroll]for(int c=0;c<3;++c)value[c]+=photons[int2(q.x,q.y+c*512)]/4096.0*w;weight+=w;}caustics[id.xy]=float4(value/weight,1);}
void reflectionTap(int2 center,int2 offset,float2 fraction,float3 N,float z,bool disc,inout float3 sum,inout float weight){
 int2 q=clamp(center+offset,int2(0,0),(int2)rayDimensions.xy-1);float4 ray=reflection[q];float2 tapUv=(q+.5)/rayDimensions.xy;float4 tapNormal=normals.SampleLevel(linearClamp,tapUv,0);float tapDepth=depth.SampleLevel(linearClamp,tapUv,0);
 float2 d=float2(offset)-fraction;float w=exp(-dot(d,d)*(disc?.32:.8))*step(0,ray.a)*pow(saturate(dot(N,normalize(tapNormal.xyz*2-1))),12)*exp(-abs(tapDepth-z)*4000);
 if(disc)w*=step(abs(tapNormal.w-6),.05);sum+=ray.rgb*w;weight+=w;
}
[numthreads(8,8,1)]void CompositeCS(uint3 id:SV_DispatchThreadID){if(any(id.xy>=(uint2)dimensionsTime.xy))return;float2 uv=(id.xy+.5)/dimensionsTime.xy;float4 normal=normals.Load(int3(id.xy,0));float z=depth.Load(int3(id.xy,0));if(z>=.999999)return;float3 p=world(uv,z);float3 current=color[id.xy].rgb;
 if(normal.w<2.5||normal.w==6){float3 N=normalize(normal.xyz*2-1),V=normalize(cameraNight.xyz-p);if(dot(N,V)<0)N=-N;float2 samplePoint=uv*rayDimensions.xy-.5;int2 center=int2(floor(samplePoint));float2 fraction=frac(samplePoint);float3 traced=0;float weight=0;
     // A bounded footprint softens the metal's reflected shelf without extra
     // ray traversal or bleeding glass/background taps across the disc hole.
     if(normal.w==6){[unroll]for(int y=-2;y<=2;++y)[unroll]for(int x=-2;x<=2;++x)reflectionTap(center,int2(x,y),fraction,N,z,true,traced,weight);}
     else{[unroll]for(int y=-1;y<=1;++y)[unroll]for(int x=-1;x<=1;++x)reflectionTap(center,int2(x,y),fraction,N,z,false,traced,weight);}
     if(weight>0){float f=normal.w==6?.85:.035+.965*pow(1-saturate(dot(N,V)),5);float3 base=environment(reflect(-V,N),normal.w==6?.12:lerp(.44,.30,cameraNight.w));current+=(traced/weight-base)*f*.65;}}
 else if(normal.w>3.5&&normal.w<4.5){float2 tex=((p.xz-causticRegion.xy)/causticRegion.z+.5)*512-.5;if(all(tex>=0)&&all(tex<511)){int2 q=int2(floor(tex));float2 t=frac(tex);float3 flux=lerp(lerp(caustics[q].rgb,caustics[q+int2(1,0)].rgb,t.x),lerp(caustics[q+int2(0,1)].rgb,caustics[q+int2(1,1)].rgb,t.x),t.y);current+=flux;}}
 color[id.xy]=float4(max(0,current),1);
}
