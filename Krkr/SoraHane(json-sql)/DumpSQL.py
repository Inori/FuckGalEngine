# -*- coding:utf-8 -*-

import sqlite3

def FormatString(name, string, count):
    #格式说明：
    #★字符串行数★字符串
    '''
    res = "★%08d★\n%s\n"%(count, string+'\n')
    
    res = "☆%08d☆\n%s★%08d★\n%s\n"%(count, string+'\n', count, string+'\n')
    '''
    res = "○%08d○%s○\n%s●%08d●%s●\n%s\n"%(count, name, string+'\n', count, name, string+'\n')
    
    '''
    res = "%s\n"%(string+'\n')
    '''
    return res

#添加STRING类型
def convert_string(bstring):
    return bstring.decode('utf8')
    
sqlite3.register_converter("STRING", convert_string)

#dump两列数据制作成列表或字典
def DumpCol(con, cmd, mode):
    c = con.cursor()
    c.execute(cmd)
    name_list = []
    for row in c:
        name_list.append([row[0], row[1]])
    if mode == 'l':
        return name_list
    elif mode == 'd':
        name_dic = {}
        for [i, name] in name_list:
            name_dic[i] = name
        return name_dic
    else:
        print('Wrong Mode')
        input()

def DumpCname(con):
    cname_dic = DumpCol(con, 'select id, name from name', 'd')
    return cname_dic

def DumpFname(con):
    fname_dic = DumpCol(con, 'select id, name from storage', 'd')
    return fname_dic

def DumpChooseText(con):
    text_list = DumpCol(con, 'select idx, text from next', 'l')
    return text_list

def DumpTitle(con):
    c = con.cursor()
    c.execute('select id, storage, title from scene')
    title_list = []
    for (i, storage, title) in c:
        title_list.append([i, storage, title])
    title_dic = {}
    for [i, storage, title] in title_list:
        title_dic[i] = [storage, title]
        
    return title_list, title_dic

def DumpText(con):
    c = con.cursor()
    c.execute('select scene, name, text from text')
    text_list = []
    for (scene, name, text) in c:
        text_list.append([scene, name, text])
    return text_list

    
def Main():
    index = sqlite3.connect('scene.sdb', detect_types=sqlite3.PARSE_DECLTYPES)
    text = sqlite3.connect('scenedata.sdb', detect_types=sqlite3.PARSE_DECLTYPES)

    #匹配文件名和人名
    text_list = DumpText(text)
    f_dic = DumpFname(index)
    c_dic = DumpCname(index)
    t_list, t_dic = DumpTitle(index)
    
    string_list = []
    for [scene, name, text] in text_list:
        if scene in t_dic:
            sc = t_dic[scene][0]
            try:
                fname = f_dic[sc]
            except:
                print('fname error', sc)
                input()
        else:
            print('scene error', scene)
            input()
        if name == 0:
            cname = ''
        elif name in c_dic:
            cname = c_dic[name]
        else:
            print('cname error')
            input()
        string_list.append([fname, cname, text])
        
    #输出到文件
    rlist = []
    j = 0
    for [fname, cname, text] in string_list:
        if fname not in rlist:
            dstname = 'script' + '\\' + fname[:-3] +'.txt'
            dst = open(dstname, 'w', encoding='utf16')
            rlist.append(fname)
            j = 0
        string = FormatString(cname, text, j)
        dst.write(string)
        j += 1

Main()
