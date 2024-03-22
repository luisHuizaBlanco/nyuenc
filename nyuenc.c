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
    int fd;
    struct stat sb;
    size_t length;
    ssize_t s;

    //mmap to extract text to encode

    fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
        handle_error("open");
    }
        
    if (fstat(fd, &sb) == -1)           
    {
        handle_error("fstat");
    }

    length = sb.st_size;

    addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    
    if (addr == MAP_FAILED)
    {
        handle_error("mmap");
    }

    printf("%s", addr);

    //encoding text

    unsigned char encstr[1000];
    int addrpos = 0;
    int encpos = 0;
    unsigned char charcount = 1;

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

    for(int i = 0; i < encpos; i += 2) 
    {
        printf("%c%d", encstr[i], encstr[i+1]);
    }

    munmap(addr, length);
    close(fd);

    exit(EXIT_SUCCESS);
}