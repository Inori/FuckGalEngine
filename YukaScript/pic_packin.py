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


#将png文件头改回\x89GNP
def ChangePngHeader(src):
    raw = bytearray(src.read())
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

dst = open('DATA02.ykc', 'rb+')
mydic = Makedic(dst)

fn = walk('pic')
for f in fn:
    #过滤非png文件
    if not fnmatch.fnmatch(f,'*.png'):
        continue
    src = open(f, 'rb')
    off = f.find('ykg')
    png_name = f[off:]
    if os.path.basename(png_name) == '0.png':
        flag = 0
        ykg_name = png_name[:-6] + '.ykg'
    elif os.path.basename(png_name) == '1.png':
        flag = 1
        ykg_name = png_name[:-6] + '.ykg'       
    else:
        flag = 0
        ykg_name = png_name[:-4] + '.ykg'
        
    if ykg_name in mydic:
        cur_size = GetSize(src)
        png_offset1, png_length1, png_offset2, png_length2 = GetPngInfo(dst, mydic[ykg_name][0])
        if flag == 0:
            org_size = png_length1
        else:
            org_size = png_length2
            
        if cur_size > org_size:
            print('\nFuck,Current File is larger than before!')
            print(f)
            input()
            continue
        else:
            zeros = org_size - cur_size
            
            pngbytes = ChangePngHeader(src)
            if flag == 0:
                dst.seek(png_offset1)
            else:
                dst.seek(png_offset2)
            dst.write(pngbytes)
            
            #补0x00
            if zeros > 0:
                for i in range(0, zeros):
                    dst.write(struct.pack('B', 0))
            print(f)
    else:
        print('\n索引中无此文件名：')
        print(name)
        input('按任意键继续\n')
        continue
            
    src.close()

dst.close()
print('\n图片导入完成\n')
input('Press Enter to exist')

        

        
        
        
    
        
    
    
    
