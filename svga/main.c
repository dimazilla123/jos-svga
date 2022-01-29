#include <inc/stdio.h>
#include <inc/string.h>

#include "svga.h"

int umain()
{
    if (!SVGA_Init())
        return 1;

    SVGA_SetVideoMode(800, 480, 32);

    SVGA_Dump();

    while (1)
    {

    }

    return 0;
}