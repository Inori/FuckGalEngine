# -*- coding:utf-8 -*-

import struct,os,fnmatch,re

LOG = open('ErrorLog.bin', 'w', encoding='utf16')

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

#将txt转换成文本列表
def makestr(lines):
    string_list = []
    num = len(lines)
    for index,line in enumerate(lines):
        s = re.match('●[0-9A-Fa-f]+●(.*)●', line)
        if s:
            name = s.group(1)
            i = 1
            string = ''
            while True:
                if index+i >= num:
                    break
                if re.match('●[0-9A-Fa-f]+●(.*)●', lines[index+i]):
                    break
                string += lines[index+i]
                i += 1
            string_list.append([name,string[:-2]])
    return string_list

#搜索二进制文件，获取name和text首地址
def GetOffset(src):
    buff = src.read()
    offset_list = []
    nameoff = textoff = 0
    while True:
        nameoff = buff.find(b'%L', textoff+5)
        textoff = buff.find(b'char\x00', textoff+5)
        if nameoff == -1 and textoff == -1:
            break
        else:
            if nameoff < textoff and nameoff != -1:           #有名字
                offset_list.append([nameoff+3, textoff+5])
            else:                                             #没有名字
                offset_list.append([0, textoff+5])
    return offset_list

#末地址
def NameEndoff(src, offset):
    if offset == 0:return 0
    src.seek(offset)  
    end_off = 0
    while True:
        byte = src.read(1)
        if byte == b'\x00':
            end_off = src.tell() - 1
            break            
        else:
            pass
    return end_off


##末地址,与NameEndoff只是结束符不同
def TextEndoff(src, offset):
    if offset == 0:return 0
    src.seek(offset)
    end_off = 0
    while True:
        byte = src.read(1)
        if byte == b'%':            
            nxt = src.read(1)
            if nxt == b'K' or nxt == b'P':
                end_off = src.tell() - 2
                break
            else:
                continue
        else:
            pass
    return end_off

#制作name，text的始末地址表
def mklist(src):
    offset_list = GetOffset(src)
    import_list = []
    for[nameoff, textoff] in offset_list:
        name_end = NameEndoff(src, nameoff)
        text_end = TextEndoff(src, textoff)
        import_list.append([nameoff, name_end, textoff, text_end])
    return import_list

def Main():
    txt_l = walk('txt')
    for fn in txt_l:
        rawname = 'Rio' + fn[3:-4] + '.ws2'
        raw = open(rawname, 'rb')
        dstname = 'ws2' + fn[3:-4] + '.ws2'
        dst = open(dstname, 'wb')
        src = open(fn, 'r', encoding = 'utf16')

        import_list = mklist(raw)
        str_list = makestr(src.readlines())

        #先把第一个字符串之前字节copy过来
        m = import_list[0][0]
        if m != 0:
            head_size = m
        else:
            head_size = import_list[0][2]
        raw.seek(0)
        head = raw.read(head_size)
        dst.write(head)
        i = 0
        num = len(import_list)
        for ([name_start,name_end,text_start,text_end], [name,text]) in zip(import_list,str_list):
            try:
                bname = name.encode('sjis')
            except:
                global LOG
                LOG.write(name+'\n')
            try:
                btext = text.encode('sjis')
            except:
                global LOG
                LOG.write(text+'\n')

            if name_start == 0:name_start = name_end = text_start

            dst.write(bname)
            raw.seek(name_end)
            ctl_byte1 = raw.read(text_start - name_end)
            dst.write(ctl_byte1)
            dst.write(btext)
            raw.seek(text_end)
            if i < num -1:
                n = import_list[i+1][2] if import_list[i+1][0] == 0 else import_list[i+1][0]
                ctl_byte2 = raw.read(n - text_end)
            else:
                ctl_byte2 = raw.read(os.path.getsize(rawname) - text_end)
            dst.write(ctl_byte2)
            i += 1
            
        raw.close()
        dst.close()
        src.close()

Main()
LOG.close()
if os.path.getsize('ErrorLog.bin') < 1:
    os.remove('ErrorLog.bin')
input('\n导入完成\n')

            
                

            
                
                
            
            
    


















    


