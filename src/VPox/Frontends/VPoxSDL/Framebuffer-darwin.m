/* $Id: Framebuffer-darwin.m $ */
/** @file
 * VPoxSDL - Darwin Cocoa helper functions.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define NO_SDL_H
#import "VPoxSDL.h"
#import <Cocoa/Cocoa.h>

void *VPoxSDLGetDarwinWindowId(void)
{
    NSView            *pView = nil;
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];
    {
        NSApplication *pApp = NSApp;
        NSWindow *pMainWnd;
        pMainWnd = [pApp mainWindow];
        if (!pMainWnd)
            pMainWnd = pApp->_mainWindow; /* UGLY!! but mApp->_AppFlags._active = 0, so mainWindow() fails. */
        pView = [pMainWnd contentView];
    }
    [pPool release];
    return pView;
}

