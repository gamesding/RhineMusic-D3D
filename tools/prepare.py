"""Pinned dependency helpers for the complete native source distribution.

Never traverse the user's music/data directories or copy Git configuration.
"""
from pathlib import Path
import argparse, hashlib, json, shutil, subprocess, urllib.request, zipfile

def sha(path):
    h=hashlib.sha256()
    with open(path,'rb') as stream:
        for block in iter(lambda:stream.read(1048576),b''): h.update(block)
    return h.hexdigest()

def download(url,target):
    target.parent.mkdir(parents=True,exist_ok=True)
    req=urllib.request.Request(url,headers={'User-Agent':'RhineMusicNative/0.3.0'})
    with urllib.request.urlopen(req,timeout=90) as response, open(target,'wb') as output:
        shutil.copyfileobj(response,output)
    return {'url':url,'sha256':sha(target),'bytes':target.stat().st_size}

def api(url):
    req=urllib.request.Request(url,headers={'User-Agent':'RhineMusicNative/0.3.0','Accept':'application/vnd.github+json'})
    with urllib.request.urlopen(req,timeout=40) as response:return json.load(response)

def copytree(source,target):
    shutil.copytree(source,target,dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__','*.pyc','.cache','.git'))

if __name__=='__main__':
    import runpy
    runpy.run_path(str(Path(__file__).with_name('build.py')),run_name='__main__')
