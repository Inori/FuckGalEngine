# -*- coding:utf-8 -*-

import struct,os,fnmatch,re,tempfile

LOG = open('ErrorLog.bin', 'w', encoding='utf16')

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

#提取txt中控制信息，转换为二维列表
def makestr(idxlines, strlines):
    sum_list = []
    for index,line in enumerate(idxlines):
        if re.match('◇[0-9A-Fa-f]+◇', line):
            startoffset_str = re.search('([0-9A-Fa-f]+)', line).group()
            startoffset = int(startoffset_str, 16)
            continue
        if re.match('☆[0-9A-Fa-f]+☆', line):
            endoffset_str = re.search('([0-9A-Fa-f]+)', line).group()
            endoffset = int(endoffset_str, 16)
            sum_list.append([startoffset, endoffset])
            continue
    i = 0
    for line in strlines:
        if re.match('★[0-9A-Fa-f]+★', line):
            string = line[10:-1]
            sum_list[i].append(string)
            i += 1
    return sum_list


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

flst = walk('script\\txt')
for fn in flst:

    strs = open(fn, 'r', encoding='utf16')
    strlines = strs.readlines()
    
    idxname = 'script\\idx' + fn[10:-4] + '.idx'
    idx = open(idxname, 'r', encoding='utf16')
    idxlines = idx.readlines()
    
    fdstname = 'done' + fn[10:-4] + '.rld'
    fdst = open(fdstname, 'wb')

    dst = tempfile.TemporaryFile()
    
    orgname = 'rld' + fn[10:-4] + '.rld'
    orgend = os.path.getsize(orgname)
    org = open(orgname, 'rb')

    de_buff = Decrypt(org, key, key_list)
    rawend = len(de_buff)
    
    raw = tempfile.TemporaryFile()
    raw.write(de_buff)
    raw.seek(0)

    sum_list = makestr(idxlines, strlines)

    
    #先把第一句话之前的部分补齐
    
    org.seek(0)
    header = org.read(0x10)
    head = raw.read(sum_list[0][0])
    dst.write(header+head)

    num = len(sum_list)
    for index,cell in enumerate(sum_list):
        try:
            bytestr = cell[2].encode('gbk') + struct.pack('B', 0)
        except:
            LOG.write(cell[2]+'\n')
            bytestr = 'Error'.encode('sjis')
        dst.write(bytestr)
        if index < num-1:
            ctllen = sum_list[index+1][0] - cell[1]
        else:
            ctllen = rawend - cell[1]
            
        raw.seek(cell[1])
        if ctllen != 0:
            ctlbyte = raw.read(ctllen)
            dst.write(ctlbyte)

    dst.seek(0)
    en_buff = Decrypt(dst, key, key_list)
    fdst.write(header+en_buff)
        
    print(fdstname)
    dst.close()
    raw.close()
    strs.close()
    idx.close()
    fdst.close()
LOG.close()
