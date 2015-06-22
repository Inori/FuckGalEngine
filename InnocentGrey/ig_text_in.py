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
        elif re.match('○[0-9]+ Msg○', line):
            string_list.append(line[10:-1])
        elif re.match('●[0-9]+●', line):
            pass
        elif re.match('●[0-9]+ Cho●', line):
            pass
        elif re.match('●[0-9]+ Msg●', line):
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


def strQ2B(ustring):
    """全角转半角"""
    rstring = ''
    for uchar in ustring:
        inside_code=ord(uchar)
        if inside_code == 12288:                              #全角空格直接转换
            inside_code = 32
        elif (inside_code >= 65281 and inside_code <= 65374): #全角字符（除空格）根据关系转化
            inside_code -= 65248

        rstring += chr(inside_code)
    return rstring
    
def strB2Q(ustring):
    """半角转全角"""
    rstring = ""
    for uchar in ustring:
        inside_code=ord(uchar)
        if inside_code == 0x20:                                 #半角空格直接转化
            inside_code = 0x3000
        elif inside_code >= 0x21 and inside_code <= 0x7E:        #半角字符（除空格）根据关系转化
            inside_code += 0xFEE0

        rstring += chr(inside_code)
    return rstring

f_lst = walk('format')
for fn in f_lst:

    src = open(fn, 'r', encoding='utf16')

    dstname = 'script' + fn[6:]
    dst = open(dstname, 'w', encoding='gbk', errors='ignore', newline='\n')

    strlist = makestr(src.readlines())
    newlist = processName(strlist)
    for string in newlist:
        dst.write(strB2Q(string) + '\n')
            
            
    print(dstname)
    src.close()
    dst.close()

