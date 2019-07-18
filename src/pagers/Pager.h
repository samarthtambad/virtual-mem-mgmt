#include "../globals.h"

#ifndef PAGER_H
#define PAGER_H

class Pager
{
public:
    Pager();
    ~Pager();
    virtual frame_t* select_victim_frame() = 0;
};

Pager::Pager()
{
}

Pager::~Pager()
{
}

#endif