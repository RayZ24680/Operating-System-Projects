/*
Ray Zahid
CS-30200-002
due: 3/6/2026

discription: The code is a simulation of a os scheduler that takes input form terminal input redirection and works with realtime or interactive processes with CPU, DISK, and TTY (terminal) requests and displays preemption, and FIFO behavior for different types of requests
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum ProcessState{READY = 0, RUNNING, WAITING, TERMINATED}ProcessState;
typedef enum ProcessType{REALTIME = 0, INTERACTIVE}ProcessType;
typedef enum RequestType{CPU = 0, DISK, TTY}RequestType;

typedef struct Request{
    RequestType type;
    int remaining_time;
    int start_time;
    struct Request* next;
}Request;
typedef struct Process{
    ProcessType type;
    ProcessState state;
    int arrival_time;
    int request_count;
    int sequenceNum;
    int deadline;
    int terminalID;
    struct Request* requestList;
    struct Process* next;
}Process;
//Global vars
Process* realTimeQueue = NULL;//real time ready queue
Process* interactiveQueue = NULL;//interactive ready queue
Process* processTable = NULL;//Process table
int realTimeCompleted = 0;//stat var: number of completed real time processes
int interactiveCompleted = 0;//stat var: number of interactive processes completed
int missedDeadline = 0;//stat var: number of real time processes that missed their deadline
int diskAccesses = 0;//stat var: number of times the disk was accessed
int diskAccessTimes[100];//list for all the disk times; used for avg disk time
int totalTime = 0;//stat var: total time for the schedular  to run
int absoluteTime = 0;//time var
int cpuBusyTime = 0;//stat var: amount of time CPU was being used by a process
 int diskBusyTime = 0;//stat var: amount of time disk was being used by a process
Process* usingCpu = NULL;//process that is currently using the CPU
 Process* usingDisk = NULL;//Process that is currently using the DISK
 Process* runningTTY = NULL;//Processes that are currently using their terminals.

//This function adds a process to the process table and process queues
void enqueueProcess(Process** p, int s/*s is a swith which decides if th eprocess needs to be added to the runningTTY or process tabel or the respective queue*/){
    Process* proc = *p;//derefrencing p to use 
    if(s == 2){
        proc->next = NULL;
        if(runningTTY == NULL){
            runningTTY = proc;
        }else{
            Process* temp = runningTTY;
            while(temp->next != NULL){
                temp = temp->next;
            }
            temp->next = proc;
        }
        return;
    }
    if(s == 0){
        Process* copy = malloc(sizeof(Process));//copying p to the process table
        memcpy(copy, proc, sizeof(Process));
        copy->next = NULL;
        copy->requestList = NULL;
        if(processTable == NULL){
            processTable = copy;
        }else{
            Process* temp = processTable;
            while(temp->next != NULL){
                temp = temp->next;
            }
            temp->next = copy;
        }
    }
    proc->next = NULL;
    if(proc->type == REALTIME){
        if(realTimeQueue == NULL){
            realTimeQueue = proc;
        }else{
            Process* temp = realTimeQueue;
            while(temp->next != NULL){
                temp = temp->next;
            }
            temp->next = proc;
        }
    }else{
        if(interactiveQueue == NULL){
            interactiveQueue = proc;
        }else{
            Process* temp = interactiveQueue;
            while(temp->next != NULL){
                temp = temp->next;
            }
            temp->next = proc;
        }
    }
    proc->state = WAITING;
}

//This function adds requests to process' request lists.
void enqueueRequest(Request** head, Request* r){
    if(r == NULL){
        return;
    }
    r->next = NULL;
    if(*head == NULL){
        *head = r;
    }else{
        Request* temp = *head;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = r;
    }
}

//This function takes out the first process from a queue
Process* dequeueProcess(Process** queue){
    Process* result;//dequeued process
    if(*queue != NULL){
        result = *queue;
        (*queue) = result->next;
        result->next = NULL;
    }else{
        result = NULL;
    }
    return result;
}

