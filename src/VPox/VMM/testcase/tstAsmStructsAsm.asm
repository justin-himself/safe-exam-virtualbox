; $Id: tstAsmStructsAsm.asm $
;; @file
; Assembly / C structure layout testcase.
;
; Make yasm/nasm create absolute symbols for the structure definition
; which we can parse and make code from using objdump and sed.
;

;
; Copyright (C) 2006-2020 Oracle Corporation
;
; This file is part of VirtualPox Open Source Edition (OSE), as
; available from http://www.virtualpox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualPox OSE distribution. VirtualPox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

%ifdef RT_ARCH_AMD64
BITS 64
%endif

%include "CPUMInternal.mac"
%include "HMInternal.mac"
%include "VMMInternal.mac"
%include "VPox/vmm/cpum.mac"
%include "VPox/vmm/vm.mac"
%include "VPox/vmm/hm_vmx.mac"
%include "VPox/sup.mac"
%ifdef DO_GLOBALS
 %include "tstAsmStructsAsm.mac"
%endif

.text
.data
.bss

