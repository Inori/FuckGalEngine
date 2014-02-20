# -*- coding:utf-8 -*-
#V2 修复bug
#V3 优化代码，重写StringFilter，修复bug

import struct,os,fnmatch,re,tempfile

LOG = open('ErrorLog.bin', 'wb')

#字符串首地址
BaseOffset = 0x39EBB4

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
    #格式说明：
    #★字符串行数★字符串
    '''
    res = "★%08d★\n%s\n"%(count, string+'\n')
    
    res = "☆%08d☆\n%s★%08d★\n%s\n"%(count, string+'\n', count, string+'\n')
    '''
    #res = "○%08d○\r\n%s\r\n●%08d●\r\n%s\r\n\r\n"%(count, string, count, string)
    #res = "○%08d○------------------------------------\n%s\n●%08d●------------------------------------\n%s\n\n"%(count, string, count, string)
    res = '●%08d●------------------------------------\n%s\n\n'%(count, string)
    '''
    res = "%s\n"%(string+'\n')
    '''

    return res



def CalIndex(src, BaseOffset):
    src.seek(BaseOffset-4)
    IndexOffset = byte2int(src.read(4)) + BaseOffset
    src.seek(IndexOffset)
    StringCount = byte2int(src.read(4))
    src.seek(0)
    return IndexOffset, StringCount

def GetEntry(src):
    indexoffset, stringcount = CalIndex(src, BaseOffset)
    offset_list = []
    src.seek(indexoffset+4)
    #print(hex(indexoffset))
    for i in range(0, stringcount):
        offset = BaseOffset + byte2int(src.read(4))
        offset_list.append(offset)
        
    entry_list = []
    for index, offset in enumerate(offset_list):
        if index < stringcount - 1:
            length = offset_list[index+1] - offset - 2 #去掉结尾\x00\x00
        else:
            length = indexoffset - offset_list[stringcount-1] - 2
        entry_list.append([offset, length])
    return entry_list


def StringFilter(string):
    for note in re.findall('\x07\x01.*?\n.*?\x00', string):
        old = re.search('\x07\x01(.*)\n(.*)\x00', note).group(0)
        t1 = re.search('\x07\x01(.*)\n(.*)\x00', note).group(1)
        t2 = re.search('\x07\x01(.*)\n(.*)\x00', note).group(2)
        new = "★%s<%s>"%(t1, t2)
        string = string.replace(old, new)

    string = string.replace('\r','◇')
    string = string.replace('\x07\x06','◎')
    string = string.replace('\x07\x04','◆')
    string = string.replace('\x07\x08','▲')
    string = string.replace('\x07\x09','△')
    string = string.replace('\x00','▽')
    #string = string.replace('\n','\n')
    '''
    for x in string.split('\n'):
        if x == '': continue
        if x[-1] == '◎':
            string = string.replace(x,x[:-1])
    '''      
    return string
    
def DumpString(src, dst, entry_list):
    j = 0
    for [offset, length] in entry_list:
        src.seek(offset)
        byte = src.read(length)
        try:
            string = byte.decode('utf16')
        except:
            LOG.write(byte+b'\n')
            print('Decode Error')
            string = 'Decode Error'
            continue
        res = StringFilter(string)
        dst.write(FormatString(res,j))
        j += 1
    
def Main():
    src = open('unzlib_exec.bin.bak', 'rb')
    dst = open('exec.txt', 'w', encoding='utf16')
    entry_list = GetEntry(src)
    DumpString(src, dst, entry_list)
    src.close()
    dst.close()

Main()











    
