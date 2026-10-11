"""Pin and verify the official offline pinyin dictionary and its MIT notice."""
from pathlib import Path
from urllib.request import Request, urlopen
import hashlib,base64,io,tarfile,json

root=Path(__file__).resolve().parents[1]
version='3.29.5'
integrity='Fk5YoRmtdHqJWqAuzYVG/NzvzeCbaG+ZEwbY6Nbps3QAIrvQQEtcm49frcyCM8i2b5najnzPDtYqrHRYTTiuwQ=='
target=root/'runtime/vendor/pinyin-pro'
stamp=target/'source.json'
if stamp.is_file():
    record=json.loads(stamp.read_text('utf-8'))
    if record.get('version')==version and all((target/name).is_file() and hashlib.file_digest((target/name).open('rb'),'sha256').hexdigest()==digest for name,digest in record.get('files',{}).items()):
        if 'traditional-map.json' in record['files']:
            record['files'].pop('traditional-map.json');record.pop('simplification',None);(target/'traditional-map.json').unlink();stamp.write_text(json.dumps(record,indent=2)+'\n','utf-8')
        print('Verified offline pinyin dictionary '+version);raise SystemExit(0)
url=f'https://registry.npmjs.org/pinyin-pro/-/pinyin-pro-{version}.tgz'
with urlopen(Request(url,headers={'User-Agent':'RhineMusic-native-build'}),timeout=60) as response:payload=response.read()
if base64.b64encode(hashlib.sha512(payload).digest()).decode()!=integrity:raise RuntimeError('Pinyin dictionary integrity mismatch')
target.mkdir(parents=True,exist_ok=True)
files={}
with tarfile.open(fileobj=io.BytesIO(payload),mode='r:gz') as archive:
    for member in archive.getmembers():
        if member.name.startswith('package/dist/esm/') and member.isfile():
            relative=Path(member.name.removeprefix('package/dist/esm/'))
            if relative.is_absolute() or '..' in relative.parts:raise RuntimeError('Unexpected dictionary path')
            data=archive.extractfile(member).read();file=target/relative;file.parent.mkdir(parents=True,exist_ok=True);file.write_bytes(data);files[relative.as_posix()]=hashlib.sha256(data).hexdigest()
    notice=next((item for item in archive.getmembers() if item.isfile() and item.name.lower() in ['package/license','package/license.md','package/license.txt']),None)
    if not notice:raise RuntimeError('Pinyin dictionary license missing')
    data=archive.extractfile(notice).read();(target/'LICENSE').write_bytes(data);files['LICENSE']=hashlib.sha256(data).hexdigest()
stamp.write_text(json.dumps({'version':version,'source':'https://github.com/zh-lx/pinyin-pro','tarball':url,'sha512':integrity,'license':'MIT','files':files},indent=2)+'\n','utf-8')
print('Prepared verified offline pinyin dictionary '+version)
