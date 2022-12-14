#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: tdSmokeTest1.py $

"""
VirtualPox Validation Kit - Smoke Test #1.
"""

__copyright__ = \
"""
Copyright (C) 2010-2020 Oracle Corporation

This file is part of VirtualPox Open Source Edition (OSE), as
available from http://www.virtualpox.org. This file is free software;
you can redistribute it and/or modify it under the terms of the GNU
General Public License (GPL) as published by the Free Software
Foundation, in version 2 as it comes in the "COPYING" file of the
VirtualPox OSE distribution. VirtualPox OSE is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

The contents of this file may alternatively be used under the terms
of the Common Development and Distribution License Version 1.0
(CDDL) only, as it comes in the "COPYING.CDDL" file of the
VirtualPox OSE distribution, in which case the provisions of the
CDDL are applicable instead of those of the GPL.

You may elect to license modified versions of this file under the
terms and conditions of either the GPL or the CDDL or both.
"""
__version__ = "$Revision: 135976 $"


# Standard Python imports.
import os;
import sys;

# Only the main script needs to modify the path.
try:    __file__
except: __file__ = sys.argv[0];
g_ksValidationKitDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))));
sys.path.append(g_ksValidationKitDir);

# Validation Kit imports.
from testdriver import reporter;
from testdriver import base;
from testdriver import vpox;
from testdriver import vpoxcon;


class tdSmokeTest1(vpox.TestDriver):
    """
    VPox Smoke Test #1.
    """

    def __init__(self):
        vpox.TestDriver.__init__(self);
        self.asRsrcs            = None;
        self.oTestVmSet         = self.oTestVmManager.getSmokeVmSet();
        self.sNicAttachmentDef  = 'mixed';
        self.sNicAttachment     = self.sNicAttachmentDef;
        self.fQuick             = False;

    #
    # Overridden methods.
    #
    def showUsage(self):
        rc = vpox.TestDriver.showUsage(self);
        reporter.log('');
        reporter.log('Smoke Test #1 options:');
        reporter.log('  --nic-attachment <bridged|nat|mixed>');
        reporter.log('      Default: %s' % (self.sNicAttachmentDef));
        reporter.log('  --quick');
        reporter.log('      Very selective testing.')
        return rc;

    def parseOption(self, asArgs, iArg):
        if asArgs[iArg] == '--nic-attachment':
            iArg += 1;
            if iArg >= len(asArgs): raise base.InvalidOption('The "--nic-attachment" takes an argument');
            self.sNicAttachment = asArgs[iArg];
            if self.sNicAttachment not in ('bridged', 'nat', 'mixed'):
                raise base.InvalidOption('The "--nic-attachment" value "%s" is not supported. Valid values are: bridged, nat' \
                        % (self.sNicAttachment));
        elif asArgs[iArg] == '--quick':
            # Disable all but a few VMs and configurations.
            for oTestVm in self.oTestVmSet.aoTestVms:
                if oTestVm.sVmName == 'tst-win2k3ent':          # 32-bit paging
                    oTestVm.asVirtModesSup  = [ 'hwvirt' ];
                    oTestVm.acCpusSup       = range(1, 2);
                elif oTestVm.sVmName == 'tst-rhel5':            # 32-bit paging
                    oTestVm.asVirtModesSup  = [ 'raw' ];
                    oTestVm.acCpusSup       = range(1, 2);
                elif oTestVm.sVmName == 'tst-win2k8':           # 64-bit
                    oTestVm.asVirtModesSup  = [ 'hwvirt-np' ];
                    oTestVm.acCpusSup       = range(1, 2);
                elif oTestVm.sVmName == 'tst-sol10-64':         # SMP, 64-bit
                    oTestVm.asVirtModesSup  = [ 'hwvirt' ];
                    oTestVm.acCpusSup       = range(2, 3);
                elif oTestVm.sVmName == 'tst-sol10':            # SMP, 32-bit
                    oTestVm.asVirtModesSup  = [ 'hwvirt-np' ];
                    oTestVm.acCpusSup       = range(2, 3);
                elif oTestVm.sVmName == 'tst-nsthwvirt-ubuntu-64':  # Nested hw.virt, 64-bit
                    oTestVm.asVirtModesSup  = [ 'hwvirt-np' ];
                    oTestVm.acCpusSup       = range(1, 2);
                else:
                    oTestVm.fSkip = True;
        else:
            return vpox.TestDriver.parseOption(self, asArgs, iArg);
        return iArg + 1;

    def actionVerify(self):
        if self.sVPoxValidationKitIso is None or not os.path.isfile(self.sVPoxValidationKitIso):
            reporter.error('Cannot find the VPoxValidationKit.iso! (%s)'
                           'Please unzip a Validation Kit build in the current directory or in some parent one.'
                           % (self.sVPoxValidationKitIso,) );
            return False;
        return vpox.TestDriver.actionVerify(self);

    def actionConfig(self):
        # Make sure vpoxapi has been imported so we can use the constants.
        if not self.importVPoxApi():
            return False;

        # Do the configuring.
        if self.sNicAttachment == 'nat':        eNic0AttachType = vpoxcon.NetworkAttachmentType_NAT;
        elif self.sNicAttachment == 'bridged':  eNic0AttachType = vpoxcon.NetworkAttachmentType_Bridged;
        else:                                   eNic0AttachType = None;
        assert self.sVPoxValidationKitIso is not None;
        return self.oTestVmSet.actionConfig(self, eNic0AttachType = eNic0AttachType, sDvdImage = self.sVPoxValidationKitIso);

    def actionExecute(self):
        """
        Execute the testcase.
        """
        return self.oTestVmSet.actionExecute(self, self.testOneVmConfig)


    #
    # Test execution helpers.
    #

    def testOneVmConfig(self, oVM, oTestVm):
        """
        Runs the specified VM thru test #1.
        """

        # Simple test.
        self.logVmInfo(oVM);
        oSession, oTxsSession = self.startVmAndConnectToTxsViaTcp(oTestVm.sVmName, fCdWait = True);
        if oSession is not None:
            self.addTask(oTxsSession);

            ## @todo do some quick tests: save, restore, execute some test program, shut down the guest.

            # cleanup.
            self.removeTask(oTxsSession);
            self.terminateVmBySession(oSession)
            return True;
        return None;

if __name__ == '__main__':
    sys.exit(tdSmokeTest1().main(sys.argv));

