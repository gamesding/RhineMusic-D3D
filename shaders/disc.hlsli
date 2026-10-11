// Reflective CD tracks form a radial diffraction grating (~1.6 micrometres).
// Colour depends on the light/view directions, rather than a painted rainbow.
void discClip(float2 uv){float radius=length(uv-.5);clip(.46-radius);clip(radius-.060);}
float2 discUv(float2 uv,float encodedPhase){float angle=-encodedPhase-1,s,c;sincos(angle,s,c);float2 p=uv-.5;return float2(c*p.x-s*p.y,s*p.x+c*p.y)+.5;}
float3 discSpectrum(float2 uv,float3 N,float3 V,float3 L,bool highQuality){
    float2 p=uv-.5;float radius=length(p);float3 T=normalize(cross(abs(N.y)<.95?float3(0,1,0):float3(1,0,0),N)),B=cross(N,T);
    float3 radial=normalize(T*p.x-B*p.y);float wavelength=abs(dot(V+L,radial))*1.6;
    float3 lambda=float3(.650,.530,.450),band=exp(-pow((wavelength-lambda)/.055,2));
    band+=exp(-pow((wavelength*.5-lambda)/.050,2))*.42;
    if(highQuality)band+=exp(-pow((wavelength/3-lambda)/.040,2))*.18;
    float tracks=smoothstep(.105,.125,radius)*(1-smoothstep(.43,.449,radius));
    return band*tracks*saturate(dot(N,L))*(highQuality?.62:.42);
}
