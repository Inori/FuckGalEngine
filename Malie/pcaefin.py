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

#将编码转换为字符
def code2string(code,code_len):
    string = ''
    for j in xrange(0,code_len,2):
        string += chr(int((code[j]+code[j+1]),16))
    return string

fl = walk('script')
for fn in fl:
    #过滤非txt文件
    if not fnmatch.fnmatch(fn,'*.txt'):
        continue
    print fn
    lines = codecs.open(fn,'rb','utf16').readlines()
    #获得文本列表
    string_list = makestr(lines)
    
    dst = open(fn[:-4],'rb+')
    start = 0x3395aa
    indexoffset = 0x7a8f06
    dst.seek(start)
    offset_list = []
    j = 0
    #逐条写入文本列表里面的文本
    for string in string_list:
        offset = dst.tell()-start
        charlist = []
        len_string = len(string)
        i = 0
        while i < len_string:
            char = string[i]
 
            if char == u'▲':
                charlist.append(code2string('07000800',8))
            elif char == u'△':
                charlist.append(code2string('07000900',8))
            elif char == u'◆':
                charlist.append(code2string('07000400',8))
            elif char == u'▽':
                charlist.append(code2string('0000',4))
            elif char == u'◎':
                charlist.append(code2string('07000600',8))
            elif char == u'▼':
                charlist.append(code2string('0a00',4))
            elif char == u'★':
                charlist.append(code2string('07000100',8))
            elif char == '\r':
                charlist.append(code2string('0d00',4))
            elif char == '\n':
                charlist.append(code2string('0a00',4))
            else:
                charlist.append(char.encode('utf-16-le'))
            i += 1
        charlist.append(code2string('0000',4))
        code = ''.join(charlist)
        dst.write(code)
        offset_list.append(struct.pack('I',offset))
        
    dst.write(struct.pack('I', len(offset_list)))
    for of in offset_list:
        dst.write(of)
print 'OK!'
