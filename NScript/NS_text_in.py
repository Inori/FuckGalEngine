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
    #res = "●%08d●%s\n\n"%(count, string)
    res = "○%08d○%s\n●%08d●%s\n\n"%(count, string,count, string)
    return res


#将txt转换成文本列表
def makestr(lines):
    string_list = []
    for index,line in enumerate(lines):
        s = re.match('●[0-9A-Fa-f]+●', line)
        if re.match('●[0-9A-Fa-f]+●', line):
            string_list.append(line[10:])
        elif re.match('○[0-9A-Fa-f]+○', line):
            continue
        elif line == '\n':
            continue
        else:
            print(line)
    return string_list


alnum = [' ', ';', '*', ',', '"', '.', '\\',':', '#',
         '\n', '_', '/', '%', '=','-', '\t', '<', '>', '&', '$', '+','!','(',')',
         '0','1','2','3','4','5','6','7','8','9',
         'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
         'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z']


andic = {}
for c in alnum:
    andic[c] = 0

def is_alnum(string):
    for s in string:
        if s in andic:
            continue
        else:
            return False
    return True


def linefilter(line):
    if line[-2] == '@' or line[-2] == '\\':
        string = line[:-2]
    else:
        string = line[:-1]
    return string

def fixline(oldstr, newstr):
    if oldstr[-2] == '@':
        string = newstr[:-1] + '@' + '\n'
    elif oldstr[-2] == '\\':
        string = newstr[:-1] + '\\' + '\n'
    else:
        string = newstr

    return string



raw = open('nscript.dat', 'r', encoding='sjis')

src = open('script_dbl.txt', 'r', encoding='utf16')
lines = src.readlines()
str_list = makestr(lines)

dst = open('nscript.out', 'w', encoding='gbk', errors='ignore', newline='\n')

log = open('log.txt', 'w')

dst_list = []
i = 0
for line in raw.readlines():
    if not is_alnum(line) and line != '\n':

        dst_list.append(fixline(line, str_list[i]))
        if(len(str_list[i])>=24):
            log.write(str(i)+'\n')
        #dst.write(FormatString(linefilter(line), i))
        i += 1
    else:
        dst_list.append(line)

for line in dst_list:
    dst.write(line)

src.close()
dst.close()
raw.close()
log.close()




























