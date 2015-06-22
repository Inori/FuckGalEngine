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

#将txt转换成文本列表
def makestr(lines):
    string_list = []
    for index,line in enumerate(lines):
        if re.match('●[0-9A-Fa-f]+●', line):
            string_list.append(line[10:])
        elif re.match('○[0-9A-Fa-f]+○', line):
            pass
        elif not line == '\n':
            print(line)
    return string_list

alnum = [' ','.',',','[',']', '_',
         '-', '\t',
         '0','1','2','3','4','5','6','7','8','9',
         'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
         'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z']

def is_alnum(string):
    for s in string:
        if s in alnum:
            continue
        else:
            return False
    return True

def m_split(string):
    if not '★' in string:
        print('name format error')
        print(string)
        input()
    return string.split('★')

fl = walk('script')
for fn in fl:
    src = open(fn, 'r', encoding='utf16')
    lines = src.readlines()
    cn_lines = makestr(lines)

    rawname = 'sc' + fn[6:-3] + 'sc'
    raw = open(rawname, 'r', encoding='sjis')
    raw_lines = raw.readlines()

    dstname = 'done' + fn[6:-3] + 'sc'
    dst = open(dstname, 'w', encoding='gbk', errors='ignore')
    
    dst_lines = []
    i = 0
    j = 0
    for line in raw_lines:
        '''
        if '★' in line:
            print('fuck!')
            input()
        '''
        if '.message' in line:
            i += 1
            #print(line.split(' '))
            *dm, name, text = line.split(' ')
            #测试
            for t in dm:
                if not is_alnum(t):
                    print('\n')
                    print(t)
                    print(line.split(' '))
                    print(fn)
                    print(j)
                    input()
            
            if not is_alnum(name):
                cur_name, cur_text = m_split(cn_lines[j])
                j += 1
                dm.append(name.encode('sjis').decode('gbk'))#保持乱码
                dm.append(cur_text)
            else:
                dm.append(name)
                dm.append(cn_lines[j])
                j += 1
                
            dst_lines.append(' '.join(dm))
        else:
            dst_lines.append(raw_lines[i])
            i += 1

    for string in dst_lines:
        dst.write(string)
    
    print(dstname)
    src.close()
    dst.close()





























