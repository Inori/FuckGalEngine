#ifndef MY_COMMON_H
#define MY_COMMON_H
	
#ifdef MY_EXPORTS
#define MY_API __declspec(dllexport)
#else
#define MY_API __declspec(dllimport)
#endif 
		
#endif	/* MY_COMMON_H */	