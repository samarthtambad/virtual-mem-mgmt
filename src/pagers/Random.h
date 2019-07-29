#include "Pager.h"

class Random : public Pager
{
    public:
        Random();
        ~Random();
        frame_t* select_victim_frame();
};

Random::Random(){

}

Random::~Random(){

}

frame_t* Random::select_victim_frame(){
    frame_t* frame = &frame_table[myrandom(frame_table.size())];
    return frame;
}