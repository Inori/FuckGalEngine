# -*- coding:utf-8 -*-
__author__ = 'asukawang'


import os,fnmatch

#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist


alnum = [' ', ';', '*', ',', '"', '.', '\\',':', '#',
         '\n', '_', '/', '%', '=','-', '\t', '<', '>', '&', '$', '+','!','(',')',
         '0','1','2','3','4','5','6','7','8','9',
         'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
         'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z']


anset = set()
for c in alnum:
    anset.add(c)

def is_alnum(string):
    for s in string:
        if s in anset:
            continue
        else:
            return False
    return True


def FormatString(string, count):
    #res = "●%08d●%s\n\n"%(count, string)
    res = "○%08d○%s\n●%08d●%s\n\n"%(count, string,count, string)
    return res

def linefilter(line):
    off = line.find('>')
    string = line[off+1:].rstrip('\n')
    return string

def main():
    fl = walk('scenario')
    for fn in fl:
        if not fnmatch.fnmatch(fn, '*.txt'):
            continue
        if '_sg.txt' in fn:
            continue
        src = open(fn, 'r', encoding='utf8')
        dstname = fn[:-4] + '_db.txt'
        dst = open(dstname, 'w', encoding='utf16')
        lines = src.readlines()
        count = 0
        for line in lines:
            if not is_alnum(line):
                dst.write(FormatString(linefilter(line), count))
                count += 1
        src.close()
        dst.close()

if __name__ == '__main__':
    main()






























