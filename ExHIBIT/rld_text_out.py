# -*- coding:utf-8 -*-

import struct,os,fnmatch,re,tempfile

LOG = open('ErrorLog.bin', 'wb')

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
    src.seek(0)
    length = len(src.read())
    src.seek(0)
    return length


#传入地址，返回字符串、长度、首地址、末地址
def StringOut(src, offset):
    src.seek(offset)

    #过滤非文本
    temp = src.read(1)
    if temp == b'\x00':
        return '', 0, 0, 0
    else:
        temp = src.read(1)
        if temp == b'\x00':
            return '', 0, 0, 0
    src.seek(offset)
        
    string = ''
    bytestream = b''

    for i in range(0, 0xffffffff):
        byte = src.read(1)
        char_tuple = struct.unpack('B', byte)
        char = char_tuple[0]
        if char == 0:
            try:
                string += str(bytestream, encoding='sjis', errors='ignore')
            except:
                global LOG
                LOG.write(bytestream+b'\n')
                string = "Error:%08x\n"%(offset)
            bytestream = b''
            endoffset = src.tell()
            break            
        else:
            bytestream += struct.pack('B', char)
    stringlen = endoffset - offset
    return string, stringlen, offset, endoffset


def FormatString(src, offset, count):
    #格式说明：
    #★字符串行数★字符串
    string ,strlen, startoffset, endoffset = StringOut(src, offset)
    res = ''
    idx = ''
    flag = False
    if string != '':
        flag = True
        res = "★%08d★%s\n\n"%(count, string)
        
        idx = "◇%08x◇\n"%(startoffset)
        idx += "☆%08x☆\n\n"%(endoffset)
    else:
        flag = False

    return flag, res, idx

#将key_real.bin每四个字节一组存入列表
def GetKeyList(fkey):
    size = os.path.getsize(fkey)
    src = open(fkey, 'rb')
    key_list = []
    for i in range(0, size, 4):
        key_list.append(byte2int(src.read(4)))
    return key_list

#返回去掉文件头的解密后的bytes
def Decrypt(src, key, key_list):
    size = GetSize(src)
    block_size = size - 0x10
    rmd = size % 4
    
    src.seek(0x10)
    count = (block_size >> 2)&0xffff
    if count > 0x3ff0:
        count = 0x3ff0
    buff = b''
    for i in range(0, count):
        en = byte2int(src.read(4))

        key_count = i & 0xFF
        temp_key = key_list[key_count] ^ key

        de = int2byte(temp_key^en)
        buff += de
    buff += src.read(rmd)
    return buff

key = 0x20121203
key_list = GetKeyList('key.bin')

f_rld = walk('rld')
for f in f_rld:
    src = open(f, 'rb')
    dstname = 'script\\txt' + f[3:-4] + '.txt'
    idxname = 'script\\idx' + f[3:-4] + '.idx'
    dst = open(dstname, encoding='utf16', mode='w')
    idx = open(idxname, encoding='utf16', mode='w')
    
    de_buff = Decrypt(src, key, key_list)
    fsize = len(de_buff)
    
    fp = tempfile.TemporaryFile()
    fp.write(de_buff)

    offset_list = []
    fp.seek(0)
    for i in range(0, fsize):
        char = fp.read(1)
        if char == b'\x2A' or char == b'\x90':
            if char == b'\x2A':
                char = fp.read(1)
                if char == b'\x00':
                    offset_list.append(fp.tell())
                else:
                    fp.seek(-1,1)
                    continue
            elif char == b'\x90':
                fp.seek(-2,1)
                char = fp.read(1)
                if char == b'\x00':
                    fp.seek(1,1)
                    char = fp.read(1)
                    if char == b'\xBA':
                        char = fp.read(1)
                        if char == b'\x00':
                            offset_list.append(fp.tell()-3)
                            offset_list.append(fp.tell())
                        else:
                            fp.seek(-1,1)
                            continue
                            
                    else:
                        fp.seek(-1,1)
                        continue
                else:
                    fp.seek(1,1)
                    continue

        else:
            continue

    j = 0
    for off in offset_list:
        flag, string, idxstr = FormatString(fp, off, j)
        if flag:
            j += 1
            dst.write(string)
            idx.write(idxstr)
    print(f)  
    dst.close()
    idx.close()
    fp.close()
 
LOG.close()

        
    
    


        
    
