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

}

Aging::~Aging(){

}

frame_t* Aging::select_victim_frame(){
    frame_t* frame = &frame_table[hand];
    return frame;
}