/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _UAPI_AMD_GPU_PCI_H
#define _UAPI_AMD_GPU_PCI_H

#include <linux/types.h>
// #include <linux/pci.h>

#include "amd-pci-ids.h"

struct amd_pci_entry {
    struct amd_pci_asic    asic;
    struct pci_dev          *pci;
};

uint32_t find_pci_devices (
    struct amd_pci_entry *entries,
    uint32_t len
);

#endif