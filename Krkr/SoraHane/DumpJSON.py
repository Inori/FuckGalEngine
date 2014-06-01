# -*- coding:utf-8 -*-

import os, json


#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist

def FormatString(string, count):
    #res = "●%08d●%s○%s○\n%s\n"%(count, name1, name2, string+'\n')
    #res = "●%08d●%s\n\n"%(count, string)
    #res = "○%08d○%s\n●%08d●%s\n\n"%(count, string, count, string)
    #res = "☆%08d☆\n%s\n\n"%(count, text)

    mode = string[0]
    text = string[1]
    #人名
    if mode:
        res = "★%08d★%s\n☆%08d☆%s\n\n"%(count, text, count, text)
    else:
        res = "○%08d○%s\n●%08d●%s\n\n"%(count, text, count, text)
    return res


def Main():
    fl = walk('scenario')
    for fn in fl:
        dstname = 'script' + fn[8:-8] + '.txt'
        dst = open(dstname, 'w', encoding='utf16')

        src = open(fn, 'r', encoding='utf_8_sig')
        
        item_list = json.load(src)
        num = len(item_list)
        
        string_list = []
        #如果后一项不是text，则判断此项不是人名，这里主要是为了防止把场景、舞台等文本导出
        #可能存在部分人名未导出的bug
        for idx, item in enumerate(item_list):
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
            if 'text' in item:
                string_list.append([False, item[1]['text']])
                
        i = 0
        for string in string_list:   
            #dst.write(string+'\n')
            dst.write(FormatString(string, i))
            i += 1
                
        print(dstname)
        src.close()
        dst.close()

Main()
