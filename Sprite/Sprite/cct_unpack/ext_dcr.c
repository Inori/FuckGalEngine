/*

    ext_dcr - extract raw data from Shockwave Director DCR/CCT files
    - binaryfail -

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include "zlib.h"

#define C2INT(a,b,c,d) ((unsigned int)((d) | (c << 8) | (b << 16) | (a << 24)))
#define bswap32(x) (((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | ((x >> 24) & 0xFF));

FILE *infile;
unsigned int fsize, zOffset;

typedef union { unsigned char s[4]; unsigned int n; } TYPE4;

typedef struct
{
    unsigned int id;
    unsigned int offset;
    unsigned int compLen;
    unsigned int decLen;
    unsigned int notCompressed;
    unsigned int type;
} ABMP_entry;

unsigned int numentries;
ABMP_entry *entry;


int writefile(const char *fname, const void *src, size_t len)
{
    FILE *outfile = fopen(fname, "wb");
    int ret;

    if (outfile == NULL)
    {
        printf("error creating %s\n", fname);
        return -1;
    }

    ret = fwrite(src, 1, len, outfile);
    fclose(outfile);

    return ret;
}

/*
void get_headermsg()
{
    unsigned char compBuf[256], decBuf[512];
    unsigned int *n;
    unsigned long compLen, decLen;
    int ret;

    fread(compBuf, 1, 4, infile);
    n = (unsigned int*)compBuf;

    if (*n != C2INT('F','c','d','r'))
    {
        printf("ERR: headermsg wrong header\n");
        return;
    }

#if 1 //decompress and write out?
    decLen = sizeof(decBuf);
    compLen = getc(infile);

    fread(compBuf, 1, compLen, infile);

    ret = uncompress(decBuf, &decLen, compBuf, compLen);

    if (ret != Z_OK)
    {
        printf("ERR: headermsg uncompress failed: %d\n", ret);
        return;
    }

    //writefile("zheader", decBuf, decLen);
    writefile("zheader", compBuf, compLen);
#else
    fseek(infile, fgetc(infile), SEEK_CUR);
#endif
}
*/

int read_integer(unsigned char *buf, unsigned int *n1)
{
    unsigned int i, p, n, temp, len;

    for (i=0; i < 6; i++)
    {
        if (buf[i] < 0x80)
            break;
    }

    len = i + 1;
    n = 0;

    for (i=0, p=len-1; i < len; i++, p--)
    {
        temp = buf[p];

        if (i > 0)
            temp -= 0x80;

        temp <<= (7*i);
        n |= temp;
    }

    *n1 = n;

    return len;
}

int get_headermsg()
{
    unsigned char readBuf[512], *bufp;
    unsigned int *n;
    unsigned int num;
    int ret, curpos;

    bufp = readBuf;

    fseek(infile, 0, SEEK_SET);
    fread(bufp, 1, 16, infile);
    bufp += 16;

    curpos = ftell(infile);
    fread(bufp, 1, 4, infile);
    ret = read_integer(bufp, &num);
    bufp += ret;
    fseek(infile, curpos + ret, SEEK_SET);

    fread(bufp, 1, num, infile);
    bufp += num;

    fread(bufp, 1, 4, infile);
    n = (unsigned int*)bufp;
    bufp += 4;

    if (*n != C2INT('F','c','d','r'))
    {
        printf("ERR: headermsg wrong header\n");
        return 1;
    }

    curpos = ftell(infile);
    fread(bufp, 1, 4, infile);
    ret = read_integer(bufp, &num);
    bufp += ret;
    fseek(infile, curpos + ret, SEEK_SET);

    fread(bufp, 1, num + 4, infile);
    bufp += num + 4;

    writefile("zheader", readBuf, bufp - readBuf);
    fseek(infile, -4, SEEK_CUR);

    return 0;
}

