# -*- coding:utf-8 -*-
__author__ = 'Asuka'

import struct,os,zlib,tempfile,shutil

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


# def FormatString(string, count):
#     res = "○%08d○%s\n●%08d●%s\n\n"%(count, string, count, string)
#
#     #res = "●%08d●%s\n\n"%(count, string)
#
#     return res

def FormatString(string, count):
    return string.replace('\\', '\\\\') + '\n'


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
    select_flag = False

    for off in offset_list:
        src.seek(off)
        type = src.read(2)
        if type == b'\x01\x20' or type == b'\x01\x21':
            string_list.append(dumpstr(src,off+2))
        elif type == b'\x01\x30':
            if select_flag:
                string_list.append(dumpstr(src,off+2))
                continue

            cmd = dumpstr(src,off+2)
            if 'select' in cmd:
                select_flag = True

    return  string_list


IMPORT_CODEPAGE = 'gbk'

def ImportStrList(dst, offset_list, str_list):
    select_flag = False

    i = 0
    for idx, off in enumerate(offset_list):
        dst.seek(off)
        type = dst.read(2)
        if type == b'\x01\x20' or type == b'\x01\x21':
            dst.seek(0, os.SEEK_END)
            new_off = dst.tell()
            byte_str = str_list[i].encode(IMPORT_CODEPAGE, errors='ignore').replace(b'\xa1\xee', b'\x87\x40') + b'\x00'
            i += 1
            dst.write(type)
            dst.write(byte_str)
            offset_list[idx] = new_off
        elif type == b'\x01\x30':
            if select_flag:
                dst.seek(0, os.SEEK_END)
                new_off = dst.tell()
                byte_str = str_list[i].encode(IMPORT_CODEPAGE, errors='ignore').replace(b'\xa1\xee', b'\x87\x40') + b'\x00'
                i += 1
                dst.write(type)
                dst.write(byte_str)
                offset_list[idx] = new_off
                continue

            cmd = dumpstr(dst,off+2)
            if 'select' in cmd:
                select_flag = True


def MakeImportStrList(new_list, old_list):
    no_enter_list = [x.rstrip('\n') for x in new_list if x != '\n']
    dst_list = []
    i = 0
    for line in old_list:
        if line == '':
            dst_list.append(line)
        else:
            dst_list.append(no_enter_list[i])
            i += 1
    return dst_list


def ReadFileLines(fname):
    src = open(fname, 'r', encoding='utf16')
    lines = src.readlines()
    src.close()
    dst_lines = [line.replace('\\\\', '\\').replace('♡', '☆') for line in lines]
    return dst_lines

def BuildTxtName(bin_name):
    return 'script' + bin_name[10:-3] + 'txt'



def RepairEntry(dst, index_off, base_off, offset_list):

    dst.seek(index_off)
    for off in offset_list:
        new_off = off - base_off
        byte_off = int2byte(new_off)
        dst.write(byte_off)

def RepairHeader(dst):
    dst.seek(0, os.SEEK_END)
    file_size = dst.tell()
    dst.seek(0)
    byte_size = int2byte(file_size - 0x10)
    dst.write(byte_size)


DO_PACK = True

def Main():
    fl = walk('uncompress')
    for fn in fl:
        print('process-> ' + fn)
        src = open(fn, 'rb+')
        fp = src
        # magic = src.read(8).decode('ascii')
        # #chech the header
        # if magic != 'CatScene':
        #     print('file not match')
        #     exit(1)
        #
        # compress_len = byte2int(src.read(4))
        # decomprss_len = byte2int(src.read(4))
        #
        # comp_buff = src.read(compress_len)
        # decomp_buff = zlib.decompress(comp_buff)
        # #check the decompress length
        # if len(decomp_buff) != decomprss_len:
        #     print('decompress length error!')
        #     exit(1)
        #
        # fp = tempfile.TemporaryFile()
        # fp.write(decomp_buff)
        # fp.seek(0)

        fp.seek(0x08)
        #0x10 is header size
        index_off = 0x10 + byte2int(fp.read(4))
        base_off = 0x10 + byte2int(fp.read(4))
        offset_list = MakeOffsetList(fp, index_off, base_off)

        str_list = GetStrList(fp, offset_list)

        if DO_PACK:

            new_cst_name = 'done' + fn[10:]
            shutil.copy(fn, new_cst_name)

            txt_name = BuildTxtName(fn)
            new_list = ReadFileLines(txt_name)

            if not new_list:
                continue

            import_str_list = MakeImportStrList(new_list, str_list)

            dst = open(new_cst_name, 'rb+')
            ImportStrList(dst, offset_list, import_str_list)
            RepairEntry(dst, index_off, base_off, offset_list)
            RepairHeader(dst)

            dst.close()

        else:
            dstname = 'script\\' + fn[11:-4] + '.txt'
            dst = open(dstname, 'w', encoding='utf16')

            i = 0
            for string in str_list:
                dst.write(FormatString(string,i))
                i += 1

            dst.close()
            #src.close()
            fp.close()





if __name__ == '__main__':
    Main()




