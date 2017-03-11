# -*- coding:utf-8 -*-

import struct

# 将4字节byte转换成整数
def byte2int(byte):
    long_tuple = struct.unpack('L', byte)
    long = long_tuple[0]
    return long

def dumpstring(file, offset):
    file.seek(offset)

    string = ''
    bytestream = b''

    for _ in range(0, 0xffffffff):
        byte = file.read(1)
        char_tuple = struct.unpack('B', byte)
        char = char_tuple[0]
        if char == 0:
            byte = file.read(1)
            char_tuple = struct.unpack('B', byte)
            char2 = char_tuple[0]

            if char2 == 0:
                string += str(bytestream, encoding='sjis')
                # string += str(b'\x81\x9a', encoding='sjis', errors='ignore')
                break
            else:
                bytestream += struct.pack('B', char)
                bytestream += struct.pack('B', char2)
        else:
            bytestream += struct.pack('B', char)
    return string


def scriptout(file, indexoffset, nameoffset, scriptoffset, count):
    res = "☆%08x☆\n" % (indexoffset)

    if nameoffset != 0:
        name = dumpstring(file, nameoffset)
        res += "★%08d★%s\n" % (count, name)
    else:
        name = ''
        res += "★%08d★%s\n" % (count, name)

    string = dumpstring(file, scriptoffset)
    res += "○%08d○%s\n" % (count, string)
    res += "●%08d●%s\n\n" % (count, string)
    return res

script_path = r'd:/archive/'
index = open(script_path + 'scenario.dat', 'rb')
src = open(script_path + 'textdata.bin', 'rb')


index.seek(0, 2)
startoffset = 0x20
endoffset = index.tell()
index.seek(0)

indexoffsetlist = []
scriptoffsetlist = []
nameoffsetlist = []

# 检测提取文本的完整性
textoffset_set = set()
src.seek(0xC)
text_count_byte = src.read(4)
text_count = byte2int(text_count_byte)
src.seek(0)

# 获取各地址存入相应列表
index.seek(startoffset)
for j in range(0, 0xffffffff):
    long_byte = index.read(4)
    long = byte2int(long_byte)
    if long == 0x80000307 or long == 0x80000406:
        indexoffsetlist.append(index.tell() - 4)

        index.seek(index.tell() + 4)

        nameoffset_byte = index.read(4)
        nameoffset = byte2int(nameoffset_byte)
        nameoffsetlist.append(nameoffset)
        textoffset_set.add(nameoffset)
        
        scriptoffset_byte = index.read(4)
        scriptoffset = byte2int(scriptoffset_byte)
        scriptoffsetlist.append(scriptoffset)

        textoffset_set.add(scriptoffset)
    elif long == 0x01010203 or long == 0x81010102 or long == 0x01000D02 or long == 0x01010804:
        indexoffsetlist.append(index.tell() - 4)

        nameoffsetlist.append(0)

        scriptoffset_byte = index.read(4)
        scriptoffset = byte2int(scriptoffset_byte)
        scriptoffsetlist.append(scriptoffset)
        
        textoffset_set.add(scriptoffset)
    elif long == 0x03000303 or long == 0x01003004 or long == 0x0404130C:
        indexoffsetlist.append(index.tell() - 4)
        index.seek(index.tell() + 4)

        nameoffsetlist.append(0)

        scriptoffset_byte = index.read(4)
        scriptoffset = byte2int(scriptoffset_byte)
        scriptoffsetlist.append(scriptoffset)
        
        textoffset_set.add(scriptoffset)
    else:
        pass
    if index.tell() >= endoffset:
        break

print('text_count', text_count)
# nameoffset 0 will be also added in the textoffset_set
print('textoffset_set', len(textoffset_set) - 1)
# textoffset_list = list(textoffset_set)
if text_count != len(textoffset_set) - 1:
    print('Warning: not all text extract!')
# 导出文本
size = len(indexoffsetlist)
fullname = script_path + 'script.txt'
dst = open(fullname, encoding='utf16', mode='w')

for k in range(0, size):
    string = scriptout(src, indexoffsetlist[k], nameoffsetlist[k], scriptoffsetlist[k], k)
    dst.write(string)
dst.close()

#     del indexoffsetlist[:]
#     del scriptoffsetlist[:]
#     del nameoffsetlist[:]

index.close()
src.close()
