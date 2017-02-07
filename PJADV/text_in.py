# -*- coding:utf-8 -*-

import struct, os, fnmatch, re

# 遍历文件夹，返回文件列表
def walk(adr):
    mylist = []
    for root, _, files in os.walk(adr):
        for name in files:
            adrlist = os.path.join(root, name)
            mylist.append(adrlist)
    return mylist

# 将4字节byte转换成整数
def byte2int(byte):
    long_tuple = struct.unpack('L', byte)
    long = long_tuple[0]
    return long

# 提取txt中offset, name和string，转换为二维列表
def makestr(lines):
    sum_list = []
    for _, line in enumerate(lines):
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

script_path = r'd:/archive/'
# 注意这会覆写原来的文件，注意文件备份！
index_path = script_path + 'scenario.dat'
dst_path = script_path + 'textdata.bin'

index = open(index_path, 'rb+')
dst = open(dst_path, 'rb+')
# jump header
dst.seek(0x10, 0)

namedic = {}
fl = walk(script_path)
for fn in fl:
    # 过滤非txt文件
    if not fnmatch.fnmatch(fn, '*.txt'):
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
#        print(cell[2])
        dst.write(cell[2].encode('gbk'))
        dst.write(struct.pack('H', 0))
        index.seek(cell[0])
        long_byte = index.read(4)
        long = byte2int(long_byte)
        if long == 0x80000307 or long == 0x80000406:
            index.seek(4, 1)
            index.write(struct.pack('L', nameoffset))
            index.write(struct.pack('L', stringoffset))
        elif long == 0x01010203 or long == 0x81010102 or long == 0x01000D02 or long == 0x01010804:
            index.write(struct.pack('L', stringoffset))
        elif long == 0x03000303 or long == 0x01003004 or long == 0x0404130C:
            index.seek(4, 1)
            index.write(struct.pack('L', stringoffset))

    src.close()
index.close()
dst.close()
