/*
J:\(同人ソフト) [ヴィンセント] [dxr]秋葉露出～変態M女調教～\秋葉露出～変態M女調教～
K:\(同人ソフト) [黒い教室] 女教師音花「淫ら縄」\女教師音花「淫ら縄」\dir
H:\tools\ency-0.8.6\ency-0.8.6\scan.c

1.副檔名.cxt(包裝的資料檔)+.dxr(專案檔)

※白金標籤社"すきしょ３"檔案組成:

2.使用的撥放器主程式檔案資訊可以看的到"Macromedia Projector"字串和版本號

CTX：Macromedia Director受保护的(不可编辑的)投影文件 

dxr 与 cxt是被Director保护的档案，包括 dir 和 cst 档案，都转换成被保护状态的档案，也就是使用者无法将此被保护的档案开启修改。而这类被保护的档案格式为 dxr 与 cxt。 
如果想将此类文档转换成其它格式的文件，可以先用Director将其打开，然后重新生成相应类型的文件即可，如果不能一次性生成你所说的MP3类型的文件，可以先生成WAV或其它格式的文件，再用其它软件围成MP3格式。 
参考资料：http://www.ourspc.com/bbs/dispbbs.asp?boardid=2&id=41

打包Director程序技巧 
    如果让用户来使用你用Director制作的程序就要求它购买一套昂贵的Director显然是不合理的，
	这就要求我们必须把制作好的多媒体程序打包，制成可以直接在Windows下运行的EXE文件，这样用户就可以脱离Director编辑环境使用。
	但把所有的文件都打包在同一个EXE文件之中是最不经济的同时也是不可取的做法，因为这样会使文件过大，程序装入内存的时间，
	造成一些效果丢失（如声音变调、动画变慢等）。我们建议的做法是：将主程序（也就是第一个执行的程序打包为EXE文件，
	也就是所谓的工程文件Projector），再利用Xtras菜单中的Update Movies命令把其它的dir文件打包为dxr文件，
	将External Cast（也就是*.cst文件）用同样的命令打包为cxt文件格式，这样就可以使得程序执行快一些，同时也可避免自己的程序被D版。

	dxr 的我已经打开了 谁能告诉我cxt的？
dxr 已经打开了 具体过程是： 新建一个movie 再编写script 内容为
on outputdir input,output
window().new(input).open()
window(input).movie.saveMovie(output)
forget(window(input))
end 
再写message为outputdir"C:\Documents and Settings\MPC2006\桌面\unit2-1.dxr" "C:\Documents and Settings\MPC2006\桌面\abc.dir"就可以 了 但是打开cxt的又是怎么样呢 这个我也是在网上查的 cxt的script和message又怎么样编写呢？ save castlib的命令到底怎么写？？初学者 没学过lingo语言 请教！！！！
*/

#include "stdafx.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

