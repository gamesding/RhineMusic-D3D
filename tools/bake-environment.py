"""Reproduce the upstream Three.js RoomEnvironment as native HDR cube data.

The room, six rotated boxes, light cards and point-light parameters come from
the preserved upstream dependency. Roughness is convolved with a normalized
GGX kernel at build time; no raster editor or generated art is involved.
"""
from pathlib import Path
import argparse, hashlib, json, math, struct
import numpy as np

parser=argparse.ArgumentParser();parser.add_argument('--target',required=True)
root=Path(parser.parse_args().target).resolve()
boxes=[((-10.906,2.009,1.846),(2.328,7.905,4.651),-.195),((-5.607,-.754,-.758),(1.970,1.534,3.955),.994),((6.167,.857,7.803),(3.927,6.285,3.687),.561),((-2.017,.018,6.124),(2.002,4.566,2.064),.333),((2.291,-.756,-2.621),(1.546,1.552,1.496),-.286),((-2.193,-.369,-5.547),(3.875,3.487,2.986),.516)]
cards=[((-16.116,14.37,8.208),(.1,2.428,2.739),50),((-16.109,18.021,-8.207),(.1,2.425,2.751),50),((14.904,12.198,-1.832),(.15,4.265,6.331),17),((-.462,8.89,14.520),(4.38,5.441,.088),43),((3.235,11.486,-12.541),(2.5,2,.1),20),((0,20,0),(1,.1,1),100)]
def normalize(value):return value/np.maximum(1e-12,np.linalg.norm(value,axis=-1,keepdims=True))
def intersect(directions,center,extent,angle=0,inside=False):
    c,s=math.cos(angle),math.sin(angle);rotation=np.array([[c,0,s],[0,1,0],[-s,0,c]],dtype=np.float64)
    center=np.array(center,dtype=np.float64)+[0,-3.5,0];origin=(-center)@rotation;directions=directions@rotation
    inverse=np.where(abs(directions)>1e-12,1/np.where(abs(directions)>1e-12,directions,1),1e12)
    a=(-np.array(extent)*.5-origin)*inverse;b=(np.array(extent)*.5-origin)*inverse
    near=np.minimum(a,b);far=np.maximum(a,b);first=near.max(axis=-1);last=far.min(axis=-1);distance=last if inside else first;valid=(last>=np.maximum(first,0))&(distance>0)
    axis=np.argmax(far if inside else near,axis=-1);normal=np.zeros_like(directions);rows=np.arange(len(normal));normal[rows,axis]=np.sign(directions[rows,axis])*-1;normal=normal@rotation.T
    return np.where(valid,distance,np.inf),normal
def room_radiance(directions):
    distance,normal=intersect(directions,(-.757,13.219,.717),(31.713,28.305,28.591),inside=True)
    for center,extent,angle in boxes:
        candidate,n=intersect(directions,center,extent,angle);closer=candidate<distance;distance=np.where(closer,candidate,distance);normal=np.where(closer[:,None],n,normal)
    point=directions*distance[:,None];light=np.array([.418,16.199-3.5,.300])-point;d2=np.sum(light*light,axis=-1);weight=np.maximum(0,np.sum(normal*normalize(light),axis=-1));falloff=np.maximum(0,1-(np.sqrt(d2)/28)**4)**2
    radiance=(900/np.maximum(d2,.01)*weight*falloff/math.pi)[:,None]*np.ones((1,3))
    for center,extent,intensity in cards:
        candidate,_=intersect(directions,center,extent);closer=candidate<distance;distance=np.where(closer,candidate,distance);radiance=np.where(closer[:,None],intensity,radiance)
    return radiance
def face_directions(face,size):
    y,x=np.mgrid[0:size,0:size];u=(x+.5)*2/size-1;v=(y+.5)*2/size-1;one=np.ones_like(u)
    data=[(one,-v,-u),(-one,-v,u),(u,one,v),(u,-one,-v),(u,-v,one),(-u,-v,-one)][face]
    return normalize(np.stack(data,axis=-1).reshape(-1,3))
def radical_inverse(index):
    value=0;fraction=.5
    while index:value+=(index&1)*fraction;index>>=1;fraction*=.5
    return value
size,mips,samples=128,8,192
destination=root/'runtime/graphics/room-environment.bin';destination.parent.mkdir(parents=True,exist_ok=True)
with destination.open('wb') as output:
    output.write(struct.pack('<4I',0x52494342,size,mips,6))
    for face in range(6):
        for level in range(mips):
            edge=max(1,size>>level);directions=face_directions(face,edge);roughness=level/(mips-1)
            tangent=normalize(np.cross(np.where(abs(directions[:,1:2])<.99,[0,1,0],[1,0,0]),directions));bitangent=np.cross(directions,tangent);value=np.zeros_like(directions);total=np.zeros(len(directions))
            alpha=max(.04,roughness*roughness)
            for sample in range(samples):
                u=(sample+.5)/samples;v=radical_inverse(sample);phi=2*math.pi*u;cosine=math.sqrt((1-v)/(1+(alpha*alpha-1)*v));sine=math.sqrt(max(0,1-cosine*cosine));half=normalize(tangent*(math.cos(phi)*sine)+bitangent*(math.sin(phi)*sine)+directions*cosine);incoming=normalize(2*np.sum(directions*half,axis=-1)[:,None]*half-directions);weight=np.maximum(0,np.sum(directions*incoming,axis=-1));value+=room_radiance(incoming)*weight[:,None];total+=weight
            value/=np.maximum(total[:,None],1e-12);pixels=np.concatenate([value,np.ones((len(value),1))],axis=-1).astype('<f2');assert np.isfinite(pixels).all();output.write(pixels.tobytes())
manifest={'origin':'Three.js RoomEnvironment (preserved upstream dependency)','geometry':'room, six rotated boxes, six light cards','pointLightIntensity':900,'ggxSamplesPerTexel':samples,'size':size,'mips':mips,'sha256':hashlib.sha256(destination.read_bytes()).hexdigest()}
(destination.parent/'room-environment.json').write_text(json.dumps(manifest,indent=2)+'\n','utf-8');print(json.dumps(manifest))
