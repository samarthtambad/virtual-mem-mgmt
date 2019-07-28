// #include <stdlib.h>
// #include <stdio.h>
#include <string>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <queue>

#include "globals.h"
#include "pagers/Pager.h"
#include "pagers/FIFO.h"

using namespace std;

// int num_pte = 64;
int num_frames = 128;

bool testing = false;
long rand_ofs = 0;
vector<long> randvals;
vector<pte_t> page_table;
// vector<process> processes;
vector<frame_t> frame_table;   // OS maintains this
deque<frame_t*> free_pool;
vector<Process*> processes;
Process *current_process;

// TOTALCOST stats
unsigned long long cost = 0;
unsigned long inst_count = 0, ctx_switches = 0, process_exits = 0;

bool print_output = false, print_pt = false, print_ft = false, print_procstats = false;
bool opt_x = false, opt_y = false, opt_f = false, opt_a = false;

Pager *pager = (Pager *) new FIFO();


void parse_random(string rand_file){
    ifstream in;
    in.open(rand_file.c_str());
    if(!in){
        printf("Error: Can't open the file named %s\n", rand_file.c_str());
        exit(1);
    }
    int num_rand;
    in >> num_rand;
    int i = 0;
    long num;
    while(in >> num){
        randvals.push_back(num);
        i++;
    }
    in.close();
}

int myrandom(int size){
    int res = (randvals[rand_ofs] % size);
    rand_ofs++;
    if(rand_ofs == randvals.size()) rand_ofs = 0;
    return res;
}

void parse_args(int argc, char *argv[], string &input_file, string &rand_file){
    int c;
    if(testing){
        printf("-------inputs:-------\n");
    } 
    while ((c = getopt (argc, argv, "a:o:f:")) != -1){
        switch(c){
            case 'a':
                char *algo;
                algo = optarg;
                if(testing) printf("algo %s\n", algo);
                break;
            case 'o':
                char *options;
                options = optarg;
                for(char* it = options; *it; ++it) {
                    switch(*it){
                        case 'O': 
                            print_output = true;
                            break;
                        case 'P':
                            print_pt = true;
                            break;
                        case 'F':
                            print_ft = true;
                            break;
                        case 'S':
                            print_procstats = true;
                            break;
                        case 'x':
                            opt_x = true;
                            break;
                        case 'y':
                            opt_y = true;
                            break;
                        case 'f':
                            opt_f = true;
                            break;
                        case 'a':
                            opt_a = true;
                            break;
                    }
                }
                if(testing) printf("options: %s\n", options);
                break;
            case 'f':
                num_frames = atoi(optarg);
                if(testing) printf("num_frames: %d\n", num_frames);
                break;
            case '?':
                if (optopt == 's')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
            default:
                abort();
        }
    }
    int idx = optind;
    if(idx < argc) input_file = argv[idx];
    idx++;
    if(idx < argc) rand_file = argv[idx];
    if(testing){
        printf("input file: %s, rand file: %s\n", input_file.c_str(), rand_file.c_str());
        printf("------------------------\n");
    }
}

string get_next_valid_line(ifstream &fin){
    string line = "";
    while(fin){
        getline(fin, line);
        if(line[0] == '#') continue;
        break;
    }
    return line;
}

frame_t* allocate_frame_from_free_list(){
    if(free_pool.empty()) return nullptr;
    frame_t* f = free_pool.front();
    free_pool.pop_front();
    return f;
}

frame_t* get_frame() {
    frame_t *frame = allocate_frame_from_free_list();
    if (frame == nullptr) frame = pager->select_victim_frame();
    return frame;
}

