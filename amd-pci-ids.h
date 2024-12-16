/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _UAPI_AMD_PCI_IDS_H
#define _UAPI_AMD_PCI_IDS_H

#include <linux/pci.h>

enum amd_device_ids {
    AMD_GPU_VEN         = 0x1002
};
enum amd_vendor_ids {
    AMD_POLARIS_DEV     = 0x67DF,
    AMD_POLARIS11       = 0x67FF,
    AMD_POLARIS20XL_DEV	= 0x6FDF,

    AMD_VEGA10_DEV      = 0x687F,

    AMD_NAVI10_DEV      = 0x731F,
    AMD_NAVI14_DEV      = 0x7340,
    AMD_NAVI21_DEV1     = 0x73BF,
    AMD_NAVI21_DEV2     = 0x73AF,
    AMD_NAVI21_DEV3     = 0x73A5,
    AMD_NAVI22_DEV      = 0x73DF,
    AMD_NAVI23_DEV      = 0x73FF,
    AMD_NAVI23_DEV1     = 0x73EF,
    AMD_NAVI31_DEV      = 0x744C,
    AMD_NAVI32_DEV      = 0x747E
};

enum amd_asic_type {
   CHIP_POLARIS10,      /* Radeon 470, 480, 570, 580, 590 */
   CHIP_POLARIS11,      /* Radeon 460, 560 */
   CHIP_POLARIS12,      /* Radeon 540, 550 */

   CHIP_VEGA10,         /* Vega 56, 64 */
   CHIP_VEGA12,
   CHIP_VEGA20,         /* Radeon VII, MI50 */

   CHIP_NAVI10,         /* Radeon 5600, 5700 */
   CHIP_NAVI12,         /* Radeon Pro 5600M */
   CHIP_NAVI14,         /* Radeon 5300, 5500 */
   CHIP_NAVI21,         /* Radeon 6800, 6900 (formerly "Sienna Cichlid") */
   CHIP_NAVI22,         /* Radeon 6700 (formerly "Navy Flounder") */
   CHIP_NAVI23,         /* Radeon 6600 (formerly "Dimgrey Cavefish") */
   CHIP_NAVI24,         /* Radeon 6400, 6500 (formerly "Beige Goby") */
   CHIP_NAVI31,         /* Radeon 7900 */
   CHIP_NAVI32,         /* Radeon 7800, 7700 */
   CHIP_NAVI33,         /* Radeon 7600, 7700S (mobile) */

   CHIP_LAST,
};
enum amd_asic_version {
    DCE_VERSION_UNKNOWN = (-1),

    DCE_VERSION_6_0,
    DCE_VERSION_6_1,
    DCE_VERSION_6_4,
    DCE_VERSION_8_0,
    DCE_VERSION_8_1,
    DCE_VERSION_8_3,
    DCE_VERSION_10_0,
    DCE_VERSION_11_0,
    DCE_VERSION_11_2,
    DCE_VERSION_11_22,
    DCE_VERSION_12_0,
    DCE_VERSION_12_1,
    DCE_VERSION_MAX,

    DCN_VERSION_1_0,
    DCN_VERSION_1_01,
    DCN_VERSION_2_0,
    DCN_VERSION_2_01,
    DCN_VERSION_2_1,
    DCN_VERSION_3_0,
    DCN_VERSION_3_01,
    DCN_VERSION_3_02,
    DCN_VERSION_3_03,
    DCN_VERSION_3_1,
    DCN_VERSION_3_14,
    DCN_VERSION_3_15,
    DCN_VERSION_3_16,
    DCN_VERSION_3_2,
    DCN_VERSION_3_21,
    // DCN_VERSION_3_5,
    // DCN_VERSION_3_51,
    // DCN_VERSION_4_01,
    DCN_VERSION_MAX
};

struct amd_pci_asic {
    enum amd_asic_type      type;
    enum amd_asic_version   version;
};

static const struct pci_device_id pciidlist[] = {
    {AMD_GPU_VEN, AMD_POLARIS_DEV, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_POLARIS10},
    {AMD_GPU_VEN, AMD_POLARIS11, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_POLARIS11},
    {AMD_GPU_VEN, AMD_POLARIS20XL_DEV,	PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_POLARIS12},

    {AMD_GPU_VEN, AMD_VEGA10_DEV, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_VEGA10},

    {AMD_GPU_VEN, AMD_NAVI10_DEV, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_NAVI10},
    {AMD_GPU_VEN, AMD_NAVI14_DEV, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_NAVI14},
    {AMD_GPU_VEN, AMD_NAVI21_DEV1, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_NAVI21},
    {AMD_GPU_VEN, AMD_NAVI21_DEV2, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_NAVI21},
    {AMD_GPU_VEN, AMD_NAVI21_DEV3, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_NAVI21},
    {AMD_GPU_VEN, AMD_NAVI22_DEV, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_NAVI22},
    {AMD_GPU_VEN, AMD_NAVI23_DEV, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_NAVI23},
    {AMD_GPU_VEN, AMD_NAVI23_DEV1, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_NAVI23},
    {AMD_GPU_VEN, AMD_NAVI31_DEV, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_NAVI31},
    {AMD_GPU_VEN, AMD_NAVI32_DEV, 		PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_NAVI32},

    {0, 0, 0},
};

#endif
