# -*- coding:utf-8 -*-

import struct,os,fnmatch,re,zlib

#只读，不可更改
COMPRESS = 0

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

    global COMPRESS
    COMPRESS = byte2int(src.read(4))
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
        orglength = byte2int(src.read(4))
        curpos = src.tell()

        src.seek(nameoffset)
        name = src.read(namelength-1).decode('ascii')
        entry_list.append([name, offset, length, orglength])
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


#将png文件头改回\x89GNP
def ChangePngHeader(data):
    raw = bytearray(data)
    raw[0:4] = b'\x89GNP'
    return raw

#传入ykg地址，返回png地址和大小
def GetPngInfo(src, offset):
    src.seek(offset)
    src.seek(8, 1)
    header_length = byte2int(src.read(4))
    src.seek(0x1c, 1)
    
    temp_png_offset1 = byte2int(src.read(4))
    png_length1 = byte2int(src.read(4))
    temp_png_offset2 = byte2int(src.read(4))
    png_length2 = byte2int(src.read(4))
    
    png_offset1 = temp_png_offset1 + offset
    png_offset2 = temp_png_offset2 + offset
    src.seek(0)
    return png_offset1, png_length1, png_offset2, png_length2

#将yks文件加密回去,返回加密后文件的全部字节
def Encrypt(data):

    offset = byte2int(data[0x20:0x24])
    size =byte2int(data[0x24:0x28])

    raw = data[offset:offset+size]
    en_bytes = b''
    for char in raw:
        byte = char ^ 0xAA
        en_bytes += struct.pack('B', byte)
        
    org_bytes = data[0:offset]
    return org_bytes + en_bytes

pic_dst = open('DATA02.ykc', 'rb+')
scr_dst = open('DATA01.ykc', 'rb+')
src = open('ls.dat', 'rb')

pic_dic = Makedic(pic_dst)
scr_dic = Makedic(scr_dst)

entry_list = ReadEntry(src)
for cell in entry_list:
    if fnmatch.fnmatch(cell[0], '*.png'):
        png_name = cell[0]
        if os.path.basename(png_name) == '0.png':
            flag = 0
            ykg_name = png_name[:-6] + '.ykg'
        elif os.path.basename(png_name) == '1.png':
            flag = 1
            ykg_name = png_name[:-6] + '.ykg'       
        else:
            flag = 0
            ykg_name = png_name[:-4] + '.ykg'

        if ykg_name in pic_dic:
            cur_size = cell[3]
            png_offset1, png_length1, png_offset2, png_length2 = GetPngInfo(pic_dst, pic_dic[ykg_name][0])
            if flag == 0:
                org_size = png_length1
            else:
                org_size = png_length2
            
            if cur_size > org_size:
                print('\nFuck,Current File is larger than before!')
                print(png_name)
                input()
                continue
            else:
                zeros = org_size - cur_size

                src.seek(cell[1])
                global COMPRESS
                if COMPRESS:
                    compressdata = src.read(cell[2])
                    data = zlib.decompress(compressdata)
                else:
                    data = src.read(cell[3])
                pngbytes = ChangePngHeader(data)
                if flag == 0:
                    pic_dst.seek(png_offset1)
                else:
                    pic_dst.seek(png_offset2)
                pic_dst.write(pngbytes)
            
                #补0x00
                if zeros > 0:
                        pic_dst.write(struct.pack('B', 0)*zeros)
                print(png_name)
        else:
            print('\n索引中无此文件')
            pirnt(png_name)
            input()
            continue
    elif fnmatch.fnmatch(cell[0], '*.yks'):
        name = cell[0]
        if name in scr_dic:
            cur_size = cell[3]
            org_size = scr_dic[name][1]
            if cur_size > org_size:
                print('Fuck,Current File is larger than before!')
                print(name)
                input()
                continue
            else:
                zeros = org_size - cur_size

                src.seek(cell[1])
                global COMPRESS
                if COMPRESS:
                    compressdata = src.read(cell[2])
                    data = zlib.decompress(compressdata)
                else:
                    data = src.read(cell[2])                
                enbytes = Encrypt(data)
                scr_dst.seek(scr_dic[name][0])
                scr_dst.write(enbytes)
            
                #补0x00
                if zeros > 0:
                        scr_dst.write(struct.pack('B', 0)*zeros)
                print(name)
        else:
            print('\n索引中无此文件')
            print(name)
            input()
            continue
        
pic_dst.close()
scr_dst.close()
src.close()  
