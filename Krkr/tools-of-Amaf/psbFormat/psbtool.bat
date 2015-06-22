@echo off
#::coding: UTF-8
rem = '''
rem set pypath="%~dp0python\python.exe"
set pypath="E:\SoftwareDevelopment\Python34\python.exe"
%pypath% -x "%~f0" %*
goto :EOF
rem '''

import os, sys, struct
import pdb, zlib
#from PIL import Image
from cStringIO import StringIO as strio

class MyUni(unicode):
	ctx = u''
	def __init__(self, ctx):
		self.ctx = ctx
	def __repr__(self):
		return 'u\'' + self.ctx.replace('\\','\\\\').replace('\r','\\r').replace('\n','\\n').encode('gb18030') + '\''

def GetStr(stream):
    retstr = []
    while 1:
        c = stream.read(1)
        if c == '\0':
            break
        retstr.append(c)
    return MyUni(''.join(retstr).decode('utf8'))

PixelList=[]
def GetValByType(stream, type):
    global StrList, DibList, PixelList
    if   type ==   1:
    	return None
    ############### float boolean ###############
    elif type ==   2:
        return 1.0
    elif type ==   3:
        return 0.0
    ############# signed int->float #############
    elif type ==   4:
        return float(ord(stream.read(1)))
    elif type ==   5:
        return float(struct.unpack('b', stream.read(1))[0])
    elif type ==   6:
        return float(struct.unpack('h', stream.read(2))[0])
    elif type ==   7: #3bytes
        ret = struct.unpack('H', stream.read(2))[0]+(ord(stream.read(1))<<16)
        if ret&0x800000:
            ret = (ret&0x7FFFFF)-0x800000
        return float(ret)
    elif type ==   8:
        return float(struct.unpack('i', stream.read(4))[0])
    elif type ==   9: #5bytes
        ret = struct.unpack('I', stream.read(4))[0]+(ord(stream.read(1))<<32)
        if ret&0x8000000000:
            ret = (ret&0x7FFFFFFFFF)-0x8000000000
        return float(ret)
    elif type == 0xA: #6bytes
        ret = struct.unpack('I', stream.read(4))[0]+(struct.unpack('H', stream.read(2))[0]<<32)
        if ret&0x800000000000:
            ret = (ret&0x7FFFFFFFFFFF)-0x800000000000
        return float(ret)
    elif type == 0xB: #7bytes
        ret = struct.unpack('I', stream.read(4))[0]+\
             (struct.unpack('H', stream.read(2))[0]<<32)+\
             (ord(stream.read(1))<<48)
        if ret&0x80000000000000:
            ret = (ret&0x7FFFFFFFFFFFFF)-0x80000000000000
        return float(ret)
    elif type == 0xC: #8bytes
        return float(struct.unpack('q', stream.read(8))[0])
    ############### unsigned int ################
    elif type == 0xD:
        return ord(stream.read(1))
    elif type == 0xE:
        return struct.unpack('H', stream.read(2))[0]
    elif type == 0xF:
        return struct.unpack('H', stream.read(2))[0]+(ord(stream.read(1))<<16)
    elif type == 0x10:
        return struct.unpack('I', stream.read(4))[0]
    ################ StringCount ################
    elif type == 0x15:
        return StrList[ord(stream.read(1))]
    elif type == 0x16:
        return StrList[struct.unpack('H', stream.read(2))[0]]
    elif type == 0x17:
        return StrList[struct.unpack('H', stream.read(2))[0]+(ord(stream.read(1))<<16)]
    elif type == 0x18:
        return StrList[struct.unpack('I', stream.read(4))[0]]
    ################### GetDIB ##################
    elif type == 0x19:
        return DibList[ord(stream.read(1))]
    elif type == 0x1A:
        return DibList[struct.unpack('H', stream.read(2))[0]]
    elif type == 0x1B:
        return DibList[struct.unpack('H', stream.read(2))[0]+(ord(stream.read(1))<<16)]
    elif type == 0x1C:
        return DibList[struct.unpack('I', stream.read(4))[0]]
    ################### float ###################
    elif type == 0x1E:
        return struct.unpack('f', stream.read(4))[0]
    elif type == 0x1F:
        return struct.unpack('d', stream.read(8))[0]
    elif type == 0x20: #Array
        return GetArray(stream, stream.tell(), 1)
    elif type == 0x21: #Dictonary
        global IDBackTrace
        Dict = {}#'OffsetOfSelf':stream.tell()}
        Key = GetArray(stream, stream.tell())
        Val = GetArray(stream, stream.tell())
        CurPos = stream.tell()
        for n in xrange(len(Key)):
            stream.seek(CurPos + Val[n])
            key = Key[n]
            val = GetVal(stream)
            if key in IDBackTrace: key = IDBackTrace[key]
            if key in Dict:
                print 'key %s collision' % key
            Dict[key]=val
        if 'pixel' in Dict:
            PixelList.append(Dict)
        return Dict
    else:
        print 'unknown type 0x%X' % type
        pdb.set_trace()

