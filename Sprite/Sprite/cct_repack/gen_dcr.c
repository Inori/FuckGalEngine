/*

    gen_dcr - repacker tool for Shockwave Director DCR/CCT files
    - binaryfail -

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"
#ifdef _WIN32
#include <direct.h>
#define CHDIR _chdir
#define STRICMP _stricmp
#else
#include <sys/stat.h>
#include <unistd.h>
#define CHDIR chdir
#define STRICMP strcasecmp
#endif

#define C2INT(a,b,c,d) ((unsigned int)((d) | (c << 8) | (b << 16) | (a << 24)))
#define bswap32(x) (((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | ((x >> 24) & 0xFF));


typedef struct
{
    unsigned int id;
    unsigned int offset;
    unsigned int compLen;
    unsigned int decLen;
    unsigned int notCompressed;
    unsigned int type;
    unsigned int doModify;
} ABMP_entry;

unsigned char *finbuf, *finp;
ABMP_entry *entry;
unsigned int numentries;
unsigned int header[3]; //0: wtf1, 1: lastId, 2:numEntries

int compareqs(const void *a, const void *b)
{
    return *(int*)a - *(int*)b;
}


int write_integer(unsigned char *dest, unsigned int num)
{
    unsigned int n;
    unsigned char temp[6];
    unsigned int i;

    n = num;
    i = 0;

    if (n >= 0x10000000)
    {
        temp[i++] = 0x80 + n/0x10000000;
        n %= 0x10000000;
    }

    if (n >= 0x200000 || i)
    {
        temp[i++] = 0x80 + n/0x200000;
        n %= 0x200000;
    }

    if (n >= 16384 || i)
    {
        temp[i++] = 0x80 + n/16384;
        n %= 16384;
    }

    if (n >= 128 || i)
    {
        temp[i++] = 0x80 + n/128;
        n %= 128;
    }

    temp[i++] = n;

    memcpy(dest, temp, i);

    return i;
}

int rebuildABMP()
{
    FILE *outfile;
    unsigned int i, j, len;
    unsigned char outbuf[16], *outp;
    unsigned int *ep;

    outfile = fopen("newABMP", "wb");
    if (outfile == NULL)
    {
        printf("error creating newABMP\n");
        return 1;
    }

    outp = outbuf;

    for (i=0; i < 3; i++)
        outp += write_integer(outp, header[i]);

    fwrite(outbuf, 1, outp - outbuf, outfile);

    for (i=0; i < numentries; i++)
    {
        ep = (unsigned int*)&entry[i];

        for (j=0; j<5; j++)
        {
            len = write_integer(outbuf, *(ep++));
            fwrite(outbuf, 1, len, outfile);
        }

        fwrite(ep++, 1, 4, outfile);
    }

    fclose(outfile);
    return 0;
}

int processEntries()
{
    unsigned int i, j, numactive;
    unsigned int *offset_list, *np;
    unsigned int curpos;

    np = (unsigned int*)finp;
    *np = 0x46474549;
    finp += 4;
    *(finp++) = 0;

    curpos = 0;
    offset_list = (unsigned int*)malloc(numentries * sizeof(int));

    for (i=0, numactive=0; i < numentries; i++)
    {
        if (entry[i].offset == 0xFFFFFFFF)
            continue;

        offset_list[numactive++] = entry[i].offset;
    }

    qsort(offset_list, numactive, sizeof(int), compareqs);

    printf("reading files...  \n");

    for (j=0; j < numactive; j++)
        for (i=0; i < numentries; i++)
            if (entry[i].offset == offset_list[j])
            {
                FILE *infile;
                char srcname[16];
                unsigned int fsize;

                sprintf(srcname, "%08u", entry[i].id);
                printf("%s\r", srcname);

                infile = fopen(srcname, "rb");
                if (infile == NULL)
                {
                    printf("error opening %s\n", srcname);
                    j = 0xFFFFFFF0;
                    break;
                }

                //if (entry[i].offset != curpos) printf("wtf...\n");

                fseek(infile, 0, SEEK_END);
                fsize = ftell(infile);
                fseek(infile, 0, SEEK_SET);

                if (entry[i].compLen != fsize && entry[i].doModify != 1)
                {
                    printf("compLen for id %u not same... edit the table data to fix it.\n", entry[i].id);
                    return 2;
                }

                if (entry[i].doModify == 1)
                {
                    unsigned long compLen;
                    unsigned char *readbuf, *temp;
                    int ret;

                    printf("  + %s     ", srcname);

                    readbuf = (unsigned char*)malloc(fsize + 12);
                    temp = readbuf + 12;
                    fread(temp, 1, fsize, infile);

                    entry[i].decLen = fsize;

                    if (entry[i].notCompressed == 1)
                    {
                        compLen = fsize;
                        memcpy(finp, temp, fsize);
                    }
                    else
                    {
                        if (temp[0] == 0x78 && temp[1] == 0xDA)
                            printf("WARN: %s already compressed? ", srcname);

                        compLen = 20*1024*1024; //!!!

                        if (temp[0]=='F' && temp[1]=='W' && temp[2]=='S')
                        {
                            unsigned int *p = (unsigned int*)readbuf;
                            temp = readbuf;

                            *(p++) = bswap32((fsize + 12));
                            *(p++) = bswap32(1);
                            *(p++) = bswap32(fsize);

                            fsize += 12;
                            entry[i].decLen = fsize;
                        }

                        ret = compress2(finp, &compLen, temp, fsize, Z_BEST_COMPRESSION);
                        if (ret != Z_OK)
                        {
                            printf("compress error, %d\n", ret);
                            return 3;
                        }
                        else
                            printf("\n");
                    }

                    entry[i].offset = curpos;
                    entry[i].compLen = compLen;

                    finp += compLen;
                    curpos += compLen;
                    free(readbuf);
                }
                else
                {
                    entry[i].offset = curpos;
                    entry[i].compLen = fsize;

                    fread(finp, 1, fsize, infile);
                    finp += fsize;
                    curpos += fsize;
                }

                fclose(infile);
                break;
            }

    free(offset_list);

    return 0;
}

int readTableFile(const char *fname)
{
    FILE *infile;
    char buf[64];
    char *bufp;
    unsigned int i, *ep;

    entry = NULL;

    infile = fopen(fname, "rb");
    if (infile == NULL)
    {
        printf("error opening %s\n", fname);
        return 1;
    }

    for (i=0; i < 3; i++)
    {
        fgets(buf, sizeof(buf), infile);
        header[i] = atoi(buf);
    }

    entry = (ABMP_entry*)malloc(header[2] * sizeof(ABMP_entry));
    ep = (unsigned int*)entry;
    numentries = 0;

    while (fgets(buf, sizeof(buf), infile))
    {
        unsigned int doModify;

        bufp = buf;

        for (i=0; i < sizeof(buf); i++)
        {
            if (buf[i] == ',')
                buf[i] = 0;

            else if (buf[i] == '\r' || buf[i] == '\n')
            {
                buf[i] = 0;
                break;
            }
        }

        if (i == 0)
            break;

        if (buf[0] == '+')
            doModify = 1;
        else
            doModify = 0;


        for (i=0; i < 5; i++)
        {
            *(ep++) = atoi(bufp);
            bufp += strlen(bufp) + 1;
        }

        *(ep++) = bswap32(*((unsigned int*)bufp));
        *(ep++) = doModify;
        numentries++;
    }

    if (numentries != header[2])
    {
        printf("numEntries is inconsistent\n");
    }

    return 0;
}

void rebuildILS()
{
    FILE *infile;
    unsigned short *ilsIndex;
    unsigned int ilsCount, i, j;
    unsigned char *outbuf, *outp, *compbuf;
    unsigned long decLen, compLen;

    infile = fopen("indexILS", "rb");
    if (infile == NULL)
    {
        printf("indexILS not present, skipping ILS rebuild\n");
        return;
    }

    ilsIndex = (unsigned short*)malloc(numentries * sizeof(short));

    fseek(infile, 0, SEEK_END);
    ilsCount = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    fread(ilsIndex, 1, ilsCount, infile);
    fclose(infile);

    ilsCount /= sizeof(short);

    outbuf = (unsigned char*)malloc(8*1024*1024); ///!
    outp = outbuf;

    printf("rebuilding ILS...\n");

    for (j=0; j < ilsCount; j++)
    {
        char srcname[32], ext[5];
        unsigned int *n = (unsigned int*)ext;
        unsigned int fsize;

        for (i=0; i<numentries; i++)
            if (entry[i].id == ilsIndex[j])
                break;

        if (i==numentries)
        {
            printf("index %u not found\n", ilsIndex[j]);
            free(outbuf);
            return;
        }

        *n = bswap32(entry[i].type);
        ext[4]=0;
        if (ext[3] == '*')
            ext[3] = '_';

        sprintf(srcname, "%08u.%s", entry[i].id, ext);
        printf("%s\r", srcname);
        infile = fopen(srcname, "rb");

        if (infile == NULL)
        {
            printf("ILS: error opening %s\n", srcname);
            free(outbuf);
            return;
        }

        fseek(infile, 0, SEEK_END);
        fsize = ftell(infile);
        fseek(infile, 0, SEEK_SET);

        if (fsize != entry[i].decLen)
        {
            printf("updating size for %s, %u -> %u\n", srcname, entry[i].decLen, fsize);
            entry[i].decLen = fsize;
            entry[i].compLen = fsize;
        }

        outp += write_integer(outp, entry[i].id);
        fread(outp, 1, fsize, infile);
        fclose(infile);
        outp += fsize;
    }

    compLen = 2*1024*1024;
    decLen = outp - outbuf;
    compbuf = (unsigned char*)malloc(compLen);
    int ret = compress2(compbuf, &compLen, outbuf, decLen, Z_BEST_COMPRESSION);

    if (ret != Z_OK)
    {
        printf("ILS compress error, %d\n", ret);
        return;
    }
    else
    {
        infile = fopen("00000002", "wb");

        if (infile != NULL)
        {
            fwrite(compbuf, 1, compLen, infile);
            fclose(infile);
        }
        else
            printf("error creating new ILS file\n");
    }

    if (entry[0].type == C2INT('I','L','S',' '))
    {
        entry[0].compLen = compLen;
        entry[0].decLen = decLen;
    }
    else
        printf("ID 2 is not ILS\n");

    free(outbuf);
    free(compbuf);
    free(ilsIndex);
}

void buildDCR(char *outname, int isCCT, char *srcdir)
{
    FILE *infile, *outfile;
    unsigned char *readbuf, *compbuf;
    unsigned int fsize;
    unsigned long compLen;
    int ret, ret2;
    unsigned char temp[16];

    printf("writing the final output...\n");

    CHDIR(".."); ///------- 

    outfile = fopen(outname, "wb");
    if (outfile == NULL)
    {
        printf("error creating %s\n", outname);
        return;
    }

    CHDIR(srcdir); ///-------

    //file header
    infile = fopen("zheader","rb");
    if (infile == NULL)
    {
        printf("error opening zheader\n");
        return;
    }

    fseek(infile, 0, SEEK_END);
    fsize = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    readbuf = (unsigned char*)malloc(fsize);
    fread(readbuf, 1, fsize, infile);

    if (isCCT == 1)
        readbuf[4] = 'C';
    else
        readbuf[4] = 'M';

    fwrite(readbuf, 1, fsize, outfile);
    fclose(infile);
    free(readbuf);

    //ABMP
    infile = fopen("newABMP", "rb");
    if (infile == NULL)
    {
        printf("error opening newABMP\n");
        return;
    }

    fseek(infile, 0, SEEK_END);
    fsize = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    readbuf = (unsigned char*)malloc(fsize);
    fread(readbuf, 1, fsize, infile);
    fclose(infile);

    compLen = 100*1024;
    compbuf = (unsigned char*)malloc(compLen);
    ret = compress2(compbuf, &compLen, readbuf, fsize, Z_BEST_COMPRESSION);
    if (ret != Z_OK)
    {
        printf("build: ABMP compress error, %d\n", ret);
    }

    ret = write_integer(temp+8, fsize); //decLen
    ret2 = write_integer(temp, compLen + ret + 1); //compLen
    temp[ret2] = 0;

    fwrite(temp, 1, ret2 + 1, outfile);
    fwrite(temp+8, 1, ret, outfile);
    fwrite(compbuf, 1, compLen, outfile);

    free(compbuf);
    free(readbuf);

    //write data
    fwrite(finbuf, 1, finp - finbuf, outfile);

    //update file size
    fsize = ftell(outfile) - 8;
    fflush(outfile);
    fseek(outfile, 4, SEEK_SET);
    fwrite(&fsize, 1, 4, outfile);

    fclose(outfile);
}

int main(int argc, char **argv)
{
    int ret, extmode;
    unsigned int fsize, i;
    char *fname;

    printf("\n");

    if (argc != 3)
    {
        printf("This tool is used to rebuild DCR and CCT files from extracted data\n\n\
usage:\n\
\tgen_dcr <input_directory> <output_file.dcr>\n");
        return 1;
    }

    fname = strrchr(argv[2], '.');
    if (fname == NULL)
    {
        printf("the output filename must have a file extension\n");
        return 3;
    }

    if (STRICMP(fname, ".dcr") == 0)
        extmode = 0;
    else if (STRICMP(fname, ".cct") == 0)
        extmode = 1;
    else
    {
        printf("the output filename's file extension must be either DCR or CCT.\n");
        return 3;
    }

    ret = CHDIR(argv[1]);
    if (ret != 0)
    {
        printf("could not open the directory %s\n", argv[1]);
        return 1;
    }

    ret = readTableFile("TABLE.txt");

    if (ret == 0)
    {
        for (i=0, fsize=64*1024*1024; i < numentries; i++)
            fsize += entry[i].compLen;

        finbuf = (unsigned char*)malloc(fsize);
        finp = finbuf;

        rebuildILS();

        ret = processEntries();

        if (ret == 0)
            ret = rebuildABMP();

        if (ret == 0)
            buildDCR(argv[2], extmode, argv[1]);

        free(finbuf);
    }

    if (entry != NULL)
        free(entry);

    if (ret == 0)
        printf("\n  > build completed successfully\n");
    else
        printf("\n  > build failed\n");

    return 0;
}
