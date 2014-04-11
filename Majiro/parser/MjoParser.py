#encoding=utf-8
import aka2key
import bytefile
from struct import unpack,pack
from pdb import set_trace as int3

class MjoParser():
    'Parse a Mjo script.'
    def __init__(self,mjo):
        self.text=[]
        self.code=0
        self.hdr=[mjo[0:0x10]]
        self.hdr+=unpack('III',str(mjo[0x10:0x1c]))
        mjo.seek(0x1c)
        self.fidx=[]
        for i in range(self.hdr[3]):
            self.fidx.append((mjo.readu32(),mjo.readu32()))
        size=mjo.readu32()
        self.vmcode=bytefile.ByteIO(mjo[mjo.tell():mjo.tell()+size])
        self.XorDec(self.vmcode)
    def XorDec(self,bf):
        for i in range(len(bf)):
            bf[i]^=aka2key.key[i&0x3ff]
    def ru8(self):
        return self.vmcode.readu8()
    def ru16(self):
        return self.vmcode.readu16()
    def ru32(self):
        return self.vmcode.readu32()

    def p800(self):
        self.text[-1]+='pushInt '+'%d'%self.vmcode.readu32()
    def p801(self):
        slen=self.vmcode.readu16()
        s=self.vmcode.read(slen)
        self.text[-1]+='pushStr "'+s.rstrip('\0')+'"'
    def p802(self):
        p1=self.vmcode.readu16()
        p2=self.vmcode.readu32()
        p3=self.vmcode.readu16()
        self.text[-1]+='pushCopy '+'(%d,%08X,%d)'%(p1,p2,p3)
    def p803(self):
        self.text[-1]+='pushFloat '+'%X'%self.vmcode.readu32()
    def p80f(self):
        p1=self.ru32()
        p2=self.ru32()
        p3=self.ru16()
        self.text[-1]+='OP80F '+'(%08X, %X, %d)'%(p1,p2,p3)
    def p810(self):
        self.text[-1]+='OP810 '+'(%08X, %X, %d)'%(self.ru32(),self.ru32(),self.ru16())
    def p829(self):
        cnt=self.ru16()
        self.text[-1]+='pushStackRepeat '+' '.join(['%d'%self.ru8() for i in range(cnt)])
    def p82b(self):
        self.text[-1]+='return'
    def p82c(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jmp '+'%08X'%dest
    def p82d(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jnz '+'%08X'%dest
    def p82e(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jz '+'%08X'%dest
    def p82f(self):
        self.text[-1]+='pop'
    def p830(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jmp2 '+'%08X'%dest
    def p831(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jne '+'%08X'%dest
    def p832(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jbeC '+'%08X'%dest
    def p833(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jaeC '+'%08X'%dest
    def p834(self):
        self.text[-1]+='Call '+'(%08X, %d)'%(self.ru32(),self.ru16())
    def p835(self):
        self.text[-1]+='Callp '+'(%08X, %d)'%(self.ru32(),self.ru16())
    def p836(self):
        self.text[-1]+='OP836 '+self.vmcode.read(self.ru16())
    def p837(self):
        self.text[-1]+='OP837 '+'(%08X, %08X)'%(self.ru32(),self.ru32())
    def p838(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jbeUnk '+'%08X'%dest
    def p839(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jaeUnk '+'%08X'%dest
    def p83a(self):
        self.text[-1]+='Line: '+'%d'%self.ru16()
    def p83b(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jmpSel0 '+'%08X'%dest
    def p83c(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jmpSel2 '+'%08X'%dest
    def p83d(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jmpSel1 '+'%08X'%dest
    def p83e(self):
        self.text[-1]+='SetStack0'
    def p83f(self):
        self.text[-1]+='Int2Float'
    def p840(self):
        self.text[-1]+='CatString "'+self.vmcode.read(self.ru16()).rstrip('\0')+'"'
    def p841(self):
        self.text[-1]+='ProcessString'
    def p842(self):
        self.text[-1]+='CtrlStr '+self.vmcode.read(self.ru16()).rstrip('\0')
    def p843(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='jmpSelX '+'%08X'%dest
    def p844(self):
        self.text[-1]+='ClearJumpTbl'
    def p845(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='SetJmpAddr3 '+'%08X'%dest
    def p846(self):
        self.text[-1]+='OP846'
    def p847(self):
        dest=self.ru32()+self.vmcode.tell()+4
        self.text[-1]+='SetJmpAddr4 '+'%08X'%dest
    def p850(self):
        cnt=self.ru16()
        addrs=[]
        sta=self.vmcode.tell()+cnt*4
        for i in range(cnt):
            addrs.append('%08X'%(self.ru32()+sta))
        self.text[-1]+='JmpInTbl '+' '.join(addrs)
    
    code800=[
        p800,p801,p802,p803,0,0,0,0,0,0,0,0,0,0,0,p80f,
        p810,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,p829,0,p82b,p82c,p82d,p82e,p82f,
        p830,p831,p832,p833,p834,p835,p836,p837,p838,p839,p83a,p83b,p83c,p83d,p83e,p83f,
        p840,p841,p842,p843,p844,p845,p846,p847,0,0,0,0,0,0,0,0,
        p850
        ]

    code10X={
        0x100:'Mul',0x101:'Fmul',0x108:'Div',0x109:'Fdiv',0x110:'Mod',0x118:'Add',
        0x119:'Fadd',0x11a:'StrAdd',0x120:'Sub',0x121:'Fsub',0x128:'Sar',0x130:'Shl',
        0x138:'IsLE',0x139:'IsFLE',0x13a:'IsLEStr',0x140:'IsL',141:'IsFL',0x142:'IsLStr',
        0x148:'IsGE',0x149:'IsFGE',0x14a:'IsGEStr',0x150:'IsG',0x151:'IsFG',0x152:'IsGStr',
        0x158:'IsE',0x159:'IsFE',0x15a:'IsEStr',0x15b:'IsE',0x15c:'IsE',0x15d:'IsE',
        0x160:'IsNE',0x161:'IsFNE',0x162:'IsNEStr',0x163:'IsNE',0x164:'IsNE',0x165:'IsNE',
        0x168:'Xor',0x170:'CondAnd',0x178:'CondOr',0x180:'And',0x188:'Or',0x190:'IsZero',
        0x191:'Nop',0x198:'Not',0x1a0:'Neg',0x1a8:'Nop',0x1a9:'Nop'
        }

    code1BX={
        0x1b8:'Mul',0x1b9:'Fmul',0x1c0:'Div',0x1c1:'Fdiv',0x1c8:'Mod',0x1d0:'Add',
        0x1d1:'Fadd',0x1d2:'StrAdd',0x1d8:'Sub',0x1d9:'Fsub',0x1e0:'Shl',0x1e8:'Sar',
        0x1f0:'And',0x1f8:'Xor',0x200:'Or'
        }

    code27X={
        0x270:'Mov',0x271:'Mov',0x272:'MovS',0x278:'Mul',0x279:'Fmul',0x280:'Div',
        0x281:'Fdiv',0x288:'Mod',0x290:'Add',0x291:'Mov',0x292:'StrAdd',0x298:'Sub',
        0x299:'Fsub',0x2a0:'Shl',0x2a8:'Sar',0x2b0:'And',0x2b8:'Xor',0x2c0:'Or'
        }
    def p10X(self):
        self.text[-1]+='St'+self.code10X[self.code]
    def p1BX(self):
        self.text[-1]+='Gbl'+self.code1BX[self.code]+' (%d, %08X, %d)'%(self.ru16(),self.ru32(),self.ru16())
    def p21X(self):
        newcode=self.code-(0x218-0x1b8)
        self.text[-1]+='Gblp'+self.code1BX[newcode]+' (%d, %08X, %d)'%(self.ru16(),self.ru32(),self.ru16())
    def p27X(self):
        self.text[-1]+='Ar'+self.code1BX[self.code]+' (%d, %08X, %d)'%(self.ru16(),self.ru32(),self.ru16())
    def p2DX(self):
        newcode=self.code-(0x2d0-0x270)
        self.text[-1]+='Arp'+self.code1BX[newcode]+' (%d, %08X, %d)'%(self.ru16(),self.ru32(),self.ru16())
    def pLen8(self):
        self.text[-1]+='OP%X '%self.code+'(%d, %08X, %d)'%(self.ru16(),self.ru32(),self.ru16())
    def Parse(self):
        while self.vmcode.tell()<len(self.vmcode):
            self.text.append('%08X\t'%self.vmcode.tell())
            self.code=self.vmcode.readu16()
            if self.code>=0x800 and self.code<=0x850:
                func=self.code800[self.code-0x800]
                if func==0:
                    int3()
                func(self)
            elif self.code<=0x1a9 and self.code>=0x100:
                self.p10X()
            elif self.code>=0x1b8 and self.code<=0x200:
                self.p1BX()
            elif self.code>=0x218 and self.code<=0x260:
                self.p21X()
            elif self.code>=0x270 and self.code<=0x2c0:
                self.p27X()
            elif self.code>=0x2d0 and self.code<=0x320:
                self.p2DX()
            elif self.code<=0x850:
                self.pLen8()
            else:
                int3()
        newt=[str(s) for s in self.text]
        return '\r\n'.join(newt)