void extract_part(unsigned int offset, unsigned int len, unsigned int declen, unsigned int id, unsigned int notCompressed)
{
    FILE *outfile;
    char outname[16];
    int pos, ret;
    unsigned char *buf, *decbuf;

    sprintf(outname, "%08u", id);

    outfile = fopen(outname, "wb");
    if (outfile == NULL)
    {
        printf("error creating: %s\n", outname);
        return;
    }

    pos = ftell(infile);

    fseek(infile, offset, SEEK_SET);

    buf = (unsigned char*)malloc(len);
    fread(buf, 1, len, infile);

    if (notCompressed == 0)
    {
        decbuf = (unsigned char*)malloc(declen);
        ret = uncompress(decbuf, (unsigned long*)&declen, buf, len);

        if (ret != Z_OK)
        {
            printf("failed to decompress %u\n", id);
        }
        else
        {
            fwrite(decbuf, 1, declen, outfile);
        }

        free(decbuf);
    }
    else
    {
        fwrite(buf, 1, len, outfile);
    }

    fclose(outfile);
    free(buf);

    fseek(infile, pos, SEEK_SET);
}

int get_ABMP(unsigned char **dest, unsigned int *len_dest)
{
    unsigned char temp[12];
    unsigned char *compBuf, *decBuf;
    unsigned int compLen, decLen, *n;
    int ret, curpos;

    fread(temp, 1, 4, infile);
    n = (unsigned int*)temp;

    if (*n != C2INT('A','B','M','P'))
    {
        printf("ABMP: wrong header\n");
        return 1;
    }

    curpos = ftell(infile);
    fread(temp, 1, sizeof(temp), infile);

    ret = read_integer(temp, &compLen);
    curpos += ret;

    if (temp[ret] != 0)
        printf("WARN: zero where?\n");

    ret = read_integer(temp + ret + 1, &decLen);
    curpos += ret + 1;
    compLen -= ret + 1;

    zOffset = curpos + compLen + 5;

    fseek(infile, curpos, SEEK_SET);

    compBuf = (unsigned char*)malloc(compLen);
    decBuf = (unsigned char*)malloc(decLen);
    fread(compBuf, 1, compLen, infile);

    ret = uncompress(decBuf, (unsigned long*)&decLen, compBuf, compLen);

    if (ret != Z_OK)
    {
        printf("ERR: getABMP uncompress failed, %d\n", ret);
        return 3;
    }

    writefile("ABMP", decBuf, decLen);

    free(compBuf);

    *dest = decBuf;
    *len_dest = decLen;

    return 0;
}

void parse_ABMP(unsigned char *abmp, unsigned int abmp_len, const char *tablename)
{
    FILE *tablefile;
    unsigned char *p;
    unsigned int wtf1, lastId, numEntries;
    unsigned int count, c;
    int readlen;

    count = 0;
    p = abmp;
    c = 0;

    readlen = read_integer(p, &wtf1);
    p += readlen;

    readlen = read_integer(p, &lastId);
    p += readlen;

    readlen = read_integer(p, &numEntries);
    p += readlen;

    entry = (ABMP_entry*)malloc(numEntries * sizeof(ABMP_entry));

    tablefile = fopen(tablename, "w");
    if (tablefile == NULL)
    {
        printf("error creating %s\n", tablename);
        return;
    }

    //printf("%d\n%d\n%d\n", wtf1, lastId, numEntries);
    fprintf(tablefile, "%d\n%d\n%d\n", wtf1, lastId, numEntries);

    while ((p-abmp) < abmp_len)
    {
        unsigned int id, offset, compLen, decLen;
        unsigned int wtf4; //wtf4: notCompressed?
        int exists;
        TYPE4 *type;

        exists = 1;

        readlen = read_integer(p, &id);
        p += readlen;

        readlen = read_integer(p, &offset);
        count++;

        if (offset == 0xFFFFFFFF)
        {
            exists--;
            count--;
        }

        p += readlen;

        readlen = read_integer(p, &compLen);
        p += readlen;

        readlen = read_integer(p, &decLen);
        p += readlen;

        wtf4 = *(p++);
        //if (wtf4 != 0) printf("not zero: %d\n", wtf4);

        type = (TYPE4*)p;
        p += sizeof(TYPE4);

        //for ILS parsing
        entry[c].id = id;
        entry[c].offset = offset;
        entry[c].compLen = compLen;
        entry[c].decLen = decLen;
        entry[c].notCompressed = wtf4;
        entry[c].type = type->n;
        c++;

        if (exists == 1 || 1)
        {
            char temp[5];
            unsigned int *tp = (unsigned int*)temp;

            *tp = bswap32(type->n);
            temp[4]=0;

#if 0 //debug readout
            printf("[%04X %c %08X] %s : %d -> %d\n", id, (wtf4 == 0) ? ' ':'+', offset, temp, compLen, decLen);
#else //readout for dump
            //printf("%d,%d,%d,%d,%d,%s\n", id, offset, compLen, decLen, wtf4, temp);
            printf("%d\r", id);
            fprintf(tablefile, "%s%d,%d,%d,%d,%d,%s\n", (id != 2) ? "+" : "", id, offset, compLen, decLen, wtf4, temp);
#endif
        }

        if (offset != 0xFFFFFFFF) extract_part(offset + zOffset, compLen, decLen, id, wtf4);
    }

    fprintf(tablefile, "\nnum files: %d | %d entries\n", count, numEntries);
    printf("num files: %d | %d entries\n", count, numEntries);

    numentries = numEntries;
    fclose(tablefile);
}

