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

#将txt转换成文本列表
def makestr(lines):
    string_list = []
    for index,line in enumerate(lines):
        s = re.match('●[0-9A-Fa-f]+●', line)
        if s:
            string_list.append(line[10:])
    return string_list

def StringFilter(string):
    if '〈ハ〉' in string:
        string = string.replace('〈ハ〉', '乹僴乺')
    if '【草太' in string and not'【草太】' in string:
        string = string.replace('【草太', '【草太】')
    return string
    
        
f_lst = walk('jp')
for fn in f_lst:
    dstname = 'done' + fn[2:]
    dst = open(dstname,'w', encoding='gbk', errors='ignore')

    rawname = 'cn' + fn[2:-2] + '.txt'
    raw = open(rawname, 'r', encoding='utf16')
    cn_lines = raw.readlines()
    cn_strlist = makestr(cn_lines)
    
    src = open(fn, 'r', encoding='sjis', errors='ignore')
    jp_lines = src.readlines()
    
    

    dstlines = []
    j = 0
    for index, line in enumerate(jp_lines):
        if (line[0] != '^'
            and line[0] != '\\'
            and line[0] != '@'
            and line[0] != '％'
            and line != '\n'):
            dstlines.append(StringFilter(cn_strlist[j]))
            j += 1
        else:
            dstlines.append(line.encode('sjis').decode('gbk'))

    for l in dstlines:
        dst.write(l)
    
    raw.close()    
    src.close()
    dst.close()
    print(dstname)



