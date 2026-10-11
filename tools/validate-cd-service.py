from pathlib import Path
import hashlib,json,subprocess
root=Path(__file__).resolve().parents[1]
with (root/'build/cd-service-validation.log').open('w',encoding='utf-8') as log:
    result=subprocess.run([str(root/'runtime/node/node.exe'),'--test','tests/cd-eject.mjs'],cwd=root,stdout=log,stderr=subprocess.STDOUT,creationflags=0x08000000)
assert result.returncode==0
text=(root/'build/cd-service-validation.log').read_text('utf-8');assert 'pass 8' in text and 'fail 0' in text
report={'ok':True,'tests':8,'pass':8,'fail':0,'actualEject':False,'sourceSha256':hashlib.file_digest((root/'backend/cd-eject.mjs').open('rb'),'sha256').hexdigest()}
(root/'build/cd-service-validation.json').write_text(json.dumps(report,indent=2),'utf-8');print('CD release/eject ordering and playback isolation: 8/8 passed.')
