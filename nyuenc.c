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

int storage_tracker[263000] = {0};

void addfragment(size_t length, unsigned char *filetext, size_t order)
{
    pthread_mutex_lock(&mutexBuffer);

    while(top > 999)
    {
        //wait for space in the buffer
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
        if(totalfrags == storage_count)
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

    storage_tracker[encfragment.order] = 1;

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
        //get from buffer
        buffer_frag = removefragment();
        
        //copy fragment order
        enc_frag.order = buffer_frag.order;

        encoding(buffer_frag.length, buffer_frag.frag, &enc_frag.frag, &enc_frag.length);
        
        //send to storage
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

        size_t fragcount = 0;

        size_t fragorder = 0;

        unsigned char **filetext = malloc(filenum * sizeof(unsigned char *));
        size_t *length = malloc(filenum * sizeof(size_t));
        size_t *frags = malloc(filenum * sizeof(size_t));
    
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

            frags[i] = fragcount;

            //we changed this so now this stored everything that we need to fragment and then encode

            close(file);
        }


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

        size_t fileleft;

        unsigned char *fragtext;

        size_t current_file = 0, current_frag = 0, encoded = 0, fragmented = 0;

        fileleft = length[current_file];

        //fragmenting files and writing them when added to the buffer

        while(encoding_complete == 0)
        {
            //frags left to send to buffer
            if(fragmented < totalfrags)
            {
                //if current amount of the current file is more than one fragment
                if(fileleft > FRAG_SIZE)
                {
                    fileleft -= FRAG_SIZE;
                    fragtext = filetext[current_file] + (current_frag * FRAG_SIZE);
                    addfragment(FRAG_SIZE, fragtext, fragorder);
                    fragorder++;
                    fragmented++;
                    current_frag++;

                }
                else
                {

                    fragtext = filetext[current_file] + (current_frag * FRAG_SIZE);
                    addfragment(fileleft, fragtext, fragorder);
                    fragorder++;
                    fragmented++;
                    current_file++;
                    fileleft = length[current_file];
                    current_frag = 0;

                }

            }

            //frags left to write
            if(encoded < totalfrags)
            {
                pthread_mutex_lock(&mutexStorage);

                //is the next fragment ready
                if(storage_tracker[encoded] == 1)
                {
                    //if we are still not on the last fragment
                    if(encoded < totalfrags - 1)
                    {

                        if(storage_tracker[encoded + 1] == 1)
                        {
                            //if the last character of the current file and the next one match
                            if(encoded_storage[encoded].frag[encoded_storage[encoded].length - 2] == encoded_storage[encoded + 1].frag[0])
                            {
                                //we increase the count of the next files first character
                                encoded_storage[encoded + 1].frag[1] = encoded_storage[encoded + 1].frag[1] + encoded_storage[encoded].frag[encoded_storage[encoded].length - 1];

                                //and we remove the last two of the current file before printing
                                encoded_storage[encoded].length -= 2;

                            }

                            fwrite(encoded_storage[encoded].frag, 1, encoded_storage[encoded].length, stdout);

                            encoded++;
                        }
                    }
                    else
                    {
                        fwrite(encoded_storage[encoded].frag, 1, encoded_storage[encoded].length, stdout);

                        encoded++;
                    }

                    
                }

                pthread_mutex_unlock(&mutexStorage);
                
            }

            if(encoded == totalfrags && fragmented == totalfrags)
            {
                pthread_mutex_lock(&mutexBuffer);
                encoding_complete = 1;
                pthread_mutex_unlock(&mutexBuffer);
            }

        }

        //waiting for all threads to end
        for(size_t i = 0; i < thread_num; i++)
        {
            pthread_join(th[i], NULL);

        }

        //closing all the mutex and conditional variables
        pthread_mutex_destroy(&mutexBuffer);
        pthread_mutex_destroy(&mutexStorage);
        pthread_cond_destroy(&buffFull);
        pthread_cond_destroy(&buffEmpty);


        //freeing allocated memory
        for(size_t i = 0; i < filenum; i++)
        {
            munmap(filetext[i], length[i]);
        }

        free(filetext);
        free(length);
        free(frags);


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

/* references 

https://www.geeksforgeeks.org/getopt-function-in-c-to-parse-command-line-arguments/

https://www.geeksforgeeks.org/thread-functions-in-c-c/

https://linux.die.net/man/3/pthread_cond_broadcast

https://loiacono.faculty.polimi.it/uploads/Teaching/CP/CP_04_Pthread_CondVar.pdf

*/