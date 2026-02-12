

#ifndef __DUMPHEX_H__
#define __DUMPHEX_H__

#include <cstdint>
#include <iostream>

void hexDump(std::ostream &outs, const uint8_t *b, uint32_t l, uint32_t p = -1);
struct HexDump{
    const uint8_t * v;
    uint32_t l;     
    uint32_t p = -1;
    
    HexDump(uint8_t * v, uint32_t l){
        this -> v = v;
        this-> l = l;
    }
    HexDump(uint8_t * v, int32_t l){
        this -> v = v;
        this-> l = (uint32_t) l;
    }
};
                    
std::ostream &operator << (std::ostream&s,HexDump h);

#endif