//This function takes out a request from a process' request list
Request* dequeueRequest(Request** r){
    Request* result;
    if(*r != NULL){
        result = (*r);
        (*r) = result->next;
        result->next = NULL;
    }else{
        result = NULL;
    }
    return result;
}

//This function initializes a Process with given parameters and NULL/0 values.
void initProcess(Process** p, int t, int at, int se){
    (*p)->type = t;
    (*p)->arrival_time = at;
    (*p)->sequenceNum = se;
    (*p)->terminalID = se;
    (*p)->deadline = 0;
    (*p)->next = NULL;
    (*p)->requestList = NULL;
    (*p)->state = WAITING;
    (*p)->request_count = 0;
}

//This function reads the data from the input file into the appropriate queues.
void readFile() {
    char word[32];//reads the first part of the line from input.txt
    int value;//reads the time part of the process/request/deadline

    Process* current = NULL;///new process
    int seq = 0;//process' sequence / terminal number
    int reqCount = 0;//number of requests the process has.

    while (scanf("%31s %d", word, &value) == 2) {
        if (strcmp(word, "INTERACTIVE") == 0 || strcmp(word, "REAL-TIME") == 0) {
            if (current != NULL) {
                current->request_count = reqCount;
                enqueueProcess(&current, 0);
                //enqueueProcess(current, 1);
            }
            current = malloc(sizeof(Process));
            if (!current) {
                exit(1);
            }
            ProcessType pt = (strcmp(word, "REAL-TIME") == 0) ? REALTIME : INTERACTIVE;//determines the class of the new process
            initProcess(&current, pt, value, seq++);
            reqCount = 0;
            continue;
        }
        if (strcmp(word, "DEADLINE") == 0) {
            if (current == NULL || current->type != REALTIME) {
                exit(1);
            }
            current->deadline = value;
            continue;
        }
        if (current == NULL) {
            exit(1);
        }
        RequestType rt;
        if (strcmp(word, "CPU") == 0) rt = CPU;
        else if (strcmp(word, "DISK") == 0) rt = DISK;
        else if (strcmp(word, "TTY") == 0) rt = TTY;
        else {
            exit(1);
        }

        Request* r = malloc(sizeof(Request));
        r->type = rt;
        r->remaining_time = value;
        r->start_time = 0;
        r->next = NULL;

        enqueueRequest(&(current->requestList), r);
        reqCount++;
    }
    if (current != NULL) {
        current->request_count = reqCount;
        enqueueProcess(&current, 0);
    }
    //printf("Here\n");
}

//This function prints a line with details everytime a process begins to run or gets terminated.
void printProcess(Process* p, int e){
    if(p == NULL){
        return;
    }
    printf("Process %d: %s %s at time %d ms\n",p->sequenceNum,(p->type == REALTIME)?"Real-time":"interactive",(e == 0)?"Started":"Terminated",absoluteTime);
}

//This function prints a summary of the stats at the end of the program
void printSummary(){
    double cpuUtil = 0.0;//fraction of time cpu was being used
    double diskUtil = 0.0;//fraction of time disk of being used
    double avgDisk = 0.0;//avg disk time
    double misses = 0.0;//percentage of real-timr prcosses thar missed their deadline
    printf("\n=============SUMMARY=================\n");
    printf("Completed real-time processes: %d\n", realTimeCompleted);
    printf("Completed interactive processes: %d\n", interactiveCompleted);

    if(realTimeCompleted > 0){
        misses = ((double)missedDeadline / (double)realTimeCompleted) * 100.0;
    }

    printf("Percentage of real-time processes that missed deadlines: %.2f%%\n", misses);
    printf("Total disk accesses: %d\n", diskAccesses);

    if(diskAccesses > 0){
        int sum = 0;
        for(int i = 0; i < diskAccesses; i++){
            sum += diskAccessTimes[i];
        }
        avgDisk = (double)sum / (double)diskAccesses;
    }

    printf("Average disk access duration: %.2f ms\n", avgDisk);
    printf("Total time elapsed: %d ms\n", absoluteTime);

    if(absoluteTime > 0){
        cpuUtil = (double)cpuBusyTime / (double)absoluteTime;
        diskUtil = (double)diskBusyTime / (double)absoluteTime;
    }

    printf("CPU utilization: %.2f\n", cpuUtil);
    printf("Disk utilization: %.2f\n", diskUtil);
}

