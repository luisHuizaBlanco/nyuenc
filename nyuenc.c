#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


int main(int argc, char *argv[])
{
    char *addr;
    int input;
    struct stat sb;
    size_t length;

    //handling arguments

    //ensuring minimum arguments to run
    if(argc < 2)
    {
        return 0;
    }

    //obtaining files for encoding

    input = open(argv[1], O_RDONLY);
    if (input == -1)
    {
        exit(0);
    }

    //mmaping to get segments of the file
        
    if (fstat(input, &sb) == -1)           
    {
        exit(0);
    }

    length = sb.st_size;

    addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, input, 0);

    if (addr == MAP_FAILED)
    {
        exit(0);
    }

    //encoding text

    unsigned char *encstr = malloc(length * 2);
    size_t addrpos = 0;
    size_t encpos = 0;
    unsigned char charcount = 1;

    if (encstr == NULL) {
        exit(0);
    }

    while (addrpos < length)
    {
        if(addr[addrpos] == addr[addrpos + 1])
        {
            charcount++;
        }
        else
        {
            encstr[encpos++] = addr[addrpos];

            encstr[encpos++] = charcount;

            charcount = 1;
        }

        addrpos++;
    }

    //writing the file

    fwrite(encstr, 1, encpos, stdout);

    munmap(addr, length);
    close(input);
    free(encstr);

}