def GetVal(stream):
    type = ord(stream.read(1))
    return GetValByType(stream, type)

def GetArray(stream, offset, val = 0):
    stream.seek(offset)
    TotalItem = GetVal(stream)
    Type  = ord(stream.read(1))
    arr = [GetValByType(stream, Type) for n in xrange(TotalItem)]
    if val and Type in [0xD, 0xE, 0xF, 0x10]:
    	newarr = []
    	CurOffset = stream.tell()
    	for offset in arr:
    		stream.seek(CurOffset + offset)
    		newarr.append(GetVal(stream))
    	arr = newarr
    return arr

ValidChr = '\t\n\r !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~'
def TestTree(c, l, r, teststr, IDList, Table1, Table2):
    l = Table1[l] + ord(c)
    if l>=len(Table1) or r!=Table2[l]:#当前字符无效
        return 0#此路不通
    teststr+=c
    haveNext=0
    id = Table1[l]
    for c in ValidChr:
        haveNext+=TestTree(c, l, l, teststr, IDList, Table1, Table2)
    #if haveNext==0:#or haveNext>1当前字串结束
    if haveNext == 0 and id and Table1[id] not in IDList:
        #print '%-5d %s'%(Table1[id], teststr)
        IDList[Table1[id]]=teststr
    return 1

def GetKeyFromID(s, T1, T2):
    l=0
    for c in s+'\0':
        l = T1[l]+ord(c)
    return T1[l]

StrList = []
DibList = []
IDBackTrace = {}
def SplitPSB(filename):
    PSB = open(filename,'rb')
    Magic = PSB.read(4)
    if Magic == 'mdf\0':
    	PSB.read(4)
    	PSB = strio(zlib.decompress(PSB.read()))
    	Magic = PSB.read(4)
    if Magic!='PSB\0':
        print 'Invalid PSB File.'
        return
    Ver, hdr, IDTable, StrOffList, StrRes, DibOffList, DibSizeList, DibData, MetaData = \
        struct.unpack('I'*9, PSB.read(36))
    if Ver!=2:
        print 'Only support PSB v2'
        return
    #GetString
    global StrList
    StrOffList = GetArray(PSB, StrOffList)
    for offset in StrOffList:
        PSB.seek(StrRes+offset)
        StrList.append(GetStr(PSB))
    #GetDIB
    global DibList
    DibOffList = GetArray(PSB, DibOffList)
    DibSizeList= GetArray(PSB, DibSizeList)
    for n in xrange(len(DibOffList)):
        offset = DibData+DibOffList[n]
        PSB.seek(offset)
        DibList.append((offset, strio(PSB.read(DibSizeList[n]))))
    #GetIDTable
    Table1 = GetArray(PSB, IDTable)
    Table2 = GetArray(PSB, PSB.tell())
    #递归测试可能的ID
    PossibleID={}
    for c in ValidChr:
        TestTree(c, 0, 0, '', IDBackTrace, Table1, Table2)
    import pprint
#    pprint.pprint(IDBackTrace)
#    pdb.set_trace()
#    for key in PossibleID:
#        val = PossibleID[key]
#        if val in IDBackTrace:
#            print 'val-key %d collision' % val
#        IDBackTrace[val] = key
    #GetMetaData
    PSB.seek(MetaData)
    Meta = GetVal(PSB)
    while Meta:
    	pprint.pprint(Meta, width=200)
    	Meta = GetVal(PSB)