//This is a helper function that finds and returns an eligible process with request type t 
Process* detachFirstEligible(Process** queue, RequestType t){
    Process* prev = NULL;//previous pointer ro current prev->next = cur
    Process* cur = *queue;//cuurent pointer in the lsit.
    while(cur != NULL){
        if(cur->arrival_time <= absoluteTime &&
           cur->requestList != NULL &&
           cur->requestList->type == t){

            if(prev == NULL){
                *queue = cur->next;
            }else{
                prev->next = cur->next;
            }
            cur->next = NULL;
            return cur;
        }
        prev = cur;
        cur = cur->next;
    }
    return NULL;
}

//This function finds the next elegible process with a CPU request and updates usingCpu
void findNextCpu(){
    if(usingCpu == NULL){
        Process* p = detachFirstEligible(&realTimeQueue, CPU);
        if(p != NULL){
            usingCpu = p;
            usingCpu->state = RUNNING;
            usingCpu->requestList->start_time = absoluteTime;
            printProcess(usingCpu, 0);
            return;
        }
        p = detachFirstEligible(&interactiveQueue, CPU);
        if(p != NULL){
            usingCpu = p;
            usingCpu->state = RUNNING;
            usingCpu->requestList->start_time = absoluteTime;
            printProcess(usingCpu, 0);
            return;
        }
        return;
    }

    if(usingCpu->type == INTERACTIVE){
        Process* p = detachFirstEligible(&realTimeQueue, CPU);
        if(p != NULL){
            int runtime = absoluteTime - usingCpu->requestList->start_time;
            usingCpu->requestList->remaining_time -= runtime;
            cpuBusyTime += runtime;
            usingCpu->state = WAITING;
            enqueueProcess(&usingCpu, 1);
            usingCpu = p;
            usingCpu->state = RUNNING;
            usingCpu->requestList->start_time = absoluteTime;
            printProcess(usingCpu, 0);
        }
    }
}

//This function finds the next elegible process with a disk request and updates usingDisk
void findNextDisk(){
    if(usingDisk != NULL){
        return;
    }
    Process* rt = detachFirstEligible(&realTimeQueue, DISK);//next realtime process with disk request
    Process* in = detachFirstEligible(&interactiveQueue, DISK);//next interactive elegible process with disk request

    if(rt == NULL && in == NULL){
        return;
    }

    if(rt != NULL && in != NULL){
        if(rt->arrival_time <= in->arrival_time){
            usingDisk = rt;
            enqueueProcess(&in, 1);
        }else{
            usingDisk = in;
            enqueueProcess(&rt, 1);
        }
    }else if(rt != NULL){
        usingDisk = rt;
    }else{
        usingDisk = in;
    }

    usingDisk->state = RUNNING;
    usingDisk->requestList->start_time = absoluteTime;
    printProcess(usingDisk, 0);
}

