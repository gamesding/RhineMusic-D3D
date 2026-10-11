"""Test connected FiiO outputs and natural repeat using the final EXE runtime."""
from pathlib import Path
import json,os,subprocess,hashlib,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1]
package=json.loads((root/'build/package/package-manifest.json').read_text('utf-8'))
portable=json.loads((root/'build/portable-validation/standalone-validation.json').read_text('utf-8'))
cache=Path(portable['runtimeDirectory']).resolve()
assert (cache/'.rhine-native-owned').read_text('utf-8').strip()==package['payloadSha256']
for name,digest in package['files'].items():
    file=cache/name;assert file.is_file() and not file.is_symlink()
    assert hashlib.file_digest(file.open('rb'),'sha256').hexdigest()==digest,name
assert hashlib.file_digest((root/'RhineMusic.exe').open('rb'),'sha256').hexdigest()==package['exeSha256']
runtime=cache/'runtime';env=os.environ.copy()
for name in list(env):
    if name.upper().startswith(('RHINE_','SONY_WALKMAN_','PYTHON','NODE_')) or name.upper()=='MUSIC_DATA_DIR':env.pop(name,None)
env['RHINE_TEST_RUNTIME_JSON']=json.dumps(str(runtime));env['PATH']=os.path.join(os.environ['SystemRoot'],'System32')+os.pathsep+os.environ['SystemRoot']
for name in ['asio-playback','wasapi-exclusive','repeat-playback']:
    print('Testing embedded runtime '+name,flush=True)
    result=subprocess.run([str(runtime/'node/node.exe'),str(root/'tests'/(name+'.mjs'))],cwd=root,env=env,creationflags=0x08000000,timeout=180)
    assert result.returncode==0
    file=root/'build'/(name+'.json');report=json.loads(file.read_text('utf-8'));assert report['ok'] and report['packagedRuntime']
    key='backend/original-core.mjs' if name=='repeat-playback' else 'runtime/vendor/audio/windows-x64/rhine-audio-host.exe'
    assert report['backendSha256' if name=='repeat-playback' else 'nativeHostSha256']==package['files'][key]
    report['packagePayloadSha256']=package['payloadSha256'];file.write_text(json.dumps(report,ensure_ascii=False,indent=2),'utf-8')
print('Embedded ASIO, exclusive WASAPI and repeat playback passed.')
