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


def FormatString(string, count):
    #res = "●%08d●%s\n"%(count, string)
    res = "○%08d○%s●%08d●%s\n"%(count, string,count, string)
    return res

alnum = [' ','.',',','[',']', '_',
         '-', '\t',
         '0','1','2','3','4','5','6','7','8','9',
         'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
         'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z']

def is_alnum(string):
    for s in string:
        if s in alnum:
            continue
        else:
            return False
    return True

fl = walk('sc')
sumlen = 0
namedic = {}
for fn in fl:
    src = open(fn, 'r', encoding='sjis')
    lines = src.readlines()

    dstname = 'script' + fn[2:-2] + 'txt'
    dst = open(dstname, 'w', encoding='utf16')
    i = 0
    j = 1
    for line in lines:
        '''
        if '★' in line:
            print('fuck!')
            input()
        '''
        if '.message' in line:
            #print(line.split(' '))
            *dm, name, text = line.split(' ')
            #测试
            sumlen += len(text.strip('\n'))*2 / 1024
            for t in dm:
                if '#' in t and not is_alnum(t[1:]):
                    name = t + ' ' + name
                    continue
                if not is_alnum(t):
                    print('\n')
                    print(t)
                    print(line.split(' '))
                    print(fn)
                    print(j)
                    input()
            
            if not is_alnum(name):
                dst.write(FormatString(name+'★'+text, i))
                namedic[name] = '';
            else:
                dst.write(FormatString(text, i))
            i += 1
        j+=1
            
    print(dstname)
    src.close()
    dst.close()

print('文本总量'+str(sumlen)+' KB')

namefile = open('name.txt', 'w', encoding='utf16')
count = 0
for name in namedic.keys():
    string = "<%04d>%s\=\n\n"%(count, name.encode('sjis').decode('gbk'))
    namefile.write(string)
    count += 1




























