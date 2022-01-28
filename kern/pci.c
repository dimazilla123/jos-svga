/* -*- Mode: C; c-basic-offset: 3 -*-
 *
 * pci.c - Simple PCI configuration interface. This implementation
 *         only supports type 1 accesses to configuration space,
 *         and it ignores the PCI BIOS.
 *
 * This file is part of Metalkit, a simple collection of modules for
 * writing software that runs on the bare metal. Get the latest code
 * at http://svn.navi.cx/misc/trunk/metalkit/
 *
 * Copyright (c) 2008-2009 Micah Dowty
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <inc/pci.h>
#include <inc/x86.h>

/*
 * There can be up to 256 PCI busses, but it takes a noticeable
 * amount of time to scan that whole space. Limit the number of
 * supported busses to something more reasonable...
 */
#define PCI_MAX_BUSSES          0x20

#define PCI_REG_CONFIG_ADDRESS  0xCF8
#define PCI_REG_CONFIG_DATA     0xCFC


/*
 * PCIConfigPackAddress --
 *
 *    Pack a 32-bit CONFIG_ADDRESS value.
 */

static uint32_t
PCIConfigPackAddress(const PCIAddress *addr, uint16_t offset)
{
   const uint32_t enableBit = 0x80000000UL;

   return (((uint32_t)addr->bus << 16) |
           ((uint32_t)addr->device << 11) |
           ((uint32_t)addr->function << 8) |
           offset | enableBit);
}


/*
 * PCI_ConfigRead32 --
 * PCI_ConfigRead16 --
 * PCI_ConfigRead8 --
 * PCI_ConfigWrite32 --
 * PCI_ConfigWrite16 --
 * PCI_ConfigWrite8 --
 *
 *    Access a device's PCI configuration space, using configuration
 *    mechanism #1. All new machines should use method #1, method #2
 *    is for legacy compatibility.
 *
 *    See http://www.osdev.org/wiki/PCI
 */

uint32_t
PCI_ConfigRead32(const PCIAddress *addr, uint16_t offset)
{
   outl(PCI_REG_CONFIG_ADDRESS, PCIConfigPackAddress(addr, offset));
   return inl(PCI_REG_CONFIG_DATA);
}

uint16_t
PCI_ConfigRead16(const PCIAddress *addr, uint16_t offset)
{
   outl(PCI_REG_CONFIG_ADDRESS, PCIConfigPackAddress(addr, offset));
   return inw(PCI_REG_CONFIG_DATA);
}

uint8_t
PCI_ConfigRead8(const PCIAddress *addr, uint16_t offset)
{
   outl(PCI_REG_CONFIG_ADDRESS, PCIConfigPackAddress(addr, offset));
   return inb(PCI_REG_CONFIG_DATA);
}

void
PCI_ConfigWrite32(const PCIAddress *addr, uint16_t offset, uint32_t data)
{
   outl(PCI_REG_CONFIG_ADDRESS, PCIConfigPackAddress(addr, offset));
   outl(PCI_REG_CONFIG_DATA, data);
}

void
PCI_ConfigWrite16(const PCIAddress *addr, uint16_t offset, uint16_t data)
{
   outl(PCI_REG_CONFIG_ADDRESS, PCIConfigPackAddress(addr, offset));
   outw(PCI_REG_CONFIG_DATA, data);
}

void
PCI_ConfigWrite8(const PCIAddress *addr, uint16_t offset, uint8_t data)
{
   outl(PCI_REG_CONFIG_ADDRESS, PCIConfigPackAddress(addr, offset));
   outb(PCI_REG_CONFIG_DATA, data);
}


/*
 * PCI_ScanBus --
 *
 *    Scan the PCI bus for devices. Before starting a scan,
 *    the caller should zero the PCIScanState structure.
 *    Every time this function is called, it returns the next
 *    device in sequence.
 *
 *    Returns true if a device was found, leaving that device's
 *    vendorId, productId, and address in 'state'.
 *
 *    Returns false if there are no more devices.
 */

bool
PCI_ScanBus(PCIScanState *state)
{
   PCIConfigSpace config;

   for (;;) {
      config.words[0] = PCI_ConfigRead32(&state->nextAddr, 0);

      state->addr = state->nextAddr;

      if (++state->nextAddr.function == 0x8) {
         state->nextAddr.function = 0;
         if (++state->nextAddr.device == 0x20) {
            state->nextAddr.device = 0;
            if (++state->nextAddr.bus == PCI_MAX_BUSSES) {
               return false;
            }
         }
      }

      if (config.words[0] != 0xFFFFFFFFUL) {
         state->vendorId = config.vendorId;
         state->deviceId = config.deviceId;
         return true;
      }
   }
}


/*
 * PCI_FindDevice --
 *
 *    Scan the PCI bus for a device with a specific vendor and device ID.
 *
 *    On success, returns true and puts the device address into 'addrOut'.
 *    If the device was not found, returns false.
 */

bool
PCI_FindDevice(uint16_t vendorId, uint16_t deviceId, PCIAddress *addrOut)
{
   PCIScanState busScan = {};

   while (PCI_ScanBus(&busScan)) {
      if (busScan.vendorId == vendorId && busScan.deviceId == deviceId) {
         *addrOut = busScan.addr;
         return true;
      }
   }

   return false;
}


/*
 * PCI_SetBAR --
 *
 *    Set one of a device's Base Address Registers to the provided value.
 */

void
PCI_SetBAR(const PCIAddress *addr, int index, uint32_t value)
{
   PCI_ConfigWrite32(addr, offsetof(PCIConfigSpace, BAR[index]), value);
}


/*
 * PCI_GetBARAddr --
 *
 *    Get the current address set in one of the device's Base Address Registers.
 *    We mask off the lower bits that are not part of the address.  IO bars are
 *    4 byte aligned so we mask lower 2 bits, and memory bars are 16-byte aligned
 *    so we mask the lower 4 bits.
 */

uint32_t
PCI_GetBARAddr(const PCIAddress *addr, int index)
{
   uint32_t bar = PCI_ConfigRead32(addr, offsetof(PCIConfigSpace, BAR[index]));
   uint32_t mask = (bar & PCI_CONF_BAR_IO) ? 0x3 : 0xf;

   return bar & ~mask;
}


/*
 * PCI_SetMemEnable --
 *
 *    Enable or disable a device's memory and IO space. This must be
 *    called to enable a device's resources after setting all
 *    applicable BARs. Also enables/disables bus mastering.
 */

void
PCI_SetMemEnable(const PCIAddress *addr, bool enable)
{
   uint16_t command = PCI_ConfigRead16(addr, offsetof(PCIConfigSpace, command));

   /* Mem space enable, IO space enable, bus mastering. */
   const uint16_t flags = 0x0007;

   if (enable) {
      command |= flags;
   } else {
      command &= ~flags;
   }

   PCI_ConfigWrite16(addr, offsetof(PCIConfigSpace, command), command);
}
