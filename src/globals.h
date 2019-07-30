#ifndef GLOBALS_H
#define GLOBALS_H

#define NUM_PTE 64

typedef enum {
    MAP_UNMAP,
    PAGEIN_OUT,
    FILEIN_OUT,
    ZEROING_PAGE,
    SEGV,
    SEGPROT,
    READ_WRITE,
    CONTEXT_SWITCH,
    PROCESS_EXIT
} execution_type_t;

int execution_time[] = {
    400,    // [0] - map/unmap
    3000,   // [1] - page-in/out
    2500,   // [2] - file-in/out
    150,    // [3] - zeroing out a page
    240,    // [4] - segv
    300,    // [5] - segprot
    1,      // [6] - read/write
    121,    // [7] - context switch
    175     // [8] - process exit
};

/*----------------------Data Structures---------------------- */
typedef struct pte_t{
    unsigned int PRESENT:1;             // PRESENT/VALID bit
    unsigned int WRITE_PROTECT:1;
    unsigned int MODIFIED:1;
    unsigned int REFERENCED:1;
    unsigned int PAGEDOUT:1;
    unsigned int INDEX:7;
    unsigned int FILE_MAPPED:1;
    unsigned int VMA_CHECK:1;
    unsigned int VMA_ASSIGNED:1;
    pte_t(): PRESENT(0), WRITE_PROTECT(0), MODIFIED(0), REFERENCED(0),
     PAGEDOUT(0), INDEX(0), FILE_MAPPED(0), VMA_CHECK(0), VMA_ASSIGNED(0){}
    // used 12 bits. free to use remaining 20 bits however.
} pte_t;

typedef struct{
    int start_vpage;
    int end_vpage;
    bool write_protected;
    bool file_mapped;
} vmaspec;

typedef struct pstat_t{
    unsigned long unmaps;
    unsigned long maps;
    unsigned long ins;
    unsigned long outs;
    unsigned long fins;
    unsigned long fouts;
    unsigned long zeros;
    unsigned long segv;
    unsigned long segprot;
    pstat_t(): unmaps(0), maps(0), ins(0), outs(0),
    fins(0), fouts(0), zeros(0), segv(0), segprot(0){}
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

typedef struct frame_t{
    int frame_num;
    std::pair<Process*, int> rev_map; // <process, vpage>
    // helper data
    int is_mapped;
    unsigned int age;
    unsigned int timelastuse;
    
    frame_t() : frame_num(0), is_mapped(false), age(0), timelastuse(0){}
} frame_t;
/*--------------------------------------------------------------*/

extern std::vector<frame_t> frame_table;
extern unsigned long inst_count;
extern int myrandom(int);
extern bool opt_a;

#endif