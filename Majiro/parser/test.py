# -*- coding: utf-8 -*-
import MjoParser
import bytefile

fname=u'終バゲーム紹介第１回本文'
fs=open(fname+'.mjo','rb')
stm=bytefile.ByteIO(fs.read())
mp=MjoParser.MjoParser(stm)
txt=mp.Parse()
fs=open(fname+'.txt','wb')
fs.write(txt)
fs.close()
fs=open(fname+'.dec','wb')
fs.write(mp.vmcode[0:])
fs.close()
