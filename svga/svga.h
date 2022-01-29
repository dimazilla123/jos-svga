#ifndef SVGA_SVGA_H
#define SVGA_SVGA_H

#include <svga/vmware/svga_reg.h>
#include <inc/pci.h>

typedef struct
{
    PCIAddress pciAddr;
    uint32_t ioBase;

    uint8_t *fbMem;
    uint32_t fbSize;

    uint32_t *fifoMem;
    uint32_t  fifoSize;

    uint32_t width;
    uint32_t height;

    uint32_t bpp;
} SVGADriver;

extern SVGADriver gSVGA;

void SVGA_Dump();

bool SVGA_Init();
void SVGA_Enable();
void SVGA_Disable();
bool SVGA_SetVideoMode(uint32_t width, uint32_t height, uint32_t bpp);

#endif