#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include "mythreads.h"
#include <assert.h>
#include <stdbool.h>
#define ARGS 2
#define DISABLE -1
typedef void * EmptyPtr;
//Structure Declarations --
//Data node
typedef struct data_node_tag {
        // private members for list.c only
        int thread_number;
        void * exit_result;
        ucontext_t thread_context;
        bool done;
        bool is_locked;
        bool WAITING;
} data_t;
//Node
typedef struct list_node_tag {
        // private members for list.c only
        data_t* data;
        struct list_node_tag *next;
        struct list_node_tag *prev;
} list_node_t;
//Header Node
typedef struct list_tag {
        //locks_array is either true or false
        list_node_t *head;
        list_node_t *tail;
        int current_list_size;
} list_t;

//Queue Node
typedef struct queue_node_tag {
        // private members for list.c only
        data_t* data;
        struct queue_node_tag *next;
} queue_node_t;
//Queeue Header
typedef struct queue_tag {
        // private members for list.c only
        queue_node_t *front;
        queue_node_t *back;
        int size;
} queue_t;

typedef struct lock_tag {
        int threadID;
        bool locked;
        //FALSE: NOT LOCKED
        //TRUE: LOCKED
}lock_t;

typedef data_t* DataPtr;
typedef list_node_t * IteratorPtr;
typedef list_t * ListPtr;
typedef  queue_t* QueueHeader;
typedef  queue_node_t* QueuePtr;
typedef lock_t Lock;
//Global Pointers and arrays
ListPtr L;
QueueHeader conditionQueue[NUM_LOCKS][CONDITIONS_PER_LOCK];
ucontext_t *current_context;
IteratorPtr idx_ptr;
IteratorPtr thread_free = NULL;
Lock locks_array[NUM_LOCKS];

//Interrupts integer and thread number
static int thread_num = 1;
int interruptsAreDisabled = 0;
/*
   interrupt Disable
   -Disable interrupts, must be enabled before
 */
static void interruptDisable(){
        assert(!interruptsAreDisabled);
        interruptsAreDisabled = 1;
}
/*
   interruptEnable

   -Enables interrutps, must be disable before calling
 */
static void interruptEnable(){
        assert(interruptsAreDisabled);
        interruptsAreDisabled = 0;
}
/*
   addWaiting

   Adds data_in to the queue of the given queue_ptr
 */
void addWaiting(QueueHeader queue_ptr, DataPtr data_in){

        QueuePtr NewNode = (QueuePtr)malloc(sizeof(queue_node_t));
        DataPtr new_data = (DataPtr)malloc(sizeof(data_t));
        if(queue_ptr->size == 0) {
                queue_ptr->front = NewNode;
                queue_ptr->back = NewNode;
        }
        else{
                queue_ptr->back->next = NewNode;
                queue_ptr->back = NewNode;
        }
        NewNode->next = NULL;
        new_data = data_in;
        NewNode->data = new_data;
        queue_ptr->size++;
}
/*
   pullwaiting

   removes the entry from the queue so that the next thread can be signaled
 */
DataPtr pullwaiting(QueueHeader queue_ptr){
        DataPtr ret_ptr = NULL;
        QueuePtr junk = queue_ptr->front;
        assert(queue_ptr->size > 0);
        ret_ptr = queue_ptr->front->data;
        queue_ptr->front = junk->next;
        queue_ptr->size--;
        junk->next = NULL;
        free(junk);
        return ret_ptr;
}
/*
   caller

   -wrapper function to run the other function given a function ptr and a argument
 */
void caller(thFuncPtr tfunc, EmptyPtr tfunc_args){
        interruptEnable();
        void * result = tfunc(tfunc_args);
        threadExit(result);
}
/*
   findNextThread

   -Looks for next avilable thread if it isnt finished, changes the idx_ptr
 */
