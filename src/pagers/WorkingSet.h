#include "Pager.h"

class WorkingSet : public Pager
{
private:
    int hand;
public:
    WorkingSet();
    ~WorkingSet();
    frame_t* select_victim_frame();
};

WorkingSet::WorkingSet(){

}

WorkingSet::~WorkingSet(){

}

frame_t* WorkingSet::select_victim_frame(){
    frame_t* frame = &frame_table[hand];
    return frame;
}