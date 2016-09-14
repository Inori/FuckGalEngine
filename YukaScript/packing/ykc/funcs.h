#pragma once

#pragma pack(1)
struct YKCHDR {
    unsigned char signature[8]; // "YKC001"
    unsigned long __product;
    unsigned long __zero;
    unsigned long toc_offset;
    unsigned long toc_length;
};

struct YKCENTRY {
    unsigned long filename_offset;
    unsigned long filename_length;
    unsigned long data_offset;
    unsigned long data_length;
    unsigned long __zero;
};

struct YKGHDR {
    unsigned char signature[8]; // "YKG000"
    unsigned long hdr_length;
    unsigned char __reserve[28];
    unsigned long rgb_offset;
    unsigned long rgb_length;
    unsigned long msk_offset;
    unsigned long msk_length;
    unsigned long __lyr_off;
    unsigned long __lyr_len;
};
#pragma pack()

int unpack(const wchar_t* path);
int repack(const wchar_t* path);
