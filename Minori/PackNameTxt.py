# -*- coding:utf-8 -*-
import struct,os,sys, re

'''
//脚本封包文件头
typedef struct _acr_header
{
	ulong index_count; //包含索引数，即字符串数
	ulong compress_flag; //是否压缩。 0没有压缩
	ulong compsize; //如果有压缩，为压缩后大小，否则等于orgsize
	ulong orgsize; //如果有压缩，为压缩前【压缩部分】大小，否则为实际大小
}acr_header;

typedef struct _acr_index
{
	ulong hash; //oldstr的hash值，用于map查找
	ulong old_str_off; //oldstr 地址
	ulong old_str_len; //oldstr 字节长度
	ulong new_str_off; //newstr 地址
	ulong new_str_len; //newstr 长度
}acr_index;
'''
COMPRESS = 0
HEADERLEN = 0x10
INDEXLEN = 0x14

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

def fixjpstr(string):
    newstr = string.replace('／', '')
    return newstr

def fixcnstr(string):
    newstr = string.replace('／', '\n')
    return newstr

#将txt转换成文本列表,重写此函数可以适应于不同格式的文本
def makestr(lines):
    oldstr_list = []
    newstr_list = []
    for index,line in enumerate(lines):
        part = re.match('<.+>(.*)\\\=(.*)\n', line)
        if part:
            oldstr = part.group(1)
            newstr = part.group(2)
            oldstr_list.append(fixjpstr(oldstr))
            newstr_list.append(fixcnstr(newstr))
        elif line != '\n':
            print(line)

    return oldstr_list, newstr_list

'''
def makestr(lines):
    oldstr_list = []
    newstr_list = []
    for index,line in enumerate(lines):
        if re.match('\[JP[0-9]+\] \[.+\]', line):
            part = re.match('\[JP[0-9]+\] \[.+\]', line).group()
            off = len(part)
            oldstr_list.append(line[off+1:-1])
            
        elif re.match('\[JP[0-9]+\]', line) or re.match('\[JP|CHOICE[0-9]+\]', line):
            oldstr_list.append(line[10:-1])
            
        elif re.match('\[EN[0-9]+\] \[.+\]', line):
            part = re.match('\[EN[0-9]+\] \[.+\]', line).group()
            off = len(part)
            newstr_list.append(line[off+1:-1])
            
        elif re.match('\[EN[0-9]+\]', line) or re.match('\[EN|CHOICE[0-9]+\]', line):
            newstr_list.append(line[10:-1])

        elif line != '\n':
            print(line)
    return oldstr_list, newstr_list
'''
def BKDRHash(bytes):
    hash = 0
    seed = 131
    for c in bytes:
        if c != 0:
            hash = hash*seed + c
    return (hash&0x7FFFFFFF)


def WriteString(src, oldstr_list, newstr_list):
    index = []
    for oldstr, newstr in zip(oldstr_list, newstr_list):

        oldoff = src.tell()
        bstring = oldstr.encode('sjis')
        hash = BKDRHash(bstring)
        oldlen = len(bstring)
        src.write(bstring)

        newoff = src.tell()
        bstring = newstr.encode('gbk')
        newlen = len(bstring)
        src.write(bstring)

        index.append([hash, oldoff, oldlen, newoff, newlen])

    filesize = src.tell()
    return index, filesize

def WriteIndex(src, index):
    src.seek(HEADERLEN)
    for [hash, oldoff, oldlen, newoff, newlen] in index:
        src.write(int2byte(hash))
        src.write(int2byte(oldoff))
        src.write(int2byte(oldlen))
        src.write(int2byte(newoff))
        src.write(int2byte(newlen))


def Main():
    text_filename = sys.argv[1]
    pac_filename = text_filename[:-4] + '.acr'

    txt = open(text_filename, 'r', encoding='utf16')
    dst = open(pac_filename, 'wb')

    lines = txt.readlines()

    oldstr_list, newstr_list = makestr(lines)

    if len(oldstr_list) != len(newstr_list):
        print('number not match')
        exit(1)

    index_count = len(newstr_list)
    dst.seek(HEADERLEN + index_count*INDEXLEN)
    index_list, filesize = WriteString(dst,oldstr_list, newstr_list)

    dst.seek(HEADERLEN)
    WriteIndex(dst, index_list)

    dst.seek(0)
    dst.write(int2byte(index_count))
    dst.write(int2byte(COMPRESS))
    dst.write(int2byte(filesize - HEADERLEN))
    dst.write(int2byte(filesize - HEADERLEN))

    txt.close()
    dst.close()

Main()


















