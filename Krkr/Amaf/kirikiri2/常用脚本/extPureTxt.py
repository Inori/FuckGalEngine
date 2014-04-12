#encoding=utf-8
import os
import re


def exOriTxt(lines):
    pat=re.compile(r'^\*[^\|]+\|(.+)')
    #patsel=re.compile(r'^\[seladd text="([^"]+)".*')
    patsel=re.compile(r'^\[seladd.*text="([^"]+)".*')
    patdisp=re.compile(r'\[dispname name=([^\]]+)\](.+)')
    newl=['#KAG Script Text Resources']
    for l in lines:
        l=l.strip('\t')
        if len(l)==0:
            continue
        elif l.startswith('[seladd'):
            mo=patsel.match(l)
            if mo:
                newl.append('【选择】'+mo.group(1))
            continue
        elif l.startswith('[dispname'):
            mo=patdisp.match(l)
            if mo:
                if mo.group(1)!='""':
                    newl.append('【'+mo.group(1)+'】'+mo.group(2))
                else:
                    newl.append(mo.group(2))
            continue
        elif (l[0]=='[') or (l[0]==';'):
            continue
        elif (l[0]=='*'):
            mo=pat.match(l)
            if mo:
                newl.append('【存档用标签名】'+mo.group(1))
            continue
        else:
            newl.append(l)
    return newl

path1='scenario'
path2='oritxt'
for f in os.listdir(path1):
    fs=open(os.path.join(path1,f),'rb')
    ls=fs.read().decode('932').split('\r\n')
    newls=exOriTxt(ls)
    fs=open(os.path.join(path2,f.replace('.ks','.txt')),'wb')
    fs.write('\r\n'.join(newls).encode('u16'))
    fs.close()
