#py3.2

import bytefile
from struct import unpack
from pdb import set_trace as int3

class PsbFile:
    psb=0
    text=[]
    tabcount=0
    retAddr=[]
    strings=[]
    dibs=[]
    def __init__(self,stm):
        if stm[0:4]!=b'PSB\x00':
            print("PSB Format Error")
        self.psb=bytefile.ByteIO(stm)

    def GetInt(self,l):
        if l<0xd or l>0x10:
            print('%x'%self.psb.tell())
            int3()
        if l==0xd:
            return self.psb.readu8()
        if l==0xe:
            return self.psb.readu16()
        if l==0xf:
            temp=self.psb.readu16()
            temp+=self.psb.readu8()*65536
            return temp
        if l==0x10:
            return self.psb.readu32()

    def p02(self):
        self.text.append(self.tabcount*'\t'+'1.0')
    def p03(self):
        self.text.append(self.tabcount*'\t'+'0.0')
    def p04(self):
        self.text.append(self.tabcount*'\t'+str(self.psb.readu8()))
    def p05(self):
        self.text.append(self.tabcount*'\t'+str(self.psb.read(1)))
    def p06(self):
        self.text.append(self.tabcount*'\t'+str(self.psb.readu16()))
    def p07(self):
        temp=self.psb.readu16()
        temp+=self.psb.readu8()*65536
        self.text.append(self.tabcount*'\t'+str(temp))
    def p08(self):
        self.text.append(self.tabcount*'\t'+str(self.psb.readu32()))
    def p09(self):
        temp=self.psb.readu32()
        temp+=self.psb.readu8()*0x100000000
        self.text.append(self.tabcount*'\t'+str(temp))
    def p0a(self):
        temp=self.psb.readu32()
        temp+=self.psb.readu16()*0x100000000
        self.text.append(self.tabcount*'\t'+str(temp))
    def p0b(self):
        temp=self.psb.readu32()
        temp+=self.psb.readu16()*0x100000000
        temp+=self.psb.readu8()*0x10000000000
        self.text.append(self.tabcount*'\t'+str(temp))
    def p0c(self):
        temp=self.psb.readu32()
        temp+=self.psb.readu32()*0x100000000
        self.text.append(self.tabcount*'\t'+str(temp))
    def p0d(self):
        self.text.append(self.tabcount*'\t'+str(self.psb.readu8()))
    def p0e(self):
        self.text.append(self.tabcount*'\t'+str(self.psb.readu16()))
    def p0f(self):
        temp=self.psb.readu16()
        temp+=self.psb.readu8()*65536
        self.text.append(self.tabcount*'\t'+str(temp))
    def p10(self):
        self.text.append(self.tabcount*'\t'+str(self.psb.readu32()))
        
    def p15(self):
        self.text.append(self.tabcount*'\t'+strings[self.psb.readu8()])
    def p16(self):
        self.text.append(self.tabcount*'\t'+strings[self.psb.readu16()])
    def p17(self):
        temp=self.psb.readu16()
        temp+=self.psb.readu8()*65536
        self.text.append(self.tabcount*'\t'+strings[temp])
    def p18(self):
        self.text.append(self.tabcount*'\t'+strings[self.psb.readu32()])
    def p19(self):
        self.text.append(self.tabcount*'\t'+'DibIdx'+str(self.psb.readu8()))
    def p1a(self):
        self.text.append(self.tabcount*'\t'+'DibIdx'+str(self.psb.readu16()))
    def p1b(self):
        temp=self.psb.readu16()
        temp+=self.psb.readu8()*65536
        self.text.append(self.tabcount*'\t'+'DibIdx'+str(temp))
    def p1c(self):
        self.text.append(self.tabcount*'\t'+'DibIdx'+str(self.psb.readu32()))
    def p1e(self):
        f=unpack('f',self.psb.read(4))
        self.text.append(self.tabcount*'\t'+str(f))
    def p1f(self):
        d=unpack('d',self.psb.read(8))
        self.text.append(self.tabcount*'\t'+str(d))
    def p20(self):
        n=GetInt(self.psb.readu8())
        self.text.append(self.tabcount*'\t'+'Array:')
        self.tabcount+=1
        t=self.psb.readu8()
        if t>0x21:
            print('%x'%self.psb.tell())
            int3()
        func=objTable[t]
        if func==0:
            print('%x'%self.psb.tell())
            int3()
        for i in range(n):
            func(self)
        self.tabcount-=1
    def p21(self):
        n=GetInt(self.psb.readu8())
        t=self.psb.readu8()
        keys=[GetInt(t) for i in range(n)]
        if n!=GetInt(self.psb.readu8()):
            print('%x'%self.psb.tell())
            int3()
        t=self.psb.readu8()
        vals=[GetInt(t) for i in range(n)]

        self.tabcount+=1
        for i in range(n):
            self.retAddr.append(self.psb.tell())
            self.text.append(self.tabcount*'\t'+str(keys[i])+': ')
            self.psb.seek(self.retAddr[-1]+vals[i])
            t=self.psb.readu8()
            func=objTable[t]
            func(self)
            if i!=n-1:
                self.psb.seek(self.retAddr.pop())
            else:
                self.retAddr.pop()
        self.tabcount-=1

        
    
    objTable=[
        0,0,p02,p03,p04,p05,p06,p07,
        p08,p09,p0a,p0b,p0c,p0d,p0e,p0f,
        p10,0,0,0,0,p15,p16,p17,
        p18,p19,p1a,p1b,p1c,p1d,p1e,p1f,
        p20,p21]

    def ParseTxt(self):
        self.psb.seek(4)
        version, offIdTree, offStrList, offStrRes, Metadata=\
                 unpack('I4xIII12xI',self.psb.read(0x24))
        
        self.psb.seek(offStrList)
        n=GetInt(self.psb.readu8())
        t=self.psb.readu8()
        for i in range(n):
            p=GetInt(t)
            pos=self.psb.tell()
            self.psb.seek(p+offStrRes)
            strings.append(self.psb.readstr().decode('utf-8'))
            self.psb.seek(pos)
        
        self.psb.seek(Metadata)
        while self.psb.tell()<offStrList:
            t=self.psb.readu8()
            func=objTable[t]
            func(self)
