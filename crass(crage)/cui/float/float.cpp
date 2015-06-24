// float.cpp : Defines the entry point for the console application.
//
#include <windows.h>
#include <stdio.h>
#include <math.h>
所在函数库为math.h、stdlib.h、string.h、float.h
int      abs(int i)                                      返回整型参数i的绝对值
double cabs(struct complex znum)        返回复数znum的绝对值
double fabs(double x)                           返回双精度参数x的绝对值
long    labs(long n)                                 返回长整型参数n的绝对值
double   exp(double x)                             返回指数函数ex的值
double frexp(double value,int *eptr)       返回value=x*2n中x的值,n存贮在eptr中
double ldexp(double value,int exp);        返回value*2exp的值
double   log(double x)                             返回logex的值
double log10(double x)                           返回log10x的值
double   pow(double x,double y)             返回xy的值
double pow10(int p)                                返回10p的值
double sqrt(double x)                             返回+√x的值
double acos(double x)                 返回x的反余弦cos-1(x)值,x为弧度
double asin(double x)                 返回x的反正弦sin-1(x)值,x为弧度
double atan(double x)                 返回x的反正切tan-1(x)值,x为弧度
double atan2(double y,double x)        返回y/x的反正切tan-1(x)值,y的x为弧度
double   cos(double x)                 返回x的余弦cos(x)值,x为弧度
double   sin(double x)                 返回x的正弦sin(x)值,x为弧度
double   tan(double x)                 返回x的正切tan(x)值,x为弧度
double cosh(double x)                 返回x的双曲余弦cosh(x)值,x为弧度
double sinh(double x)                 返回x的双曲正弦sinh(x)值,x为弧度
double tanh(double x)                 返回x的双曲正切tanh(x)值,x为弧度
double hypot(double x,double y)        返回直角三角形斜边的长度(z),
                                       x和y为直角边的长度,z2=x2+y2
double ceil(double x)                 返回不小于x的最小整数
double floor(double x)                 返回不大于x的最大整数
void   srand(unsigned seed)            初始化随机数发生器
int     rand()                         产生一个随机数并返回这个数
double poly(double x,int n,double c[])从参数产生一个多项式
double modf(double value,double *iptr)将双精度数value分解成尾数和阶
double fmod(double x,double y)        返回x/y的余数
double frexp(double value,int *eptr)   将双精度数value分成尾数和阶
double atof(char *nptr)               将字符串nptr转换成浮点数并返回这个浮点数
double atoi(char *nptr)               将字符串nptr转换成整数并返回这个整数
double atol(char *nptr)               将字符串nptr转换成长整数并返回这个整数
char   *ecvt(double value,int ndigit,int *decpt,int *sign)
         将浮点数value转换成字符串并返回该字符串
char   *fcvt(double value,int ndigit,int *decpt,int *sign)
         将浮点数value转换成字符串并返回该字符串
char   *gcvt(double value,int ndigit,char *buf)
         将数value转换成字符串并存于buf中,并返回buf的指针
char *ultoa(unsigned long value,char *string,int radix)
         将无符号整型数value转换成字符串并返回该字符串,radix为转换时所用基数
char   *ltoa(long value,char *string,int radix)
         将长整型数value转换成字符串并返回该字符串,radix为转换时所用基数
char   *itoa(int value,char *string,int radix)
         将整数value转换成字符串存入string,radix为转换时所用基数
double atof(char *nptr) 将字符串nptr转换成双精度数,并返回这个数,错误返回0
int    atoi(char *nptr) 将字符串nptr转换成整型数, 并返回这个数,错误返回0
long   atol(char *nptr) 将字符串nptr转换成长整型数,并返回这个数,错误返回0
double strtod(char *str,char **endptr)将字符串str转换成双精度数,并返回这个数,
long   strtol(char *str,char **endptr,int base)将字符串str转换成长整型数,
                                               并返回这个数,
int          matherr(struct exception *e)
              用户修改数学错误返回信息函数(没有必要使用)
double       _matherr(_mexcep why,char *fun,double *arg1p,
                      double *arg2p,double retval)
                用户修改数学错误返回信息函数(没有必要使用)
unsigned int _clear87()   清除浮点状态字并返回原来的浮点状态
void         _fpreset()   重新初使化浮点数学程序包
unsigned int _status87() 返回浮点状态字
int main(int argc, char* argv[])
{
	double b;

	b = argc;
	if (b <= 0)
		printf("Hello World! %f %f\n", sqrt(b));
	return 0;
}

