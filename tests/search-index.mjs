import {test} from 'node:test';
import assert from 'node:assert/strict';
import {pinyinKeys,withSearchKeys} from '../backend/search-index.mjs';

test('traditional titles, contextual pronunciation and initials are searchable offline',()=>{
    assert.equal(pinyinKeys('白雲長在天').full,'baiyunchangzaitian');
    assert.equal(pinyinKeys('重慶森林').full,'chongqingsenlin');
    assert.equal(pinyinKeys('音樂').full,'yinyue');
    assert.equal(pinyinKeys('黃玉清').initials,'hyq');
    assert.equal(pinyinKeys('周杰倫').full,'zhoujielun');
});
test('search enrichment preserves tags and does not persist into original records',()=>{
    const album={id:'one',title:'回想曲之情話綿綿',artist:'黃玉清',tracks:[{id:'song',title:'後來',artist:'劉若英'}]};
    const original=structuredClone(album),enriched=withSearchKeys(album,'華語流行樂');
    assert.deepEqual(album,original);assert.equal(enriched.title,original.title);assert.equal(enriched.tracks[0].title,'後來');
    assert.ok(enriched.searchPinyin.includes('huixiangquzhiqinghuamianmian'));
    assert.ok(enriched.tracks[0].searchPinyin.includes('houlai'));
});
test('mixed Latin titles and repeated queries retain stable search keys',()=>{
    assert.equal(pinyinKeys('2008 回想曲 (2019 Remaster)').full,'2008huixiangqu2019remaster');
    assert.deepEqual(pinyinKeys('青春'),pinyinKeys('青春'));
    assert.equal(pinyinKeys('').full,'');
});
