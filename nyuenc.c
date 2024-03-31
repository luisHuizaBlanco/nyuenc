#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t mutexBuffer, mutexStorage;
pthread_cond_t buffFull, buffEmpty;

size_t top = 0;

size_t head = 0;

size_t tail = 0;

size_t storage_count = 0;

int encoding_complete = 0;

size_t totalfrags = 0;

#define FRAG_SIZE 4096

struct fragment
{
    size_t length;
    size_t order;
    unsigned char *frag;
};

struct fragment frag_buffer[1000];

struct fragment encoded_storage[263000];

void addfragment(size_t length, unsigned char *filetext, size_t order)
{
    pthread_mutex_lock(&mutexBuffer);

    while(top > 999)
    {
        //for space in the buffer
        pthread_cond_wait(&buffFull, &mutexBuffer);

    }

    frag_buffer[top].length = length;
    frag_buffer[top].frag = filetext;
    frag_buffer[top].order = order;

    top++;

    if(top == 1)
    {
        pthread_cond_signal(&buffEmpty);
    }
    
    pthread_mutex_unlock(&mutexBuffer);

}

struct fragment removefragment()
{
    pthread_mutex_lock(&mutexBuffer);

    while(top < 1)
    {
        if(encoding_complete == 1)
        {
            pthread_cond_broadcast(&buffEmpty);

            pthread_mutex_unlock(&mutexBuffer);

            pthread_exit(0);
        }
        
        pthread_cond_wait(&buffEmpty, &mutexBuffer);
    }

    struct fragment fragfrombuffer;

    top--;

    fragfrombuffer = frag_buffer[top];

    if(top == 999)
    {
        pthread_cond_signal(&buffFull);
    }

    pthread_mutex_unlock(&mutexBuffer);

    return fragfrombuffer;
    
}

void store_encodedfragment(struct fragment encfragment)
{

    pthread_mutex_lock(&mutexStorage);
    encoded_storage[encfragment.order] = encfragment;

    storage_count++;
    pthread_mutex_unlock(&mutexStorage);
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

void *encoder_thread()
{
    struct fragment buffer_frag, enc_frag;

    while(1)
    {
        buffer_frag = removefragment();

        enc_frag.order = buffer_frag.order;

        encoding(buffer_frag.length, buffer_frag.frag, &enc_frag.frag, &enc_frag.length);

        store_encodedfragment(enc_frag);
    }

    return NULL;
}


int main(int argc, char *argv[])
{
    
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
        
        //get number of files to encode
        size_t filenum = argc - 3;

        size_t totalfrags = 0, fragcount = 0;

        size_t fragorder = 0;

        unsigned char **filetext = malloc(filenum * sizeof(unsigned char *));
        size_t *length = malloc(filenum * sizeof(size_t));

        //setting up and deploying the encoder threads
        pthread_t th[thread_num];

        pthread_mutex_init(&mutexBuffer, NULL);
        pthread_mutex_init(&mutexStorage, NULL);
        pthread_cond_init(&buffFull, NULL);
        pthread_cond_init(&buffEmpty, NULL);


        for(size_t i = 0; i < thread_num; i++)
        {
            pthread_create(&th[i], NULL, &encoder_thread, NULL);

        }

    
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

            length[i] = sb.st_size;

            //check by how many times length can be divided by 4096

            if(length[i] > FRAG_SIZE)
            {
                if(length[i] % FRAG_SIZE == 0)
                {
                    fragcount = (length[i] / FRAG_SIZE);
                }
                else
                {
                    fragcount = (length[i] / FRAG_SIZE) + 1;
                }

                totalfrags += fragcount;
            }
            else
            {
                fragcount = 1;
                totalfrags += fragcount;
            }

            //get all the text in the file
            
            filetext[i] = mmap(NULL, length[i], PROT_READ, MAP_PRIVATE, file, 0);

            if (filetext[i] == MAP_FAILED)
            {
                exit(0);
            }

            //track current spot of the file
            size_t fileleft = length[i];

            unsigned char *fragtext;

            //start making fragments to add to the buffer
            for(size_t j = 0; j < fragcount; j++)
            {
                if(fileleft > FRAG_SIZE)
                {
                    fileleft -= FRAG_SIZE;

                    fragtext = filetext[i] + (j * FRAG_SIZE);

                    addfragment(FRAG_SIZE, fragtext, fragorder);

                    fragorder++;

                }
                else
                {

                    fragtext = filetext[i] + (j * FRAG_SIZE);

                    //fprintf(stderr, "%20s\n",fragtext);

                    addfragment(fileleft, fragtext, fragorder);

                    fragorder++;

                }
            }

            //if the buffer is full, wait so the encoder threads give room to continue
            //continue through each file the same way
            //count how many fragments have been made for the buffer

            close(file);
        }

        //wait until totalfrags == storage_count
        pthread_mutex_lock(&mutexBuffer);
        encoding_complete = 1;
        pthread_mutex_unlock(&mutexBuffer);

        //wait until the number of fragments in the encoded fragments matches the number of fragments made, then end the encoder threads
        for(size_t i = 0; i < thread_num; i++)
        {
            pthread_join(th[i], NULL);

        }

        pthread_mutex_destroy(&mutexBuffer);
        pthread_mutex_destroy(&mutexStorage);
        pthread_cond_destroy(&buffFull);
        pthread_cond_destroy(&buffEmpty);

        for(size_t i = 0; i < storage_count; i++)
        {
            //if this isnt the last fragment
            if(i < storage_count - 1)
            {
                //if the last character of the current file and the next one match

                if(encoded_storage[i].frag[encoded_storage[i].length - 2] == encoded_storage[i + 1].frag[0])
                {
                    //we increase the count of the next files first character
                    encoded_storage[i + 1].frag[1] = encoded_storage[i + 1].frag[1] + encoded_storage[i].frag[encoded_storage[i].length - 1];

                    //and we remove the last two of the current file before printing
                    encoded_storage[i].length -= 2;

                }

            }

            fwrite(encoded_storage[i].frag, 1, encoded_storage[i].length, stdout);
            
        }

        for(size_t i = 0; i < filenum; i++)
        {
            munmap(filetext[i], length[i]);
        }

        free(filetext);
        free(length);

        //start writing the encoded fragments into stdout recursively, looking for the fragment with no prev address as the first one


    }
    else
    {
        //sequential encoding
        unsigned char *filetext;

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