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

def strfilter(string):
    r = re.search('（\d*）', string)
    if r:
        m = r.group()
        p = string.find(m)
        string = string[:p] + '\n'
    return string
        

fl = walk('cnscript')
for fn in fl:
    dstname = 'done' + fn[8:]
    rawname = 'jpscript' + fn[8:]
    src = open(fn, 'r', encoding='utf16')
    raw = open(rawname, 'r', encoding='sjis', errors='ignore')
    dst = open(dstname, 'w', encoding='gbk', errors='ignore')
    
    cnlines = src.readlines()
    jplines = raw.readlines()
    strlist = []
    i = 0
    for line in jplines:
        if (line == '\n'
            or line[:2] == 'EF'
            or line[:2] == '**'
            or line[:2] == '//'
            or line[:2] == 'BG'
            or line[:2] == 'CG'
            or line[:2] == 'KM'
            or line[:2] == 'SE'
            or line[:2] == 'DG'
            or line[:3] == 'FCG'
            or line[:2] == 'MV'):
            strlist.append(line)
        else:
            strlist.append(strfilter(cnlines[i]))
            i += 1
    for line in strlist:
        dst.write(line)

    src.close()
    dst.close()
    raw.close()
            
            
            