void pagefault_handler(pte_t *pte, int vpage){
    
    // check if pte part of VMA
    if(!pte->VMA_CHECK){
        for(int i = 0; i < current_process->num_vma; i++){
            if(vpage >= current_process->vma_specs[i].start_vpage && vpage <= current_process->vma_specs[i].end_vpage){
                pte->VMA_ASSIGNED = 1;
                pte->WRITE_PROTECT = current_process->vma_specs[i].write_protected;
                pte->FILE_MAPPED = current_process->vma_specs[i].file_mapped;
            }
        }
        pte->VMA_CHECK = 1;
    }
    if(!pte->VMA_ASSIGNED){
        if(print_output) printf(" SEGV\n");
        cost = cost + execution_time[READ_WRITE];
        cost = cost + execution_time[SEGV];
        current_process->stats->segv += 1;
        return;
    }
    
    frame_t* newframe = get_frame();

    if(newframe->is_mapped){    // unmap old frame
        Process* old_process = newframe->rev_map.first;
        pte_t* old_pte = &old_process->page_table[newframe->rev_map.second];
        if(print_output) printf(" UNMAP %d:%d\n", newframe->rev_map.first->pid, newframe->rev_map.second);
        cost = cost + execution_time[MAP_UNMAP];
        old_process->stats->unmaps += 1;

        if(old_pte->MODIFIED){  // dirty. save changes to disk OUT/FOUT
            if(old_pte->FILE_MAPPED){
                if(print_output) printf(" FOUT\n");
                cost = cost + execution_time[FILEIN_OUT];
                old_process->stats->fouts += 1;
            } else {
                if(print_output) printf(" OUT\n");
                old_pte->PAGEDOUT = 1;
                cost = cost + execution_time[PAGEIN_OUT];
                old_process->stats->outs += 1;
            }
            old_pte->MODIFIED = 0;
        }

        old_pte->PRESENT = 0;
        newframe->is_mapped = false;
        newframe->rev_map = make_pair(nullptr, -1);
        
    }

    if(pte->FILE_MAPPED){
        if(print_output) printf(" FIN\n");
        cost = cost + execution_time[FILEIN_OUT];
        current_process->stats->fins += 1;
    } else {
        if(pte->PAGEDOUT){
            if(print_output) printf(" IN\n");
            cost = cost + execution_time[PAGEIN_OUT];
            current_process->stats->ins += 1;
        } else {
            pte->PRESENT = 0;
            pte->MODIFIED = 0;
            pte->REFERENCED = 0;
            pte->INDEX = 0;
            pte->PAGEDOUT = 0;
            if(print_output) printf(" ZERO\n");
            cost = cost + execution_time[ZEROING_PAGE];
            current_process->stats->zeros += 1;
        }
    }
    
    // map newframe to vpage of current_process
    pte->PRESENT = 1;
    pte->INDEX = newframe->frame_num;
    newframe->is_mapped = true;
    newframe->rev_map = make_pair(current_process, vpage);
    if(print_output) printf(" MAP %d\n", pte->INDEX);
    cost = cost + execution_time[MAP_UNMAP];
    current_process->stats->maps += 1;

}

void print_stats(){
    if(print_pt){   // print the page table info of each process
        Process* p;
        pte_t* ptable;
        for(int i = 0; i < processes.size(); i++){
            p = processes[i];
            ptable = p->page_table;
            printf("PT[%d]: ", i);
            for(int j = 0; j < NUM_PTE; j++){
                if(!ptable[j].PRESENT){
                    if(ptable[j].PAGEDOUT) printf("# ");
                    else printf("* ");
                } else {
                    printf("%d:", j);
                    if(ptable[j].REFERENCED) printf("R");
                    else printf("-");
                    if(ptable[j].MODIFIED) printf("M");
                    else printf("-");
                    if(ptable[j].PAGEDOUT) printf("S");
                    else printf("-");
                    printf(" ");
                }
            }
            printf("\n");
        }
        p = nullptr;
        ptable = nullptr;
    }
    if(print_ft){   // print frame table info
        frame_t * fte;
        printf("FT: ");
        for(int i = 0; i < frame_table.size(); i++){
            fte = &frame_table[i];
            if(fte->is_mapped){
                printf("%d:%d ", fte->rev_map.first->pid, fte->rev_map.second);
            } else {
                printf("* ");
            }
        }
        printf("\n");
        fte = nullptr;
    }
    if(print_procstats){    // print stats
        Process* proc;
        pstat_t* pstats;
        for(int i = 0; i < processes.size(); i++){
            proc = processes[i];
            pstats = proc->stats;
            printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                proc->pid,
                pstats->unmaps, pstats->maps, pstats->ins, pstats->outs,
                pstats->fins, pstats->fouts, pstats->zeros,
                pstats->segv, pstats->segprot);
        }
        printf("TOTALCOST %lu %lu %lu %llu\n", inst_count, ctx_switches, process_exits, cost);
        proc = nullptr;
        pstats = nullptr;
    }
}

