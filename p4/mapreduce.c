#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "mapreduce.h"

#define INIT_NUM_SLOTS 2

typedef struct _ValueNode{
    char* value;
    struct _ValueNode* next;
} ValueNode;

typedef struct {
    char* key;
    ValueNode *val_head;
    ValueNode *cur_node; // newed added by yfsong
} KeyNode;

typedef struct {
    pthread_mutex_t lock;
    KeyNode **key_nodes;
    int num_slots;
    int num_full;
} Partition;

int num_files;
char** file_names;
Mapper map_func;
int num_mapper_ths;
Reducer reduce_func;
int num_reducer_ths;
Partitioner partition_func;

Partition** all_partitions;


void reduce_launcher();
void* reduce_wrapper(void* partition_idx);
void sort_all(Partition** all_partitions);
KeyNode* binarySearch(Partition* partition, char* key);
int comparator(const void* p1, const void* p2);
void MR_Emit(char *key, char *value);
void expand(Partition* ppartition);
char *getter(char *key, int partition_number);
void* map_wrapper(void* start_idx);
void map_launcher();

char *getter(char *key, int partition_number)
{
//    printf("calling get_next() with %s %d\n", key, partition_number);
    // here, the algorithm seems super smart
    // everytime getter is called, we need to use the key to find the corresponding partitionentry
    // but since each partitionentry can only be accessed by one thread, why not just add a cache
    Partition *part = all_partitions[partition_number];
    KeyNode* keynode = binarySearch(part, key);
    if (keynode == NULL){
        return NULL;
    }

    else
    {
        if (keynode->cur_node == NULL){
//            printf("keynode NULL222\n");
            return NULL;
        }

        char *v = keynode->cur_node->value;
        keynode->cur_node = keynode->cur_node->next;
        return v;
    }
}

void reduce_launcher()
{
    pthread_t reducer_thids[num_reducer_ths]; // why is the length of the thids num+1 but not num
    int *rdc_idx_ptr;
    for(int i = 0;i<num_reducer_ths;i++)
    {
        rdc_idx_ptr = (int*)malloc(sizeof(int));
        *rdc_idx_ptr = i;
        if(pthread_create(&reducer_thids[i], NULL, reduce_wrapper, (void*)rdc_idx_ptr) != 0)
        {
            printf("Cannot create enough recuder threads, exiting...\n");
            exit(1);
        }
    }
    for(int i = 0;i<num_reducer_ths;i++)
    {
        pthread_join(reducer_thids[i], NULL);
    }
    return;
}

void* reduce_wrapper(void* partition_idx)
{
    int part_id = *((int*)partition_idx);
    Partition *part = all_partitions[part_id];
    // how and when to lock the partition?
    int i = 0;
    while(part->key_nodes[i] != NULL)
    {
        reduce_func(part->key_nodes[i]->key, getter, part_id);
        i++;
    }
    return NULL;
}

void sort_all(Partition** all_partitions)
{
    for (int i = 0; i < num_reducer_ths; i++){
        qsort(all_partitions[i]->key_nodes, all_partitions[i]->num_slots,
              sizeof(KeyNode*), comparator);
    }
}

KeyNode* binarySearch(Partition* partition, char* key)
{
    int last = partition->num_full-1;
    int first = 0;

    while(first <= last)
    {
        int mid = (first + last)/2;
        KeyNode * cur_kn = partition->key_nodes[mid];
        if(strcmp(key, cur_kn->key) == 0)
            return cur_kn;
        else if(strcmp(key, cur_kn->key) > 0)
            first = mid+1;
        else
            last = mid-1;
    }
    return NULL;
}

int comparator(const void* p1, const void* p2)
{
    KeyNode* ps1 = *((KeyNode**) p1);
    KeyNode* ps2 = *((KeyNode**) p2);
    if (ps1 == NULL)
        return 1;
    else if (ps2 == NULL)
        return -1;
    else
        return strcmp(ps1->key, ps2->key);
}

void MR_Emit(char *key, char *value)
{
    int partition_number = partition_func(key, num_reducer_ths);
    Partition *partition = all_partitions[partition_number];
    pthread_mutex_lock(&(partition->lock));
    if (partition->num_full == partition->num_slots){
        expand(partition);
    }
    int hash_idx = MR_DefaultHashPartition(key, partition->num_slots);
    int probe;

    KeyNode* get_node = partition->key_nodes[hash_idx];
    if (get_node == NULL){ // no key in this slot
        KeyNode *key_node = (KeyNode*)malloc(sizeof(KeyNode));
        key_node->key = strdup(key); // copy
        ValueNode *val_node = (ValueNode*) malloc(sizeof(ValueNode));
        val_node->value = strdup(value);
        val_node->next = NULL;
        key_node->val_head = val_node;
        key_node->cur_node = key_node->val_head;
        partition->key_nodes[hash_idx] = key_node;
        partition->num_full++;
        pthread_mutex_unlock(&(partition->lock));
        return;
    }
    else if (strcmp(get_node->key, key) == 0){ // found the right key
        ValueNode *val_node = (ValueNode*) malloc(sizeof(ValueNode));
        val_node->value = strdup(value);
        val_node->next = get_node->val_head;
        get_node->val_head = val_node;
        get_node->cur_node = get_node->val_head;
        pthread_mutex_unlock(&(partition->lock));
        return;
    }
    // linear probe to deal w/ collision
    probe = (hash_idx+1) % partition->num_slots;
    while (probe != -1 && probe != hash_idx){
        KeyNode* get_node = partition->key_nodes[probe];
        if (get_node == NULL){ // no key in this slot
            KeyNode *key_node = (KeyNode*)malloc(sizeof(ValueNode));
            key_node->key = strdup(key); // copy
            ValueNode *val_node = (ValueNode*) malloc(sizeof(ValueNode));
            val_node->value = strdup(value);
            val_node->next = NULL;
            key_node->val_head = val_node;
            key_node->cur_node = key_node->val_head;
            partition->key_nodes[probe] = key_node;
            partition->num_full++;
            probe = -1;
        }else if (strcmp(get_node->key, key) == 0){ // found the right key
            ValueNode *val_node = (ValueNode*) malloc(sizeof(ValueNode));
            val_node->value = strdup(value);
            val_node->next = get_node->val_head;
            get_node->val_head = val_node;
            get_node->cur_node = get_node->val_head;
            probe = -1;
        }else{
            probe = (probe + 1) % partition->num_slots;
        }
    }

    if (probe != -1){
        printf("Insert: linear probe failed, need more slots!");
        exit(1);
    }

    pthread_mutex_unlock(&(partition->lock));

    return;
}

