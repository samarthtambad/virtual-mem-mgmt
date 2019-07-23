#include "Pager.h"

class FIFO : public Pager
{
    private:
        std::deque<frame_t>::iterator hand;
    public:
        FIFO();
        ~FIFO();
        frame_t* select_victim_frame();
};

FIFO::FIFO(){
    hand = frame_table.begin();
}

FIFO::~FIFO(){

}

frame_t* FIFO::select_victim_frame(){
    frame_t* frame = &(*hand);
    if(hand == frame_table.end()) hand = frame_table.begin();
    else hand++;
    return frame;
}