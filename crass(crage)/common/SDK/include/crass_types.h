#ifndef CRASS_TYPES_H
#define CRASS_TYPES_H

typedef char				s8;
typedef short				s16;
typedef int					s32;
typedef __int64				s64;
typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned __int64	u64;

#ifndef QWORD
typedef	unsigned __int64	QWORD;
#endif

#define BUILD_QWORD(lo, hi)	((u64)(((u32)lo) | ((u64)hi << 32)))
#define QWORD_LOW(qw)		((u32)(*(u32 *)&qw))
#define QWORD_HIGH(qw)		((u32)(((u64)qw) >> 32))

#endif	/* CRASS_TYPES_H */
