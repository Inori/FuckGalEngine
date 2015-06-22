import pdb

class ByteIO:
	"Operate String as a File."
	pData=0
	def __init__(self, Src):
		self.pData=0
		self.b=bytearray(Src)
	def __len__(self):
		return len(self.b)
	def __getitem__(self,i):
		return self.b[i]
	def __getslice__(self,i,j):
		return self.b[i:j]
	def __setitem__(self,i,y):
		self.b[i]=y
	def read(self, Bytes=-1):
		if Bytes<-1:
			return bytearray('')
		elif Bytes==-1:
			tData=self.b[self.pData:]
			self.pData=len(self.b)
		else:
			tData=self.b[self.pData:self.pData+Bytes]
			self.pData+=Bytes
		return tData
	def tell(self):
		return self.pData
	def seek(self, Offset, mode=0):
		if mode==1: Offset+=self.pData
		elif mode==2: Offset=len(self.b)-Offset
		if Offset<0:
			self.pData+=Offset
		elif Offset<len(self.b):
			self.pData=Offset
		else:
			self.pData=len(self.b)
	def readu32(self):
		tData=self.b[self.pData:self.pData+4]
		self.pData+=4
		if len(tData)<4:
			return 0
		u32=(tData[3]<<24)+(tData[2]<<16)+(tData[1]<<8)+(tData[0])
		return u32
	def readu16(self):
            tData=self.b[self.pData:self.pData+2]
            self.pData+=2
            if len(tData)<2:
                return 0
            return (tData[1]<<8)+(tData[0])
	def readu8(self):
		if self.pData>=len(self.b):
			return 0
		self.pData+=1
		return self.b[self.pData-1]
	def readstr(self):
		if self.pData>=len(self.b):
			return b''
		slen = 0
		while True:
			if self.b[self.pData+slen] == 0: break
			slen += 1
		tData=self.b[self.pData:self.pData+slen]
		self.pData += slen
		return tData
		
		

			
			
