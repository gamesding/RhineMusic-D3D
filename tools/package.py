"""Package program files only; never walk the user's library or app-home."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,sys,zipfile
p=argparse.ArgumentParser();p.add_argument("--no-build",action="store_true");args=p.parse_args()
root=Path(__file__).resolve().parent.parent
if not (root/'CMakeLists.txt').is_file() or not (root/'native/main.cpp').is_file():raise RuntimeError('Expected the RhineMusic D3D project root')
if not args.no_build:
    subprocess.run([sys.executable,str(root/'tools/build.py')],cwd=root,check=True,creationflags=0x08000000)
    raise SystemExit(0)
out=root/'build/package';out.mkdir(parents=True,exist_ok=True)
files={}
for directory in ['backend','shaders','runtime']:
    for file in (root/directory).rglob('*'):
        if not file.is_file():continue
        rel=file.relative_to(root)
        if set(rel.parts)&{'.cache','__pycache__','music-data','music-data-v3','verification','fixtures','.git'}:continue
        if file.suffix in {'.pyc','.pdb','.map','.lib','.exp','.obj'}:continue
        if file.name in {'preferences.json','config.json','index.json'}:raise RuntimeError('Personal-state filename in package: '+str(rel))
        if 'node_modules' in rel.parts and any(part in {'three','@kitlangton'} for part in rel.parts):continue
        if file.suffix in {'.blend','.blend1'}:continue
        if file.is_symlink():raise RuntimeError('Unexpected package symlink')
        files[rel.as_posix()]=file
for name in ['RhineMusicD3D.exe','RhineDeviceProbe.exe']:
    files[name]=root/name
for name in ['LICENSE','NOTICE.md']:
    files[name]=root/'upstream'/name
for vendor in ['scepad/license','dlss','dxc','fsr1','fsr','smaa','node']:
    source=root/'vendor'/vendor
    for file in source.rglob('*'):
        if file.is_file() and 'license' in file.name.lower():files['licenses/'+file.relative_to(root/'vendor').as_posix()]=file
files['licenses/sdk-sources.json']=root/'vendor/sdk-sources.json'
files['licenses/native-port-notice.txt']=root/'docs/DEPENDENCIES.txt'
for name in ['vendor/smaa/SMAA.hlsl','vendor/fsr1/ffx-fsr/ffx_a.h','vendor/fsr1/ffx-fsr/ffx_fsr1.h']:
    files[name]=root/name
missing=[str(file) for file in files.values() if not file.is_file()]
if missing:raise RuntimeError('Missing package input: '+str(missing))
hashes={name:hashlib.file_digest(file.open('rb'),'sha256').hexdigest() for name,file in sorted(files.items())}
manifest=''.join(digest+'  '+name+'\n' for name,digest in hashes.items())
(out/'runtime-files.sha256').write_text(manifest,encoding='utf-8',newline='\n')
payload=out/'native-runtime.zip'
with zipfile.ZipFile(payload,'w',zipfile.ZIP_DEFLATED,compresslevel=7) as archive:
    for name,file in sorted(files.items()):archive.write(file,name)
    archive.writestr('runtime-files.sha256',manifest)
digest=hashlib.file_digest(payload.open('rb'),'sha256').hexdigest();(out/'native-runtime.sha256').write_text(digest,encoding='ascii')
subprocess.run([str(Path(os.environ.get('SystemRoot','C:/Windows'))/'Microsoft.NET/Framework64/v4.0.30319/csc.exe'),'/nologo','/target:winexe','/platform:x64','/optimize+','/win32manifest:'+str(root/'windows/launcher.manifest'),'/win32icon:'+str(root/'windows/app.ico'),'/out:'+str(root/'RhineMusic.exe'),'/r:System.Windows.Forms.dll','/r:System.IO.Compression.dll','/r:System.IO.Compression.FileSystem.dll','/resource:'+str(payload)+',Rhine.Native.zip','/resource:'+str(out/'native-runtime.sha256')+',Rhine.Native.sha256','/resource:'+str(out/'runtime-files.sha256')+',Rhine.Native.files',str(root/'windows/Standalone.cs')],check=True)
report={'privatePackage':True,'fileCount':len(files),'uncompressedBytes':sum(file.stat().st_size for file in files.values()),'payloadBytes':payload.stat().st_size,'exeBytes':(root/'RhineMusic.exe').stat().st_size,'payloadSha256':digest,'exeSha256':hashlib.file_digest((root/'RhineMusic.exe').open('rb'),'sha256').hexdigest(),'files':hashes}
(out/'package-manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps({key:value for key,value in report.items() if key!='files'},indent=2))
