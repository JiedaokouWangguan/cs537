#include<stdio.h>
#include<stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include<sys/stat.h>
#include <unistd.h>
#include<sys/sysinfo.h>
#include<pthread.h>
#include<assert.h>
#include<sys/mman.h>

#define BLOCK_SIZE (128*1024*1024)
#define _FILE_OFFSET_BITS 64




typedef struct listnode listnode;

typedef struct
{
    int count;
    char c;
} __attribute__((packed)) Node;

struct listnode
{
    char firstc;
    int count;
    listnode *next;
};

typedef struct
{
    int fin;
    off_t offset;
    size_t thres;
    listnode *node;
    int ticket;
    int islast;
} myarg;

int nprocs;
int total_nprocs;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int cur_ticket = 0;
pthread_mutex_t lock_ticket = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_ticket = PTHREAD_COND_INITIALIZER;



void *mythread(void *arg)
{

    myarg *argument = (myarg *)arg;
    Node *nodes = malloc(sizeof(Node) * BLOCK_SIZE);

    int myticket = argument->ticket;
    listnode *head = argument->node;
    int fin = argument->fin;
    size_t thrsz = argument->thres;
    off_t offset = argument->offset;
    int bufferIndex = 0;
    int islast = argument->islast;
    char *src;
    if((src = mmap(0, thrsz, PROT_READ, MAP_PRIVATE, fin, offset)) == MAP_FAILED)
        exit(1);
    char last = src[0];
    int count = 1;
    int i = 1;
    
    while(i < thrsz && src[i] == last)
    {
        count++;
        i++;
    }
    head->firstc = last;





    assert(pthread_mutex_lock(&lock_ticket) == 0);
    assert(pthread_cond_broadcast(&cond_ticket) == 0);
    assert(pthread_mutex_unlock(&lock_ticket) == 0);



    for (; i < thrsz; ++i)
    {
        if(src[i] != src[i-1])
        {
            nodes[bufferIndex].count = count;
            nodes[bufferIndex].c = last;
            bufferIndex++;

            last = src[i];
            count = 1;
        }
        else
        {
            count++;
        }
    }
    nodes[bufferIndex].count = count;
    nodes[bufferIndex].c = last;
    bufferIndex++;
    munmap(src, thrsz);

    assert(pthread_mutex_lock(&lock_ticket) == 0);
    while(myticket != cur_ticket || (!islast && (head->next == NULL || head->next->firstc == '\0')    ))
    {
        assert(pthread_cond_wait(&cond_ticket, &lock_ticket) == 0);
    }

    if(head->count != 0)
    {
        nodes[0].count += head->count;
    }
    if(!islast && head->next->firstc == last)
    {
        head->next->count += nodes[bufferIndex-1].count;
        bufferIndex--;
    }
    fwrite(nodes, sizeof(Node), bufferIndex, stdout);
    free(nodes);
    bufferIndex = 0;
    cur_ticket++;
    assert(pthread_cond_broadcast(&cond_ticket) == 0);
    assert(pthread_mutex_unlock(&lock_ticket) == 0);


    assert(pthread_mutex_lock(&lock) == 0);
    nprocs ++;
    assert(pthread_cond_signal(&cond) == 0);
    assert(pthread_mutex_unlock(&lock) == 0);

    return NULL;
}




int main(int argc, char *argv[])
{
    nprocs = get_nprocs();
    total_nprocs = nprocs;
    if (argc == 1)
    {
        printf("%s\n", "pzip: file1 [file2 ...]");
        exit(1);
    }
    listnode dummy;
    listnode *current = &dummy;
    int ticket = 0;
    for (int i = 1; i < argc; i++)
    {
        FILE *filestream = fopen(argv[i], "r");       
		if (NULL == filestream)
        {
            printf("%s\n", "pzip: cannot open file");
            exit(1);
        }
		int file = fileno(filestream); 

        struct stat sbuf;
        if(fstat(file, &sbuf) < 0)
            exit(1);
        off_t offset = 0;
        while (offset < (long long)sbuf.st_size)
        {
            int islast = 0;
            size_t thrsz;
            if(((long long)sbuf.st_size - offset) > BLOCK_SIZE)
                thrsz = BLOCK_SIZE; else { thrsz = (long long)sbuf.st_size - offset; if(i == argc-1) islast = 1; } int rc = pthread_mutex_lock(&lock); assert(rc == 0); while(nprocs == 0) { rc = pthread_cond_wait(&cond, &lock); assert(rc == 0); } pthread_t p; listnode *n = malloc(sizeof(listnode)); n->firstc = '\0'; n->count = 0; current->next = n;
            current = current->next;
            myarg *argument = malloc(sizeof(myarg));
            argument->fin = file;
            argument->node = current;
            argument->offset = offset;
            argument->thres = thrsz;
            argument->ticket = ticket++;
            argument->islast = islast;
            // init argument
            nprocs--;
            assert(pthread_create(&p, NULL, mythread, argument) == 0);
            assert(pthread_mutex_unlock(&lock) == 0);

            // update offset
            assert(rc == 0);
            offset += thrsz;
        }
    }



    int rc = pthread_mutex_lock(&lock);
    assert(rc == 0);
    while(nprocs != total_nprocs)
    {
        rc = pthread_cond_wait(&cond, &lock);
        assert(rc == 0);
    }
    rc = pthread_mutex_unlock(&lock);
    assert(rc == 0);


    return 0;
}



