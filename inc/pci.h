/* -*- Mode: C; c-basic-offset: 3 -*-
 *
 * pci.h - Simple PCI configuration interface. This implementation
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

#ifndef __PCI_H__
#define __PCI_H__

#include <inc/types.h>

typedef union PCIConfigSpace {
   uint32_t words[16];
   struct {
      uint16_t vendorId;
      uint16_t deviceId;
      uint16_t command;
      uint16_t status;
      uint16_t revisionId;
      uint8_t  subclass;
      uint8_t  classCode;
      uint8_t  cacheLineSize;
      uint8_t  latTimer;
      uint8_t  headerType;
      uint8_t  BIST;
      uint32_t BAR[6];
      uint32_t cardbusCIS;
      uint16_t subsysVendorId;
      uint16_t subsysId;
      uint32_t expansionRomAddr;
      uint32_t reserved0;
      uint32_t reserved1;
      uint8_t  intrLine;
      uint8_t  intrPin;
      uint8_t  minGrant;
      uint8_t  maxLatency;
   };
} __attribute__ ((__packed__)) PCIConfigSpace;

typedef struct PCIAddress {
   uint8_t bus, device, function;
} PCIAddress;

typedef struct PCIScanState {
   uint16_t     vendorId;
   uint16_t     deviceId;
   PCIAddress nextAddr;
   PCIAddress addr;
} PCIScanState;

// BAR bits
#define PCI_CONF_BAR_IO          0x01
#define PCI_CONF_BAR_64BIT       0x04
#define PCI_CONF_BAR_PREFETCH    0x08

uint32_t PCI_ConfigRead32(const PCIAddress *addr, uint16_t offset);
uint16_t PCI_ConfigRead16(const PCIAddress *addr, uint16_t offset);
uint8_t PCI_ConfigRead8(const PCIAddress *addr, uint16_t offset);
void PCI_ConfigWrite32(const PCIAddress *addr, uint16_t offset, uint32_t data);
void PCI_ConfigWrite16(const PCIAddress *addr, uint16_t offset, uint16_t data);
void PCI_ConfigWrite8(const PCIAddress *addr, uint16_t offset, uint8_t data);

bool PCI_ScanBus(PCIScanState *state);
bool PCI_FindDevice(uint16_t vendorId, uint16_t deviceId, PCIAddress *addrOut);
void PCI_SetBAR(const PCIAddress *addr, int index, uint32_t value);
uint32_t PCI_GetBARAddr(const PCIAddress *addr, int index);
void PCI_SetMemEnable(const PCIAddress *addr, bool enable);

#endif /* __PCI_H__ */
