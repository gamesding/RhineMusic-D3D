"""Package program files only; never walk the user's library or app-home."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,sys,zipfile
p=argparse.ArgumentParser();p.add_argument("--no-build",action="store_true");args=p.parse_args()
root=Path(__file__).resolve().parent.parent
if not (root/'CMakeLists.txt').is_file() or not (root/'native/main.cpp').is_file():raise RuntimeError('Expected the RhineMusic D3D project root')
if not args.no_build:
    subprocess.run([sys.executable,str(root/'tools/build.py')],cwd=root,check=True,creationflags=0x08000000)
    raise SystemExit(0)
out=root/'build/package';out.mkdir(parents=True,exist_ok=True)
release=root/'Release';release.mkdir(parents=True,exist_ok=True)
player=release/'RhineMusic.exe'
configuration=release/'RhineConfiguration.exe'
configuration_input=root/'build/configuration/RhineConfiguration.exe'
if not configuration_input.is_file():raise RuntimeError('Build the standalone configuration program before packaging')
shutil.copy2(configuration_input,configuration)
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
subprocess.run([str(Path(os.environ.get('SystemRoot','C:/Windows'))/'Microsoft.NET/Framework64/v4.0.30319/csc.exe'),'/nologo','/utf8output','/codepage:65001','/target:winexe','/platform:x64','/optimize+','/win32manifest:'+str(root/'windows/launcher.manifest'),'/win32icon:'+str(root/'windows/app.ico'),'/out:'+str(player),'/r:System.Windows.Forms.dll','/r:System.IO.Compression.dll','/r:System.IO.Compression.FileSystem.dll','/resource:'+str(payload)+',Rhine.Native.zip','/resource:'+str(out/'native-runtime.sha256')+',Rhine.Native.sha256','/resource:'+str(out/'runtime-files.sha256')+',Rhine.Native.files',str(root/'windows/Standalone.cs')],check=True)
report={'version':'0.1','privatePackage':True,'fileCount':len(files),'uncompressedBytes':sum(file.stat().st_size for file in files.values()),'payloadBytes':payload.stat().st_size,'exeBytes':player.stat().st_size,'configurationBytes':configuration.stat().st_size,'configurationSha256':hashlib.file_digest(configuration.open('rb'),'sha256').hexdigest(),'payloadSha256':digest,'exeSha256':hashlib.file_digest(player.open('rb'),'sha256').hexdigest(),'files':hashes}
(out/'package-manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
for name in ['LICENSE','NOTICE.md']:shutil.copy2(root/'upstream'/name,release/name)
shutil.copy2(root/'docs/DEPENDENCIES.txt',release/'THIRD-PARTY.txt')
(release/'使用说明.txt').write_text('Rhine Music Direct3D · 0.1\n\n1. 双击 RhineConfiguration.exe，选择自动或自定义，并按确认保存图形设置。\n2. 出现设置成功提示后按确定，配置程序关闭。\n3. 手动双击 RhineMusic.exe 启动播放器，在音乐库设置中添加音乐目录。\n\n老机器或显示异常时，可选 Direct3D 11；必要时使用软件兼容渲染。\n配置程序本身使用 Windows 软件绘制，不依赖 Direct3D 12。\n运行无需另行安装 Electron、Node.js 或 Python；需要 Windows 10/11 x64 与系统 .NET Framework 4.8。\n\n完整源码与编译说明：https://github.com/gamesding/RhineMusic-D3D\n反馈：https://github.com/gamesding/RhineMusic-D3D/issues\n',encoding='utf-8-sig')
public_files=[player,configuration,release/'LICENSE',release/'NOTICE.md',release/'THIRD-PARTY.txt',release/'使用说明.txt']
(release/'SHA256SUMS.txt').write_text(''.join(hashlib.file_digest(file.open('rb'),'sha256').hexdigest()+'  '+file.name+'\n' for file in public_files),encoding='utf-8')
with zipfile.ZipFile(release/'RhineMusic-D3D-v0.1-Windows-x64.zip','w',zipfile.ZIP_DEFLATED,compresslevel=6) as distribution:
    for file in [*public_files,release/'SHA256SUMS.txt']:distribution.write(file,'RhineMusic-D3D-0.1/'+file.name)
print(json.dumps({key:value for key,value in report.items() if key!='files'},indent=2))
