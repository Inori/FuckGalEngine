/* camellia.h	ver 1.0

Copyright (c) 2006
 NTT (Nippon Telegraph and Telephone Corporation) . All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer as
   the first lines of this file unmodified.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY NTT ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL NTT BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CAMELLIA_H
#define CAMELLIA_H


#define CAMELLIA_TABLE_BYTE_LEN 272
#define CAMELLIA_TABLE_WORD_LEN (CAMELLIA_TABLE_BYTE_LEN / 4)

typedef unsigned int KEY_TABLE_TYPE[CAMELLIA_TABLE_WORD_LEN]; /* to match with WORD */


/*
 *-------------------------------------------------------------------------------------------------
 * Generates the key table from raw Key.
 *-------------------------------------------------------------------------------------------------
 */
void Camellia_Ekeygen
(
    const int keyBitLength, 
    const unsigned char *rawKey, 
    KEY_TABLE_TYPE keyTable
);



/*
 *-------------------------------------------------------------------------------------------------
 * 
 *-------------------------------------------------------------------------------------------------
 */
void Camellia_EncryptBlock
( 
    const int keyBitLength, 
    const unsigned char *plaintext, 
    const KEY_TABLE_TYPE keyTable, 
    unsigned char *cipherText 
);

/*
 *-------------------------------------------------------------------------------------------------
 * 
 *-------------------------------------------------------------------------------------------------
 */
void Camellia_DecryptBlock
( 
    const int keyBitLength, 
    const unsigned char *cipherText, 
    const KEY_TABLE_TYPE keyTable, 
    unsigned char *plaintext 
);




#endif /* #ifndef CAMELLIA_H */
