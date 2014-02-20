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

def FormatString(name, string, count):
    #格式说明：
    #★字符串行数★字符串
    '''
    res = "★%08d★\n%s\n"%(count, string+'\n')
    
    res = "☆%08d☆\n%s★%08d★\n%s\n"%(count, string+'\n', count, string+'\n')
    '''
    '''
    res = "○%08d○%s○\n%s●%08d●%s●\n%s\n"%(count, name, string+'\n', count, name, string+'\n')
    
    
    res = "●%08d●%s●\n%s\n"%(count, name, string+'\n')
    '''
    res = "%s\n%s\n"%(name, string+'\n')
    '''
    res = "%s\n"%(string+'\n')
    '''
    return res

#搜索二进制文件，获取name和text首地址
def GetOffset(src):
    buff = src.read()
    offset_list = []
    nameoff = textoff = 0
    while True:
        nameoff = buff.find(b'%L', textoff+5)
        textoff = buff.find(b'char\x00', textoff+5)
        if nameoff == -1 and textoff == -1:
            break
        else:
            if nameoff < textoff and nameoff != -1:           #有名字
                offset_list.append([nameoff+3, textoff+5])
            else:                                             #没有名字
                offset_list.append([0, textoff+5])
    return offset_list

#传入地址，返回字符串、长度、首地址、末地址
def DumpName(src, offset):
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
                global LOG
                LOG.write(bytestream+b'\n')
                string = "Error:%08x\n"%(offset)
            bytestream = b''
            break            
        else:
            bytestream += byte
    return string


#与DumpName只是结束符不同
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
        if byte == b'%':            
            nxt = src.read(1)
            if nxt == b'K' or nxt == b'P':
                try:
                    string += str(bytestream, encoding='sjis')
                except:
                    global LOG
                    LOG.write(bytestream+b'\n')
                    string = "Error:%08x\n"%(offset)
                bytestream = b''
                break
            else:
                bytestream = bytestream + byte + nxt
                continue
        else:
            bytestream += byte
    return string

#将地址列表转换为文本列表
def Convert(src, offset_list):
    str_list = []
    for[nameoff, textoff] in offset_list:
        name = DumpName(src, nameoff)
        text = DumpText(src, textoff)
        str_list.append([name, text])
    return str_list


def Main():
    fl = walk('Rio')
    for fn in fl:
        dstname ='txt' + fn[3:-4] + '.txt'
        dst = open(dstname, 'w', encoding='utf16')
        src = open(fn, 'rb')
        
        offset_list = GetOffset(src)
        str_list = Convert(src, offset_list)
        j = 0
        for [name, text] in str_list:
            res = FormatString(name, text, j)
            dst.write(res)
            j += 1
        print(os.path.basename(fn) + '-->' + os.path.basename(dstname))
        src.close()
        dst.close()
        
    fl = walk('txt')
    for fn in fl:
        if os.path.getsize(fn) < 1:
            os.remove(fn)

Main()
LOG.close()
if os.path.getsize('ErrorLog.bin') < 1:
    os.remove('ErrorLog.bin')
input('\n导出完成\n')

            
    
        
        


            
    



