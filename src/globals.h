#ifndef GLOBALS_H
#define GLOBALS_H

#define NUM_PTE 64

/*----------------------Data Structures---------------------- */
typedef struct {
    unsigned int VALID:1;             // PRESENT/VALID bit
    unsigned int  WRITE_PROTECT:1;
    unsigned int  MODIFIED:1;
    unsigned int  REFERENCED:1;
    unsigned int  PAGEDOUT:1;
    unsigned int INDEX:7;
    // used 12 bits. free to use remaining 20 bits however.
} pte_t;

typedef struct{
    int start_vpage;
    int end_vpage;
    bool write_protected;
    bool file_mapped;
} vmaspec;

typedef struct{
    unsigned long unmaps;
    unsigned long maps;
    unsigned long ins;
    unsigned long outs;
    unsigned long fins;
    unsigned long fouts;
    unsigned long zeroes;
    unsigned long segv;
    unsigned long segprot;
} pstat_t;

class Process
{
    public:
        int pid;
        int num_vma;
        vmaspec *vma_specs;
        pte_t *page_table;
        pstat_t *stats;
        Process(int, int);
        ~Process();
};
Process::Process(int pid, int num_vma){
    this->pid = pid;
    this->num_vma = num_vma;
    this->vma_specs = new vmaspec[num_vma];
    this->page_table = new pte_t[NUM_PTE];
    this->stats = new pstat_t;
}

typedef struct {
    int frame_num;
    int is_mapped;
    std::pair<Process*, int> rev_map; // <process, vpage>
    // helper data
} frame_t;
/*--------------------------------------------------------------*/

extern std::deque<frame_t> frame_table; 

#endif