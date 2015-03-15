# -*- coding:utf-8 -*-

#检查count_text.py导出的文本是否被script_out.py导出的文本全部包含
import struct, os, re


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

#将txt转换成文本列表
def makestr(lines):
    string_list = []
    num = len(lines)
    for index,line in enumerate(lines):
        if re.match('◆[0-9A-Fa-f]+◆', line) or re.match('◎[0-9A-Fa-f]+◎', line):
            string = line[10:]
            string_list.append(string[:])
    return string_list

src1 = open('test.txt', 'r', encoding = 'utf16')
src2 = open('script0.txt', 'r', encoding = 'utf16')
log = open('log.txt', 'w', encoding='utf16')

raw_lines = src1.readlines()
new_lines = makestr(src2.readlines())

for string in raw_lines:
    if not string in new_lines:
        log.write(string)

src1.close()
src2.close()


       
        
        
    
    





            
            
            
    
