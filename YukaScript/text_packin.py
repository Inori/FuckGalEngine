# -*- coding:utf-8 -*-

import struct,os,fnmatch,re


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

#读取文件头
def ReadIndex(src):
    src.seek(0x08)
    header_len = byte2int(src.read(4))
    src.seek(4,1)
    indexoffset = byte2int(src.read(4))
    indexlength = byte2int(src.read(4))
    src.seek(0)
    return header_len, indexoffset, indexlength

#读取索引
def ReadEntry(src):
    entry_list = []
    header_len, indexoffset, indexlength = ReadIndex(src)
    src.seek(indexoffset)
    
    for i in range(0, indexlength//20):
        nameoffset = byte2int(src.read(4))
        namelength = byte2int(src.read(4))
        offset = byte2int(src.read(4))
        length = byte2int(src.read(4))
        src.seek(4,1)
        curpos = src.tell()

        src.seek(nameoffset)
        name = src.read(namelength-1).decode('ascii')
        entry_list.append([name, offset, length])
        src.seek(curpos)
    src.seek(0)
    return entry_list

#将索引信息制作成字典
def Makedic(src):
    mydic = {}
    entry_list = ReadEntry(src)

    for cell in entry_list:
        mydic[cell[0]] = [cell[1], cell[2]]
    return mydic

#将yks文件加密回去,返回加密后文件的全部字节
def Encrypt(src):
    src.seek(0x20)
    offset = byte2int(src.read(4))
    size =byte2int(src.read(4))

    src.seek(offset)
    raw = src.read(size)
    en_bytes = b''
    for char in raw:
        byte = char ^ 0xAA
        en_bytes += struct.pack('B', byte)
        
    src.seek(0, 0)
    org_bytes = src.read(offset)
    src.seek(0, 0)
    return org_bytes + en_bytes

dst = open('DATA01.ykc', 'rb+')
mydic = Makedic(dst)

fn = walk('done')
for f in fn:
    #过滤非yks文件
    if not fnmatch.fnmatch(f,'*.yks'):
        continue
    src = open(f, 'rb')
    off = f.find('yks')
    name = f[off:]
    if name in mydic:
        cur_size = GetSize(src)
        org_size = mydic[name][1]
        if cur_size > org_size:
            print('Fuck,Curent File is larger than before!')
            print(f)
            input()
            continue
        else:
            zeros = org_size - cur_size
            
            enbytes = Encrypt(src)
            dst.seek(mydic[name][0])
            dst.write(enbytes)
            
            #补0x00
            if zeros > 0:
                for i in range(0, zeros):
                    dst.write(struct.pack('B', 0))
            print(f)

        

        
        
        
    
        
    
    
    
