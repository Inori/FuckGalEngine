# -*- coding:utf-8 -*-

import struct,os

#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist


fl = walk('patch2')
for fn in fl:
    newname = fn.encode('gbk').decode('sjis')
    os.rename(fn, newname)
    print(newname)



