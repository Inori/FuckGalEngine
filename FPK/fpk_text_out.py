# -*- coding:utf-8 -*-

import os, re

#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist

def FormatString(string, count):
    #格式说明：
    #★字符串行数★字符串
    res = ''
    flag = False
    if string != '':
        flag = True
        '''
        res = "★%08d★\n%s\n"%(count, string)
        '''
        res = "☆%08d☆\n%s★%08d★\n%s\n"%(count, string, count, string)
 
    else:
        flag = False

    return flag, res


fl = walk('script')
for fn in fl:
    dstname = fn + '.txt'
    src = open(fn, 'r', encoding='sjis', errors='ignore')
    dst = open(dstname, 'w', encoding='utf16')

    lines = src.readlines()
    for line in lines:
        if (line != '\n'
            and line[:2] != 'EF'
            and line[:2] != '**'
            and line[:2] != '//'
            and line[:2] != 'BG'
            and line[:2] != 'CG'
            and line[:2] != 'KM'
            and line[:2] != 'SE'
            and line[:2] != 'DG'
            and line[:3] != 'FCG'
            and line[:2] != 'MV'):
            dst.write(line)

    src.close()
    dst.close()
            
            
            




