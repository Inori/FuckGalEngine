// This function is written by puu.  Original license is unspecified.

// LZ圧縮の展開
int decode_lz(unsigned char *buf_in, unsigned size_in, unsigned char *buf_out, unsigned size_out)
{
  unsigned ptr_in, ptr_out, ptr_src;
  int lz_back, lz_rept, bitvalue;
  unsigned short lz_bitd, lz_bitm;
  unsigned char ch;

  ptr_in = ptr_out = 0;
  lz_bitm = 0x0000;

  #define GETBYTE(VALUE) { \
    if (ptr_in+1>size_in) \
      return 1; \
    VALUE = buf_in[ptr_in++]; \
  }

  #define GETBIT(VALUE) { \
    if (!(lz_bitm<<=1)) { \
      if (ptr_in+2>size_in) \
        return 1; \
      lz_bitd = *(unsigned short *)&buf_in[ptr_in]; \
      ptr_in += 2; \
      lz_bitm = 0x0001; \
    } \
    VALUE = (lz_bitm&lz_bitd)?1:0; \
  }

  while (1) {
    GETBIT(bitvalue)
    if (bitvalue) { // "1"
      GETBYTE(ch)
      if (ptr_out>=size_out)
        return 1;
      buf_out[ptr_out++] = ch;
    }
    else { // "0"
      GETBIT(bitvalue)
      if (!bitvalue) { // "00"
        GETBIT(bitvalue)
        if (!bitvalue) { // "000"
          GETBYTE(ch)
          if (ch==255) // EOF
            break;
          lz_back = (int)ch+1;
        }
        else { // "001"
          lz_back = 256;
          GETBIT(bitvalue)
          if (bitvalue)
            lz_back += 1024;
          GETBIT(bitvalue)
          if (bitvalue)
            lz_back += 512;
          GETBIT(bitvalue)
          if (bitvalue)
            lz_back += 256;
          GETBYTE(ch)
          lz_back += (int)ch;
        }
        lz_rept = 2;
      }
      else { // "01"
        GETBIT(bitvalue)
        lz_back = bitvalue;
        GETBIT(bitvalue)
        if (bitvalue) { // "01?1"
          GETBYTE(ch)
          lz_back *= 256;
          lz_back += (int)ch+1;
        }
        else { // "01?0"
          GETBIT(bitvalue)
          if (bitvalue) { // "01?01"
            GETBYTE(ch)
            lz_back *= 256;
            lz_back += (int)ch+513;
          }
          else { // "01?00"
            GETBIT(bitvalue)
            lz_back *= 2;
            lz_back += bitvalue;
            GETBIT(bitvalue)
            if (bitvalue) { // "01?00?1"
              GETBYTE(ch)
              lz_back *= 256;
              lz_back += (int)ch+1025;
            }
            else { // "01?00?0"
              GETBIT(bitvalue)
              lz_back *= 2;
              lz_back += bitvalue;
              GETBIT(bitvalue)
              if (bitvalue) { // "01?00?0?1"
                GETBYTE(ch)
                lz_back *= 256;
                lz_back += (int)ch+2049;
              }
              else { // "01?00?0?0"
                GETBIT(bitvalue)
                lz_back *= 2;
                lz_back += bitvalue;
                GETBYTE(ch)
                lz_back *= 256;
                lz_back += (int)ch+4097;
              }
            }
          }
        }
        GETBIT(bitvalue)
        if (bitvalue) { // "…1"
          lz_rept = 3;
        }
        else { // "…0"
          GETBIT(bitvalue)
          if (bitvalue) { // "…01"
            lz_rept = 4;
          }
          else { // "…00"
            GETBIT(bitvalue)
            if (bitvalue) { // "…001"
              lz_rept = 5;
            }
            else { // "…000"
              GETBIT(bitvalue)
              if (bitvalue) { // "…0001"
                lz_rept = 6;
              }
              else { // "…0000"
                GETBIT(bitvalue)
                if (bitvalue) { // "…00001"
                  GETBIT(bitvalue)
                  lz_rept = bitvalue+7;
                }
                else { // "…00000"
                  GETBIT(bitvalue)
                  if (bitvalue) { // "…000001"
                    GETBYTE(ch)
                    lz_rept = (int)ch+17;
                  }
                  else { // "…000000"
                    lz_rept = 9;
                    GETBIT(bitvalue)
                    if (bitvalue)
                      lz_rept += 4;
                    GETBIT(bitvalue)
                    if (bitvalue)
                      lz_rept += 2;
                    GETBIT(bitvalue)
                    if (bitvalue)
                      lz_rept += 1;
                  }
                }
              }
            }
          }
        }
      }
      if (lz_back>ptr_out)
        return 1;
      if (ptr_out+lz_rept>size_out)
        return 1;
      ptr_src = ptr_out-lz_back;
      while (--lz_rept>=0)
        buf_out[ptr_out++] = buf_out[ptr_src++];
    }
  }
  if (ptr_out!=size_out || ptr_in!=size_in)
    return 1;
  return 0;
}