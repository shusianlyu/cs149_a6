/**
 * file: countnames_threaded.c
 * author: Shu Sian (Jessie) Lyu
 * description: program that takes exact two files on the command line and
 * have two threads that update the name counts of two input files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

//thread mutex lock for access to the log index
//when you print log messages from each thread
pthread_mutex_t tlock1 = PTHREAD_MUTEX_INITIALIZER;
//thread mutex lock for critical sections of allocating THREADDATA
pthread_mutex_t tlock2 = PTHREAD_MUTEX_INITIALIZER;
//thread mutex lock for access to the name counts data structure
pthread_mutex_t tlock3 = PTHREAD_MUTEX_INITIALIZER;
//thread mutex lock for deleting THREADDATA
pthread_mutex_t tlock4 = PTHREAD_MUTEX_INITIALIZER;

void* thread_runner(void*);              // thread runner function
void log_print(char *message);           // log print function
struct NAME_NODE *lookup(char *name);    // search name in the list function
void insert_node(char *name);            // insert name or update name info in the list function

pthread_t tid1, tid2;

//struct points to the thread that created the object.
//This is useful for you to know which is thread1. Later thread1 will also deallocate.
struct THREADDATA_STRUCT
{
    pthread_t creator;
};
typedef struct THREADDATA_STRUCT THREADDATA;
THREADDATA* p=NULL;
//variable for indexing of messages by the logging function.
int logindex=0;
int *logip = &logindex;

// The name counts.
// You can use any data structure you like, here are 2 proposals: a linked list OR an array (up to 100 names).
// The linked list will be faster since you only need to lock one node, while for the array you need to lock the whole array.
// You can use a linked list template from A5. You should also consider using a hash table, like in A5 (even faster).
struct NAME_STRUCT{
    char name[30];          // name
    int count;              // counts of name
};
typedef struct NAME_STRUCT THREAD_NAME;

//node with name_info for a linked list
struct NAME_NODE
{
    THREAD_NAME name_count;           // name_info node
    struct NAME_NODE *next;           // ptr to next name_info node
};
typedef struct NAME_NODE NAME_NODE;
static NAME_NODE* NAME_TOP = NULL;    // ptr to the top of the list

/** function lookup search
 * if the name is already in the list
 */
