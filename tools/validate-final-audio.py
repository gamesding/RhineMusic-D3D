from pathlib import Path
import hashlib,json,subprocess,sys
root=Path(__file__).resolve().parents[1]
package=json.loads((root/'build/package/package-manifest.json').read_text('utf-8'))
checked=[]
for name,digest in package['files'].items():
    if name.startswith(('runtime/','backend/')):
        assert hashlib.file_digest((root/name).open('rb'),'sha256').hexdigest()==digest,name
        checked.append(name)
subprocess.run([sys.executable,'tools/run-audio-validation.py'],cwd=root,check=True,creationflags=0x08000000)
file=root/'build/audio-validation.json';data=json.loads(file.read_text('utf-8'));assert data['exitCode']==0
data['runtimeMatchesFinalPackage']=True;data['programFilesChecked']=len(checked);data['packagePayloadSha256']=package['payloadSha256']
data['nativeHostSha256']=package['files']['runtime/vendor/audio/windows-x64/rhine-audio-host.exe']
file.write_text(json.dumps(data,ensure_ascii=False,indent=2),'utf-8')
print('Final audio/service regression has SHA-256 provenance for the delivered runtime.')