//This function checks if usingCpu is done and removes/re enqueues it.
void checkCPU(){
    if(usingCpu == NULL || usingCpu->requestList == NULL){
        return;
    }

    Request* req = usingCpu->requestList;

    if(usingCpu->type == REALTIME && usingCpu->deadline < absoluteTime){
        missedDeadline++;
        int runtime = absoluteTime - req->start_time;
        cpuBusyTime += runtime;
        dequeueRequest(&(usingCpu->requestList));
        usingCpu->request_count--;

        if(usingCpu->requestList == NULL){
            usingCpu->state = TERMINATED;
            printProcess(usingCpu, 1);
            realTimeCompleted++;
        }else{
            RequestType next = usingCpu->requestList->type;
            if(next == CPU || next == DISK){
                enqueueProcess(&usingCpu, 1);
            }else{
                enqueueProcess(&usingCpu, 2);
            }
        }
        usingCpu = NULL;
        return;
    }

    if(req->remaining_time <= 0){
        int runtime = absoluteTime - req->start_time;
        cpuBusyTime += runtime;
        dequeueRequest(&(usingCpu->requestList));
        usingCpu->request_count--;

        if(usingCpu->requestList == NULL){
            usingCpu->state = TERMINATED;
            printProcess(usingCpu, 1);
            if(usingCpu->type == REALTIME)
                realTimeCompleted++;
            else
                interactiveCompleted++;
        }else{
            RequestType next = usingCpu->requestList->type;
            if(next == CPU || next == DISK){
                enqueueProcess(&usingCpu, 1);
            }else{
                enqueueProcess(&usingCpu, 2);
            }
        }
        usingCpu = NULL;
        return;
    }
}

//This function checks if usingDisk is done and removes/re enqueues it.
void checkDisk(){
    if(usingDisk == NULL){
        return;
    }

    if(usingDisk->type == REALTIME &&
       usingDisk->deadline < absoluteTime &&
       usingDisk->requestList != NULL){

        int runtime = absoluteTime - usingDisk->requestList->start_time;
        diskBusyTime += runtime;
        missedDeadline++;
        diskAccessTimes[diskAccesses++] = runtime;

        dequeueRequest(&(usingDisk->requestList));

        if(usingDisk->requestList == NULL){
            usingDisk->state = TERMINATED;
            printProcess(usingDisk, 1);
            realTimeCompleted++;
        }else{
            RequestType next = usingDisk->requestList->type;
            if(next == CPU || next == DISK){
                enqueueProcess(&usingDisk, 1);
            }else{
                enqueueProcess(&usingDisk, 2);
            }
        }
        usingDisk->request_count--;
        usingDisk = NULL;
        return;
    }

    if(usingDisk->requestList != NULL &&
       usingDisk->requestList->remaining_time <= 0){

        int runtime = absoluteTime - usingDisk->requestList->start_time;
        diskBusyTime += runtime;
        diskAccessTimes[diskAccesses++] = runtime;

        dequeueRequest(&(usingDisk->requestList));

        if(usingDisk->requestList == NULL){
            usingDisk->state = TERMINATED;
            printProcess(usingDisk, 1);
            if(usingDisk->type == REALTIME)
                realTimeCompleted++;
            else
                interactiveCompleted++;
        }else{
            RequestType next = usingDisk->requestList->type;
            if(next == CPU || next == DISK){
                enqueueProcess(&usingDisk, 1);
            }else{
                enqueueProcess(&usingDisk, 2);
            }
        }
        usingDisk->request_count--;
        usingDisk = NULL;
        return;
    }
}

//this is a helper function that checks if a process has a terminal open already
bool hasOpenTerminal(int tId){
    Process* temp = runningTTY;
    while(temp != NULL){
        if(temp->terminalID == tId){
            return true;
        }
        temp = temp->next;
    }
    return false;
}

//This function finds all processes ready with TTY request and runs them
void findReadyTTY(){
    Process* rt = realTimeQueue;//abbreviation to realTimeQueue
    Process* in = interactiveQueue;//abbreviation to interactiveQueue

    if(rt != NULL &&
       rt->arrival_time <= absoluteTime &&
       rt->requestList != NULL &&
       rt->requestList->type == TTY &&
       !hasOpenTerminal(rt->terminalID)){

        Process* p = dequeueProcess(&realTimeQueue);//next elegible tty process
        p->state = RUNNING;
        p->requestList->start_time = absoluteTime;
        p->next = NULL;
        enqueueProcess(&p, 2);
        printProcess(p, 0);
    }

    if(in != NULL &&
       in->arrival_time <= absoluteTime &&
       in->requestList != NULL &&
       in->requestList->type == TTY &&
       !hasOpenTerminal(in->terminalID)){

        Process* p = dequeueProcess(&interactiveQueue);
        p->state = RUNNING;
        p->requestList->start_time = absoluteTime;
        p->next = NULL;
        enqueueProcess(&p, 2);
        printProcess(p, 0);
    }
}