void findNextThread(){
        do {
                idx_ptr = idx_ptr->next;
        } while(idx_ptr->data->done != false || idx_ptr->data->WAITING != false);
        //check to see if you need to free thread if so free it
}
/*
   free_stack

   -frees the stack_ptr of the thred that is done, called during threadYield
 */
void free_stack(){
        //free it everytime the thread_free is not null
        if(thread_free != NULL) {
                free(thread_free->data->thread_context.uc_stack.ss_sp);
                //set to null, not needed anymore
                thread_free = NULL;
        }
}
/*
   addthread

   -Add a thread to the main running list
 */
int addThread(ListPtr list_ptr){

        IteratorPtr NewNode = (list_node_t*)malloc(sizeof(list_node_t));
        DataPtr new_data = (DataPtr)malloc(sizeof(data_t));
        if(list_ptr->head == NULL && list_ptr->tail == NULL) {
                list_ptr->head = NewNode;
                list_ptr->tail = NewNode;
                list_ptr->tail = list_ptr->head;
                NewNode->prev = NULL;
                NewNode->next = NewNode;
                new_data->thread_number = thread_num;
                new_data->done = false;
                new_data->is_locked = false;
                new_data->WAITING = false;
                NewNode->data = new_data;

                //saves context?
                list_ptr->current_list_size++;
        }
        else{
                list_ptr->tail->next = NewNode;
                NewNode->prev = list_ptr->tail;
                list_ptr->tail = NewNode;
                NewNode->next= list_ptr->head;
                new_data->thread_number = thread_num;
                new_data->done = false;
                new_data->is_locked = false;
                new_data->WAITING = false;
                NewNode->data = new_data;

                //saves context?
                list_ptr->current_list_size++;
        }
        return thread_num;
}
/*
   threadInit

   --First function that is ran in the program, initallizes and mallocs the arrays and lists
 */
void threadInit(){
        interruptDisable();
        L = (ListPtr) malloc(sizeof(list_t));
        L->head = NULL;
        L->tail = NULL;
        L->current_list_size = 0;
        for(int i = 0; i < NUM_LOCKS; i++) {
                locks_array[i].locked = false;
                locks_array[i].threadID = DISABLE;
                for(int j = 0; j < CONDITIONS_PER_LOCK; j++) {
                        conditionQueue[i][j] = (QueueHeader)malloc(sizeof(queue_t));
                }
        }
        addThread(L);
        getcontext(&L->head->data->thread_context);
        char * a_new_stack = malloc(STACK_SIZE);
        L->head->data->thread_context.uc_stack.ss_sp = a_new_stack;
        L->head->data->thread_context.uc_stack.ss_size = STACK_SIZE;
        L->head->data->thread_context.uc_stack.ss_flags = 0;
        idx_ptr = L->head;
        current_context = &idx_ptr->data->thread_context;
        interruptEnable();
}
/*
   threadCreate

   -Create a thread, then swap the context ot said thread after it is added to the list
 */
int threadCreate(thFuncPtr funcPtr, EmptyPtr argPtr){
        interruptDisable();
        thread_num++;
        int thread_value = addThread(L);
        IteratorPtr rover = L->head;
        while(rover->data->thread_number != thread_value) {
                rover = rover->next;
        }
        assert(rover != NULL);
        getcontext(&rover->data->thread_context);
        rover->data->thread_context.uc_stack.ss_sp = malloc(STACK_SIZE);
        rover->data->thread_context.uc_stack.ss_size = STACK_SIZE;
        rover->data->thread_context.uc_stack.ss_flags = 0;

        ucontext_t *old_context = &idx_ptr->data->thread_context;
        idx_ptr = rover;
        current_context =&idx_ptr->data->thread_context;
        makecontext(current_context,(void (*)(void))caller,ARGS,funcPtr,argPtr);
        swapcontext(old_context, current_context);
        interruptEnable();
        return thread_num;
}
/*
   threadYield

   -Calls the next thread that is avilable to run (w/ findNextThread) and swaps to it
 */
