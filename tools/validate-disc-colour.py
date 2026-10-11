"""Check visible colour in delivered disc screenshots, beyond renderer flags."""
from pathlib import Path
import json,hashlib
from PIL import Image
root=Path(__file__).resolve().parents[1]
package=json.loads((root/'build/package/package-manifest.json').read_text('utf-8'))
optical=json.loads((root/'build/cd-media-validation/validation.json').read_text('utf-8'))
assert optical['ok'] and optical['packagedRuntime'] and optical['packagePayloadSha256']==package['payloadSha256']
cases=[]
for name in ['dx11-off-1600x900-day','dx11-ssr-1600x900-day','dx12-rtx-1600x900-day']:
    file=root/'build/cd-media-validation'/name/'disc-detail.png'
    image=Image.open(file).convert('RGB');assert image.size==(1600,900)
    # Fixed fully settled reference view: the interior of the optical disc,
    # excluding the hole, surrounding glass, UI and unrelated album artwork.
    colourful=0;pixels=0
    for y in range(305,637):
        for x in range(246,587):
            radius2=(x-423)**2+(y-475)**2
            if not 65**2<radius2<155**2:continue
            r,g,b=image.getpixel((x,y));pixels+=1
            colourful+=max(r,g,b)-min(r,g,b)>24
    cases.append({'case':name,'colourfulPixels':colourful,'sampledPixels':pixels,'imageSha256':hashlib.file_digest(file.open('rb'),'sha256').hexdigest()})
print(json.dumps(cases,indent=2))
assert cases[1]['colourfulPixels']>cases[0]['colourfulPixels']+1000,'SSR rainbow did not appear in the rendered disc'
assert cases[2]['colourfulPixels']>cases[0]['colourfulPixels']+1000,'DXR rainbow did not appear in the rendered disc'
result={'ok':True,'packagePayloadSha256':package['payloadSha256'],'cases':cases,'scope':'Actual settled day screenshots; no physical disc insertion or visual quality rating.'}
(root/'build/disc-colour-validation.json').write_text(json.dumps(result,indent=2),'utf-8')
print('The final SSR and DXR disc screenshots visibly contain rainbow colour.')
