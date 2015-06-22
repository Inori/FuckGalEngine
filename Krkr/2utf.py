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

#将4字节byte转换成整数
def byte2int(byte):
    long_tuple=struct.unpack('L',byte)
    long = long_tuple[0]
    return long

#将整数转换为4字节二进制byte
def int2byte(num):
    return struct.pack('L',num)


flist = walk('data')
for fn in flist:
    src = open(fn, 'r', encoding='sjis‘， errors='ignore')
    dstname = 'patch' + fn[4:]
    dst = open(dstname, 'w', encoding='utf16')

    try:
        lines = src.read()
    except:
        print(fn)
        continue
    dst.write(lines)

    src.close()
    dst.close()
    
    
