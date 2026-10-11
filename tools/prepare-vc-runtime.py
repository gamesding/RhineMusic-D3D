from pathlib import Path
import hashlib,json
root=Path(__file__).resolve().parents[1];target=root/'runtime/graphics'
data=json.loads((target/'vc-runtime-assets.json').read_text('utf-8'))
for name,record in data.items():
    with (target/name).open('rb') as stream:assert hashlib.file_digest(stream,'sha256').hexdigest()==record['sha256'],name
print('Verified bundled app-local Microsoft VC runtime:',len(data))
