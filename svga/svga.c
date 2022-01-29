#include <inc/stdio.h>

#include <inc/pci.h>
#include <svga/pci.c>

#include <svga/vmware/svga_reg.h>

int umain()
{
    cprintf("SVGA loaded\n");

    PCIAddress addr = {};
    if (!PCI_FindDevice(PCI_VENDOR_ID_VMWARE, PCI_DEVICE_ID_VMWARE_SVGA2, &addr))
    {
        cprintf("SVGA is not found\n");
        return 1;
    }

    cprintf("SVGA found: bus = %d, device = %d, function = %d\n", addr.bus, addr.device, addr.function);

    return 0;
}