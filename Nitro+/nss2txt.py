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


def FormatString(string, count):
    #格式说明：
    #★字符串行数★字符串
    '''
    res = "★%08d★\n%s\n"%(count, string+'\n')
    
    res = "☆%08d☆\n%s★%08d★\n%s\n"%(count, string+'\n', count, string+'\n')
    '''
    '''
    res = "○%08d○%s○\n%s●%08d●%s●\n%s\n"%(count, name, string+'\n', count, name, string+'\n')
    '''
    
    res = "●%08d●\n%s\n"%(count, string)

    
    return res


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

def StringFilter(string):
    text = re.search(r'\[text.+\]', string).group()
    string = string.replace(text, '')
    code_list = re.findall('{.+?}', string,re.S)
    for code in code_list:
        string = string.replace(code, '<code>')
    return string

def StringFilter1(lines):
    new_list = []
    for line in lines:
        if line[-4:-1] == '<K>':
            string = line[:-4] + '\n'
            new_list.append(string)
        else:
            new_list.append(line)
    return new_list

def Main():
    fl = walk('nss')
    for fn in fl:
        dstname = 'script' + fn[3:-4] + '.txt'
        dst = open(dstname, 'w', encoding='utf16')
        src = open(fn, 'r', encoding='sjis', errors='ignore')

        
        lines = src.readlines()
        #lines = StringFilter1(rawlines)
        str_list = makestr(lines)
        j = 0
        for string in str_list:
            res = FormatString(StringFilter(string), j)
            dst.write(res)
            j += 1
        print(fn, '-->', dstname)
        src.close()
        dst.close()
    fl = walk('script')
    for fn in fl:
        if os.path.getsize(fn) < 1:
            os.remove(fn)


Main()
            



















        
    
    