//This function checks for all TTY requests that have been cemoleted and removes/re enqueues them.
void checkTTY(){
    Process* previous = NULL;
    Process* current = runningTTY;

    while(current != NULL){
        bool toRemove = false;

        if(current->type == REALTIME &&
           current->deadline < absoluteTime &&
           current->requestList != NULL){

            missedDeadline++;
            dequeueRequest(&(current->requestList));
            toRemove = true;
        }
        else if(current->requestList != NULL &&
                current->requestList->remaining_time <= 0){

            dequeueRequest(&(current->requestList));
            toRemove = true;
        }

        if(toRemove){
            Process* next = current->next;

            if(previous == NULL){
                runningTTY = next;
            }else{
                previous->next = next;
            }

            current->next = NULL;

            if(current->requestList == NULL){
                current->state = TERMINATED;
                printProcess(current, 1);
                if(current->type == REALTIME)
                    realTimeCompleted++;
                else
                    interactiveCompleted++;
            }else{
                current->state = WAITING;
                RequestType nextType = current->requestList->type;
                if(nextType == CPU || nextType == DISK){
                    enqueueProcess(&current, 1);
                }else{
                    enqueueProcess(&current, 2);
                }
            }

            current = next;
            continue;
        }

        previous = current;
        current = current->next;
    }
}

//This function simulated time increment
void incrementTime(){
    absoluteTime++;
    //printf("time = %d\n", absoluteTime);

    if(usingCpu != NULL && usingCpu->requestList != NULL){
        usingCpu->requestList->remaining_time--;
    }
    if(usingDisk != NULL && usingDisk->requestList != NULL){
        usingDisk->requestList->remaining_time--;
    }

    Process* temp = runningTTY;
    while(temp != NULL){
        if(temp->requestList != NULL){
            temp->requestList->remaining_time--;
        }
        temp = temp->next;
    }

    checkCPU();
    checkDisk();
    checkTTY();

    findNextCpu();
    findNextDisk();
    findReadyTTY();
}

//Helps free allocated memory to requests
void freeRequests(Request* r){
    while(r != NULL){
        Request* next = r->next;
        free(r);
        r = next;
    }
}

//Helps free aloocated memory to processes.
void freeProcessList(Process* p){
    while(p != NULL){
        Process* next = p->next;
        freeRequests(p->requestList);
        free(p);
        p = next;
    }

}

//Frees all allocated memory
void freeAll(){
    Process* pt = processTable;
    while(pt != NULL){
        Process* next = pt->next;
        free(pt);
        pt = next;
    }
    freeProcessList(realTimeQueue);
    freeProcessList(interactiveQueue);
    freeProcessList(runningTTY);

    if(usingCpu != NULL){
        freeRequests(usingCpu->requestList);
        free(usingCpu);
    }

    if(usingDisk != NULL){
        freeRequests(usingDisk->requestList);
        free(usingDisk);
    }
}

//Main function.
int main(){
    readFile();

    while(1){
        bool cpuBusy = (usingCpu != NULL);//checks if cpu is busy
        bool diskBusy = (usingDisk != NULL);//checks if disk is being accessed
        bool ttyBusy = (runningTTY != NULL);//check if any running terminals remain

        bool queueEmpty = (realTimeQueue == NULL) && (interactiveQueue == NULL);
        bool notStuck = !cpuBusy && !diskBusy && !ttyBusy;

        if(queueEmpty && notStuck){
            break;
        }

        incrementTime();
    }

    printSummary();
    freeAll();
    return 0;
}