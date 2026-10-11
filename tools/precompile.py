"""Compile official and native shaders ahead of time, retaining source fingerprints."""
from pathlib import Path
import subprocess,hashlib,json
root=Path(__file__).resolve().parent.parent
if not (root/'CMakeLists.txt').is_file() or not (root/'native/main.cpp').is_file():raise RuntimeError('Expected the RhineMusic D3D project root')
import os,winreg
with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,r'SOFTWARE\Microsoft\Windows Kits\Installed Roots',0,winreg.KEY_READ|winreg.KEY_WOW64_32KEY) as key:
    kit=Path(winreg.QueryValueEx(key,'KitsRoot10')[0])
candidates=sorted(kit.glob('bin/*/x64/fxc.exe'),key=lambda p:tuple(int(x) for x in p.parts[-3].split('.')),reverse=True)
if not candidates:raise RuntimeError('Windows 10/11 SDK x64 fxc.exe missing; install the SDK through Visual Studio Installer')
fxc=candidates[0]
dxc=root/'vendor/dxc/bin/x64/dxc.exe'
output=root/'shaders/compiled';output.mkdir(parents=True,exist_ok=True)
sources={
 'scene':{'vs_5_0':['SceneVS','ShadowVS','ScreenVS','StarVS'],'ps_5_0':['OpaquePS','GlassPS','BackgroundPS','PostPS','FinalPS','ViewerBlendPS','FsrInputPS','CopyPS','AoPS','StarPS']},
 'smaa':{'vs_5_0':['EdgeVS','WeightVS','BlendVS'],'ps_5_0':['EdgePS','WeightPS','BlendPS']},
 'fsr11':{'ps_5_0':['EasuPS','RcasPS']},
 'rays':{'cs_6_5':['ReflectionCS','ClearPhotonsCS','PhotonCS','ResolveCausticCS','CompositeCS']}}
dependencies={'scene':['shaders/disc.hlsli'],'rays':['shaders/disc.hlsli'],'smaa':['vendor/smaa/SMAA.hlsl'],'fsr11':['vendor/fsr1/ffx-fsr/ffx_a.h','vendor/fsr1/ffx-fsr/ffx_fsr1.h']}
manifest={'programs':{}}
def digest(file):
    with file.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
for source,profiles in sources.items():
    inputs={name:digest(root/name) for name in ['shaders/'+source+'.hlsl',*dependencies.get(source,[])]}
    for profile,entries in profiles.items():
        for entry in entries:
            key=source+'-'+entry+'-'+profile;binary=output/(key+'.cso')
            command=[str(dxc),'-E',entry,'-T',profile,'-O3','-Ges','-Zpr','-Fo',str(binary)] if source=='rays' else [str(fxc),'/nologo','/E',entry,'/T',profile,'/O3','/Ges','/Fo',str(binary)]
            completed=subprocess.run(command+[str(root/('shaders/'+source+'.hlsl'))],capture_output=True,text=True)
            if completed.returncode:raise RuntimeError(key+': '+completed.stdout+completed.stderr)
            manifest['programs'][key]={'inputs':inputs,'sha256':digest(binary)}
(output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'compiledShaders':len(manifest['programs']),'compiledBytes':sum(file.stat().st_size for file in output.glob('*.cso'))}))
