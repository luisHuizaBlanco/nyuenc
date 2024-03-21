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
    char letter = "";
    int count = 0;
    int fd;
    struct stat sb;
    size_t length;
    ssize_t s;

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

    for(int i = 0; i < length; i++)
    {
        if((strcmp(addr[i], letter)) != 0)
        {
            if(count > 0)
            {
                //for saving each letter and count
            }

            letter = strdup(addr[i]);

            count = 1;
        }
        else
        {
            count++;
        }

    }

    munmap(addr, length);
    close(fd);

    exit(EXIT_SUCCESS);
}