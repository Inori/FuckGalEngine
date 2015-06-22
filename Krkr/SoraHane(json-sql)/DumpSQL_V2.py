# -*- coding:utf-8 -*-

import sqlite3

def FormatString(rname, dname, string, count):
    res = "○%08d○%s○%s○\n%s\n●%08d●%s●%s●\n%s\n\n"%(count, rname, dname, string, count, rname, dname, string)
    
    '''
    res = "%s\n"%(string+'\n')
    '''
    return res

#添加STRING类型
def convert_string(bstring):
    return bstring.decode('utf8')
    
sqlite3.register_converter("STRING", convert_string)

def DumpItem(con, cmd):
    c = con.cursor()
    c.execute(cmd)
    item = []
    for t in c:
        item.append(t)
    return item

#获取scenedata.sdb里text表的每个条目
def GetItem(scenedata):
    return DumpItem(scenedata, 'select scene, name, disp, text from text')

#获取scene.sdb里scene表的每个条目
def GetScene(scene):
    return DumpItem(scene, 'select id, storage from scene')

#获取scene.sdb里name表里的角色名
def GetName(scene):
    return DumpItem(scene, 'select id, name from name')

#获取scene.sdb里storage表里的ks文件名
def GetStorage(scene):
    return DumpItem(scene, 'select id, name from storage')

#整合以上所有信息：依据scenedata.sdb中text表的scene项，关联scene.sdb中的scene表
#所有参数均为每个元素为 元组 的 列表，如[(), (), () ...]
def MakeAll(item, scene, name, storage):
    #将name表做成字典，键为id，值为name（角色名）
    name_dic = {}
    for idx, cname in name:
        if cname == None: cname = ''
        name_dic[idx] = cname

    #将storage表做成字典，键为id，值为name（文件名）
    storage_dic = {}
    for idx, fname in storage:
        storage_dic[idx] = fname

    #将scene表做成字典，键为id，值为storage(文件名)
    scene_dic = {}
    for idx, nstorage in scene:
        if nstorage in storage_dic:
            scene_dic[idx] = storage_dic[nstorage]
        else:
            input('no such key in storage_dic')
            exit(1)
            
    #将item表做成列表
    #变量说明：rcname:real character name, dcname:display character name, fname:file name
    item_list = []
    for nscene, nname, disp, text in item:
        if nname == 0:
            rcname = ''
        else:
            rcname = name_dic[nname]

        if disp == None:
            dcname = ''
        else:
            dcname = disp

        fname = scene_dic[nscene]

        item_list.append([fname, rcname, dcname, text])

    return item_list
    

    
def Main():
    scene_c = sqlite3.connect('scene.sdb', detect_types=sqlite3.PARSE_DECLTYPES)
    scenedata_c = sqlite3.connect('scenedata.sdb', detect_types=sqlite3.PARSE_DECLTYPES)

    item = GetItem(scenedata_c)
    scene = GetScene(scene_c)
    name = GetName(scene_c)
    storage = GetStorage(scene_c)
    
    item_list = MakeAll(item, scene, name, storage)

    n = 0
    rlist = []
    for fname, rcname, dcname, text in item_list:
        if not fname in rlist:
            dstname = 'script\\' + fname + '.txt'
            print(dstname)
            dst = open(dstname, 'w', encoding='utf16')
            rlist.append(fname)
            n = 0
        dst.write(FormatString(rcname, dcname, text, n))
        n += 1


Main()
