#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


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

    //obtaining num of files for encoding
    size_t filenum = argc - 1;

    unsigned char **encstr = malloc(filenum * sizeof(unsigned char *));
    size_t *enclength = malloc(filenum * sizeof(size_t));

    for(size_t i = 0; i < filenum; i++)
    {
        input = open(argv[i + 1], O_RDONLY);
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
        
        encstr[i] = malloc(length * 2);
        size_t addrpos = 0;
        size_t encpos = 0;
        unsigned char charcount = 1;

        if (encstr[i] == NULL) {
            exit(0);
        }

        while (addrpos < length - 1)
        {
            if(addr[addrpos] == addr[addrpos + 1])
            {
                charcount++;
                
            }
            else
            {
                encstr[i][encpos++] = addr[addrpos];

                encstr[i][encpos++] = charcount;

                charcount = 1;
            }

            addrpos++;
        }

        encstr[i][encpos++] = addr[addrpos];

        encstr[i][encpos++] = charcount;

        enclength[i] = encpos;

        munmap(addr, length);
        close(input);
    }

    //writing the file

    for(size_t i = 0; i < filenum; i++)
    {
        if(i < filenum - 1)
        {
            if(encstr[i][enclength[i] - 2] == encstr[i + 1][0])
            {
                encstr[i+1][1] = encstr[i+1][1] + encstr[i][enclength[i] - 1];

                enclength[i] -= 2;

            }
        }

        fwrite(encstr[i], 1, enclength[i], stdout);
        free(encstr[i]);
        
    }

    free(encstr);
    free(enclength);
    
}