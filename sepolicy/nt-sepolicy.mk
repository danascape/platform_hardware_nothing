#
# SPDX-FileCopyrightText: 2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += hardware/nothing/sepolicy/public
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += hardware/nothing/sepolicy/private

BOARD_VENDOR_SEPOLICY_DIRS += hardware/nothing/sepolicy/vendor

ifeq ($(BOARD_USES_QCOM_HARDWARE),true)
BOARD_VENDOR_SEPOLICY_DIRS += hardware/nothing/sepolicy/qcom/vendor

ifeq ($(filter $(UM_4_9_FAMILY) $(UM_4_14_FAMILY) $(UM_4_19_FAMILY) $(UM_5_4_FAMILY),$(TARGET_BOARD_PLATFORM)),)
BOARD_VENDOR_SEPOLICY_DIRS += hardware/nothing/sepolicy/qcom/dmabuf
endif
endif
