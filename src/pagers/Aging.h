#include "Pager.h"

class Aging : public Pager
{
private:
    int hand;
public:
    Aging();
    ~Aging();
    frame_t* select_victim_frame();
};

Aging::Aging(){
    hand = 0;
}

Aging::~Aging(){

}

frame_t* Aging::select_victim_frame(){
    frame_t* ref_fte;
    frame_t* min_age_fte;
    pte_t* ref_pte;
    int start_scan = 0, end_scan = 0, count = 0;
    unsigned int min_age = 0xFFFFFFFF;

    ref_fte = &frame_table[hand];
    ref_pte = &ref_fte->rev_map.first->page_table[ref_fte->rev_map.second];
    start_scan = hand;
    end_scan = (start_scan == 0) ? (frame_table.size() - 1) : (start_scan - 1);

    if(opt_a) printf("ASELECT %d-%d | ", start_scan, end_scan);
    
    // aging for all valid pte
    while(count < frame_table.size()){
        ref_fte = &frame_table[hand];
        ref_pte = &ref_fte->rev_map.first->page_table[ref_fte->rev_map.second];
        ref_fte->age = ref_fte->age >> 1;  // shifts age to the right
        if(ref_pte->REFERENCED) ref_fte->age = (ref_fte->age | 0x80000000);
        ref_pte->REFERENCED = 0;

        if(ref_fte->age < min_age){
            min_age = ref_fte->age;
            min_age_fte = ref_fte;
        }

        if(opt_a) printf("%d:%x ", hand, ref_fte->age);

        if(hand >= frame_table.size() - 1) hand = 0;
        else hand++;
        count++;
    }

    if(opt_a) printf("| %d\n", min_age_fte->frame_num);

    hand = min_age_fte->frame_num;
    if(hand >= frame_table.size() - 1) hand = 0;
    else hand++;

    return min_age_fte;
}