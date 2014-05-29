# -*- coding:utf-8 -*-

import struct,os,fnmatch,re

LOG = open('ErrorLog.bin', 'w', encoding='utf16')

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






def Main():
    name_list = []
    txt_l = walk('raw')
    for fn in txt_l:
        dstname = 'done' + fn[3:]
        dst = open(dstname, 'w' ,encoding = 'utf16')
        src = open(fn, 'r', encoding = 'utf16')

        str_list = makestr(src.readlines())
        for line in str_list:
            dst.write(line)

            
        dst.close()
        src.close()
        print(fn)
Main()

input('\n导入完成\n')

            
                

            
                
                
            
            
    


















    


