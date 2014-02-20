# -*- coding:utf-8 -*-

import struct,os,fnmatch

def mkdir(path):
    # 去除首位空格
    path=path.strip()
    # 去除尾部 \ 符号
    path=path.rstrip("\\")
 
    # 判断路径是否存在
    # 存在     True
    # 不存在   False
    isExists=os.path.exists(path)
 
    # 判断结果
    if not isExists:
        # 如果不存在则创建目录
        # 创建目录操作函数
        os.makedirs(path)
        return True
    else:
        # 如果目录存在则不创建
        return False
 


#遍历文件夹，返回文件列表
def walk(adr):
    mylist=[]
    for root,dirs,files in os.walk(adr):
        for name in files:
            adrlist=os.path.join(root, name)
            mylist.append(adrlist)
    return mylist


f_txt = walk('script')
for fn in f_txt:

    src = open(fn, encoding='utf-16')

    strlist = src.readlines()
    size = len(strlist)
    src.seek(0, 0)
    

    for j in range(0, size):
        i = j % 200
        if i != 0 :
            temp_str = src.readline()
            dst.write(temp_str)
        else:
            k = j // 200
            path = fn[:-4] + '\\'
            mkdir(path)
            fullname =  fn[:-4] + '\\' + fn[7:-4] +  str(k) + '.txt'
            dst = open(fullname, encoding='utf-16', mode='w')
            
            temp_str = src.readline()
            dst.write(temp_str)
            
            print(fullname)


