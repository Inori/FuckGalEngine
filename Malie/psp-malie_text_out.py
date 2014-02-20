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

def FormatString(string, count):
    #格式说明：
    #★字符串行数★字符串
    '''
    res = "★%08d★\n%s\n"%(count, string+'\n')
    '''
    res = "☆%08d☆\n%s★%08d★\n%s\n"%(count, string+'\n', count, string+'\n')
    

    return res

def GetEntry(src):
    start = 0x2889a8
    src.seek(start)
    end = 0x2c0c0c -4
    count = (end-start)//8

    entry_list = []
    for i in range(0, count):
        offset = byte2int(src.read(4))
        length = byte2int(src.read(4))
        entry_list.append([offset, length])
    return entry_list

def StringFilter(string):
    dic = {
            b'\x07\x06':'◎'.encode('sjis'),
            b'\x07\x04':'\n'.encode('sjis'),
            b'\x07\x08':'▲'.encode('sjis'),
            b'\x07\x3c':'【'.encode('sjis'),
            b'\x06\x0c':'<large>'.encode('sjis'),
            b'\x07\x09':'△'.encode('sjis')
            
          }
    dic2 = {
            b'\x0A':'】'.encode('sjis'),
            b'\x01':'<'.encode('sjis'),
            b'\x02':'><color>'.encode('sjis'),
            b'\x00':'▽'.encode('sjis')
           }


    strlen = len(string)
    count2 = strlen
    k = 0
    for j in range(0, count2):
        if string[k:k+1] in dic2:  
            string = string[:k] + dic2[string[k:k+1]] + string[k+1:]
        k += 1

    keylist = list(dic.keys())
    count = len(keylist)
    for i in range(0, count):
        if keylist[i] in string:
            string = string.replace(keylist[i], dic[keylist[i]])
            


           
    return string

src = open('exec.dat', 'rb')
dst = open('exec.dat.txt', 'w', encoding='utf16')
entry = GetEntry(src)

j = 0
for [offset, length] in entry:
    src.seek(offset + 0x2c0c0c)
    bstring = src.read(length)
    try:
        #string = bstring.decode('sjis', errors='ignore')
        string = StringFilter(bstring).decode('sjis', errors='ignore')
    except:
        LOG.write(bstring+b'\n')
        print('Decode Error')
        string = 'Decode Error'
        continue
    dst.write(FormatString(string,j))
    #dst.write(string[:-2]+'\n')
    j += 1

src.close()
dst.close()    











    

