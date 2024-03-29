#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// pthread_mutex_t mutex;

/*
void *encoder(){


        
}
*/

// struct fragment
// {
//     size_t order;
//     size_t filenum;
//     size_t length;
//     unsigned char *addr;
// };

// const size_t SIZE = 100;

// struct fragment buff[SIZE];
// size_t head = 0;
// size_t tail = 0;

void encoding(size_t length, unsigned char *filetext, unsigned char **result_p, size_t *resultlength)
{
    unsigned char *result = malloc(length * 2);
    size_t textpos = 0;
    size_t encpos = 0;
    unsigned char charcount = 1;

    if (result == NULL) {
        exit(0);
    }

    while (textpos < length - 1)
    {
        if(filetext[textpos] == filetext[textpos + 1])
        {
            //if the current character is the same as the following, add 1 to charcount
            charcount++;
            
        }
        else
        {
            //we add the current character to the encoded array and the counter next to it
            result[encpos++] = filetext[textpos];

            result[encpos++] = charcount;

            charcount = 1;
        }

        textpos++;
    }

    result[encpos++] = filetext[textpos];

    result[encpos++] = charcount;

    *resultlength = encpos;

    *result_p = result;

}

// void addfragment(unsigned char *addr, size_t order, size_t filenum, size_t length)
// {
//     while(head + 1 = tail)
//     {
//         //wait for space in the buffer
//     }
//     tail++;

//     buffer[tail].order = order;
//     buffer[tail].filenum = filenum;
//     buffer[tail].length = length;
//     buffer[tail].addr = addr;
 
// }

// void remfragment()
// {

// }

int main(int argc, char *argv[])
{
    unsigned char *filetext;
    int file;
    struct stat sb;
    size_t length;

    //handling arguments
    //handle the -j argument and where to start looking for files
    int opt, fileind;
    //bool for if using sequential or threaded
    int multi = 0;
    //num of threads
    size_t thread_num = 0;

    //ensuring minimum arguments to run
    if(argc < 2)
    {
        return 0;
    }

    while ((opt = getopt(argc, argv, "j:")) != -1) {
        switch (opt) {
        case 'j':
            //multithread encoding
            thread_num = atoi(optarg);

            if(thread_num < 1)
            {
                return 0;
            }

            fileind = optind + 1;
            multi = 1;

            break;
        default: 
            //sequential encoding
            fileind = 1;
            multi = 0;

            break;
        }
    }

    if(multi == 1)
    {
        printf("%d\n" , fileind);
    }
    else
    {
        //obtaining num of files for encoding
        size_t filenum = argc - 1;

        //setting up memory for storing the encoded files, and the length of each
        unsigned char **encstr = malloc(filenum * sizeof(unsigned char *));
        size_t *enclength = malloc(filenum * sizeof(size_t));

        for(size_t i = 0; i < filenum; i++)
        {
            
            file = open(argv[i + 1], O_RDONLY);
            if (file == -1)
            {
                exit(0);
            }

            //mmaping to get segments of the file
                
            if (fstat(file, &sb) == -1)           
            {
                exit(0);
            }

            length = sb.st_size;

            filetext = mmap(NULL, length, PROT_READ, MAP_PRIVATE, file, 0);

            if (filetext == MAP_FAILED)
            {
                exit(0);
            }

            //encoding text
            encoding(length, filetext, &encstr[i], &enclength[i]);

            munmap(filetext, length);
            close(file);
        }

        //writing the file

        for(size_t i = 0; i < filenum; i++)
        {
            //if this isnt the last file
            if(i < filenum - 1)
            {
                //if the last character of the current file and the next one match
                if(encstr[i][enclength[i] - 2] == encstr[i + 1][0])
                {
                    //we increase the count of the next files first character
                    encstr[i+1][1] = encstr[i+1][1] + encstr[i][enclength[i] - 1];

                    //and we remove the last two of the current file before printing
                    enclength[i] -= 2;

                }
            }

            fwrite(encstr[i], 1, enclength[i], stdout);
            free(encstr[i]);
            
        }

        free(encstr);
        free(enclength);

    }
    
    
}