# -*- coding:utf-8 -*-

import struct,os,fnmatch,re


#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist

fl = walk('en')
for fn in fl:
    src = open(fn, 'r', encoding='utf8')
    dstname = 'en_nn' + fn[2:]
    dst = open(dstname, 'w', encoding='utf_8_sig')
    lines = src.readlines()
    for line in lines:
        if line[0] == '#':
            dst.write(line)
        else:
            dst.write(line[7:])
