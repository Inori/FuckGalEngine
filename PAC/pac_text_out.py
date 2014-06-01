# -*- coding:utf-8 -*-

import struct,os,fnmatch,re,zlib

LOG = open('log.txt', 'wb')
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


    #传入地址，返回字符串、长度、首地址、末地址
def DumpText(src, offset):
    if offset == 0:return ''
    '''
    src.seek(offset)
    #过滤非文本
    temp = src.read(1)
    if temp == b'\x00':
        return ''
    else:
        temp = src.read(1)
        if temp == b'\x00':
            return ''
    '''
    src.seek(offset)  
    string = ''
    bytestream = b''
    while True:
        byte = src.read(1)
        if byte == b'\x00':
            try:
                string += str(bytestream, encoding='sjis')
            except:
                LOG.write(bytestream+b'\n')
                string = "Error:%08x\n"%(offset)
            bytestream = b''
            break            
        else:
            bytestream += byte
    return string

src = open('TEXT.DAT', 'rb')
dst = open('text.txt', 'w', encoding='utf16')

src.seek(0x0c)
count = byte2int(src.read(4))
text_list = []
for i in range(0, count):
    num = byte2int(src.read(4))
    offset = src.tell()
    text = DumpText(src, offset)
    text_list.append([num, text])

for [num, text] in text_list:
    #string = FormatString(text, num)
    string = text + '\n'
    dst.write(string)

src.close()
dst.close()
LOG.close()


