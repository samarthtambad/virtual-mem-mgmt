#include "Pager.h"

class NRU : public Pager
{
private:
    int hand;
public:
    NRU();
    ~NRU();
    frame_t* select_victim_frame();
};

NRU::NRU(){

}

NRU::~NRU(){

}

frame_t* NRU::select_victim_frame(){
    frame_t* frame = &frame_table[hand];
    return frame;
}