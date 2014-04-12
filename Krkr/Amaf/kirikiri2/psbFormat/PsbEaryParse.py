#py3.2

import bytefile
from struct import unpack
from pdb import set_trace as int3

def GetInt(psb,l):
    if l<0xd or l>0x10:
        print('%x'%psb.tell())
        int3()
    if l==0xd:
        return psb.readu8()
    if l==0xe:
        return psb.readu16()
    if l==0xf:
        temp=psb.readu16()
        temp+=psb.readu8()*65536
        return temp
    if l==0x10:
        return psb.readu32()

lenTbl=[
    -1,0,0,0,0,1,2,3,
    4,5,6,7,8,1,2,3,
    4,-1,-1,-1,-1,-2,-2,-2,
    -2,1,2,3,-1,4,4,8,
    -3,-4]

def ParsePsb(psb):
    psb.seek(4)
    version, offIdTree, offStrList, offStrRes, offMetadata=\
             unpack('I4xIII12xI',psb.read(0x24))
    psb.seek(offStrList)
    n=GetInt(psb,psb.readu8())
    t=psb.readu8()
    strings=[]
    for i in range(n):
        p=GetInt(psb,t)
        pos=psb.tell()
        psb.seek(p+offStrRes)
        strings.append(psb.readstr().decode('utf-8'))
        psb.seek(pos)

    psb.seek(offMetadata)
    text=[]
    while psb.tell()<offStrList:
        o=psb.readu8()
        #print('%x'%(psb.tell()-1),':','%x'%o)
        t=lenTbl[o]
        if t==-1:
            print('%x'%psb.tell())
            int3()
        elif t==-2:
            len1=GetInt(psb,o-(0x15-0xd))
            text.append(strings[len1])
        elif t==-3:
            len1=GetInt(psb,psb.readu8())
            type1=psb.readu8()
            for i in range(len1):
                GetInt(psb,type1)
        elif t==-4:
            len1=GetInt(psb,psb.readu8())
            type1=psb.readu8()
            for i in range(len1):
                GetInt(psb,type1)
            len2=GetInt(psb,psb.readu8())
            type2=psb.readu8()
            for i in range(len2):
                GetInt(psb,type2)
        else:
            psb.seek(t,1)
    return '\r\n'.join(text)

fs=open('1_01.ks.scn','rb')
stm=fs.read()
bio=bytefile.ByteIO(stm)
txt=ParsePsb(bio)
fs=open('dec.txt','wb')
fs.write(txt.encode('utf-8'))
fs.close()
            
