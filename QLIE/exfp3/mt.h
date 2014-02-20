#ifndef MT_H
#define MT_H

extern "C" {

void mt_xor_state(unsigned char* buff, unsigned long len);
void init_genrand(unsigned long s);
unsigned long genrand_int32(void);

}

#endif /* MT_H */