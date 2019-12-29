#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <alloca.h>


int tlb_hit_count = 0;
int tlb_miss_count = 0;
int page_fault_count = 0;
int current_empty_page = 0;
int current_empty_frame = 0;
int current_empty_tlb_block = 0;
int **physical_mem;

struct vTable {
    int *pageArr; 
    int *frameArr; 
    int length;
    
};

struct tlb_structure {
    int *pageArr; 
    int *frameArr;
   
};

typedef struct vTable V;
typedef struct tlb_structure T;


void read_backing_store(int pnum,V* ptable);
void tlb_insert(int pnum, int fnum,T* tlb);
void translate_addr(int virtual_add,T* tlb,V* ptable);


int main(int argc, char* argv[]){
	//printf("%s",argv[0]);
	V* virtu_table; 
	T* tlb;
	char logical_address[12];
	double num_translations = 0;
	double tlb_hit_rate;
	double page_fault_rate;
	int i,j,k;
	
	if (argc != 2) {
        printf("Cannot open files properly.");
        return -1;
    }
    
    
        
    FILE* address_file = fopen(argv[1], "r");

    if (address_file == NULL) {
        printf("File cannot open.");
        return -1;
    }
  
    virtu_table = malloc(sizeof(V));
    virtu_table->length = 256;
    virtu_table->pageArr = malloc(sizeof(int) * 256);
    virtu_table->frameArr = malloc(sizeof(int) * 256);
   
    
    
     for (i = 0; i < 256; i++) {
        virtu_table->pageArr[i] = 0;
    }
    
    
    tlb = malloc(sizeof(T));
   tlb->pageArr = malloc(sizeof(int) * 16);
    tlb->frameArr = malloc(sizeof(int) * 16);
        
    for (i = 0; i < 16; i++) {
        tlb->pageArr[i] = 0;
    }
    
    physical_mem = (int **)malloc(256*sizeof(int *));
    for(j=0;j < 256;j++){
    	physical_mem[j] = (int *)malloc(256*sizeof(int));
    }
    
    for(j=0;j<256;j++){
    	for(k=0;k<256;k++){
    		physical_mem[j][k] = 0;
    	}
    }
    
        
    
    while (fgets(logical_address, 12, address_file) != NULL) {
        int v_add = atoi(logical_address); // converting from ascii to int        
              
        translate_addr(v_add,tlb,virtu_table);
        num_translations++;  
    }
    
    tlb_hit_rate = tlb_hit_count/num_translations;
    page_fault_rate = page_fault_count/num_translations;
    
    printf("\nTLB hit rate : %lf\t Page fault rate: %lf",tlb_hit_rate,page_fault_rate);


}


void translate_addr(int virtual_add,T* tlb,V* ptable)
{
    int frame_number = -1;
    int pnum = ((virtual_add & 0xff00)>>8);        
    int offset_num = (virtual_add & 0xff);  
    int translated_address;
    int val;
   

   
    for (int i = 0; i < 16; i++) {
        if (tlb->pageArr[i] == pnum) {
            frame_number = tlb->frameArr[i];
            tlb_hit_count++;
            //printf("\n$$tlb hit$$\n");
            break;
        }
    }
   

   
    if (frame_number == -1) {
        tlb_miss_count++; 
        for(int i = 0; i < current_empty_page; i++){
            if(ptable->pageArr[i] == pnum){  
                frame_number = ptable->frameArr[i]; 
                break; 
            }
        }
        
        
        
        if(frame_number == -1) {  
            page_fault_count++;   
            //printf("\n--page_fault--\n");
            read_backing_store(pnum,ptable);
            frame_number = current_empty_frame - 1;  
            
        }
    }
         
        tlb_insert(pnum, frame_number,tlb);
    
  	 translated_address = (frame_number << 8) | offset_num;
    val = physical_mem[frame_number][offset_num];

    	
   printf("\nGive logical address: %d\t Translated physical address: %d\t Value at that address: %d", virtual_add,translated_address,val) ;
    }


void tlb_insert(int pnum, int fnum,T* tlb)
{
    int i;
    for(i = 0; i < current_empty_tlb_block; i++){
        if(tlb->pageArr[i] == pnum){
            break;
        }
    }

       if(i == current_empty_tlb_block){
        if(current_empty_tlb_block < 16){  
            tlb->pageArr[current_empty_tlb_block] = pnum;    
            tlb->frameArr[current_empty_tlb_block] = fnum;
        }
        else{ 

           
            tlb->pageArr[current_empty_tlb_block - 1] = pnum;
            tlb->frameArr[current_empty_tlb_block - 1] = fnum;

            
            for(i = 0; i < 15; i++){
                tlb->pageArr[i] = tlb->pageArr[i + 1];
                tlb->frameArr[i] = tlb->frameArr[i + 1];
            }
        }
    }

 
    if(current_empty_tlb_block < 16) { 
        current_empty_tlb_block++;
    }
}


void read_backing_store(int pnum,V* ptable)
{
	char read_buf[256];
	FILE* bstore = fopen("BACKING_STORE.bin", "rb");

    if (bstore == NULL) {
        printf("Cannot open backing store.");
        return;
    }
   
    fseek(bstore, pnum * 256, SEEK_SET);   
    fread(read_buf, sizeof(signed char), 256, bstore);
        
    for (int i = 0; i < 256; i++) {
        physical_mem[current_empty_frame][i] = read_buf[i];
    }
   
    
    ptable->pageArr[current_empty_page] = pnum;
    ptable->frameArr[current_empty_page] = current_empty_frame;

   
    current_empty_frame++;
    current_empty_page++;
    fclose(bstore);
}

