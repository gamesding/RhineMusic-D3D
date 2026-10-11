// Native port of the original music shell, print-light field and Liquid Glass.
// Geometry and texture proportions are shared by the DX11 and DX12 paths.
cbuffer Frame : register(b0) {
 row_major float4x4 viewProjection;
 row_major float4x4 previousViewProjection;
 row_major float4x4 inverseViewProjection;
 row_major float4x4 lightViewProjection;
 float4 cameraNight;
 float4 selectedDetail;
 float4 dimensionsTime;
 float4 focusQuality;
 float4 glassRectangle;
 float4 glassPointer;
 float4 reflectionSettings;
 float4 spotPosition;
 float4 spotAim;
 float4 resourceQuality;
};
cbuffer Draw : register(b1) {float4 material,materialColor,materialFinish,materialAttenuation,materialEmission;};
struct Instance {row_major float4x4 world;row_major float4x4 previous;float4 uv;float4 data;};
StructuredBuffer<Instance> instances : register(t0);
Texture2D<float4> atlas : register(t0);
Texture2DArray<float4> covers : register(t1);
Texture2D<float4> sourceColor : register(t2);
Texture2D<float> sceneDepth : register(t3);
Texture2D<float4> sceneNormal : register(t4);
Texture2D<float4> rayReflection : register(t5);
Texture2D<float4> caustics : register(t6);
Texture2D<float> shadowDepth : register(t7);
TextureCube<float4> studioEnvironment : register(t8);
SamplerState linearClamp : register(s0);
SamplerState coverSampler : register(s1);
SamplerComparisonState shadowSampler : register(s2);
struct GeometryIn {float3 position:POSITION;float3 normal:NORMAL;float2 uv:TEXCOORD0;uint instance:SV_InstanceID;};
struct GeometryOut {float4 position:SV_Position;float3 world:TEXCOORD0;float3 normal:TEXCOORD1;float3 local:TEXCOORD2;float3 origin:TEXCOORD3;float2 uv:TEXCOORD4;float4 coverUV:TEXCOORD5;float4 data:TEXCOORD6;float4 clip:TEXCOORD7;float4 previousClip:TEXCOORD8;};
GeometryOut SceneVS(GeometryIn vertex) {
 Instance item=instances[vertex.instance+(uint)material.y];GeometryOut result;
 float4 world=mul(float4(vertex.position,1),item.world);
 result.world=world.xyz;result.position=mul(world,viewProjection);if(item.data.w>0&&abs(item.data.w-material.z)>.1)result.position=float4(2,2,2,1);
 result.clip=result.position;result.previousClip=mul(mul(float4(vertex.position,1),item.previous),previousViewProjection);
 result.normal=normalize(mul(float4(vertex.normal,0),item.world).xyz);result.local=vertex.position;
 result.origin=mul(float4(0,0,0,1),item.world).xyz;result.uv=vertex.uv;result.coverUV=item.uv;result.data=item.data;return result;
}
float4 ShadowVS(GeometryIn vertex):SV_Position{Instance item=instances[vertex.instance+(uint)material.y];if(item.data.w>0&&abs(item.data.w-material.z)>.1)return float4(2,2,2,1);return mul(mul(float4(vertex.position,1),item.world),lightViewProjection);}
struct ScreenOut {float4 position:SV_Position;float2 uv:TEXCOORD0;};
ScreenOut ScreenVS(uint id:SV_VertexID){ScreenOut result;result.uv=float2((id<<1)&2,id&2);result.position=float4(result.uv*float2(2,-2)+float2(-1,1),0,1);return result;}
float3 linearRGB(float3 value){return value<=.04045?value/12.92:pow((value+.055)/1.055,2.4);}
float3 srgb(float3 value){return value<=.0031308?value*12.92:1.055*pow(max(0,value),1/2.4)-.055;}
float3 filmic(float3 color){
 const float3x3 inputMat=float3x3(.59719,.35458,.04823,.07600,.90834,.01566,.02840,.13383,.83777);
 const float3x3 outputMat=float3x3(1.60475,-.53108,-.07367,-.10208,1.10813,-.00605,-.00327,-.07276,1.07602);
 color=mul(inputMat,color/.6);color=(color*(color+.0245786)-.000090537)/(color*(.983729*color+.4329510)+.238081);return saturate(mul(outputMat,color));
}
float3 environment(float3 direction,float roughness){return studioEnvironment.SampleLevel(linearClamp,direction,roughness*7).rgb*(resourceQuality.z>.5||resourceQuality.w>.5?lerp(.48,.68,cameraNight.w):lerp(.25,.16,cameraNight.w));}
float shadow(float3 world,float3 normal){if(focusQuality.w<1)return 1;float4 clip=mul(float4(world+normal*.035,1),lightViewProjection);float3 p=clip.xyz/clip.w;p.xy=p.xy*float2(.5,-.5)+.5;if(any(p.xy<0)||any(p.xy>1)||p.z<0||p.z>1)return 1;float sum=0;[unroll]for(int y=-1;y<=1;y++)[unroll]for(int x=-1;x<=1;x++)sum+=shadowDepth.SampleCmpLevelZero(shadowSampler,p.xy+float2(x,y)*4/focusQuality.w,p.z-.0003);return lerp(.68,1,sum/9);}
struct SurfaceOut {float4 color:SV_Target0;float2 motion:SV_Target1;float reactive:SV_Target2;float4 normal:SV_Target3;};
SurfaceOut baseSurface(GeometryOut input){SurfaceOut result;float2 current=input.clip.xy/input.clip.w*float2(.5,-.5)+.5,previous=input.previousClip.xy/input.previousClip.w*float2(.5,-.5)+.5;result.motion=(previous-current)*dimensionsTime.xy;result.reactive=0;result.normal=float4(normalize(input.normal)*.5+.5,material.x);result.color=0;return result;}
float guidedLight(float3 origin,out float laneLight,out float hotLight){float lane=abs(origin.x-selectedDetail.x),core=exp(-pow(lane/3.2,4)),spill=exp(-pow(lane/7.2,4));laneLight=.58*core+.42*spill;float row=(origin.z-selectedDetail.z)/5.5,neighbor=(origin.z-selectedDetail.z)/2.4;hotLight=exp(-pow((origin.z-selectedDetail.z)/1.1,2));return .58*core*lerp(.12,1,exp(-row*row))+.42*spill*exp(-neighbor*neighbor);}
float3 diffuseLight(float3 world,float3 normal){
 float night=cameraNight.w;float hemiWeight=normal.y*.5+.5;float3 hemisphere=lerp(linearRGB(float3(.706,.631,.549)),linearRGB(float3(1,.980,.961)),hemiWeight)*lerp(.32,.21,night);hemisphere=lerp(hemisphere,lerp(linearRGB(float3(.337,.439,.549)),linearRGB(float3(.886,.933,1)),hemiWeight)*.21,night);
 if(resourceQuality.z>.5||resourceQuality.w>.5)hemisphere=lerp(lerp(linearRGB(float3(.706,.631,.549)),linearRGB(float3(1,.980,.961)),hemiWeight)*.65,lerp(linearRGB(float3(.337,.439,.549)),linearRGB(float3(.886,.933,1)),hemiWeight)*.9,night);
 float3 keyColor=lerp(linearRGB(float3(1,.969,.929)),linearRGB(float3(.898,.941,1)),night);float3 key=keyColor*(resourceQuality.z>.5||resourceQuality.w>.5?lerp(1.4,1.7,night):lerp(.45,.30,night))*saturate(dot(normal,normalize(float3(-6,14,-5))))*shadow(world,normal);float3 fill=(resourceQuality.z>.5||resourceQuality.w>.5?.6:.045)*saturate(dot(normal,normalize(float3(7,8,-10))));
 float3 toSpot=spotPosition.xyz-world;float d2=max(.01,dot(toSpot,toSpot));float3 L=toSpot*rsqrt(d2),axis=normalize(spotPosition.xyz-spotAim.xyz);float cone=smoothstep(cos(.32),cos(.32*(1-.95)),dot(L,axis));float3 spotColor=lerp(linearRGB(float3(1,.890,.698)),linearRGB(float3(.859,.914,1)),night);float3 spot=spotColor*spotPosition.w*cone/d2*saturate(dot(normal,L));return (hemisphere+key+fill+spot)/3.14159265;
}
#include "disc.hlsli"
float3 ssr(float3 origin,float3 direction,float roughness,out float found);
float3 discColor(GeometryOut input,bool screenReflection){
 discClip(input.uv);float2 uv=discUv(input.uv,input.data.x);float3 base=input.data.z>=0?covers.Sample(coverSampler,float3(uv,input.data.z)).rgb:atlas.Sample(coverSampler,input.coverUV.xy+uv*input.coverUV.zw).rgb;float3 N=normalize(input.normal),V=normalize(cameraNight.xyz-input.world);if(dot(N,V)<0)N=-N;float3 reflected=environment(reflect(-V,N),.12);
 if(screenReflection&&reflectionSettings.x==1){float found;float3 traced=ssr(input.world+N*.025,reflect(-V,N),.12,found);reflected=lerp(reflected,traced,found*.8);}
 float3 output=linearRGB(base)*lerp(.34,.25,cameraNight.w)+reflected*.68;if(reflectionSettings.x>0)output+=discSpectrum(input.uv,N,V,normalize(float3(-6,14,9)),reflectionSettings.x>1);return output;
}
float3 printColor(GeometryOut input){
 float2 uv=saturate(input.uv);float4 sample=input.data.z>=0?covers.Sample(coverSampler,float3(uv,input.data.z)):atlas.Sample(coverSampler,input.coverUV.xy+uv*input.coverUV.zw);clip(sample.a-.025);float3 color=sample.rgb;
 float3 distance=(input.origin-selectedDetail.xyz)/float3(6,3.35,5.5);float local=exp(-dot(distance,distance));float3 normal=normalize(input.normal);float3 light=diffuseLight(input.world,normal)+studioEnvironment.SampleLevel(linearClamp,normal,7).rgb*(resourceQuality.z>.5?lerp(.48,.68,cameraNight.w):lerp(.25,.16,cameraNight.w));if(resourceQuality.w>.5)return linearRGB(color);if(resourceQuality.z>.5)return linearRGB(color)*min(1,light);return linearRGB(color)*min(1,light)*lerp(lerp(.5,.12,cameraNight.w),1,local);
}
SurfaceOut OpaquePS(GeometryOut input){SurfaceOut result=baseSurface(input);float3 normal=normalize(input.normal);
 if(material.x==3){if(input.data.x<-.5){result.color=float4(discColor(input,false),1);result.normal.w=6;result.reactive=.88;}else{result.color=float4(printColor(input),1);result.normal.w=3;}}
 else if(material.x>=5){float3 V=normalize(cameraNight.xyz-input.world);float metal=materialFinish.y;float3 base=materialColor.rgb,F0=lerp(.04,base,metal),fresnel=F0+(1-F0)*pow(1-saturate(dot(normal,V)),5);result.color=float4(base*diffuseLight(input.world,normal)*(1-metal)+environment(reflect(-V,normal),materialFinish.x)*fresnel+materialEmission.rgb,materialColor.a);result.normal.w=5;}
 else {float3 day=linearRGB(float3(.847,.788,.725)),night=linearRGB(float3(.043,.094,.157));float key=saturate(dot(normal,normalize(float3(-6,14,-5))));float light=lerp(.32,.21,cameraNight.w)+key*lerp(.45,.30,cameraNight.w);result.color=float4(lerp(day,night,cameraNight.w)*light*shadow(input.world,normal),1);result.normal.w=4;}
 float distance=length(input.world-cameraNight.xyz),fog=resourceQuality.z>.5?saturate((distance-13.5)/13.):smoothstep(focusQuality.x+lerp(5,-1,selectedDetail.w),focusQuality.x+lerp(25,12,selectedDetail.w),distance);result.color.rgb=lerp(result.color.rgb,lerp(linearRGB(float3(.918,.898,.882)),linearRGB(float3(.0275,.0667,.1216)),cameraNight.w),fog);return result;
}
float3 reconstruct(float2 uv,float depth){float4 world=mul(float4(uv*float2(2,-2)+float2(-1,1),depth,1),inverseViewProjection);return world.xyz/world.w;}
float3 ssr(float3 origin,float3 direction,float roughness,out float found){found=0;float3 color=0;float distance=.08;[loop]for(int step=0;step<32;step++){distance+=.12+step*.025;float3 p=origin+direction*distance;float4 clip=mul(float4(p,1),viewProjection);float2 uv=clip.xy/clip.w*float2(.5,-.5)+.5;if(any(uv<0)||any(uv>1)||clip.w<0)break;float depth=sceneDepth.SampleLevel(linearClamp,uv,0);float3 hit=reconstruct(uv,depth);float separation=length(p-cameraNight.xyz)-length(hit-cameraNight.xyz);if(separation>0&&separation<.12+roughness*.15){float edge=saturate(min(min(uv.x,uv.y),min(1-uv.x,1-uv.y))*12);found=edge;return sourceColor.SampleLevel(linearClamp,uv,0).rgb*edge;}}return color;}
SurfaceOut GlassPS(GeometryOut input){
 SurfaceOut result=baseSurface(input);if(material.x==3){clip(-.5-input.data.x);result.color=float4(discColor(input,true),1);result.normal.w=6;result.reactive=.88;return result;}float night=cameraNight.w,surface=material.x,clarity=saturate(input.data.y);float3 N=normalize(input.normal),V=normalize(cameraNight.xyz-input.world);if(dot(N,V)<0)N=-N;
 float transmission=surface==0?lerp(.88,.96,night):surface==1?lerp(.66,.84,night):lerp(.56,.66,night);
 float thickness=surface==0?lerp(.10,.026,night):surface==1?lerp(.10,.06,night):lerp(.07,.035,night);
 float rough=surface==0?lerp(lerp(.48,.4,night),lerp(.44,.3,night),clarity):surface==1?lerp(.34,.25,night):lerp(.46,.4,night);
 float3 day=surface==0?linearRGB(float3(.953,.941,.914)):surface==1?linearRGB(float3(.902,.867,.82)):linearRGB(float3(.933,.91,.875));float3 color=lerp(day,linearRGB(float3(1,.992,.98)),night);
 if(resourceQuality.w>.5){float quality=input.data.z<0?0:1;float reveal=surface==0?smoothstep(1-1.24*clarity,1-1.24*clarity+.24,saturate(input.local.y/3.7)):0;transmission=surface==0?lerp(lerp(.78,materialFinish.z,quality),.985,reveal):surface==1?lerp(0,materialFinish.z,quality):materialFinish.z;thickness=surface==0?lerp(materialFinish.w,.018,reveal):materialFinish.w;rough=surface==0?lerp(lerp(.28,.42,quality),.025,reveal):materialFinish.x;color=lerp(materialColor.rgb,linearRGB(float3(1,.992,.98)),night);if(surface==0&&quality==0){float height=smoothstep(.1,1,saturate(input.local.y/3.7));color*=lerp(lerp(float3(.40,.30,.20),float3(1,.98,.94),height),lerp(float3(.68,.76,.86),float3(1,1,1),height),night);}}
 float2 screen=input.position.xy/dimensionsTime.xy;float3 refracted=refract(-V,N,1/1.46);float4 displaced=mul(float4(input.world+refracted*thickness*8,1),viewProjection);float2 refUV=displaced.xy/displaced.w*float2(.5,-.5)+.5;
 float2 blur=rough*float2(3,3)/(dimensionsTime.xy*resourceQuality.y);float3 behind=sourceColor.Sample(linearClamp,refUV).rgb*.4;behind+=sourceColor.Sample(linearClamp,refUV+float2(blur.x,blur.y)).rgb*.15;behind+=sourceColor.Sample(linearClamp,refUV+float2(-blur.x,blur.y)).rgb*.15;behind+=sourceColor.Sample(linearClamp,refUV+float2(blur.x,-blur.y)).rgb*.15;behind+=sourceColor.Sample(linearClamp,refUV-blur).rgb*.15;
 float3 attenuation=pow(linearRGB(float3(.953,.914,.859)),thickness/lerp(1.2,4.5,night));float fresnel=.035+(.965)*pow(1-saturate(dot(N,V)),5);float3 reflection=environment(reflect(-V,N),rough);
 if(reflectionSettings.x==1){float found;float3 traced=ssr(input.world+N*.025,reflect(-V,N),rough,found);reflection=lerp(reflection,traced,found);}
 if(reflectionSettings.x>2){float4 traced=rayReflection.SampleLevel(linearClamp,screen,0);reflection=lerp(reflection,traced.rgb,traced.a);}
 float3 diffuse=color*(diffuseLight(input.world,N)+studioEnvironment.SampleLevel(linearClamp,N,7).rgb*lerp(.25,.16,night)*.65*.08);float3 output=lerp(diffuse,behind*attenuation,transmission)*(1-fresnel)+reflection*fresnel*.65+environment(reflect(-V,N),.2)*(.16*fresnel);
 float laneLight,hot;float guided=guidedLight(input.origin,laneLight,hot);if(resourceQuality.z<.5&&resourceQuality.w<.5)output*=lerp(.96,1.04,guided);float edge=exp(-max(0,input.local.x+2.225)*1.7),height=saturate((input.local.y-.175)/3.35),rim=exp(-max(0,3.525-input.local.y)*24),grazing=1-saturate(abs(dot(N,V)));
 float3 scatter=lerp(linearRGB(float3(1,.859,.639)),linearRGB(float3(.808,.898,1)),night)*lerp(1,.72,night);output+=scatter*guided*((surface==1?.30:.14)*edge+(surface==1?.016:.003))*lerp(1,.62,height);output+=scatter*laneLight*hot*(rim*(.22+.78*edge)+(surface==1?.18:.035)*edge*grazing)*.8;
 if(input.data.x>.5&&surface==1&&(input.local.x< -1.9703||input.local.x>=2.1685)){float3 gold=lerp(linearRGB(float3(.706,.502,.173)),linearRGB(float3(.867,.780,.486)),saturate((input.local.y-.175)/3.35));float metalFresnel=.68+.32*pow(1-saturate(dot(N,V)),5);output=gold*diffuse*(1-metalFresnel)+environment(reflect(-V,N),.24)*gold*metalFresnel+gold*.055;result.reactive=0;}
 float distance=length(input.world-cameraNight.xyz),fog=resourceQuality.z>.5?saturate((distance-13.5)/13.):smoothstep(focusQuality.x+lerp(5,-1,selectedDetail.w),focusQuality.x+lerp(25,12,selectedDetail.w),distance);output=lerp(output,lerp(linearRGB(float3(.918,.898,.882)),linearRGB(float3(.0275,.0667,.1216)),night),fog);
 result.color=float4(output,1);result.reactive=max(result.reactive,saturate(transmission*.8+fresnel*.2));return result;
}
float hash(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}
float4 BackgroundPS(ScreenOut input):SV_Target{return float4(lerp(linearRGB(float3(.918,.898,.882)),linearRGB(float3(.0275,.0667,.1216)),cameraNight.w),1);}
StructuredBuffer<float2> starPositions:register(t9);
float4 StarVS(uint vertex:SV_VertexID,uint star:SV_InstanceID):SV_Position{const float2 corners[6]={float2(-.5,-.5),float2(.5,-.5),float2(.5,.5),float2(-.5,-.5),float2(.5,.5),float2(-.5,.5)};float2 position=starPositions[star]+corners[vertex]*3*reflectionSettings.y/(dimensionsTime.xy*dimensionsTime.z);return float4(position,0,1);}
float4 StarPS():SV_Target{return float4(linearRGB(float3(219,234,255)/255.),.6*cameraNight.w);}
float4 CopyPS(ScreenOut input):SV_Target{return sourceColor.SampleLevel(linearClamp,input.uv,0);}
float2 AoPS(ScreenOut input):SV_Target {
 float2 uv=input.uv;float depth=sceneDepth.SampleLevel(linearClamp,uv,0);if(depth>=.999999)return float2(1,1e4);float3 p=reconstruct(uv,depth);float distance=length(p-cameraNight.xyz);float3 normal=sceneNormal.SampleLevel(linearClamp,uv,0).xyz*2-1;
 float occlusion=0;int count=min(64,(int)focusQuality.y);[loop]for(int i=0;i<count;i++){float angle=i*2.399963,scale=sqrt((i+.5)/max(1,count));float2 offset=float2(cos(angle),sin(angle))*scale*max(1,dimensionsTime.y*.18/max(distance,1));float2 sampleUV=uv+offset/dimensionsTime.xy;float d=sceneDepth.SampleLevel(linearClamp,sampleUV,0);float3 delta=reconstruct(sampleUV,d)-p;float len=length(delta);occlusion+=saturate(dot(normal,delta)/max(.001,len)-.08)*saturate(1-len/.18);}
 return float2(max(.78,1-occlusion/max(1,count)),distance);
}
float4 PostPS(ScreenOut input):SV_Target {
 float2 uv=input.uv;float depth=sceneDepth.SampleLevel(linearClamp,uv,0);float4 result=sourceColor.SampleLevel(linearClamp,uv,0);if(depth>=.999999)return result;float3 p=reconstruct(uv,depth);float distance=length(p-cameraNight.xyz);float3 normal=sceneNormal.SampleLevel(linearClamp,uv,0).xyz*2-1;
 if(focusQuality.y>0){float2 size=dimensionsTime.xy*resourceQuality.x,base=floor(uv*size-.5);float value=0,weight=0;[unroll]for(int ay=0;ay<2;ay++)[unroll]for(int ax=0;ax<2;ax++){float2 q=(base+float2(ax,ay)+.5)/size;float2 ao=rayReflection.SampleLevel(linearClamp,q,0).xy;float2 bilinear=1-abs(q-uv)*size;float w=max(0,bilinear.x)*max(0,bilinear.y)*exp(-abs(ao.y-distance)*24);value+=ao.x*w;weight+=w;}result.rgb*=weight>.00001?value/weight:1;}
 float aperture=lerp(.0003,.0008,selectedDetail.w)*focusQuality.z;float coc=clamp(abs(distance-focusQuality.x)*aperture*dimensionsTime.y,0,7);if(coc>.12){float3 sum=result.rgb*.25;float weight=.25;[unroll]for(int k=0;k<12;k++){float angle=k*2.399963;float2 shift=float2(cos(angle),sin(angle))*coc*sqrt((k+.5)/12)/dimensionsTime.xy;float d=sceneDepth.SampleLevel(linearClamp,uv+shift,0);float sampleDistance=length(reconstruct(uv+shift,d)-cameraNight.xyz);float w=(sampleDistance<distance-1? .025:.0625);sum+=sourceColor.SampleLevel(linearClamp,uv+shift,0).rgb*w;weight+=w;}result.rgb=sum/weight;}
 return result;
}
float roundBox(float2 p,float2 extent,float radius){float2 q=abs(p)-extent+radius;return min(max(q.x,q.y),0)+length(max(q,0))-radius;}
float4 FsrInputPS(ScreenOut input):SV_Target{return float4(srgb(filmic(max(0,sourceColor.SampleLevel(linearClamp,input.uv,0).rgb)*lerp(1.05,1.08,cameraNight.w))),1);}
float4 ViewerBlendPS(ScreenOut input):SV_Target{return float4(sourceColor.SampleLevel(linearClamp,input.uv,0).rgb,reflectionSettings.x);}
float4 FinalPS(ScreenOut input):SV_Target{
 float2 uv=input.uv;float3 color=sourceColor.SampleLevel(linearClamp,uv,0).rgb;if(reflectionSettings.y>0)color=linearRGB(color);float2 center=glassRectangle.xy,extent=glassRectangle.zw*.5;float radius=min(resourceQuality.x,min(extent.y,extent.x));float2 p=input.position.xy-center;float sdf=roundBox(p,extent,radius);
 if(glassPointer.w>0&&sdf<.8){float2 q=abs(p)-extent+radius;float2 N=normalize((q.x>0||q.y>0)?max(q,0)*sign(p):(q.x>q.y?float2(sign(p.x),0):float2(0,sign(p.y))));float edge=1-smoothstep(0,18,-sdf);float2 offset=(N*(7.5*edge+1.2*edge*edge)+p*.014)*glassPointer.w;float2 refracted=uv-offset/glassPointer.z/dimensionsTime.xy;float blur=1.8+1.2*(1-edge);float2 pixel=1/(dimensionsTime.xy*glassPointer.z);float3 transmitted=sourceColor.SampleLevel(linearClamp,refracted,0).rgb*.4;transmitted+=sourceColor.SampleLevel(linearClamp,refracted+float2(blur,0)*pixel,0).rgb*.15;transmitted+=sourceColor.SampleLevel(linearClamp,refracted+float2(-blur,0)*pixel,0).rgb*.15;transmitted+=sourceColor.SampleLevel(linearClamp,refracted+float2(0,blur)*pixel,0).rgb*.15;transmitted+=sourceColor.SampleLevel(linearClamp,refracted-float2(0,blur)*pixel,0).rgb*.15;float dispersion=.55*edge;transmitted.r=lerp(transmitted.r,sourceColor.SampleLevel(linearClamp,refracted+N*dispersion*pixel,0).r,edge*.6);transmitted.b=lerp(transmitted.b,sourceColor.SampleLevel(linearClamp,refracted-N*dispersion*pixel,0).b,edge*.6);
 if(reflectionSettings.y>0)transmitted=linearRGB(transmitted);float3 tint=lerp(float3(.71,.69,.62),float3(.026,.038,.058),cameraNight.w);float3 glass=lerp(transmitted,tint,lerp(.56,.69,cameraNight.w));float bevel=exp(-abs(sdf+1.4)*.42);float2 light=float2(cos(dimensionsTime.w*.4488),sin(dimensionsTime.w*.4488));float moving=pow(saturate(dot(N,light)),12),pointer=exp(-length(input.position.xy-glassPointer.xy)/78);glass+=float3(.27,.29,.31)*bevel*(.24+.72*moving+.7*pointer)+float3(.025,.028,.031)*pointer;color=lerp(color,glass,(1-smoothstep(-.6,.8,sdf))*glassPointer.w);}
 return float4(srgb(reflectionSettings.y>0?max(0,color):filmic(max(0,color)*lerp(1.05,1.08,cameraNight.w))),1);
}
