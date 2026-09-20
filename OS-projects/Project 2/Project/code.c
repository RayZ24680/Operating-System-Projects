#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Define the Page table entry as a struct with fields which will be used for both page table and the TLB
typedef struct TableEntry{
    int vpn;
    int ppn;
    struct TableEntry* next;
}TableEntry;


//Global variables
const int PAGE_SIZE = 256;
TableEntry* pageTable;//page table pointer
TableEntry TLB[16];//TLB with only 16 entries
int frameCount = 0;//currently assigned frames
int totalAddressesAccess = 0;//total address access counter
int pageFaultCount = 0;//page fault counter
char* backingStore;//pointer to the backing store (memory)
signed char* physicalMem;//pointer to the physical memory(memory)
int tlbReplaceIndex = 0;//uses first in first out algorithm to replace the TLB entries
int tlbHits = 0;//counter for TLB hits


//Initializes the TLB to remove any lingering garbage values.
void initTLB(){
    for(int i = 0; i < 16; i++){
        TLB[i].ppn = -1;
        TLB[i].vpn = -1;
        TLB[i].next = NULL;
    }
}
//Debug tool prints out the TLB
void printTLB(){
    for(int i = 0; i < 16; i++){
        printf("%d ", TLB[i].vpn);
    }
    printf("\n");
}
//reads the Backing store (.bin) file and loads it into the programm for quick future accesses.
char* loadMemory(char* filename, size_t* readSize){
    FILE* memFile = fopen(filename, "rb");
    if(!memFile){printf("Cannot load memory file\n");return NULL;}

    fseek(memFile, 0, SEEK_END);
    long fileSize = ftell(memFile);
    rewind(memFile);
    char* buffer = malloc(fileSize);
    if(!buffer){printf("Failed to allocate memmory\n");fclose(memFile);return NULL;}

    size_t read = fread(buffer, 1, fileSize, memFile);
    if(read != fileSize){
        printf("Memory read failed\n");
        fclose(memFile);
        free(buffer);
        return NULL;
    }

    fclose(memFile);
    *readSize = fileSize;
    return buffer;
}

//Adds a new entry to the page table, no replacement since the Virtual and Physical address space is the same.
void addTableEntry(int vpn, int ppn){
    TableEntry* new = malloc(sizeof(TableEntry));
    if(!new){
        printf("Failed to allocate memeory for new page table entry\n");
        return;
    }
    if (pageTable == NULL){
        new->vpn = vpn;
        new->ppn = ppn;
        new->next = NULL;
        pageTable = new;
    }
    else{
        TableEntry* current = pageTable;
        while(current->next != NULL){
            current = current->next;
        }
        new->vpn = vpn;
        new->ppn = ppn;
        new->next = NULL;
        current->next = new;
    }

}
//checks is an entry is inside the page table
TableEntry* findTableEntry(int vpn){
    TableEntry* current = pageTable;
    while(current != NULL){
        if(current->vpn == vpn){
            return current;
        }
        current = current->next;
    }
    return NULL;
}
//adds an entry to the TLB, replaces if needed.
void addTlbEntry(int vpn, int ppn){
    TLB[tlbReplaceIndex].vpn = vpn;
    TLB[tlbReplaceIndex].ppn = ppn;
    tlbReplaceIndex = (tlbReplaceIndex + 1) % 16;
}
//searches for an entry in the TLB
TableEntry* lookInTlb(int vpn){
    for(int i = 0; i < 16; i++){
        if(TLB[i].vpn == vpn){
            tlbHits++;
            return &TLB[i];
        }
    }
    return NULL;
}
//Since page table is a linked list it requires malloc hence free.
void freePageTable(){
    TableEntry* current = pageTable;
    while(current != NULL){
        TableEntry* temp = current;
        current = current->next;
        free(temp);
    }
}

//main, goes through the addresses.txt file and calls the appropriate functions to translate addresses and place entries in the page table and the TLB
int main(int argc, char* argv[]){
    if(argc != 2){
        printf("ERROR Usage: ./code addresses.txt\n");
        return 1;
    }
    initTLB();
    int vAddress;
    size_t size;
    backingStore = loadMemory("BACKING_STORE.bin",&size);
    physicalMem = malloc(size);
    if(physicalMem == NULL){printf("Failed to allocate physical memory\n"); return 1;}
    char* filename = argv[1];
    FILE* input = fopen(filename, "r");

    while(fscanf(input, "%d",&vAddress) == 1){

        totalAddressesAccess++;
        int vpn = (vAddress & 0xFF00) >> 8;
        int offset = vAddress & 0x00FF;
        TableEntry* entry = lookInTlb(vpn);
        
        if(entry == NULL){
            entry = findTableEntry(vpn);
            if(entry != NULL && lookInTlb(vpn) == NULL){
                addTlbEntry(entry->vpn, entry->ppn);
            }
        }
        int pAddress;
        if(entry == NULL){
            pageFaultCount++;
            int ppn = frameCount++;
            addTableEntry(vpn, ppn);
            addTlbEntry(vpn, ppn);
            memcpy(&physicalMem[ppn * PAGE_SIZE], &backingStore[vpn * PAGE_SIZE], PAGE_SIZE);
            pAddress = (ppn << 8) | offset;
        }
        else{
            pAddress = (entry->ppn << 8) | offset;
        }
        //printTLB();
        printf("Virtual address: %d Physical address: %d Value: %i\n", vAddress, pAddress, physicalMem[pAddress]);
    }
    printf("Number of Translated Addresses = %d\n", totalAddressesAccess);
    printf("Page Faults = %d\n", pageFaultCount);
    printf("Page Fault Rate = %.3f\n", (float)pageFaultCount/totalAddressesAccess);
    printf("TLB Hits = %d\n", tlbHits);
    printf("TLB Hit Rate = %.3f\n", (float)tlbHits/totalAddressesAccess);
    freePageTable();

    return 0;
}







