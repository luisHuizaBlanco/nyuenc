#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t mutex;
pthread_cond_t buffemp;

#define FRAG_SIZE 4096

struct fragment
{
    size_t length;
    size_t order;
    unsigned char *frag;
};

struct fragment fragbuffer[256];

size_t head;

size_t tail;

void *encoder_thread()
{
    struct fragment enc_frag;

    while(1)
    {
        //enter shared memory
        pthread_mutex_lock(&mutex);
        //take fragment from buffer
        //leave shared memory
        pthread_mutex_unlock(&mutex);
        //encode fragment

        //enter shared memory
        //add encoded fragment to the storage for all fragments
        //leave shared memory
    }
}


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

void addfragment(size_t length, unsigned char *filetext, size_t order)
{
    while(head + 1 == tail)
    {
        //for space in the buffer

    }

    tail++;

    fragbuffer[tail].length = length;
    fragbuffer[tail].frag = filetext;
    fragbuffer[tail].order = order;



}

// void removefragment(size_t length, unsigned char *filetext, size_t order)
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
    int opt;
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

            multi = 1;

            break;
        default: 
            //sequential encoding
            multi = 0;

            break;
        }
    }

    if(multi == 1)
    {
        //setting up and deploying the encoder threads
        pthread_t th[thread_num];

        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&buffemp, NULL);

        for(size_t i = 0; i < thread_num; i++)
        {
            pthread_create(&th[i], NULL, &encoder_thread, NULL);

        }

        //get number of files to encode
        size_t filenum = argc - 3;

        size_t totalfrags = 0, fragcount = 0;

        size_t fragorder = 0;

        for(size_t i = 0; i < filenum; i++)
        {
            //open file
            file = open(argv[i + 3], O_RDONLY);
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

            //check by how many times length can be divided by 4096

            if(length > FRAG_SIZE)
            {
                fragcount = 1;
                totalfrags++;
            }
            else
            {
                if(fragcount % FRAG_SIZE == 0)
                {
                    fragcount = (length / FRAG_SIZE);
                }
                else
                {
                    fragcount = (length / FRAG_SIZE) + 1;
                }

                totalfrags += fragcount;
            }

            //get all the text in the file
            
            filetext = mmap(NULL, length, PROT_READ, MAP_PRIVATE, file, 0);

            if (filetext == MAP_FAILED)
            {
                exit(0);
            }

            

            //start making fragments to add to the buffer
            for(size_t i = 0; i < fragcount; i++)
            {
                if(length >= FRAG_SIZE)
                {
                    addfragment(length, filetext, fragorder);
                }
                else
                {
                    unsigned char *fragtext;

                    memcpy(fragtext, filetext, FRAG_SIZE);

                    addfragment(FRAG_SIZE, fragtext, fragorder);
                }
            }

            
            //if the buffer is full, wait so the encoder threads give room to continue
            //continue through each file the same way
            //count how many fragments have been made for the buffer

            munmap(filetext, length);
            close(file);
        }

        //wait until the number of fragments in the encoded fragments matches the number of fragments made, then end the encoder threads
        for(size_t i = 0; i < thread_num; i++)
        {
            pthread_join(th[i], NULL);

        }

        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&buffemp);


        //start writing the encoded fragments into stdout recursively, looking for the fragment with no prev address as the first one


    }
    else
    {
        //sequential encoding

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