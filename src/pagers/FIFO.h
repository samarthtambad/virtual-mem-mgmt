#include "Pager.h"

class FIFO : public Pager
{
    private:
        int hand;
    public:
        FIFO();
        ~FIFO();
        frame_t* select_victim_frame();
};

FIFO::FIFO(){
    hand = 0;
}

FIFO::~FIFO(){

}

frame_t* FIFO::select_victim_frame(){
    frame_t* frame = &frame_table[hand];
    if(hand == frame_table.size() - 1) hand = 0;
    else hand++;
    return frame;
}