void parse_input(string input_file){
    
    // read and parse input file
    ifstream in;
    in.open(input_file.c_str());
    if(!in){
        printf("Error: Can't open the file named %s\n", input_file.c_str());
        exit(1);
    } 

    string line;
    int num_processes = 0;
    
    // read number of processes
    line = get_next_valid_line(in);
    num_processes = stoi(line);

    for(int i = 0; i < num_processes; i++){
        line = get_next_valid_line(in);
        int num_vma = stoi(line);
        Process *proc = new Process(i, num_vma);
        for(int j = 0; j < num_vma; j++){
            in >> proc->vma_specs[j].start_vpage >> proc->vma_specs[j].end_vpage >> proc->vma_specs[j].write_protected >> proc->vma_specs[j].file_mapped;
        }
        in.ignore(numeric_limits<streamsize>::max(), '\n');
        processes.push_back(proc);
        proc = nullptr;
    }

    if(testing){
        printf("num of processes: %d\n", num_processes);
        for(int i = 0; i < num_processes; i++){
            printf("\t%d : num of vmas: %d\n", i, processes[i]->num_vma);
            for(int j = 0; j < processes[i]->num_vma; j++){
                printf("\t\t%d : %d %d %d %d\n", j, processes[i]->vma_specs[j].start_vpage, processes[i]->vma_specs[j].end_vpage, processes[i]->vma_specs[j].write_protected, processes[i]->vma_specs[j].file_mapped);
            }
        }
    }

    // parse instructions
    char c;
    pte_t *pte;
    while(in >> c){
        if(c == '#'){
            in.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        int procid, vpage;
        switch(c){
            case 'c':   // context switch
                in >> procid;
                current_process = processes[procid];
                if(print_output) printf("%lu: ==> %c %d\n", inst_count, c, procid);
                ctx_switches = ctx_switches + 1;
                cost = cost + execution_time[CONTEXT_SWITCH];
                break;
            case 'r':   // read
                in >> vpage;
                pte = &current_process->page_table[vpage];
                if(print_output) printf("%lu: ==> %c %d\n", inst_count, c, vpage);
                if(!pte->PRESENT){   // pagefault
                    pagefault_handler(pte, vpage);
                }
                if(pte->VMA_ASSIGNED){
                    pte->REFERENCED = 1;
                    cost = cost + execution_time[READ_WRITE];
                }
                break;
            case 'w':   // write
                in >> vpage;
                pte = &current_process->page_table[vpage];
                if(print_output) printf("%lu: ==> %c %d\n", inst_count, c, vpage);
                if(!pte->PRESENT){   // pagefault
                    pagefault_handler(pte, vpage);
                }
                if(pte->VMA_ASSIGNED){
                    if(pte->WRITE_PROTECT){
                        if(print_output) printf(" SEGPROT\n");
                        pte->REFERENCED = 1;
                        current_process->stats->segprot += 1;
                        cost = cost + execution_time[READ_WRITE];
                        cost = cost + execution_time[SEGPROT];
                    } else {
                        pte->REFERENCED = 1;
                        pte->MODIFIED = 1;
                        cost = cost + execution_time[READ_WRITE];
                    }
                }
                break;
            case 'e':
                in >> procid;
                if(print_output) printf("%lu: ==> %c %d\n", inst_count, c, procid);
                
                printf("EXIT current process %d\n", current_process->pid);
                process_exits = process_exits + 1;
                cost = cost + execution_time[PROCESS_EXIT];
                
                // loop through all pte of current process and unmap all valid 
                for(int i = 0; i < NUM_PTE; i++){
                    pte = &current_process->page_table[i];
                    
                    if(pte->PRESENT){

                        if(print_output) printf(" UNMAP %d:%d\n", current_process->pid, i);
                        cost = cost + execution_time[MAP_UNMAP];
                        current_process->stats->unmaps += 1;
                        frame_t* fte = &frame_table[pte->INDEX];
                        fte->is_mapped = false;
                        fte->rev_map = make_pair(nullptr, -1);

                        if(pte->MODIFIED && pte->FILE_MAPPED){
                            if(print_output) printf(" FOUT\n");
                            cost = cost + execution_time[FILEIN_OUT];
                            current_process->stats->fouts += 1;
                            pte->MODIFIED = 0;
                        }
                        
                        free_pool.push_back(fte);
                        fte = nullptr;

                        pte->PRESENT = 0;
                    }
                    pte->MODIFIED = 0;
                    pte->REFERENCED = 0;
                    pte->PAGEDOUT = 0;
                }
                break;
            default: break;
        }
        in.ignore(numeric_limits<streamsize>::max(), '\n');
        inst_count = inst_count + 1;
    }
    pte = nullptr;
    print_stats();
    in.close();
}

int main(int argc, char *argv[]){

    // parse the arguments
    string input_file, rand_file;
    parse_args(argc, argv, input_file, rand_file);

    // initialize frame_table and free_pool
    frame_table.resize(num_frames);
    for(int i = 0; i < num_frames; i++){
        frame_table[i].frame_num = i;
        frame_table[i].is_mapped = false;
        frame_table[i].rev_map = make_pair(nullptr, -1);
        free_pool.push_back(&frame_table[i]);
    }

    parse_random(rand_file);
    parse_input(input_file);

    return 0;
}