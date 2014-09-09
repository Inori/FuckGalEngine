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
        if re.match('○[0-9]+○', line):
            string_list.append(line[6:-1])
        elif re.match('○[0-9]+ Cho○', line):
            string_list.append(line[10:-1])
        elif re.match('●[0-9]+●', line):
            pass
        elif re.match('●[0-9]+ Cho●', line):
            pass
        elif line != '\n':
            print(line)
    return string_list

def processName(strlist):
    rst_list = []
    for line in strlist:
        obj = re.match('\[(.+)\]', line)
        if obj:
            name = obj.group(1)
            string = line.replace(obj.group(0), '')

            rst_list.append('仈' + name)
            rst_list.append(string)
        else:
            rst_list.append(line)
    return  rst_list


def StringFilter(string):
    left = b'\x6a\x22'.decode('utf16')
    right = b'\x6b\x22'.decode('utf16')
    if left in string:
        string = string.replace(left, '《')
    if right in string:
        string = string.replace(right, '》')
    return string
        

f_lst = walk('format')
for fn in f_lst:

    src = open(fn, 'r', encoding='utf16')

    dstname = 'script' + fn[6:]
    dst = open(dstname, 'w', encoding='gbk', errors='ignore', newline='\n')

    strlist = makestr(src.readlines())
    newlist = processName(strlist)
    for string in newlist:
        dst.write(string + '\n')
            
            
    print(dstname)
    src.close()
    dst.close()

