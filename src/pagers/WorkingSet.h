#include "Pager.h"

class WorkingSet : public Pager
{
    private:
        int hand;
        int TAU;
    public:
        WorkingSet();
        ~WorkingSet();
        frame_t* select_victim_frame();
};

WorkingSet::WorkingSet(){
    hand = 0;
    TAU = 49;
}

WorkingSet::~WorkingSet(){

}

frame_t* WorkingSet::select_victim_frame(){
    frame_t* ref_fte;
    pte_t* ref_pte;
    frame_t* victim_frame = nullptr;
    int start_scan = 0, end_scan = 0, count = 0;
    int current_virtual_time = inst_count;
    int smallest_time = inst_count;
    frame_t* smallest_time_frame = &frame_table[hand];
    
    start_scan = hand;
    end_scan = (start_scan == 0) ? (frame_table.size() - 1) : (start_scan - 1);

    if(opt_a) printf("ASELECT %d-%d | ", start_scan, end_scan);

    while(count < frame_table.size()){
        ref_fte = &frame_table[hand];
        ref_pte = &ref_fte->rev_map.first->page_table[ref_fte->rev_map.second];
        
        ref_fte->age = current_virtual_time - ref_fte->timelastuse;
        if(opt_a) printf("%d(%d %d:%d %d) ", ref_fte->frame_num, ref_pte->REFERENCED, ref_fte->rev_map.first->pid, ref_fte->rev_map.second, ref_fte->timelastuse);

        if(ref_pte->REFERENCED){
            ref_fte->timelastuse = current_virtual_time;
            ref_pte->REFERENCED = 0;
        } else {
            if(ref_fte->age > TAU){
                if(opt_a) printf("STOP(%d) ", count + 1);
                victim_frame = ref_fte;
                break;
            } else {    // remember smallest time
                if(ref_fte->timelastuse < smallest_time){
                    smallest_time = ref_fte->timelastuse;
                    smallest_time_frame = ref_fte;
                }
            }
        }
        if(hand >= frame_table.size() - 1) hand = 0;
        else hand++;
        count++;
    }

    if(victim_frame == nullptr){
        victim_frame = smallest_time_frame;
        hand = smallest_time_frame->frame_num;
    }

    if(opt_a) printf("| %d\n", victim_frame->frame_num);
    
    if(hand == frame_table.size() - 1) hand = 0;
    else hand++;

    return victim_frame;
}