void expand(Partition* partition){
    // expand the partition by doubling the size
    KeyNode **new_key_nodes = (KeyNode**)malloc(partition->num_slots*2*sizeof(KeyNode*));
    for (int i = 0; i < partition->num_slots*2; i++)
        new_key_nodes[i] = NULL;
    KeyNode **old_key_nodes = partition->key_nodes;
    int old_num_slots = partition->num_slots;
    partition->key_nodes = new_key_nodes;
    partition->num_slots *= 2;
    for (int i = 0; i < old_num_slots; i++){
        KeyNode* key_node = old_key_nodes[i];
        if (key_node != NULL){
            int hash_idx = MR_DefaultHashPartition(key_node->key, partition->num_slots);
            KeyNode* get_node = partition->key_nodes[hash_idx];
            if (get_node == NULL){
                partition->key_nodes[hash_idx] = key_node;
                continue;
            }
            int probe = (hash_idx + 1) % partition->num_slots;
            while (probe != -1 && probe != hash_idx){
                KeyNode* get_node = partition->key_nodes[probe];
                if (get_node == NULL){
                    partition->key_nodes[probe] = key_node;
                    probe = -1;
                }else{
                    probe = (probe + 1) % partition->num_slots;
                }
            }
            if (probe != -1){
                printf("Expand: linear probing failed.");
                exit(1);
            }
        }
    }
    free(old_key_nodes);

    return;

}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

void* map_wrapper(void* start_idx){
    int file_idx = *((int*)start_idx);
//    printf("mapper_idx: %d\n", file_idx);
    while (file_idx <= num_files){
        map_func(file_names[file_idx]);
        file_idx += num_mapper_ths;
    }
    return NULL;
}

void map_launcher(){
    pthread_t mapper_thids[num_mapper_ths + 1];

    int i, *start_idx_ptr;
    for (i = 1; i <= num_mapper_ths; i++){
        start_idx_ptr = (int*)malloc(sizeof(int));
        *start_idx_ptr = i;
        if (pthread_create(&mapper_thids[i], NULL,
                           map_wrapper, (void*)start_idx_ptr) != 0){
            printf("Cannot create enough mapper threads, exiting...\n");
            exit(1);
        }
    }

    for (i = 1; i <= num_mapper_ths; i++){
        pthread_join(mapper_thids[i], NULL);
    }

    return;
}

void MR_Run(int argc, char *argv[],
            Mapper map, int num_mappers,
            Reducer reduce, int num_reducers,
            Partitioner partition){

    num_files = argc - 1;
    file_names = argv;
    map_func = map;
    num_mapper_ths = num_mappers;
    reduce_func = reduce;
    num_reducer_ths = num_reducers;
    partition_func = partition;

    // init partitions
    all_partitions = malloc(sizeof(Partition*) * num_reducers);
    for(int i = 0;i<num_reducers;i++)
    {
        all_partitions[i] = malloc(sizeof(Partition));
        pthread_mutex_init(&(all_partitions[i]->lock), NULL);
        all_partitions[i]->key_nodes = malloc(INIT_NUM_SLOTS*sizeof(KeyNode*));
        for (int j = 0; j < INIT_NUM_SLOTS; j++){
            all_partitions[i]->key_nodes[j] = NULL;
        }
        all_partitions[i]->num_slots = INIT_NUM_SLOTS;
        all_partitions[i]->num_full = 0;
        // all_partitions[i] = malloc(sizeof(Partition));
    }
    map_launcher();
    sort_all(all_partitions);
    reduce_launcher();
}


void free_mem()
{
    Partition *partition;
    for(int i = 0;i<num_reducer_ths;i++)
    {
        partition = all_partitions[i];
        
        for(int j = 0;j<partition->num_full;j++)
        {
            KeyNode *cur_keynode = partition->key_nodes[j];
            ValueNode *cur_valuenode = cur_keynode->val_head;
            while(cur_valuenode != NULL)
            {
                ValueNode *tmp_value = cur_valuenode->next;
                free(cur_valuenode);
                cur_valuenode = tmp_value;
            }
            free(cur_keynode);
        }
        free(partition);
    }
    free(all_partitions);
}

