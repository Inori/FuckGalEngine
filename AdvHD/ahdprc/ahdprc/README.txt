
AdvHD (WillPlus Engine V2) Resource Processing Collection

用于新版 WillPlus 引擎(V2)的资源处理
提供解包, 封包, 脚本转换, 提取与整合, 翻译文本全文替换, 系统图片提取与封装这几项服务
解包与封包程序借助 asmodean 的代码完成

已经在游戏 "お嬢様はご機嫌ナナメ" 的资源文件上测试通过

====================
用户文档:

此工具为命令行程序. 作为小工具集合, 有一定的命令行参数
用法: ahdprc MODE arguments
您可以随时添加 -h 参数获得快速使用说明
模式:
  arc: 用于 arc 文件的解包和封包
  pna: 用于 pna 复合图形的解包和封包
  ror: 用于将文件每个字节右循环移位
  ws2: 用于游戏脚本的提取, 转换与整合
  rep: 用于全文本批量替换

arc: ahdprc arc <-u|-r> [-c] FILE1 [FILE2] ...
     -u  解包
     -r  封包
	 -c  解包时按照日文编码转换文件名, 封包时按照中文编码转换文件名
	     可以避免在脚本中替换文件名这种繁琐事情
	     没有此参数则按照系统的默认编码来转换文件名
	 支持通配符. 解包的文件都会放在和文件同级的同名目录内,
	 同样, 打包生成的文件和目录名相同
pna: ahdprc pna <-u|-r> FILE1 [FILE2] ...
     与 arc 类似
	 bug: 通配符模式支持有一定问题, 会随机出现内存泄露的情况. 现阶段不知如何调试, 抱歉
ror: ahdprc ror <-n NUM> FILE1 [FILE2] ...
     ahdprc ror <-n NUM> INFILE -o OUTFILE
	 -n  指定要移位的数量, WillPlus 的游戏一般为 2
	 -o  可以指定输出到不同的文件
	 也可以使用通配符, 但不会留下备份
ws2: (提取) ahdprc ws2 -i IN  -t TXTO -b BINO
     (过滤) ahdprc ws2 -o OUT IN
     (合成) ahdprc ws2 -o OUT -t TXTI -b BINI
     -i  指定输入混合脚本文件
	 -o  指定输出混合脚本文件
	 -t  指定作为文本的文件
	 -b  指定作为二进制的文件
	 此功能生成的文本和过滤后的文本都是 UTF-8 编码
	 所选模式由您使用的参数决定
	 用法: 1. 提取文本, 2. 过滤翻译后的文本, 3. 合成文本, 并根据输出的日志微调
	 输出日志请使用系统的重定向功能(重定向 stdout), 程序里没有提供
rep: ahdprc rep <-l LIST> FILE1 [FILE2 ...]
     -l  指定替换的规则列表
	     每一行格式: Source;Target
		 在 ./repeg.txt 中有示例格式. 编辑请存为 UTF-8 编码, 并绕开 BOM (第一行为空)
	 可以用通配符批量操作. 注意此功能不保留备份
	 
完整示例(这个不是批处理):

从 Rio.arc 到 Chs.arc

ahdprc arc Rio.arc -u -c
ahdprc ror -n 2 Rio\*.ws2
md txt
md bin
for %i in (Rio\*.ws2) do ahdprc ws2 -i %i -t txt\%~ni.txt -b bin\%~nxi
rem 现在翻译 txt 目录里的东西
md tf
for %i in (txt\*.txt) do ahdprc ws2 %i -o tf\%~nxi
rem 准备第一次合成
md Chs
for %i in (bin\*.ws2) do ahdprc ws2 -b %i -t tf\%~ni.txt -o Chs\%~nxi 1>> log.txt
rem 打开 log.txt 确定哪些字符需要在新的字体中表现出来, 并做出替换列表
rem 替换之前请注意备份
ahdprc rep -l rep.txt tf\*.txt
rem 替换后删除原来的日志文件, 再次合成, 从新的日志中确定字符
......(略
rem 原文件夹中还有一个 Pan.dat, 不要忘记
copy Rio\Pan.dat Chs\Pan.dat
ahdprc arc -r Chs -c

附加:
关于文本格式:

○000233○角色○;------------
「正文」
●000233●角色译名●
「正文译文」

译文写在黑色圈的势力范围内. 此格式从前辈那里学来借用
译文区保留了原文, 方便随时出品测试补丁

====================
开发者文档:

./src     提供完整源码, 语言为 C++, 含有少量注释
./src/lib 编译所需的代码库

编译器: MinGW (GCC - 4.7.1)
IDE输出节选:
mingw32-g++.exe -Wall -fexceptions  -O2  -std=c++11 -Wextra -Winvalid-pch -fstack-check -fbounds-check -fstack-protector-all   -IE:\code\__lib  -c D:\project\ahdprc\wpscrpt.cpp -o obj\Release\wpscrpt.o
mingw32-g++.exe -LE:\code\__lib  -o bin\Release\ahdprc.exe obj\Release\main.o obj\Release\package.o obj\Release\pnapack.o obj\Release\replace.o obj\Release\rorfile.o obj\Release\wpscrpt.o   -s -lprincess -lssp -static  
Output size is 561.00 KB
