
#include "pch.h"
#include "usr.h"

void phelp()
{
    printf("usage: iowsc <file1> -do <text>\n"
           "       iowsc <file1> <file2> -go <script>\n"
           "\n"
           "       -g    Generate file\n"
           "       -d    Dissociate file\n"
           "       -o    Output file\n"
           "       -h    Print this help\n");
    exit(0);
}

const char* marker = "<!marker for merging. should not delete>";

int main(int argc, char** argv)
{
    int opt = 0;
    int mode = 0;
    opterr = 0;
    char* output = 0;
    while(~(opt = getopt(argc, argv, "gdo:h?")))
    {
        switch(opt)
        {
        case 'g':
            mode += 'g';
            break;
        case 'd':
            mode += 'd';
            break;
        case 'o':
            output = optarg;
            break;
        case 'h':
        case '?':
            phelp();
        }
    }

    argc -= optind;
    argv += optind;

    /////////////////////////////////////////////////

    try {

        if (mode == 'g')
        {
            if (argc != 2) phelp();
            gen(argv[0], argv[1], output);
        }
        else if (mode == 'd')
        {
            if (argc != 1) phelp();
            dis(argv[0], output);
        }
        else phelp();

    }
    catch(...)
    {
        fprintf(stderr, "!operaton fail\n");
        return 1;
    }

    return 0;
}
