#include "Pager.h"

class Clock : public Pager
{
    private:
        int hand;
    public:
        Clock();
        ~Clock();
        frame_t* select_victim_frame();
};

Clock::Clock(){
    hand = 0;
}

Clock::~Clock(){

}

frame_t* Clock::select_victim_frame(){
    frame_t* frame;
    frame = &frame_table[hand];
    pte_t* ref_pte = &frame->rev_map.first->page_table[frame->rev_map.second];
    while(ref_pte->REFERENCED){
        ref_pte->REFERENCED = 0;
        if(hand == frame_table.size() - 1) hand = 0;
        else hand++;
        
        frame = &frame_table[hand];
        ref_pte = &frame->rev_map.first->page_table[frame->rev_map.second];
    }
    
    if(hand == frame_table.size() - 1) hand = 0;
    else hand++;
    return frame;
}