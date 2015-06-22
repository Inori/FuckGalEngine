# -*- coding:utf-8 -*-

import struct,os,fnmatch,re,zlib

#父文件夹名不会被写入

FOLDERNAME = 'res'
#1为压缩，0为不压缩
COMPRESS = 1

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

#获取文件大小
def GetSize(src):
    length = len(src.read())
    src.seek(0)
    return length

#写文件头
def WriteHeader(dst, indexoffset, indexlength):
    dst.seek(0)
    
    sig = b'FUYINPAK'
    dst.write(sig)
    
    header_length = 0x18
    dst.write(int2byte(header_length))

    global COMPRESS
    dst.write(int2byte(COMPRESS))

    dst.write(int2byte(indexoffset))
    dst.write(int2byte(indexlength))

#写文件数据
def WriteFile(dst, filelist):
    item_list = []
    if dst.tell() < 0x18:
        dst.seek(0x18)
    for fn in filelist:
        off = fn.find('\\') + 1
        name = fn[off:].encode('gbk')
        offset = dst.tell()
        orgsize = os.path.getsize(fn)
        src = open(fn, 'rb')
        data = src.read()
        global COMPRESS
        if COMPRESS == 1:
            compressdata = zlib.compress(data, 9)
            size = len(compressdata)
            dst.write(compressdata)
        else:
            size = len(data)
            dst.write(data)
        src.close()
        print(fn)
        item_list.append([name, offset, size, orgsize])
    return item_list

#写文件名索引
def WriteNameIndex(dst, item_list):
    entry_list = []
    for cell in item_list:
        name_t = cell[0] + struct.pack('B', 0)
        name_offset = int2byte(dst.tell())
        name_len = int2byte(len(name_t))
        offset = int2byte(cell[1])
        length = int2byte(cell[2])
        orgsize = int2byte(cell[3])
        entry_list.append([name_offset, name_len, offset, length, orgsize])

        dst.write(name_t)
    return entry_list

def WriteEntryIndex(dst, entry_list):
    indexoffset = dst.tell()
    for cell in entry_list:
        dst.write(cell[0]+cell[1]+cell[2]+cell[3]+cell[4])
    indexlength = dst.tell() - indexoffset
    return indexoffset, indexlength

def Main():
    
    dst = open(FOLDERNAME+'.dat', 'wb')
    scr_list = walk(FOLDERNAME)
    scr_itemlist = WriteFile(dst, scr_list)
    scr_entrylist = WriteNameIndex(dst, scr_itemlist)
    indexoffset, indexlength= WriteEntryIndex(dst, scr_entrylist)
    WriteHeader(dst, indexoffset, indexlength)

Main()




    
        
    
