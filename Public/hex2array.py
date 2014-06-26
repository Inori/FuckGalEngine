import sys


#用于将文件的二进制(16进制显示)内容转换成C++等编程语言里的数组，供复制粘贴
src = open(sys.argv[1], 'rb')
dst = open(sys.argv[1]+'.txt', 'w')

buffer = src.read()
array_list = []
for c in buffer:
    hs = hex(c)
    if len(hs) != 4:
        hs = hs[:2] + '0' + hs[2:]
    array_list.append(hs)

dst.write(','.join(array_list))

dst.close()
src.close()
