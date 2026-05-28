#ifndef FISCHER_HEUN_INTERNAL_H
#define FISCHER_HEUN_INTERNAL_H
 
typedef struct {
    int b;
    int nb;
    int n;
    int st_levels;
    int num_types;
 
    int            *block_min;
    int            *cartoon;
    int           **st;
    unsigned char **in_block;
} FH_State;
 
#endif