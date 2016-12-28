# -*- coding:utf-8 -*-

import struct,os,fnmatch,re,zlib

#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist

fl = walk('script')
dst = open('AsyncScript.txt', 'w', encoding='utf16')
for fn in fl:
    src = open(fn, 'r', encoding='utf16')
    lines = src.read()
    dst.write(lines + '\n\n')
    src.close()


dst.close()














