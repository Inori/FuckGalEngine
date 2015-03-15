# -*- coding:utf-8 -*-

import struct


#将4字节byte转换成整数
def byte2int(byte):
    long_tuple=struct.unpack('L',byte)
    long = long_tuple[0]
    return long

def dumpstring(file,offset):
    file.seek(offset)

    string = ''
    bytestream = b''
    
    for i in range(0, 0xffffffff):
        byte = file.read(1)
        char_tuple = struct.unpack('B', byte)
        char = char_tuple[0]
        if char == 0:
            byte = file.read(1)
            char_tuple = struct.unpack('B', byte)
            char2 = char_tuple[0]

            if char2 == 0:
                string += str(bytestream, encoding='sjis', errors='ignore')
                #string += str(b'\x81\x9a', encoding='sjis', errors='ignore')
                break
            else:
                bytestream += struct.pack('B', char)
                bytestream += struct.pack('B', char2)
        else:
            bytestream += struct.pack('B', char)   
    return string


def scriptout(file,indexoffset,nameoffset,scriptoffset,count):
    res = "☆%08x☆\n"%(indexoffset)
    
    if nameoffset != 0:
        name=dumpstring(file,nameoffset)
        res += "★%08d★%s\n"%(count, name)
    else:
        name=''
        res += "★%08d★%s\n"%(count, name)
        
    string = dumpstring(file,scriptoffset)
    res += "○%08d○%s\n"%(count, string)
    res += "●%08d●%s\n\n"%(count, string)
    return res
        


#startoffset=[0x469c, 0x6c400, 0xaed98, 0xe7178, 0x113964]
#endoffset = [0x6c3c0, 0xaebdc, 0xe7118, 0x113900, 0x16d67f]
startoffset=[0]
endoffset = [0xd2020]

index = open('scenario.dat', 'rb')
src = open('textdata.bin', 'rb')  

indexoffsetlist=[]
scriptoffsetlist=[]
nameoffsetlist=[]


#获取各地址存入相应列表
for i in range(0,1):
    index.seek(startoffset[i])
    for j in range(0,0xffffffff):
        
        long_byte = index.read(4)
        long = byte2int(long_byte)
        if long == 0x80000307 or long == 0x80000406:        
            indexoffsetlist.append(index.tell()-4)
            
            index.seek(index.tell()+4)
            
            nameoffset_byte=index.read(4)
            nameoffset=byte2int(nameoffset_byte)
            nameoffsetlist.append(nameoffset)
            
            scriptoffset_byte=index.read(4)
            scriptoffset=byte2int(scriptoffset_byte)
            scriptoffsetlist.append(scriptoffset)
        elif long == 0x01010203:
            indexoffsetlist.append(index.tell()-4)

            nameoffsetlist.append(0)
            
            scriptoffset_byte=index.read(4)
            scriptoffset=byte2int(scriptoffset_byte)
            scriptoffsetlist.append(scriptoffset)
        elif long == 0x01010804:
            indexoffsetlist.append(index.tell()-4)

            nameoffsetlist.append(0)

            scriptoffset_byte=index.read(4)
            scriptoffset=byte2int(scriptoffset_byte)
            scriptoffsetlist.append(scriptoffset)
        elif long == 0x03000303:
            indexoffsetlist.append(index.tell()-4)
            index.seek(index.tell()+4)

            nameoffsetlist.append(0)
            
            scriptoffset_byte=index.read(4)
            scriptoffset=byte2int(scriptoffset_byte)
            scriptoffsetlist.append(scriptoffset)
            
        else:
            pass
        if index.tell() >= endoffset[i]:
            break
        
#导出文本
    size = len(indexoffsetlist)
    fullname = 'script'+str(i)+'.txt'
    dst = open(fullname, encoding='utf16', mode='w')
    
    for k in range(0,size):
        string=scriptout(src,indexoffsetlist[k],nameoffsetlist[k],scriptoffsetlist[k],k)
        dst.write(string)
    dst.close()
    
    del indexoffsetlist[:]
    del scriptoffsetlist[:]
    del nameoffsetlist[:]

index.close()
src.close() 

       
        
        
    
    





            
            
            
    
