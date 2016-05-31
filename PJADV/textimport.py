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

#提取txt中offset, name和string，转换为二维列表
def makestr(lines):
    sum_list = []
    for index,line in enumerate(lines):
        if '☆' in line:
            offset_str = re.search('([0-9A-Fa-f]+)', line).group()
            offset = int(offset_str, 16)
            continue
        if '★' in line:
            name = line[10:-1]
            continue
        if '●' in line:
            string = line[10:-1]
            sum_list.append([offset, name, string])
            continue
            
    return sum_list


index = open('scenario.dat', 'rb+')
dst = open('textdata.bin', 'rb+')
# 这里跳过textdata.bin的头部，它包含了文本的章节信息。同时你需要手动在最后添加原有textdata.bin文件的尾部信息
dst.seek(0x20, 0)

namedic={}

fl = walk('script')
for fn in fl:
    #过滤非txt文件
    if not fnmatch.fnmatch(fn,'*.txt'):
        continue
    print(fn)
    
    src = open(fn, 'r', encoding='utf-16')
    lines = src.readlines()
    sumlist = makestr(lines)
    for cell in sumlist:
        if cell[1] != '':
            if cell[1] not in namedic:
                nameoffset = dst.tell()
                dst.write(cell[1].encode('gbk'))
                dst.write(struct.pack('H', 0))
                namedic[cell[1]] = nameoffset
            else:
                nameoffset = namedic[cell[1]]
        else:
            nameoffset = 0
        stringoffset = dst.tell()
        dst.write(cell[2].encode('gbk'))
        dst.write(struct.pack('H', 0))
        
        index.seek(cell[0])
        long_byte = index.read(4)
        long = byte2int(long_byte)
        if long == 0x80000307 or long == 0x80000406:        
            index.seek(4, 1)
            index.write(struct.pack('L', nameoffset))
            index.write(struct.pack('L', stringoffset))
        elif long == 0x01010203:
            index.write(struct.pack('L', stringoffset))
        elif long == 0x03000303:
            index.seek(4, 1)
            index.write(struct.pack('L', stringoffset))

    src.close()
index.close()
dst.close()
