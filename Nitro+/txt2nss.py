# -*- coding:utf-8 -*-

import struct,os,fnmatch,re

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


def ScriptParser(lines):
    string_list = []
    num = len(lines)
    for index,line in enumerate(lines):
        if re.match('●[0-9A-Fa-f]+●', line):
            i = 1
            string = ''
            while True:
                if index+i >= num:
                    break
                if re.match('●[0-9A-Fa-f]+●', lines[index+i]):
                    break
                string += lines[index+i]
                i += 1
            string_list.append(string[:-1].replace('"', '”'))
    return string_list


#将txt转换成文本列表
def makestr(lines):
    string_list = []
    num = len(lines)
    for index,line in enumerate(lines):
        if '<PRE @box0>' in line:
            i = 1
            string = ''
            while True:
                if index+i >= num:
                    break
                if '</PRE>' in lines[index+i]:
                    break
                string += lines[index+i]
                i += 1
            string_list.append(string[:-1])
    return string_list

def RecoverNSS(lines, boxlist):
    string_list = []
    num = len(lines)
    j = 0
    needed = True
    for index,line in enumerate(lines):
        if '<PRE @box0>' in line:
            needed = False
            i = 1
            while True:
                if index+i >= num:
                    break
                if '</PRE>' in lines[index+i]:
                    break
                i += 1
            string_list.append( '<PRE @box0>\n' + boxlist[j] + '\n</PRE>\n')
            j += 1
        else:
            if needed:
                string_list.append(line.encode('sjis').decode('gbk'))
            if '</PRE>' in line:
                needed = True
    return string_list

def StringFilter(string):
    text = re.search(r'\[text.+\]', string).group()
    string = string.replace(text, '')
    code_list = re.findall('{.+?}', string, re.S)
    for code in code_list:
        string = string.replace(code, '<code>')
    return string

def BoxRecover(jpbox, cnbox):
    text = re.search(r'\[text.+\]', jpbox).group()
    code_list = re.findall('{.+?}', jpbox, re.S)
    for code in code_list:
        cnbox = cnbox.replace('<code>', code.encode('sjis').decode('gbk'), 1)
    return text + cnbox

def Main():
    fl = walk('nss')
    for fn in fl:
        dstname = 'done' + fn[3:]
        dst = open(dstname, 'w', encoding='gbk', errors='ignore')

        srcname = 'script' + fn[3:-4] + '.txt'
        src = open(srcname, 'r', encoding='utf16')

        raw = open(fn, 'r', encoding='sjis', errors='ignore')

        jp_lines = raw.readlines()
        cn_lines = src.readlines()

        jp_list = makestr(jp_lines)
        cn_list = ScriptParser(cn_lines)

        if len(jp_list) != len(cn_list):
            input('error!')
            exit(1)

        box_list = []
        for jpbox, cnbox in zip(jp_list, cn_list):
            box_list.append(BoxRecover(jpbox, cnbox))

        dst_list = RecoverNSS(jp_lines, box_list)
        for string in dst_list:
            dst.write(string)

        print(fn, '-->', dstname)
        src.close()
        dst.close()
        raw.close()


Main()
            



















        
    
    