#    pdb.set_trace()
    #ImgData = Meta['source'].values()[0]['icon']
    filename, ext = os.path.splitext(sys.argv[1])
    for res in PixelList:
        offset, stream = res['pixel']
        size = (int(res['width']),int(res['height']))
        if 'type' in res and res['type']==u'CI8': #16x8 tiles
            if res['palType']!=u'RGBA8':
                pdb.set_trace()
            if 'pal' in res:
                pal = res['pal']
            else:
                pal = res[GetKeyFromID('pal', Table1, Table2)]
            pal=pal[1].read()
            if len(pal)!=1024:
                pdb.set_trace()
            pal=Image.fromstring('RGBA', (16,16), pal, "raw", "BGRA").tostring()
            w, h = size
            xx = w / 16
            yy = h / 8
            blocksize = 16*8
            totalsize = w * h
            img = Image.new('P', size)
            for y in range(yy):
                for x in range(xx):
                    pos = (x*16, y*8)
                    block = stream.read(blocksize)
                    block += '\0'*(blocksize-len(block))
                    img.paste(Image.fromstring('L', (16, 8), block), pos)
            bmp = strio()
            img.save(bmp,'bmp')
            bmp.seek(0)
            bmp=bmp.read()
            bmp=bmp[:54]+pal+bmp[1078:]
            open(filename+'_%08X.bmp'%offset,'wb').write(bmp)
        else: #normal RGBA
            Image.fromstring('RGBA', size, stream.read(), "raw", "BGRA").save(filename+'_%08X.png'%offset)

def JoinPSB(filename, picname):
    PSB = open(filename,'rb+')
    PSB.seek(0)
    if PSB.read(4)!='PSB\0':
        print 'Invalid PSB File.'
        return
    Ver, hdr, IDTable, StrOffList, StrRes, DibOffList, DibSizeList, DibData, MetaData = \
        struct.unpack('I'*9, PSB.read(36))
    if Ver!=2:
        print 'Only support PSB v2'
        return
    #GetString
    global StrList
    StrOffList = GetArray(PSB, StrOffList)
    for offset in StrOffList:
        PSB.seek(StrRes+offset)
        StrList.append(GetStr(PSB))
    #GetDIB
    global DibList
    DibOffList = GetArray(PSB, DibOffList)
    DibSizeList= GetArray(PSB, DibSizeList)
    for n in xrange(len(DibOffList)):
        offset = DibData+DibOffList[n]
        PSB.seek(offset)
        DibList.append((offset, strio(PSB.read(DibSizeList[n]))))
    #GetIDTable
    Table1 = GetArray(PSB, IDTable)
    Table2 = GetArray(PSB, PSB.tell())
    #递归测试可能的ID
    PossibleID={}
    for c in ValidChr:
        TestTree(c, 0, 0, '', IDBackTrace, Table1, Table2)
#    for key in PossibleID:
#        val = PossibleID[key]
#        if val in IDBackTrace:
#            print 'val-key %d collision' % val
#        IDBackTrace[val] = key
    #GetMetaData
    PSB.seek(MetaData)
    Meta = GetVal(PSB)
    #ImgData = Meta['source'].values()[0]['icon']
    filename, ext = os.path.splitext(sys.argv[1])
    tga = open(picname,'rb')
    tga.read(18)
    pal = Image.fromstring('RGBA', (16,16), tga.read(1024), "raw", "BGRA").tostring()
    offset = int(os.path.splitext(picname)[0].split('_')[-1], 16)
    for res in PixelList:
        if offset != res['pixel'][0]:
            continue
        size = (int(res['width']), int(res['height']))
        img = Image.fromstring('L', size, tga.read()).transpose(Image.FLIP_TOP_BOTTOM)
        oridib = res['pixel'][1]
#        print size
        w, h = size
        remain = len(oridib.read())
        dib=[]
        for y in range(h / 8):
            for x in range(w / 16):
                pos = (x*16, y*8, x*16+16, y*8+8)
                block = img.crop(pos).tostring()
                if remain == 0:
                    block=''
                elif len(block)>remain:
                    block=block[:remain]
                    remain=0
                else:
                    remain-=len(block)
                dib.append(block)
        paloffset = res[GetKeyFromID('pal', Table1, Table2)][0]
        PSB.seek(paloffset)
        PSB.write(pal)
        PSB.seek(offset)
        PSB.write(''.join(dib))

if len(sys.argv)==2:
    SplitPSB(sys.argv[1])
elif len(sys.argv)==3 and sys.argv[2][-4:].lower()=='.tga':
    JoinPSB(sys.argv[1], sys.argv[2])