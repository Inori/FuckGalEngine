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
    #格式说明：
    #★字符串行数★字符串
    res = ''
    flag = False
    if string != '':
        flag = True
        '''
        res = "★%08d★\n%s\n"%(count, string)
        '''
        res = "☆%08d☆%s★%08d★%s\n"%(count, string, count, string)
 
    else:
        flag = False

    return res

def StringFilter(string):
    left = b'\x6a\x22'.decode('utf16')
    right = b'\x6b\x22'.decode('utf16')
    string = string.replace(left, '《')
    string = string.replace(right, '》')

    string = string.replace('[np]', '')
    string = string.replace('[r]', '')
    
    return string
    
        

f_lst = walk('Txt')
for fn in f_lst:
    dstname = 'script' + fn[3:]
    dst = open(dstname,'w', encoding='utf16')
    
    src = open(fn, 'r', encoding='sjis', errors='ignore')
    lines = src.readlines()

    j = 0
    for line in lines:
        if line[0:4] == ' msg':
            dst.write(FormatString(line[5:], j))
            j += 1
                

    print(dstname)
    src.close()
    dst.close()

