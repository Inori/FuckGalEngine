# -*- coding:utf-8 -*-

import struct,os,fnmatch

#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist



#将key_real.bin每四个字节一组存入列表
keyfile = open('key_real.bin', 'rb')
key_tuple_list = []
keyfile.seek(0, 2)
count = keyfile.tell()
keyfile.seek(0, 0)

for i in range(0, count, 4):
    key_tuple_list.append(struct.unpack('L', keyfile.read(4)))
keylist = [x[0] for x in key_tuple_list]

    
#解密
f_txt = walk('script')

for fn in f_txt:
    src = open(fn, 'rb')
    fullname = 'rld'+'\\'+fn[7:-4]+'.rld'
    dst = open(fullname, 'rb+')
    
    dst.seek(0, 2)
    size = dst.tell()
    dst.seek(16, 0)


    j = ((size-0x10) >> 2)&0xffff
    if j > 0x3ff0:
        j = 0x3ff0


    for k in range(0, j):
        en_str = src.read(4)
        en_temp = struct.unpack('L', en_str)
        
        key_count = k
        key_count = key_count & 0xff
        temp_key = keylist[key_count] ^ 0x20121203
        
        de_temp = en_temp[0] ^ temp_key
        
        de_str = struct.pack('L', de_temp)
        
        dst.write(de_str)


        
        
    
    
    
