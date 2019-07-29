#include "Pager.h"

class NRU : public Pager
{
    private:
        int hand;
        int count;
        frame_t** frame_class;
        unsigned long prev_inst_count;
    public:
        NRU();
        ~NRU();
        frame_t* select_victim_frame();
};

NRU::NRU(){
    hand = 0;
    count = 0;
    prev_inst_count = -1;   // to account for 0th instruction
    frame_class = new frame_t*[4];
}

NRU::~NRU(){

}

frame_t* NRU::select_victim_frame(){
    frame_t* ref_fte;
    pte_t* ref_pte;
    int class_idx = 0;
    int num_frames = frame_table.size();

    count = 0;
    for(int i = 0; i <= 3; i++) frame_class[i] = nullptr;

    // find first frame of all classes
    while(true){
        if(count >= num_frames) break;
        
        ref_fte = &frame_table[hand];
        ref_pte = &ref_fte->rev_map.first->page_table[ref_fte->rev_map.second];
        class_idx = (2 * ref_pte->REFERENCED) + ref_pte->MODIFIED;
        if(frame_class[class_idx] == nullptr) frame_class[class_idx] = ref_fte;

        if(class_idx == 0) break;
        
        if(hand == num_frames - 1) hand = 0;
        else hand++;
        count++;
    }

    // reset R bit for all valid PTE every 50 instructions
    if((inst_count - prev_inst_count) >= 50){
        for(int i = 0; i < num_frames; i++){
            ref_fte = &frame_table[i];
            if(ref_fte->is_mapped){
                ref_pte = &ref_fte->rev_map.first->page_table[ref_fte->rev_map.second];
                ref_pte->REFERENCED = 0;
            }
        }
        prev_inst_count = inst_count;
    }

    // choose the lowest numbered class frame
    for(int j = 0; j <= 3; j++){
        if(frame_class[j] != nullptr){
            ref_fte = frame_class[j];
            hand = ref_fte->frame_num;
            if(hand >= num_frames - 1) hand = 0;
            else hand++;
            break;
        }
    }

    return ref_fte;
}