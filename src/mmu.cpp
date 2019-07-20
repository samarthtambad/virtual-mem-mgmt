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
deque<frame_t> frame_table;   // OS maintains this
deque<frame_t> free_pool;   
process current_process;
process *processes;

Pager *pager = (Pager *) new FIFO(&frame_table);


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

void pagefault_handler(){

}

frame_t* allocate_frame_from_free_list(){
    return nullptr;
}

frame_t* get_frame() {
    frame_t *frame = allocate_frame_from_free_list();
    if (frame == NULL) frame = pager->select_victim_frame();
    return frame;
}

/*
while (get_next_instruction(&operation, &vpage)) {
    // handle special case of “c” and “e” instruction
    // now the real instructions for read and write
    pte_t *pte = &current_process.page_table[vpage];// in reality this is done by hardware
    if ( ! pte->present) {
        // this in reality generates the page fault exception and now you execute
        frame_t *newframe = get_frame();
        //-> figure out if/what to do with old frame if it was mapped
        //   see general outline in MM-slides under Lab3 header
        //   see whether and how to bring in the content of the access page.
    }
    // check write protection
    // simulate instruction execution by hardware by updating the R/M PTE bits
    update_pte(read/modify) bits based on operations.
}
*/

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
    processes = new process[num_processes];

    // read num_processes no. of process specs
    for(int i = 0; i < num_processes; i++){
        line = get_next_valid_line(in);
        processes[i].num_vma = stoi(line);
        processes[i].vma_specs = new vmaspec[processes[i].num_vma];
        processes[i].page_table = new pte_t[NUM_PTE];
        for(int j = 0; j < processes[i].num_vma; j++){
            in >> processes[i].vma_specs[j].start_vpage >> processes[i].vma_specs[j].end_vpage >> processes[i].vma_specs[j].write_protected >> processes[i].vma_specs[j].file_mapped;
        }
        in.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    if(testing){
        printf("num of processes: %d\n", num_processes);
        for(int i = 0; i < num_processes; i++){
            printf("\t%d : num of vmas: %d\n", i, processes[i].num_vma);
            for(int j = 0; j < processes[i].num_vma; j++){
                printf("\t\t%d : %d %d %d %d\n", j, processes[i].vma_specs[j].start_vpage, processes[i].vma_specs[j].end_vpage, processes[i].vma_specs[j].write_protected, processes[i].vma_specs[j].file_mapped);
            }
        }   
    }

    // parse instructions
    char c;
    unsigned long long counter = 0;
    while(in >> c){
        int procid, vpage;
        switch(c){
            case 'c':   // context switch
                in >> procid;
                current_process = processes[procid];
                printf("%llu: ==> %c %d\n", counter, c, procid);
                // if(testing) printf("c %d\n", procid);
                break;
            case 'r':   // read
                in >> vpage;
                printf("%llu: ==> %c %d\n", counter, c, vpage);
                // if present, print zero else pagefault
                
                // if(testing) printf("r %d\n", vpage);
                break;
            case 'w':
                in >> vpage;
                printf("%llu: ==> %c %d\n", counter, c, vpage);
                // if(testing) printf("w %d\n", vpage);
                break;
            case 'e':
                in >> procid;
                printf("%llu: ==> %c %d\n", counter, c, procid);
                // if(testing) printf("e %d\n", procid);
                break;
            default: break;
        }
        in.ignore(numeric_limits<streamsize>::max(), '\n');
        printf("count %c: %llu\n",c, counter);
        counter = counter + 1;
    }
    in.close();
}

int main(int argc, char *argv[]){

    // parse the arguments
    string input_file, rand_file;
    parse_args(argc, argv, input_file, rand_file);

    
    parse_random(rand_file);
    parse_input(input_file);

    // for(int i = 0; i < 105; i++){
    //     printf("%d\n", myrandom(100));
    // }

    return 0;
}