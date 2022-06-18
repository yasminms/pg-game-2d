#ifndef DiamondView_h
#define DiamondView_h

#include "TilemapView.h"
#include <iostream>
using namespace std;

class DiamondView : public TilemapView
{
public:
    void computeDrawPosition(const int height, const int col, const int row, const float tw, const float th, float &targetx, float &targety) const
    {
        // targetx = col * tw + row * tw / 2;
        // targety = row * th / 2;
        targetx = col * (tw / 2) + row * (tw / 2);
        targety = col * (th / 2) - row * (th / 2) + height/2;
    }

    void computeMouseMap(int &col, int &row, const float tw, const float th, const float mx, const float my) const
    {
        float tw2 = tw / 2.0f;
        float th2 = th / 2.0f;

        row = my / th2;
        col = (mx - row * tw2) / tw;
    }

    void computeTileWalking(int &col, int &row, const int direction) const
    {
        switch (direction)
        {
        case DIRECTION_NORTH:
            col--;
            row += 2;
            break;
        case DIRECTION_EAST:
            col++;
            break;
        case DIRECTION_SOUTH:
            col++;
            row -= 2;
            break;
        case DIRECTION_WEST:
            col--;
            break;
        case DIRECTION_NORTHEAST:
            row++;
            break;
        case DIRECTION_SOUTHEAST:
            col++;
            row--;
            break;
        case DIRECTION_SOUTHWEST:
            row--;
            break;
        case DIRECTION_NORTHWEST:
            col--;
            row++;
            break;
        }
    }
};

#endif