"""Official OpenCC tables, pinned to the 1.4.2 release commit."""
from pathlib import Path
from urllib.request import Request,urlopen
import hashlib,json

root=Path(__file__).resolve().parents[1];target=root/'runtime/vendor/opencc';stamp=target/'source.json'
revision='025f371dc76b598d77384fbdab90c937471844d8'
if stamp.is_file():
    record=json.loads(stamp.read_text('utf-8'))
    if record.get('commit')==revision and all((target/name).is_file() and hashlib.file_digest((target/name).open('rb'),'sha256').hexdigest()==digest for name,digest in record['files'].items()):
        print('Verified offline OpenCC 1.4.2 dictionaries');raise SystemExit(0)
target.mkdir(parents=True,exist_ok=True);files={};dictionaries={}
for name,path in [('LICENSE','LICENSE'),('TSCharacters.txt','data/dictionary/TSCharacters.txt'),('TSPhrases.txt','data/dictionary/TSPhrases.txt'),('TWVariants.txt','data/dictionary/TWVariants.txt'),('HKVariants.txt','data/dictionary/HKVariants.txt')]:
    url=f'https://raw.githubusercontent.com/BYVoid/OpenCC/{revision}/{path}'
    with urlopen(Request(url,headers={'User-Agent':'RhineMusic-native-build'}),timeout=60) as response:data=response.read()
    (target/name).write_bytes(data);files[name]=hashlib.sha256(data).hexdigest()
    if name.endswith('.txt'):
        entries={}
        for line in data.decode('utf-8').splitlines():
            if not line or line.startswith('#'):continue
            source,translations=line.split('\t',1);entries[source]=translations.split(' ')[0]
        dictionaries[name]=entries
characters=dictionaries['TSCharacters.txt'];variants={}
for name in ['TWVariants.txt','HKVariants.txt']:
    for standard,variant in dictionaries[name].items():
        if len(standard)==len(variant)==1 and standard!=variant:variants[variant]=standard
for variant,standard in variants.items():characters.setdefault(variant,characters.get(standard,standard))
def fold(char):
    seen=set()
    while char in characters and characters[char]!=char and char not in seen:seen.add(char);char=characters[char]
    return char
characters={char:fold(char) for char in characters}
for name,values in [('search-characters.json',characters),('phrases.json',dictionaries['TSPhrases.txt'])]:
    data=json.dumps(values,ensure_ascii=False,separators=(',',':')).encode('utf-8');(target/name).write_bytes(data);files[name]=hashlib.sha256(data).hexdigest()
stamp.write_text(json.dumps({'version':'1.4.2','commit':revision,'source':'https://github.com/BYVoid/OpenCC','license':'Apache-2.0','derivedTables':'First-choice character and longest-phrase maps; original dictionaries retained unchanged','files':files},indent=2)+'\n','utf-8')
print('Prepared official OpenCC 1.4.2 dictionaries')
