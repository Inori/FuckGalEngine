# -*- coding:utf-8 -*-
__author__ = 'Asuka'

import struct,os,zlib,tempfile

def mkdir(path):
    # 去除首位空格
    path=path.strip()
    # 去除尾部 \ 符号
    path=path.rstrip("\\")

    # 判断路径是否存在
    # 存在     True
    # 不存在   False
    isExists=os.path.exists(path)

    # 判断结果
    if not isExists:
        # 如果不存在则创建目录
        # 创建目录操作函数
        os.makedirs(path)
        return True
    else:
        # 如果目录存在则不创建
        return False


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


def FormatString(string, count):
    res = "○%08d○%s\n●%08d●%s\n\n"%(count, string, count, string)

    #res = "●%08d●%s\n\n"%(count, string)

    return res


def MakeOffsetList(src, idx_off, base_off):
    src.seek(idx_off)
    count = (base_off - idx_off) // 4

    offset_list = []
    for i in range(0, count):
        offset_list.append(byte2int(src.read(4))+base_off)

    return offset_list

def dumpstr(src, offset):
    src.seek(offset)
    stream = b''
    while True:
        b = src.read(1)
        if b == b'\x00':
            break
        stream += b
    return  stream.decode('sjis', errors='ignore')

def GetStrList(src, offset_list):
    string_list = []
    select_flat = False

    for off in offset_list:
        src.seek(off)
        type = src.read(2)
        if type == b'\x01\x20' or type == b'\x01\x21':
            string_list.append(dumpstr(src,off+2))
        elif type == b'\x01\x30':
            if select_flat:
                string_list.append(dumpstr(src,off+2))
                continue

            cmd = dumpstr(src,off+2)
            if cmd == 'select':
                input('found cmd: select!')
                continue
            elif cmd == 'fselect':
                select_flat = True


    return  string_list

def Main():
    fl = walk('scene')
    for fn in fl:
        src = open(fn, 'rb')
        magic = src.read(8).decode('ascii')
        #chech the header
        if magic != 'CatScene':
            print('file not match')
            exit(1)

        compress_len = byte2int(src.read(4))
        decomprss_len = byte2int(src.read(4))

        comp_buff = src.read(compress_len)
        decomp_buff = zlib.decompress(comp_buff)
        #check the decompress length
        if len(decomp_buff) != decomprss_len:
            print('decompress length error!')
            exit(1)

        fp = tempfile.TemporaryFile()
        fp.write(decomp_buff)
        fp.seek(0)

        fp.seek(0x08)
        #0x10 is header size
        index_off = 0x10 + byte2int(fp.read(4))
        base_off = 0x10 + byte2int(fp.read(4))
        offset_list = MakeOffsetList(fp, index_off, base_off)

        str_list = GetStrList(fp, offset_list)

        dstname = 'script' + fn[5:-4] + '.txt'
        dst = open(dstname, 'w', encoding='utf16')

        i = 0
        for string in str_list:
            dst.write(FormatString(string,i))
            i += 1

        dst.close()
        src.close()
        fp.close()
        print(dstname)

Main()
