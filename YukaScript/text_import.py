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

#处理 '♪'
def StringFilter(string):
    
    if '♪' in string:
        string = string.replace('♪','☆')
    
    return string
    
#提取txt中控制信息，转换为二维列表
def makestr(lines):
    sum_list = []
    for index,line in enumerate(lines):
        if re.match('◎[0-9A-Fa-f]+◎', line):
            indexoffset_str = re.search('([0-9A-Fa-f]+)', line).group()
            indexoffset = int(indexoffset_str, 16)
            continue
        if re.match('○[0-9A-Fa-f]+○', line):
            length_str = re.search('([0-9A-Fa-f]+)', line).group()
            length = int(length_str, 16)
            continue
        if re.match('◇[0-9A-Fa-f]+◇', line):
            startoffset_str = re.search('([0-9A-Fa-f]+)', line).group()
            startoffset = int(startoffset_str, 16)
            continue
        if re.match('☆[0-9A-Fa-f]+☆', line):
            endoffset_str = re.search('([0-9A-Fa-f]+)', line).group()
            endoffset = int(endoffset_str, 16)
            continue
        if re.match('★[0-9A-Fa-f]+★', line):
            string = StringFilter(line[10:-1])
            sum_list.append([indexoffset, length, startoffset, endoffset, string])
            continue
        if line != '\n':
            print(line)
            
    return sum_list

ftxt = walk('script')
for fn in ftxt:
    #过滤非txt文件
    if not fnmatch.fnmatch(fn,'*.txt'):
        continue
    
    rawname = 'raw' + fn[6:-4] + '.yks'
    dstname = 'done' + fn[6:-4] + '.yks'
    src = open(fn, 'r', encoding='utf16')
    raw = open(rawname, 'rb')
    dst = open(dstname, 'w+b')

    raw.seek(0, 2)
    rawend = raw.tell()

    raw.seek(0x20)
    textoffset = byte2int(raw.read(4))
    raw.seek(0, 0)
    
    lines = src.readlines()
    sum_list = makestr(lines)

    #先把第一句话之前的部分补齐
    beflen = sum_list[0][2]
    befbytes = raw.read(beflen)
    dst.write(befbytes)
    
    diff = 0
    sum_diff = 0
    num = len(sum_list)
    for index,cell in enumerate(sum_list):
        try:
            bytestr = cell[4].encode('gbk', errors='ignore')
        except:
            bytestr = '???'.encode('gbk')
            #bytestr = cell[4].encode('sjis')
        length = len(bytestr)

        #减去末尾0x00站位
        diff = length - (cell[1]-1)
        sum_diff += diff
        if index < num-1:
            nextsize = sum_list[index+1][2] - cell[3]
        else:
            nextsize = rawend - cell[3]
            
        stroffset = dst.tell()
        dst.write(bytestr)
        dst.write(struct.pack('B', 0))

        raw.seek(cell[3])
        nextctl = raw.read(nextsize)
        dst.write(nextctl)
        
        nextstroff = dst.tell()
        
        if index < num-1:
            count = (sum_list[index+1][0] - cell[0])//16
        else:
            count = (textoffset - cell[0])//16

        dst.seek(cell[0])
        dst.seek(8, 1)
        dst.write(int2byte(stroffset-textoffset))
        
        j = 0
        for i in range(0, count-1):
            flagoff = cell[0]+16 + j*0x10
            raw.seek(flagoff)
            flag = byte2int(raw.read(4))
           
            if flag == 4 or flag == 5:
                raw.seek(4, 1)
                new = byte2int(raw.read(4)) + sum_diff
                dst.seek(flagoff+8)
                dst.write(int2byte(new))
            elif flag == 8 or flag == 9:
                new = byte2int(raw.read(4)) + sum_diff
                dst.seek(flagoff+4)
                dst.write(int2byte(new))
                
                raw.seek(4, 1)
                new = byte2int(raw.read(4)) + sum_diff
                dst.seek(flagoff+12)
                dst.write(int2byte(new))
            elif flag == 1:
                new = byte2int(raw.read(4)) + sum_diff
                dst.seek(flagoff+4)
                dst.write(int2byte(new))
                
                new = byte2int(raw.read(4)) + sum_diff
                dst.seek(flagoff+8)
                dst.write(int2byte(new))                
            elif flag == 0:
                new = byte2int(raw.read(4)) + sum_diff
                dst.seek(flagoff+4)
                dst.write(int2byte(new))
            elif flag == 0xA or flag == 0xB:
                j += 1
                continue
            else:
                print('Fuck New Flag Found!')
                print(hex(flag))
                print(hex(cell[0]+16 + j*0x10))
                input()
            j += 1
            
        dst.seek(nextstroff)
        
    #更新文件头
    dst.seek(0, 2)
    dstend = dst.tell()
    textsize = dstend - textoffset
    dst.seek(0x24)
    dst.write(int2byte(textsize))

    
    dst.close()
    raw.close()
    src.close()
    print(dstname)

input('导入完成')
    

    

    
    