void threadYield(){
        interruptDisable();
        ucontext_t* old_context = &idx_ptr->data->thread_context;
        findNextThread();
        current_context = &idx_ptr->data->thread_context;
        swapcontext(old_context, current_context);
        interruptEnable();
        free_stack();
}
/*
   threadJoin

   --Waits for the current thread to finish then saves the reuslt (if result isnt null)
 */
void threadJoin(int thread_id, void **result){
        interruptDisable();
        if(thread_id > thread_num) {
                return;
        }
        IteratorPtr rover = L->head;
        while(rover->data->thread_number != thread_id) {
                rover = rover->next;
        }
        IteratorPtr done_ptr = rover;
        interruptEnable();
        while(done_ptr->data->done != true) {
                //time to wait until im done
                if(!interruptsAreDisabled) {
                        threadYield();
                }
                else{
                        interruptEnable();
                        threadYield();
                }
        }
        if(result != NULL) {
                *result = rover->data->exit_result;
                return;
        }
}
/*
   threadExit

   -Called after a thread is finished (with caller) saves the result and marks as done
 */
void threadExit(void *result){
        interruptDisable();
        if(idx_ptr->data->thread_number == 1) {
                exit(0);
        }
        else{
                idx_ptr->data->done = true;
                //thread stack_ptr to free
                thread_free = idx_ptr;
                idx_ptr->data->exit_result = result;
                interruptEnable();
                threadYield();

        }
}
/*
   threadLock

   -locks the current thread so that for critical sections threads have to wait until
   the lock is avilable
 */
void threadLock(int lockNum){
        interruptDisable();
        if(lockNum < 0 || lockNum > NUM_LOCKS) {
                exit(0);
        }
        else{
                //spin locks, keep asking if I have the lock, and tell
                //program that the lockNum is requested
                while(locks_array[lockNum].locked == true) {
                        if(!interruptsAreDisabled) {
                                threadYield();
                        }
                        else{
                                interruptEnable();
                                threadYield();
                        }
                }
                //should be unlocked after this
                locks_array[lockNum].locked = true;
                locks_array[lockNum].threadID = idx_ptr->data->thread_number;
                idx_ptr->data->is_locked = true;
                if(interruptsAreDisabled) {
                        interruptEnable();
                }

        }
}
/*
   threadUnlock

   --Unlocks the current thread if it is locked but does nothing special if already unlocked

   p.s. if a thread never calls this function then there is deadlock!
 */
void threadUnlock(int lockNum){
        interruptDisable();
        if(lockNum < 0 || lockNum > NUM_LOCKS) {
                exit(0);
        }
        else{
                locks_array[lockNum].locked = false;
                locks_array[lockNum].threadID = DISABLE;
                idx_ptr->data->is_locked = false;
                interruptEnable();
        }
}
/*
   threadWait

   --Puts the thread in a waiting state that will wait until it is signaled by another thread
   to come out
 */
void threadWait(int lockNum, int conditionNum){
        interruptDisable();
        if(locks_array[lockNum].locked != true) {
                fprintf(stderr,"Lock is NOT locked, exiting");
                exit(0);
        }
        addWaiting(conditionQueue[lockNum][conditionNum],idx_ptr->data);
        idx_ptr->data->WAITING = true;
        interruptEnable();
        threadUnlock(lockNum);
        threadYield();
        threadLock(lockNum);
}
/*
   threadSignal

   --Removes a waiting thread from the waiting list and allows it to run again
 */
void threadSignal(int lockNum, int conditionNum){
        interruptDisable();
        DataPtr data_to_add = pullwaiting(conditionQueue[lockNum][conditionNum]);
        IteratorPtr rover = idx_ptr;
        while(rover->data->thread_number != data_to_add->thread_number) {
                rover = rover->next;
        }
        rover->data->WAITING = false;
        interruptEnable();
}
