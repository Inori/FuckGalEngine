# -*- coding:utf-8 -*-

import struct,os,fnmatch,re,zlib

#字符串首地址
BaseOffset = 0x39EBB4

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
    num = len(lines)
    for index,line in enumerate(lines):
        if re.search('●[0-9A-Fa-f]+●', line):
            i = 1
            string = ''
            while True:
                if index+i >= num:
                    break
                if re.search('○[0-9A-Fa-f]+○', lines[index+i]) or re.search('●[0-9A-Fa-f]+●', lines[index+i]):
                    break
                '''
                if lines[index+i] == '\n':
                    string += lines[index+i]
                else:
                    temp = lines[index+i][:-1] + '◎' + '\n'
                    string += temp
                '''
                '''
                temp = lines[index+i][:-1] + '◎' + '\n'
                string += temp
                '''
                string += lines[index+i]
                i += 1
            string_list.append(string[:-2])
    return string_list

def StringFilter(string):
    for note in re.findall('★.*?<.*?>', string):
        old = re.search('★(.*)<(.*)>', note).group(0)
        t1 = re.search('★(.*)<(.*)>', note).group(1)
        t2 = re.search('★(.*)<(.*)>', note).group(2)
        new = '\x07\x01%s\n%s\x00'%(t1, t2)
        string = string.replace(old, new)

    string = string.replace('◇','\r')
    string = string.replace('◎','\x07\x06')
    string = string.replace('◆','\x07\x04')
    string = string.replace('▲','\x07\x08')
    string = string.replace('△','\x07\x09')
    string = string.replace('▽','\x00')
    return string

def importstr(src, strlist):
    offset_list = []
    src.seek(BaseOffset)
    for string in strlist:
        byte = string.encode('utf-16-le') + b'\x00\x00'
        offset_list.append(src.tell())
        src.write(byte)
    lastoff = src.tell()
    return offset_list, lastoff
        
def writeindex(src, offset_list, indexoff):
    src.seek(BaseOffset-4)
    src.write(int2byte(indexoff-BaseOffset))
    src.seek(indexoff)
    src.write(int2byte(len(offset_list)))
    for off in offset_list:
        src.write(int2byte(off-BaseOffset))

def main():
    src = open('exec.txt','r', encoding='utf16')
    dst = open('unzlib_exec.bin','rb+')

    lines = src.readlines()
    '''
    for x in makestr(lines):
        print(x)
    '''
    strlist = [StringFilter(x) for x in makestr(lines)]

    offset_list, lastoff = importstr(dst, strlist)
    writeindex(dst, offset_list, lastoff)

    src.close()
    dst.close()

main()
        
    
    
    

    