struct NAME_NODE *lookup(char *name){
    struct NAME_NODE *temp = NAME_TOP;
    while(temp != NULL){
        if (strcmp(temp->name_count.name, name) == 0){
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

/** function insert_node inserts each distinct name
* with their counts into a linked list
*/
void insert_node(char *name){
    if (NAME_TOP == NULL) {
        // initialize the stack with name
        NAME_TOP = (NAME_NODE *) malloc(sizeof(NAME_NODE));
        // no recovery needed if allocation failed, this is
        // only used in debugging, not in porduction
        if (NAME_TOP == NULL) {
            printf("insert_node: memory allocation error\n");
            exit(1);
        }
        strcpy(NAME_TOP->name_count.name, name);
        NAME_TOP->name_count.count = 1;
        NAME_TOP->next = NULL;
    }else {
        struct NAME_NODE *np = lookup(name);
        if (np == NULL){ // case 1: the name is not found in the list
            np = (struct NAME_NODE*) malloc(sizeof(*np));
            if (np == NULL){
                printf("insert_node: memory allocation error\n");
                exit(1);
            }
            NAME_NODE *curNode = NAME_TOP;
            while(curNode->next != NULL){
                curNode = curNode->next;
            }
            strcpy(np->name_count.name, name);
            np->name_count.count = 1;
            np->next = NULL;
            curNode->next = np;
        } // end of first if in else
        else{  // case 2: the name is found in the list
            np->name_count.count += 1;  // add count to name
        }
    }

}


/*********************************************************
// function main
*********************************************************/
int main(int argc, char **argv)
{
    //TODO similar interface as A2: give as command-line arguments three filenames of numbers (the numbers in the files are newline-separated).
    printf("create first thread\n");
    pthread_create(&tid1, NULL, thread_runner, argv[1]);      // pass first file into thread_runner

    printf("create second thread\n");
    pthread_create(&tid2, NULL, thread_runner, argv[2]);      // pass second file into thread_runner

    printf("wait for first thread to exit\n");
    pthread_join(tid1,NULL);
    printf("first thread exited\n");
    printf("wait for second thread to exit\n");
    pthread_join(tid2,NULL);
    printf("second thread exited\n");

    printf("======================Name Count Result======================\n");
    NAME_NODE *curNode = NAME_TOP;
    while(curNode != NULL){
        printf("%s: %d\n", curNode->name_count.name, curNode->name_count.count);
        curNode = curNode->next;
    }
    exit(0);
}//end main

/**********************************************************************
// function thread_runner runs inside each thread
**********************************************************************/
void* thread_runner(void* x)
{
    pthread_t me;
    me = pthread_self();
    char *file_name = x;        // track which file we are on

    char message[200];
//    sprintf(message, "This is thread %p (p=%p)", me, p);
//    log_print(message);

    pthread_mutex_lock(&tlock2); // critical section starts
    if (p==NULL) {  // check if it has been malloc'ed yet
        sprintf(message, "This is thread %p (p=%p)", me, p);
        log_print(message);
        p = (THREADDATA*) malloc(sizeof(THREADDATA));
        p->creator=me;
    }else{
        sprintf(message, "This is thread %p (p=%p)", me, p);
        log_print(message);
    }
    pthread_mutex_unlock(&tlock2);  // critical section ends

    if (p!=NULL && p->creator==me) {
        sprintf(message, "This is thread %p and I created THREADDATA %p",me,p);
        log_print(message);
    } else {
        sprintf(message, "This is thread %p and I can access the THREADDATA %p", me, p);
        log_print(message);
    }

    // open file
    FILE *fp = fopen(file_name, "r");
    if(fp == NULL){
        perror("range: cannot open file.\n");
        exit(1);
    }
    sprintf(message, "opened file %s", file_name);
    log_print(message);
    pthread_mutex_lock(&tlock3); // critical section starts for accessing data structure
    int line_index = 1;
    char buffer[30];
    while(fgets(buffer, 30, fp) != NULL){
        if (strlen(buffer) == 1){
            printf("Warning - file %s line %d is empty.\n", file_name, line_index);
        } else{
            if (buffer[strlen(buffer) - 1] == '\n'){
                buffer[strlen(buffer) - 1] = 0;      // replace newline with null
            }
            line_index++;
            insert_node(buffer);
        }
    }
    pthread_mutex_unlock(&tlock3);   // critical section ends for accessing data structure

    pthread_mutex_lock(&tlock4);
    if (p!=NULL && p->creator==me) {
        sprintf(message, "This is thread %p and I delete THREADDATA",me);
        log_print(message);
        free(p);
        p = NULL;
    } else {
        sprintf(message, "This is thread %p and I can access the THREADDATA",me);
        log_print(message);
    }
    pthread_mutex_unlock(&tlock4);

    pthread_exit(NULL);
    return NULL;
}//end thread_runner

/**********************************************************************
// function log_print prints log message
**********************************************************************/
void log_print(char *message){

    pthread_t m;
    m = pthread_self();
    pthread_mutex_lock(&tlock1); // critical section starts for printing message

    // variables to store date and time components
    int hours, minutes, seconds, day, month, year;
    // time_t is arithmetic time type
    time_t now;
    // Obtain current time
    // time() returns the current time of the system as a time_t value
    time(&now);
    // localtime converts a time_t value to calendar time and
    // returns a pointer to a tm structure with its members
    // filled with the corresponding values
    struct tm *local = localtime(&now);
    hours = local->tm_hour;       // get hours since midnight (0-23)
    minutes = local->tm_min;      // get minutes passed after the hour (0-59)
    seconds = local->tm_sec;      // get seconds passed after minute (0-59)
    day = local->tm_mday;         // get day of month (1 to 31)
    month = local->tm_mon + 1;    // get month of year (0 to 11)
    year = local->tm_year + 1900; // get year since 1900


    // print local time
    if (hours < 12) // before midday
        printf("Logindex %d, thread %p, PID %d, %02d/%02d/%d %02d:%02d:%02d am: %s\n", ++(*logip), m, getpid(), day, month, year, hours, minutes, seconds, message);
    else // after midday
        printf("Logindex %d, thread %p, PID %d, %02d/%02d/%d %02d:%02d:%02d pm: %s\n", ++(*logip), m, getpid(), day, month, year, hours, minutes, seconds, message);

    pthread_mutex_unlock(&tlock1); // critical section ends for printing message

}