void parse_ILS()
{
    unsigned char *inbuf, *ilsbuf, *bufp, *last;
    unsigned short *ilsIndex;
    unsigned long decLen;
    int ret, ilsCount;

    if (entry[0].type != C2INT('I','L','S',' '))
    {
        printf("ERR: entry is not ILS\n");
        return;
    }

    ilsIndex = (unsigned short*)malloc(numentries * sizeof(short));
    inbuf = (unsigned char*)malloc(entry[0].compLen);
    ilsbuf = (unsigned char*)malloc(entry[0].decLen);
    bufp = ilsbuf;
    last = ilsbuf + entry[0].decLen;

    fseek(infile, zOffset, SEEK_SET);
    fread(inbuf, 1, entry[0].compLen, infile);

    decLen = entry[0].decLen;
    ret = uncompress(ilsbuf, &decLen, inbuf, entry[0].compLen);

    if (ret != Z_OK)
    {
        printf("ERR: ILS uncompress failed, %d\n", ret);
        return;
    }

    free(inbuf);

    ilsCount = 0;

    while (bufp < last)
    {
        char outname[32], ext[5];
        unsigned int id, i, j, *n;
        TYPE4 *type;

        bufp += read_integer(bufp, &id);

        for (i=0; i<numentries; i++)
            if (entry[i].id == id)
                break;

        type = (TYPE4*)&entry[i].type;

        n = (unsigned int*)ext;
        *n = bswap32(type->n);
        ext[4] = 0;

        for (j=0; j<4; j++)
            if (ext[j] == '*')
                ext[j] = '_';

        sprintf(outname, "%08u.%s", id, ext);

        writefile(outname, bufp, entry[i].decLen);
        bufp += entry[i].decLen;
        ilsIndex[ilsCount] = id;
        ilsCount++;

        //printf("%u\n", entry[i].id);
    }

    free(ilsbuf);

    writefile("indexILS", ilsIndex, ilsCount*sizeof(short));

    printf("\nILS : %d items\n", ilsCount);
}

int main(int argc, char **argv)
{
    unsigned char *abmp_buf;
    unsigned int abmp_len;
    int ret;

    if (argc != 3)
    {
        printf("\n\
This tool is used to extract raw data from DCR and CCT files.\n\
 -- build 20131215-1000 -- \n\n\
usage:\n\
\tdcr_extract <input_file.dcr> <output_directory>\n");
        return 1;
    }

    infile=fopen(argv[1], "rb");

    if (infile == NULL)
    {
        printf("error opening %s\n", argv[1]);
        return 1;
    }

    fread(&fsize, 1, 4, infile);

    if (fsize != 0x52494658)
    {
        printf("ERR: wrong header\n");
        return 2;
    }

    _mkdir(argv[2]);
    _chdir(argv[2]);

    fread(&fsize, 1, 4, infile);

    ret = get_headermsg();

    if (ret == 0)
    {
        ret = get_ABMP(&abmp_buf, &abmp_len);

        if (ret == 0)
        {
            parse_ABMP(abmp_buf, abmp_len, "TABLE.txt");
            parse_ILS();

            free(abmp_buf);
            free(entry);
        }
    }

    fclose(infile);
    return 0;
}
