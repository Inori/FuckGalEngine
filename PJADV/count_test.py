# -*- coding:utf-8 -*-
#不考虑索引文件，导出全部文本，用于查漏
import struct, os


#将4字节byte转换成整数
def byte2int(byte):
    long_tuple=struct.unpack('L',byte)
    long = long_tuple[0]
    return long

def DumpString(src, offset):
    #if offset == 0:return ''

    src.seek(offset)  
    string = ''
    bytestream = b''
    strlen = 0
    while True:
        byte = src.read(1)
        if byte == b'\x00':
            try:
                string += str(bytestream, encoding='sjis', errors='ignore')
            except:
                string = "Error:%08x\n"%(offset)
                input('Error')
            strlen = len(bytestream)
            bytestream = b''
            break            
        else:
            bytestream += byte
    return string, strlen


def scriptout(file,indexoffset,nameoffset,scriptoffset,count):
    res = "●%08x●\n"%(indexoffset)
    
    if nameoffset != 0:
        name=dumpstring(file,nameoffset)
        res += "◎%08d◎%s\n"%(count, name)
    else:
        name=''
        res += "◎%08d◎%s\n"%(count, name)
        
    string = dumpstring(file,scriptoffset)
    res += "◆%08d◆%s\n\n"%(count, string)
    return res
        

src = open('textdata.bin', 'rb')
dst = open('test.txt', 'w', encoding = 'utf16')


size = os.path.getsize('textdata.bin')
pos = 0
str_count = 0
while True:
    string, length = DumpString(src, pos)
    pos += (length + 2)
    dst.write(string+'\n')
    str_count += 1
    if pos >= size:
        break

print(str_count)
src.close()
dst.close()


       
        
        
    
    





            
            
            
    
