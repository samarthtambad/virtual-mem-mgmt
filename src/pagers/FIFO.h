#include "Pager.h"

class FIFO : public Pager
{
private:
    std::deque<frame_t> frame_table;
public:
    FIFO(std::deque<frame_t>*);
    ~FIFO();
    frame_t* select_victim_frame();
};

FIFO::FIFO(std::deque<frame_t>* ftbl){
    this->frame_table = *ftbl;
}

FIFO::~FIFO(){

}

frame_t* FIFO::select_victim_frame(){
    frame_t f = this->frame_table.front();
    this->frame_table.pop_front();
    return &f;
}