; $Id: VPoxDTraceR0A.asm $
;; @file
; VPoxDTraceR0 - Assembly Hacks.
;
; Contributed by: bird
;

;
; Copyright (C) 2012-2020 Oracle Corporation
;
; This file is part of VirtualPox Open Source Edition (OSE), as
; available from http://www.virtualpox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the Common
; Development and Distribution License Version 1.0 (CDDL) only, as it
; comes in the "COPYING.CDDL" file of the VirtualPox OSE distribution.
; VirtualPox OSE is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY of any kind.
;



%include "iprt/asmdefs.mac"

BEGINCODE


extern NAME(dtrace_probe)

GLOBALNAME dtrace_probe6
    jmp     NAME(dtrace_probe)

