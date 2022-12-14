@REM REM @file
@REM VirtualPox Test Execution Service Init Script for NATted VMs.
@REM

@REM
REM
REM Copyright (C) 2006-2020 Oracle Corporation
REM
REM This file is part of VirtualPox Open Source Edition (OSE), as
REM available from http://www.virtualpox.org. This file is free software;
REM you can redistribute it and/or modify it under the terms of the GNU
REM General Public License (GPL) as published by the Free Software
REM Foundation, in version 2 as it comes in the "COPYING" file of the
REM VirtualPox OSE distribution. VirtualPox OSE is distributed in the
REM hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
REM
REM The contents of this file may alternatively be used under the terms
REM of the Common Development and Distribution License Version 1.0
REM (CDDL) only, as it comes in the "COPYING.CDDL" file of the
REM VirtualPox OSE distribution, in which case the provisions of the
REM CDDL are applicable instead of those of the GPL.
REM
REM You may elect to license modified versions of this file under the
REM terms and conditions of either the GPL or the CDDL or both.
REM

%SystemDrive%\Apps\TestExecService.exe --foreground --display-output ^
--cdrom D:\ --scratch C:\Temp\vpoxtest --auto-upgrade ^
--tcp-connect 10.0.2.2
pause

