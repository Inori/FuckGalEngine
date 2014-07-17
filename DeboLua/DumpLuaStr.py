# -*- coding:utf-8 -*-
import os, re


#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist


def FormatString(string, count):
    '''
    res = "★%08d★\n%s\n"%(count, string+'\n')
    res = "☆%08d☆\n%s★%08d★\n%s\n"%(count, string+'\n', count, string+'\n')
    '''

    res = "○%08d○%s●%08d●%s\n"%(count, string, count, string)
    #res = "●%08d●%s●\n%s\n"%(count, name, string)

    return res

alnum = [' ', ';', '*', ',', '"', '.', '\\',':', '#', '~', '[', ']', '_', '^',
         '\n', '_', '/', '%', '=','-', '\t', '<', '>', '&', '$', '+','!','(',')', '{', '}',
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

def cvtstr(string):
    bstring = string.encode('ascii')
    regex = re.compile(rb'\\(\d{1,3})')

    def replace(match):
        m = match.group(1)
        return int(match.group(1)).to_bytes(1, byteorder='big')

    retstr = regex.sub(replace, bstring).decode('sjis')
    return retstr

def matchstr(string):
    lprog = [
    re.compile('_text\("(.*)"\)'),
    re.compile('text\("(.*)"\)'),
    re.compile('name\("(.*)"\)'),
    re.compile('save\((.*)\)'),
    re.compile('select\((.*)\)'),
    re.compile('param\((.*)\)'),
    re.compile('Message\((.*)\)')
    ]

    for prog in lprog:
        obj = prog.search(string)
        if obj:
            return obj.group(1)
    return ''

def mkstrlist(lines):
    strlist = []
    for line in lines:
        mstr = matchstr(line)
        if mstr:
            strlist.append(cvtstr(mstr))
        elif not is_alnum(line):
            print(line)  #for debug

    return strlist

def Main():
    fl = walk('script')
    for fn in fl:
        src = open(fn, 'r')

        dstname = 'text' + fn[6:-4] + '.txt'
        dst = open(dstname, 'w', encoding='utf16')

        strlist = mkstrlist(src.readlines())

        i = 0
        for s in strlist:
            curline = s.replace('\n', '↙')+'\n'
            dst.write(FormatString(curline, i))
            i += 1

        src.close()
        dst.close()

Main()

