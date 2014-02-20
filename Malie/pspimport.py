# -*- coding:utf-8 -*-

import struct,os,fnmatch,codecs

#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist

#将码表转成字典
def makedic():
    dic={}
    tbl=codecs.open('in.tbl','r+b','utf_16')
    for line in tbl:
        line_split=line.replace('\r\n','').split(u'=')
        dic[line_split[1]]=line_split[0].lower()
    tbl.close()
    return dic

mydic=makedic()

#将txt转换成文本列表
def makestr(lines):
    string_list = []
    num = len(lines)
    for index,line in enumerate(lines):
        if u'####' in line:
            i = 1
            string = ''
            while True:
                if index+i >= num:
                    break
                if '####' in lines[index+i]:
                    break
                string += lines[index+i]
                i += 1
            string_list.append(string[:-4])
    return string_list

#将码表的编码转换为字符
def code2string(code,code_len):
    string = ''
    for j in xrange(0,code_len,2):
        string += chr(int((code[j]+code[j+1]),16))
    return string

fl = walk('PSP')
for fn in fl:
    #过滤非txt文件
    if not fnmatch.fnmatch(fn,'*.txt'):
        continue
    print fn
    lines = codecs.open(fn,'rb','utf16').readlines()
    #获得文本列表
    string_list = makestr(lines)
    
    dst = open(fn[:-4],'rb+')
    start = 0x36a32e
    dst.seek(start)
    j = 0
    #逐条写入文本列表里面的文本
    for string in string_list:
        offset = dst.tell()-start
        charlist = []
        len_string = len(string)
        i = 0
        while i < len_string:
            char = string[i]
 
            if char in mydic:
                charlist.append(mydic[char])
            elif char == '\r':
                charlist.append('0d')
            elif char == '\n':
                charlist.append('0a')
            else:
                pass
            i += 1
        charlist.append('00')
        code = ''.join(charlist)
        code_len = len(code)
        string_len = len(code2string(code,code_len))-1
        dst.write(code2string(code,code_len))
        pos = dst.tell()


        dst.seek(0x31c450+2+j)
        j += 8
        dst.write(struct.pack('I',offset))
        dst.write(struct.pack('I',string_len))
        dst.seek(pos)
 
