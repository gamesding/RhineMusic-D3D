import {pinyin,polyphonic} from '../runtime/vendor/pinyin-pro/index.mjs';
import {readFileSync} from 'node:fs';
const simplifiedMap=JSON.parse(readFileSync(new URL('../runtime/vendor/opencc/search-characters.json',import.meta.url),'utf8'));
const phrases=JSON.parse(readFileSync(new URL('../runtime/vendor/opencc/phrases.json',import.meta.url),'utf8'));
const phraseLengths=new Map();for(const phrase of Object.keys(phrases)){const first=phrase[0];phraseLengths.set(first,Math.max(phrase.length,phraseLengths.get(first)||0));}
function simplify(text){let result='';for(let i=0;i<text.length;){let matched=false;for(let length=Math.min(phraseLengths.get(text[i])||0,text.length-i);length>1;length--){const phrase=text.slice(i,i+length);if(phrases[phrase]){result+=phrases[phrase];i+=length;matched=true;break;}}if(!matched){const char=String.fromCodePoint(text.codePointAt(i));result+=simplifiedMap[char]||char;i+=char.length;}}return result;}

// Read-only, in-memory search keys. No tag, filename or saved library index is
// rewritten. Cache separate fields so a large library need not transliterate
// again during the service's regular poll or when playback advances.
const cache=new Map();
export function pinyinKeys(value){
    const text=String(value||'');
    if(cache.has(text))return cache.get(text);
    const simplified=simplify(text);
    const words=pinyin(simplified,{toneType:'none',type:'array',nonZh:'spaced'});
    const normalize=text=>text.normalize('NFKD').replace(/\p{M}/gu,'').replace(/[^\p{L}\p{N}]/gu,'').toLowerCase();
    const key={full:normalize(words.join('')),initials:normalize(words.map(word=>/^[a-zü]+$/i.test(word)?word[0]:word).join('')),alternatives:[normalize(words.join('').replaceAll('ü','v'))]};
    const pronunciations=polyphonic(simplified,{toneType:'none',type:'array',nonZh:'spaced'});
    if(pronunciations.length===words.length){for(let i=0;i<words.length;i++)for(const word of pronunciations[i])if(word!==words[i]){const variant=[...words];variant[i]=word;key.alternatives.push(normalize(variant.join('')),normalize(variant.map(value=>/^[a-zü]+$/i.test(value)?value[0]:value).join('')));}}
    if(cache.size>=32768)cache.clear();cache.set(text,key);return key;
}
export function searchKeys(...fields){
    const values=fields.map(pinyinKeys);return [...new Set(values.flatMap(value=>[value.full,value.initials,...value.alternatives]).filter(Boolean))];
}
export function withSearchKeys(album,genre=''){
    return {...album,searchPinyin:searchKeys(album.title,album.artist,genre),tracks:album.tracks.map(track=>({...track,searchPinyin:searchKeys(track.title,track.artist)}))};
}
