/* $Id: deletefile-1.cpp $ */
/** @file
 * VirtualPox Windows Guest Shared Folders FSD - Simple Testcase.
 */

/*
 * Copyright (C) 2019-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <windows.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        SetLastError(0);
        if (DeleteFileA(argv[i]))
            printf("%s: deleted\n", argv[i]);
        else
            fprintf(stderr, "%s: DeleteFileA failed: %u\n", argv[i], GetLastError());
    }
    return 0;
}

