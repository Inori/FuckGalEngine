# -*- coding:utf-8 -*-
__author__ = 'Asuka'

import os, re

#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist

def FormatString(count, str1, str2):

    res = "○%d○%s\n●%d●%s\n\n"%(count,str1, count, str2)

    return res
#恢复原来的行字符串
def formatline(line):
    line = line[6:]
    m = re.match('\[(.*)\]', line)
    if m:
        name = m.group(1)
        if name == '':
            text = line[2:-1]
            return text
        fmtname = m.group()
        nm =  re.search('（(.*)）', name)
        if  nm:
            dummy = nm.group()
            name = name.replace(dummy, '')
        text = '「' + line[len(fmtname):-1] + '」'
        string = name + text
        return string
    else:
        print('no [] found!')
        input()
        exit(1)

    return ''

#将文本做成列表
def makelist(lines):
    jp_list = []
    cn_list = []
    for line in lines:
        if line[0] == 'O':
            jp_list.append(formatline(line))
        elif line[0] == 'C':
            cn_list.append(formatline(line))
        elif line == '\n':
            pass
        else:
            print('line format wrong!')
            input()
            exit(1)
    return jp_list, cn_list

#将psp文本做成字典，键为日文，值为中文
def makedic(jp_list, cn_list):
    dic = {}
    for jp, cn in zip(jp_list, cn_list):
        dic[jp] = cn
    return dic

#将txt转换成文本列表
def makestr(lines):
    jp__list = []
    cn_list = []
    for index,line in enumerate(lines):
        s = re.match('○[0-9]+○', line)
        if s:
            tag = s.group()
            taglen = len(tag)
            string = line[taglen:-1]
            jp__list.append(string)

        s = re.match('●[0-9]+●', line)
        if s:
            tag = s.group()
            taglen = len(tag)
            string = line[taglen:-1]
            cn_list.append(string)
    return jp__list, cn_list

def Main():
    psp = open('psp.txt', 'r', encoding='utf8')
    log = open('log.txt', 'w')
    txt = open('nomatch.txt', 'w', encoding='utf16')
    
    print('making dic........')
    psp_jp_list , psp_cn_list = makelist(psp.readlines())
    psp_dic = makedic(psp_jp_list , psp_cn_list)
    print('making over!')

    fl = walk('text')
    for fn in fl:
        src = open(fn, 'r', encoding='utf16')

        dstname = 'psp2pc'+ fn[4:-13] + '.txt'
        dst = open(dstname, 'w', encoding='utf16')

        txt.write('>>>>>>>>>>>>>>>>>>>>>>>'+dstname+'>>>>>>>>>>>>>>>>>>>>>>>\n\n')
        
        pc_jp_list , pc_cn_list = makestr(src.readlines())

        if len(pc_jp_list) != len(pc_cn_list):
            input(fn + 'lines not equal')
            exit(1)

        dst_list = []
        i = 1
        for jp, cn in zip(pc_jp_list, pc_cn_list):

            if jp in psp_dic:
                dst_list.append([jp, psp_dic[jp]])
            else:
                dst_list.append([jp, cn])
                string = FormatString(i, jp, cn)
                txt.write(string)
                log.write(fn + ':' + str(i) + '\n\n')
            i += 1



        j = 1
        for [jp ,cn] in dst_list:
            string = FormatString(j, jp, cn)
            dst.write(string)
            j += 1
        src.close()
        dst.close()
        print(dstname)

    psp.close()
    log.close()
    txt.close()

Main()

