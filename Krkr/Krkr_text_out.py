# -*- coding:utf-8 -*-

import struct,os,fnmatch,re,zlib

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
    res = ''
    flag = False
    if string != '':
        flag = True
        '''
        res = "★%08d★\n%s\n"%(count, string)
        '''
        res = "☆%08d☆\n%s★%08d★\n%s\n"%(count, string, count, string)
 
    else:
        flag = False

    return flag, res

def StringFilter(string):
    left = b'\x6a\x22'.decode('utf16')
    right = b'\x6b\x22'.decode('utf16')
    if left in string:
        string = string.replace(left, '《')
    if right in string:
        string = string.replace(right, '》')
    return string
    
        

f_lst = walk('script')
for fn in f_lst:
    dstname = 'Txt' + fn[6:-3] + '.txt'
    dst = open(dstname,'w', encoding='utf16')
    src = open(fn, 'r', encoding='utf16')
    lines = src.readlines()

    num = len(lines)
    stringline = ''
    j = 0
    for index, line in enumerate(lines):
        
        if (line[0] != '*'
            and line[0] != '*'
            and line[0] != '['
            and line != '\n'):
            if index < num -1:
                if line[:5] == '@nm t' or line[:7] =='@mlt_nm':
                    stringline += line
                    stringline = StringFilter(stringline)
                    flag, string= FormatString(stringline, j)
                    dst.write(string)
                    j += 1
                    stringline = ''
                    continue
                if lines[index+1] != '\n' and lines[index+1][0] != '@' and lines[index+1][0] != '*' and lines[index+1][0] != '[':
                    if line[0] != '@':
                        stringline += line
                    else:
                        continue
                elif line[0] != '@':
                    stringline += line
                    stringline = StringFilter(stringline)
                    flag, string= FormatString(stringline, j)
                    j += 1
                    dst.write(string)
                    stringline = ''
    print(dstname)
    src.close()
    dst.close()

