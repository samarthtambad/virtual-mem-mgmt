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

bool testing = true;
long rand_ofs = 0;
vector<long> randvals;
vector<pte_t> page_table;
// vector<process> processes;
vector<frame_t> frame_table;   // OS maintains this
deque<frame_t*> free_pool;   
Process *current_process;

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
        printf("inputs:\n");
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
                if(testing) printf("options: %s\n", options);
                break;
            case 'f':
                // int num_frames;
                num_frames = atoi(optarg);
                // frame_table.resize(num_frames);
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

void print_free_pool(){
    printf("Free Frames: \n");
    for(deque<frame_t*>::iterator iter = free_pool.begin(); iter != free_pool.end(); ++iter){
        printf("frame %d\n", (*iter)->frame_num);
    }
}

void pagefault_handler(pte_t *pte, int vpage){
    frame_t* newframe = get_frame();

    /*----------unmap old frame and save it if needed----*/
    if(newframe->is_mapped){    // unmap old frame
        printf("UNMAP %d:%d\n", newframe->rev_map.first->pid, newframe->rev_map.second);
        pte_t* old_pte = &newframe->rev_map.first->page_table[newframe->rev_map.second];
        if(old_pte->REFERENCED){

        }
        if(old_pte->MODIFIED){  // save changes to disk OUT/FOUT
            printf("OUT\n");
        }
        newframe->is_mapped = false;
        newframe->rev_map = make_pair(nullptr, -1);
    }
    /*----------------------------------------------------*/


    /*------zero out the virtual page------*/ 
    pte->PRESENT = 0;
    pte->MODIFIED = 0;
    pte->REFERENCED = 0;
    // pte->INDEX = -1;
    // pte->PAGEDOUT = 0;   // to reset or not?
    printf("ZERO\n");
    /*-------------------------------------*/


    /*-------map newframe to vpage of current_process-----*/
    pte->PRESENT = 1;
    pte->INDEX = newframe->frame_num;
    newframe->is_mapped = true;
    newframe->rev_map = make_pair(current_process, vpage);
    printf("MAP %d\n", pte->INDEX);
    /*----------------------------------------------------*/
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
    vector<Process*> processes;

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
    
    // print_free_pool();

    // parse instructions
    char c;
    pte_t *pte;
    unsigned long long counter = 0;
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
                printf("%llu: ==> %c %d\n", counter, c, procid);
                break;
            case 'r':   // read
                in >> vpage;
                pte = &current_process->page_table[vpage];
                printf("%llu: ==> %c %d\n", counter, c, vpage);
                if(!pte->PRESENT){   // pagefault
                    pagefault_handler(pte, vpage);
                }
                pte->REFERENCED = 1;
                break;
            case 'w':   // write
                in >> vpage;
                pte = &current_process->page_table[vpage];
                printf("%llu: ==> %c %d\n", counter, c, vpage);
                if(!pte->PRESENT){   // pagefault
                    pagefault_handler(pte, vpage);
                }
                pte->MODIFIED = 1;
                break;
            case 'e':
                in >> procid;
                printf("%llu: ==> %c %d\n", counter, c, procid);
                break;
            default: break;
        }
        in.ignore(numeric_limits<streamsize>::max(), '\n');
        counter = counter + 1;
    }
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