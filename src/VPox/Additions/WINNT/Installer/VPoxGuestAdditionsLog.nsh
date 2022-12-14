; $Id: VPoxGuestAdditionsLog.nsh $
;; @file
; VPoxGuestAdditionLog.nsh - Logging functions.
;

;
; Copyright (C) 2013-2020 Oracle Corporation
;
; This file is part of VirtualPox Open Source Edition (OSE), as
; available from http://www.virtualpox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualPox OSE distribution. VirtualPox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

;
; Macro for enable/disable logging
; @param   "true" to enable logging, "false" to disable.
;
!macro _logEnable enable

  ${If} ${enable} == "true"
    LogSet on
    ${LogVerbose} "Started logging into separate file"
  ${Else}
    ${LogVerbose} "Stopped logging into separate file"
    LogSet off
  ${EndIf}

!macroend
!define LogEnable "!insertmacro _logEnable"

;
; Macro for (verbose) logging
; @param   Text to log.
;
!macro _logVerbose text

  LogText "${text}"
  IfSilent +2
    DetailPrint "${text}"

!macroend
!define LogVerbose "!insertmacro _logVerbose"

;
; Sends a logging text to the running instance of VPoxTray
; which then presents to text via balloon popup in the system tray (if enabled).
;
; @param   Message type (0=Info, 1=Warning, 2=Error).
; @param   Message text.
;
; @todo Add message timeout as parameter.
;
!macro _logToVPoxTray type text

    ${LogVerbose} "${text}"
!if $%VPOX_WITH_GUEST_INSTALL_HELPER% == "1"
    Push $0
    ; Parameters:
    ; - String: Description / Body
    ; - String: Title / Name of application
    ; - Integer: Type of message: 0 (Info), 1 (Warning), 2 (Error)
    ; - Integer: Time (in msec) to show the notification
    VPoxGuestInstallHelper::VPoxTrayShowBallonMsg "${text}" "VirtualPox Guest Additions Setup" ${type} 5000
    Pop $0 ; Get return value (ignored for now)
    Pop $0 ; Restore original $0 from stack
!endif

!macroend
!define LogToVPoxTray "!insertmacro _logToVPoxTray"
