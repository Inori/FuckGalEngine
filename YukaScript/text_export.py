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

#传入地址，返回字符串、长度、首地址、末地址
def StringOut(src, offset):
    src.seek(offset)

    string = ''
    bytestream = b''

    for i in range(0, 0xffffffff):
        byte = src.read(1)
        char_tuple = struct.unpack('B', byte)
        char = char_tuple[0]
        if char == 0:
            string += str(bytestream, encoding='sjis', errors='ignore')
            bytestream = b''
            endoffset = src.tell()
            break            
        else:
            bytestream += struct.pack('B', char)
    stringlen = endoffset - offset
    return string, stringlen, offset, endoffset

def FormatString(src, offset, flagoffset, count):
    #格式说明：
    #●flag地址●字符串长度●字符串首地址●字符串末地址●
    #★字符串行数★字符串
    
    string, length, startoffset, endoffset = StringOut(src, offset)
    flag = False
    if string[0:3] != 'yks' and string[0:3] != 'ykg' and string[0:3] != 'ogg' and string[0:3] != 'YKG':
        res = "◎%08x◎\n"%(flagoffset)
        res += "○%08x○\n"%(length)
        res += "◇%08x◇\n"%(startoffset)
        res += "☆%08x☆\n"%(endoffset)
        res += "★%08d★\n%s\n\n"%(count, string)
        flag = True
    else:
        res = ''
        flag = False
    return flag, res
    
fl = walk('script')
for fn in fl:
    #过滤非yks文件
    '''
    if not fnmatch.fnmatch(fn,'*.YKS'):
        continue
    '''
    print(fn)
    src = open(fn, 'rb')
    fullname = 'txt' + fn[6:-4] + '.txt'
    dst = open(fullname, encoding = 'utf16', mode = 'w')

    src.seek(0x18)
    indexoffset = byte2int(src.read(4))
    indexsize = byte2int(src.read(4)) << 4
    textoffset = byte2int(src.read(4))

    src.seek(indexoffset)
    j = 0
    count = 0
    for i in range(0, indexsize, 0x10):
        flagoffset = indexoffset + j*0x10
        src.seek(flagoffset)
        j += 1
        flag = byte2int(src.read(4))
        if flag == 5:
            src.seek(4, 1)
            offset = byte2int(src.read(4)) + textoffset
            src.seek(offset)
            is_havestr, string = FormatString(src, offset, flagoffset, count)
            if is_havestr:
                dst.write(string)
                count += 1
            
    src.close()
    dst.close()
    

    
    
