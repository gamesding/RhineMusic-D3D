from pathlib import Path
from prepare import api, download
import argparse, json

parser = argparse.ArgumentParser()
parser.add_argument('--target', required=True)
root = Path(parser.parse_args().target).resolve()
manifest_path = root / 'vendor/sdk-sources.json'
manifest = json.loads(manifest_path.read_text('utf-8'))
revision = api('https://api.github.com/repos/iryoku/smaa/commits/master')['sha']
records = {}
for name in ['SMAA.hlsl', 'Textures/AreaTex.h', 'Textures/SearchTex.h', 'LICENSE.txt']:
    records[name] = download('https://raw.githubusercontent.com/iryoku/smaa/' + revision + '/' + name, root / 'vendor/smaa' / name)
manifest['smaa'] = {'repository': 'iryoku/smaa', 'revision': revision, 'mode': 'SMAA 1x HIGH', 'files': records}
manifest_path.write_text(json.dumps(manifest, indent=2) + '\n', 'utf-8')
print(json.dumps({'smaaRevision': revision}))
