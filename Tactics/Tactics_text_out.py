# -*- coding:utf-8 -*-

import struct,os,fnmatch,re

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

def FormatString(name1, name2, string, count):
    #格式说明：
    #★字符串行数★字符串
    '''
    res = "★%08d★\n%s\n"%(count, string+'\n')
    
    res = "☆%08d☆\n%s★%08d★\n%s\n"%(count, string+'\n', count, string+'\n')
    '''
    '''
    res = "○%08d○%s○\n%s●%08d●%s●\n%s\n"%(count, name, string+'\n', count, name, string+'\n')
    '''
    res = "●%08d●%s○%s○\n%s\n"%(count, name1, name2, string+'\n')
    
    return res


#搜索二进制文件，获取name和text首地址
def GetOffset(src):
    size = len(src.read())
    src.seek(0x10)
    indexoffset = byte2int(src.read(4))
    count = (size - indexoffset) // 4
    src.seek(indexoffset)
    buff = src.read()
    offset_list = []
    off = 0
    j = 0
    for i in range(0, count):
        if buff[j:j+4] == b'\x69\x00\x00\x00':     
            offset_list.append(indexoffset+j+4)
        j += 4
    return offset_list

#传入地址，返回字符串、长度、首地址、末地址
def DumpString(src, offset):
    if offset == 0:return ''
    src.seek(offset)
    #过滤非文本
    temp = src.read(1)
    if temp == b'\x00':
        return ''
    else:
        temp = src.read(1)
        if temp == b'\x00':
            return ''

    src.seek(offset)  
    string = ''
    bytestream = b''
    while True:
        byte = src.read(1)
        if byte == b'\x00':
            try:
                string += str(bytestream, encoding='sjis', errors='ignore')
            except:
                global LOG
                LOG.write(bytestream+b'\n')
                string = "Error:%08x\n"%(offset)
            bytestream = b''
            break            
        else:
            bytestream += byte
    return string

num = 0
#将地址列表转换为文本列表
def Convert(src, offset_list):
    str_list = []
    for off in offset_list:
        src.seek(off)
        sname_off = byte2int(src.read(4))
        rname_off = byte2int(src.read(4))
        src.seek(4,1)
        textoff = byte2int(src.read(4))
        
        show_name = DumpString(src, sname_off)
        real_name = DumpString(src, rname_off)
        text = DumpString(src, textoff)
        global num
        num += len(text)
        str_list.append([show_name, real_name, text])
    return str_list

def Main():
    fl = walk('bin')
    for fn in fl:
        dstname ='script' + fn[3:-4] + '.txt'
        dst = open(dstname, 'w', encoding='utf16')
        src = open(fn, 'rb')

        offset_list = GetOffset(src)
        str_list = Convert(src, offset_list)
        j = 0
        for [sname, rname, text] in str_list:
            res = FormatString(sname, rname, text, j)
            dst.write(res)
            j += 1
        print(os.path.basename(fn) + '-->' + os.path.basename(dstname))
        src.close()
        dst.close()
        
    fl = walk('script')
    for fn in fl:
        if os.path.getsize(fn) < 1:
            os.remove(fn)

'''
src = open('00_common_01_02.bin', 'rb')
offset_list = GetOffset(src)
print(offset_list)
str_list = Convert(src, offset_list)

'''

Main()
LOG.close()
if os.path.getsize('ErrorLog.bin') < 1:
    os.remove('ErrorLog.bin')

print(num)
print(num*2/(1024*1024))
input('\n导出完成\n')





















