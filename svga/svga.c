#include <inc/stdio.h>

#include <inc/pci.h>

#include <inc/x86.h>
#include <inc/lib.h>
#include <inc/string.h>

#include <svga/vmware/svga_reg.h>

#include "svga.h"

SVGADriver gSVGA;

static uint32_t
SVGA_ReadReg(uint32_t index)
{
    outl(gSVGA.ioBase + SVGA_INDEX_PORT, index);
    return inl(gSVGA.ioBase + SVGA_VALUE_PORT);
}

static void
SVGA_WriteReg(uint32_t index, uint32_t value)
{
    outl(gSVGA.ioBase + SVGA_INDEX_PORT, index);
    outl(gSVGA.ioBase + SVGA_VALUE_PORT, value);
}

void SVGA_Dump()
{
    cprintf("SVGA[%p] {\n"
            "    ioBase = %u\n"
            "    fbSize = %u\n"
            "    fbMem  = %p\n"
            "    fifoSize = %u\n"
            "    fifoMem  = %p\n"
            "}\n",
                &gSVGA,
                gSVGA.ioBase,
                gSVGA.fbSize,
                gSVGA.fbMem,
                gSVGA.fifoSize,
                gSVGA.fifoMem);
}

bool SVGA_Init()
{
    cprintf("SVGA initialisation\n");
    memset(&gSVGA, 0, sizeof(gSVGA));

    if (!PCI_FindDevice(PCI_VENDOR_ID_VMWARE, PCI_DEVICE_ID_VMWARE_SVGA2, &gSVGA.pciAddr))
    {
        cprintf("SVGA is not found\n");
        return false;
    }

    cprintf("SVGA found: bus = %d, device = %d, function = %d\n", gSVGA.pciAddr.bus, gSVGA.pciAddr.device, gSVGA.pciAddr.function);

    gSVGA.ioBase = PCI_GetBARAddr(&gSVGA.pciAddr, 0);
    gSVGA.fbSize = SVGA_ReadReg(SVGA_REG_FB_SIZE);
    gSVGA.fifoSize = SVGA_ReadReg(SVGA_REG_MEM_SIZE);

    gSVGA.fbMem = (uint8_t*)(uint64_t)PCI_GetBARAddr(&gSVGA.pciAddr, 1);
    gSVGA.fifoMem = (uint32_t*)(uint64_t)PCI_GetBARAddr(&gSVGA.pciAddr, 2);
    SVGA_Dump();

    int r = sys_alloc_region(0, gSVGA.fbMem, gSVGA.fbSize, PROT_RW);

    if (r < 0)
    {
        cprintf("SVGA_Init: Cannot map framebuffer\n");
        return false;
    }

    r = sys_alloc_region(0, gSVGA.fifoMem, gSVGA.fifoSize, PROT_RW);
    if (r < 0)
    {
        cprintf("SVGA_Init: Cannot map fifo\n");
        return false;
    }

    PCI_SetMemEnable(&gSVGA.pciAddr, true);

    SVGA_Enable();
    SVGA_Dump();

    return true;
}

void SVGA_Enable()
{
    gSVGA.fifoMem[SVGA_FIFO_MIN] = SVGA_FIFO_NUM_REGS * sizeof(uint32_t);
    gSVGA.fifoMem[SVGA_FIFO_MAX] = gSVGA.fifoSize;
    gSVGA.fifoMem[SVGA_FIFO_NEXT_CMD] = gSVGA.fifoMem[SVGA_FIFO_MIN];
    gSVGA.fifoMem[SVGA_FIFO_STOP] = gSVGA.fifoMem[SVGA_FIFO_MIN];

    SVGA_WriteReg(SVGA_REG_ENABLE, true);
    SVGA_WriteReg(SVGA_REG_CONFIG_DONE, true);
}
void SVGA_Disable()
{
    SVGA_WriteReg(SVGA_REG_ENABLE, false);
}

bool SVGA_SetVideoMode(uint32_t width, uint32_t height, uint32_t bpp)
{
    SVGA_WriteReg(SVGA_REG_DISPLAY_WIDTH, width);
    SVGA_WriteReg(SVGA_REG_DISPLAY_HEIGHT, height);
    SVGA_WriteReg(SVGA_REG_BITS_PER_PIXEL, bpp);
    SVGA_WriteReg(SVGA_REG_ENABLE, true);

    gSVGA.width = width;
    gSVGA.height = height;
    gSVGA.bpp = bpp;

    return true;
}