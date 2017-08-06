# -*- coding:utf-8 -*-

import os,re

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
    for index,line in enumerate(lines):
        s = re.match('●[0-9A-Fa-f]+●', line)
        if s:
            string_list.append(line[10:])
        elif line == '\n' or re.match('○[0-9A-Fa-f]+○', line):
            pass
        else:
            print(line)
            pass
    return string_list

def StringFilter(string):

    string = string.replace('〜', '～')
    lst = re.findall('\[.+?\]', string)
    if not lst: return string

    for part in lst:
        if 'rb' in part:
            temp = part.replace('，', ',')
            string = string.replace(part, temp)
        elif 'pc' in part:
            temp = part.replace('pc，', 'pc,')
            string = string.replace(part, temp)

    return string

def StringFilter2(string):
    if '，' in string:
        string = string.replace('，', ',')
    return string
    
        
f_lst = walk('jp')
for fn in f_lst:
    dstname = 'done' + fn[2:]
    #dst = open(dstname, 'w', encoding='gbk', errors='ignore')
    dst = open(dstname,'w', encoding='utf16', errors='ignore')

    print(dstname)

    rawname = 'cn' + fn[2:-2] + '.txt'
    raw = open(rawname, 'r', encoding='utf16')
    cn_lines = raw.readlines()
    cn_strlist = makestr(cn_lines)

    #src = open(fn, 'r', encoding='sjis', errors='ignore')
    src = open(fn, 'r', encoding='utf16', errors='ignore')
    jp_lines = src.readlines()
    
    selection_name = 'selection' + fn[2:-2] + '.txt'
    if os.path.exists(selection_name):
        selection = open(selection_name, 'r', encoding='utf16')
        selection_lines = selection.readlines()

    dstlines = []
    i = 0
    j = 0
    for index, line in enumerate(jp_lines):
        if line[0:8] == '^select,':
            dstlines.append(StringFilter2(selection_lines[i]))
            i += 1
            continue
        
        if (line[0] != '^'
            and line[0] != '\\'
            and line[0] != '@'
            and line[0] != '％'
            and line != '\n'):
            dstlines.append(StringFilter(cn_strlist[j]))
            j += 1
        else:
            #dstlines.append(line.encode('sjis').decode('gbk'))
            dstlines.append(line)

    for l in dstlines:
        dst.write(l)
    
    raw.close()    
    src.close()
    dst.close()
    

print('success')

