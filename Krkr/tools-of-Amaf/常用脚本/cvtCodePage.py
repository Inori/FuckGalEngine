import os

exts=['tjs','ks','csv','func','ini','txt']
newpath='patch1'

cvted=[]

for root,dirs,files in os.walk('patch'):
    for f in files:
        needCvt=False
        f=f.lower()
        for ext in exts:
            if f.endswith(ext):
                needCvt=True
                break
        if not needCvt:
            continue
        fs=open(os.path.join(root,f),'rb')
        txt=fs.read().decode('932')
        if f in cvted:
            print(f)
        fs=open(os.path.join(newpath,f),'wb')
        fs.write(txt.encode('u16'))
        fs.close()
        cvted.append(f)
