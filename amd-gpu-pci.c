/* SPDX-License-Identifier: GPL-2.0 */
#include "include/debug.h"
#include "amd-gpu-pci.h"

static enum amd_asic_version get_dce_version (
    enum amd_asic_type asic
){
    switch (asic)
    {
    case CHIP_POLARIS10:
    case CHIP_POLARIS11:
    case CHIP_POLARIS12:
        return DCE_VERSION_11_2;

    case CHIP_VEGA10:
    case CHIP_VEGA12:
        return DCE_VERSION_12_0;

    case CHIP_VEGA20:
        return DCE_VERSION_12_1;

    case CHIP_NAVI10:
    case CHIP_NAVI14:
        return DCN_VERSION_2_0;
    case CHIP_NAVI21:
        return DCN_VERSION_3_0;
    // case CHIP_NAVI22: // vangogh (steam deck)
    //     return DCN_VERSION_3_01; // correct?
    case CHIP_NAVI23:
        return DCN_VERSION_3_02;
    case CHIP_NAVI24:
        return DCN_VERSION_3_03;

    case CHIP_NAVI31:
    case CHIP_NAVI32:
        return DCN_VERSION_3_2;

    case CHIP_NAVI33:
        return DCN_VERSION_3_21;

    // Avoid compiler warnings
    case CHIP_NAVI12:
    case CHIP_NAVI22:
    case CHIP_LAST:
        break;
    }

    return DCE_VERSION_UNKNOWN;
}

uint32_t find_pci_devices (
    struct amd_pci_entry *entries,
    uint32_t len
){
    struct pci_dev *pci_dev = NULL;
    const struct pci_device_id *match;
    uint32_t count = 0;

    while (NULL != (pci_dev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, pci_dev))) {
        match = pci_match_id(pciidlist, pci_dev);
        if (match && count < len) {
            entries[count].pci = pci_dev;
            entries[count].asic.type = match->driver_data;

            // TODO - check valid
            entries[count].asic.version = get_dce_version(match->driver_data);
            count++;
            
            I2C_DBG("Detected AURA capable GPU %x:%x",
                pci_dev->subsystem_vendor,
                pci_dev->subsystem_device);
        }
    }

    return count;
}
