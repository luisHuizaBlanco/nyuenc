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
    int input, output;
    struct stat sb;
    size_t length;
    ssize_t s;

    //mmap to extract text to encode

    if(argc < 2)
    {
        return 0;
    }

    input = open(argv[1], O_RDONLY);
    if (input == -1)
    {
        handle_error("open input");
    }
        
    if (fstat(input, &sb) == -1)           
    {
        handle_error("fstat");
    }

    length = sb.st_size;

    addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, input, 0);
    
    if (addr == MAP_FAILED)
    {
        handle_error("mmap");
    }

    //encoding text

    unsigned char *encstr = malloc(length * 2);
    int addrpos = 0;
    int encpos = 0;
    unsigned char charcount = 1;

    if (encstr == NULL) {
        handle_error("malloc");
    }

    while (addr[addrpos] != '\0')
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

    fwrite(encstr, 1, sizeof(encstr), stdout);

    munmap(addr, length);
    close(input);
    close(output);
    free(encstr);

}