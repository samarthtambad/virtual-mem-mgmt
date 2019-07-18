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

typedef struct {
    std::pair<int, int> rev_map; // <pid, vpage>
    // helper data
} frame_t;

typedef struct{
    int start_vpage;
    int end_vpage;
    bool write_protected;
    bool file_mapped;
} vmaspec;

typedef struct{
    int num_vma;
    vmaspec *vma_specs;
    pte_t *page_table;
} process;
/*--------------------------------------------------------------*/

#endif