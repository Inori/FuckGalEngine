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
    '''
    res = "★%08d★\n%s\n"%(count, string+'\n')
    
    res = "☆%08d☆\n%s★%08d★\n%s\n"%(count, string+'\n', count, string+'\n')
    '''
    
    res = "○%08d○%s●%08d●%s\r\n"%(count, string, count, string)
    
    '''
    res = "●%08d●%s●\n%s\n"%(count, name, string)
    '''
    return res

def StringFilter(string):
    left = b'\x6a\x22'.decode('utf16')
    right = b'\x6b\x22'.decode('utf16')
    if left in string:
        string = string.replace(left, '《')
    if right in string:
        string = string.replace(right, '》')
    return string
    
        
strlen = 0
f_lst = walk('scenario')
for fn in f_lst:
    dstname = 'script' + fn[8:-2] + '.txt'
    dst = open(dstname,'w', encoding='utf16')
    src = open(fn, 'r', encoding='sjis')
    lines = src.readlines()

    num = len(lines)
    stringline = ''
    j = 0
    for index, line in enumerate(lines):
        if (line[0] != '^'
            and line[0] != '\\'
            and line[0] != '@'
            and line[0] != '％'
            and line != '\n'):
            string = FormatString(line, j)
            dst.write(string)
            j += 1
            strlen += len(line)*2
        
    src.close()
    dst.close()
    fl = walk('script')
    for fn in fl:
        if os.path.getsize(fn) < 10:
            os.remove(fn)

    print(dstname)
print('\n文本总量' + str(strlen/1024) + ' KB\n')


