# -*- coding:utf-8 -*-

import os, json
from collections import OrderedDict

#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist


def makestr(lines):
    str_list = []
    for line in lines:
        if line[0] != '★':
            str_list.append(line[:-1])
    return str_list


def Main():
    fl = walk('script')
    for fn in fl:
        dstname = 'json' + fn[6:-4] + '.ks.json'
        dst = open(dstname, 'w', encoding='utf_8_sig')

        rawname = 'scenario' + fn[6:-4] + '.ks.json'
        raw = open(rawname, 'r', encoding='utf_8_sig')

        src = open(fn, 'r', encoding='utf16')



        item_list = json.load(raw, object_pairs_hook=OrderedDict)
        #num = len(item_list)

        string_list = makestr(src.readlines())

        i = 0
        for idx, item in enumerate(item_list):
            '''
            if len(item) == 1:
                if idx+1 >= num: break
                if 'text' in item_list[idx+1]:
                    string_list.append([True, item[0]])
            if len(item) == 2:
                if idx+1 >= num: break
                if 'text' in item_list[idx+1]:
                    if 'disp' in item[1]:
                        string_list.append([True, item[0]])
                        string_list.append([True, item[1]['disp']])
                    if 'voice' in item[1]:
                        string_list.append([True, item[0]])
            '''
            if 'text' in item:
                item[1]['text'] = string_list[i]
                i += 1

        json.dump(item_list, dst, ensure_ascii=False, separators = (',', ':'))


        print(dstname)
        raw.close()
        src.close()
        dst.close()

Main()


