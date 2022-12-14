#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VirtualPox Installer Wrapper Driver.

This installs VirtualPox, starts a sub driver which does the real testing,
and then uninstall VirtualPox afterwards.  This reduces the complexity of the
other VPox test drivers.
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
import os
import sys
import re
#import socket
import tempfile
import time

# Only the main script needs to modify the path.
try:    __file__
except: __file__ = sys.argv[0];
g_ksValidationKitDir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)));
sys.path.append(g_ksValidationKitDir);

# Validation Kit imports.
from common             import utils, webutils;
from common.constants   import rtexitcode;
from testdriver         import reporter;
from testdriver.base    import TestDriverBase;



class VPoxInstallerTestDriver(TestDriverBase):
    """
    Implementation of a top level test driver.
    """


    ## State file indicating that we've skipped installation.
    ksVar_Skipped = 'vpoxinstaller-skipped';


    def __init__(self):
        TestDriverBase.__init__(self);
        self._asSubDriver   = [];   # The sub driver and it's arguments.
        self._asBuildUrls   = [];   # The URLs passed us on the command line.
        self._asBuildFiles  = [];   # The downloaded file names.
        self._fAutoInstallPuelExtPack = True;

    #
    # Base method we override
    #

    def showUsage(self):
        rc = TestDriverBase.showUsage(self);
        #             0         1         2         3         4         5         6         7         8
        #             012345678901234567890123456789012345678901234567890123456789012345678901234567890
        reporter.log('');
        reporter.log('vpoxinstaller Options:');
        reporter.log('  --vpox-build    <url[,url2[,..]]>');
        reporter.log('      Comma separated list of URL to file to download and install or/and');
        reporter.log('      unpack.  URLs without a schema are assumed to be files on the');
        reporter.log('      build share and will be copied off it.');
        reporter.log('  --no-puel-extpack');
        reporter.log('      Indicates that the PUEL extension pack should not be installed if found.');
        reporter.log('      The default is to install it if found in the vpox-build.');
        reporter.log('  --');
        reporter.log('      Indicates the end of our parameters and the start of the sub');
        reporter.log('      testdriver and its arguments.');
        return rc;

    def parseOption(self, asArgs, iArg):
        """
        Parse our arguments.
        """
        if asArgs[iArg] == '--':
            # End of our parameters and start of the sub driver invocation.
            iArg = self.requireMoreArgs(1, asArgs, iArg);
            assert not self._asSubDriver;
            self._asSubDriver = asArgs[iArg:];
            self._asSubDriver[0] = self._asSubDriver[0].replace('/', os.path.sep);
            iArg = len(asArgs) - 1;
        elif asArgs[iArg] == '--vpox-build':
            # List of files to copy/download and install.
            iArg = self.requireMoreArgs(1, asArgs, iArg);
            self._asBuildUrls = asArgs[iArg].split(',');
        elif asArgs[iArg] == '--no-puel-extpack':
            self._fAutoInstallPuelExtPack = False;
        elif asArgs[iArg] == '--puel-extpack':
            self._fAutoInstallPuelExtPack = True;
        else:
            return TestDriverBase.parseOption(self, asArgs, iArg);
        return iArg + 1;

    def completeOptions(self):
        #
        # Check that we've got what we need.
        #
        if not self._asBuildUrls:
            reporter.error('No build files specfiied ("--vpox-build file1[,file2[...]]")');
            return False;
        if not self._asSubDriver:
            reporter.error('No sub testdriver specified. (" -- test/stuff/tdStuff1.py args")');
            return False;

        #
        # Construct _asBuildFiles as an array parallel to _asBuildUrls.
        #
        for sUrl in self._asBuildUrls:
            sDstFile = os.path.join(self.sScratchPath, webutils.getFilename(sUrl));
            self._asBuildFiles.append(sDstFile);

        return TestDriverBase.completeOptions(self);

    def actionExtract(self):
        reporter.error('vpoxinstall does not support extracting resources, you have to do that using the sub testdriver.');
        return False;

    def actionCleanupBefore(self):
        """
        Kills all VPox process we see.

        This is only supposed to execute on a testbox so we don't need to go
        all complicated wrt other users.
        """
        return self._killAllVPoxProcesses();

    def actionConfig(self):
        """
        Install VPox and pass on the configure request to the sub testdriver.
        """
        fRc = self._installVPox();
        if fRc is None:
            self._persistentVarSet(self.ksVar_Skipped, 'true');
            self.fBadTestbox = True;
        else:
            self._persistentVarUnset(self.ksVar_Skipped);

        ## @todo vpox.py still has bugs preventing us from invoking it seperately with each action.
        if fRc is True and 'execute' not in self.asActions and 'all' not in self.asActions:
            fRc = self._executeSubDriver([ 'verify', ]);
        if fRc is True and 'execute' not in self.asActions and 'all' not in self.asActions:
            fRc = self._executeSubDriver([ 'config', ]);
        return fRc;

    def actionExecute(self):
        """
        Execute the sub testdriver.
        """
        return self._executeSubDriver(self.asActions);

    def actionCleanupAfter(self):
        """
        Forward this to the sub testdriver, then uninstall VPox.
        """
        fRc = True;
        if 'execute' not in self.asActions and 'all' not in self.asActions:
            fRc = self._executeSubDriver([ 'cleanup-after', ], fMaySkip = False);

        if not self._killAllVPoxProcesses():
            fRc = False;

        if not self._uninstallVPox(self._persistentVarExists(self.ksVar_Skipped)):
            fRc = False;

        if utils.getHostOs() == 'darwin':
            self._darwinUnmountDmg(fIgnoreError = True); # paranoia

        if not TestDriverBase.actionCleanupAfter(self):
            fRc = False;

        return fRc;


    def actionAbort(self):
        """
        Forward this to the sub testdriver first, then wipe all VPox like
        processes, and finally do the pid file processing (again).
        """
        fRc1 = self._executeSubDriver([ 'abort', ], fMaySkip = False);
        fRc2 = self._killAllVPoxProcesses();
        fRc3 = TestDriverBase.actionAbort(self);
        return fRc1 and fRc2 and fRc3;


    #
    # Persistent variables.
    #
    ## @todo integrate into the base driver. Persisten accross scratch wipes?

    def __persistentVarCalcName(self, sVar):
        """Returns the (full) filename for the given persistent variable."""
        assert re.match(r'^[a-zA-Z0-9_-]*$', sVar) is not None;
        return os.path.join(self.sScratchPath, 'persistent-%s.var' % (sVar,));

    def _persistentVarSet(self, sVar, sValue = ''):
        """
        Sets a persistent variable.

        Returns True on success, False + reporter.error on failure.

        May raise exception if the variable name is invalid or something
        unexpected happens.
        """
        sFull = self.__persistentVarCalcName(sVar);
        try:
            oFile = open(sFull, 'w');
            if sValue:
                oFile.write(sValue.encode('utf-8'));
            oFile.close();
        except:
            reporter.errorXcpt('Error creating "%s"' % (sFull,));
            return False;
        return True;

    def _persistentVarUnset(self, sVar):
        """
        Unsets a persistent variable.

        Returns True on success, False + reporter.error on failure.

        May raise exception if the variable name is invalid or something
        unexpected happens.
        """
        sFull = self.__persistentVarCalcName(sVar);
        if os.path.exists(sFull):
            try:
                os.unlink(sFull);
            except:
                reporter.errorXcpt('Error unlinking "%s"' % (sFull,));
                return False;
        return True;

    def _persistentVarExists(self, sVar):
        """
        Checks if a persistent variable exists.

        Returns true/false.

        May raise exception if the variable name is invalid or something
        unexpected happens.
        """
        return os.path.exists(self.__persistentVarCalcName(sVar));

    def _persistentVarGet(self, sVar):
        """
        Gets the value of a persistent variable.

        Returns variable value on success.
        Returns None if the variable doesn't exist or if an
        error (reported) occured.

        May raise exception if the variable name is invalid or something
        unexpected happens.
        """
        sFull = self.__persistentVarCalcName(sVar);
        if not os.path.exists(sFull):
            return None;
        try:
            oFile = open(sFull, 'r');
            sValue = oFile.read().decode('utf-8');
            oFile.close();
        except:
            reporter.errorXcpt('Error creating "%s"' % (sFull,));
            return None;
        return sValue;


    #
    # Helpers.
    #

    def _killAllVPoxProcesses(self):
        """
        Kills all virtual box related processes we find in the system.
        """

        for iIteration in range(22):
            # Gather processes to kill.
            aoTodo = [];
            for oProcess in utils.processListAll():
                sBase = oProcess.getBaseImageNameNoExeSuff();
                if sBase is None:
                    continue;
                sBase = sBase.lower();
                if sBase in [ 'vpoxsvc', 'vpoxsds', 'virtualpox', 'virtualpoxvm', 'vpoxheadless', 'vpoxmanage', 'vpoxsdl',
                              'vpoxwebsrv', 'vpoxautostart', 'vpoxballoonctrl', 'vpoxbfe', 'vpoxextpackhelperapp', 'vpoxnetdhcp',
                              'vpoxnetnat', 'vpoxnetadpctl', 'vpoxtestogl', 'vpoxtunctl', 'vpoxvmmpreload', 'vpoxxpcomipcd', ]:
                    aoTodo.append(oProcess);
                if sBase.startswith('virtualpox-') and sBase.endswith('-multiarch.exe'):
                    aoTodo.append(oProcess);
                if iIteration in [0, 21]  and  sBase in [ 'windbg', 'gdb', 'gdb-i386-apple-darwin', ]:
                    reporter.log('Warning: debugger running: %s (%s)' % (oProcess.iPid, sBase,));
            if not aoTodo:
                return True;

            # Kill.
            for oProcess in aoTodo:
                reporter.log('Loop #%d - Killing %s (%s, uid=%s)'
                             % ( iIteration, oProcess.iPid, oProcess.sImage if oProcess.sName is None else oProcess.sName,
                                 oProcess.iUid, ));
                utils.processKill(oProcess.iPid); # No mercy.

            # Check if they're all dead like they should be.
            time.sleep(0.1);
            for oProcess in aoTodo:
                if utils.processExists(oProcess.iPid):
                    time.sleep(2);
                    break;

        return False;

    def _executeSync(self, asArgs, fMaySkip = False):
        """
        Executes a child process synchronously.

        Returns True if the process executed successfully and returned 0.
        Returns None if fMaySkip is true and the child exits with RTEXITCODE_SKIPPED.
        Returns False for all other cases.
        """
        reporter.log('Executing: %s' % (asArgs, ));
        reporter.flushall();
        try:
            iRc = utils.processCall(asArgs, shell = False, close_fds = False);
        except:
            reporter.errorXcpt();
            return False;
        reporter.log('Exit code: %s (%s)' % (iRc, asArgs));
        if fMaySkip and iRc == rtexitcode.RTEXITCODE_SKIPPED:
            return None;
        return iRc == 0;

    def _sudoExecuteSync(self, asArgs):
        """
        Executes a sudo child process synchronously.
        Returns a tuple [True, 0] if the process executed successfully
        and returned 0, otherwise [False, rc] is returned.
        """
        reporter.log('Executing [sudo]: %s' % (asArgs, ));
        reporter.flushall();
        iRc = 0;
        try:
            iRc = utils.sudoProcessCall(asArgs, shell = False, close_fds = False);
        except:
            reporter.errorXcpt();
            return (False, 0);
        reporter.log('Exit code [sudo]: %s (%s)' % (iRc, asArgs));
        return (iRc == 0, iRc);

    def _executeSubDriver(self, asActions, fMaySkip = True):
        """
        Execute the sub testdriver with the specified action.
        """
        asArgs = list(self._asSubDriver)
        asArgs.append('--no-wipe-clean');
        asArgs.extend(asActions);
        return self._executeSync(asArgs, fMaySkip = fMaySkip);

    def _maybeUnpackArchive(self, sMaybeArchive, fNonFatal = False):
        """
        Attempts to unpack the given build file.
        Updates _asBuildFiles.
        Returns True/False. No exceptions.
        """
        def unpackFilter(sMember):
            # type: (string) -> bool
            """ Skips debug info. """
            sLower = sMember.lower();
            if sLower.endswith('.pdb'):
                return False;
            return True;

        asMembers = utils.unpackFile(sMaybeArchive, self.sScratchPath, reporter.log,
                                     reporter.log if fNonFatal else reporter.error,
                                     fnFilter = unpackFilter);
        if asMembers is None:
            return False;
        self._asBuildFiles.extend(asMembers);
        return True;


    def _installVPox(self):
        """
        Download / copy the build files into the scratch area and install them.
        """
        reporter.testStart('Installing VirtualPox');
        reporter.log('CWD=%s' % (os.getcwd(),)); # curious

        #
        # Download the build files.
        #
        for i in range(len(self._asBuildUrls)):
            if webutils.downloadFile(self._asBuildUrls[i], self._asBuildFiles[i],
                                     self.sBuildPath, reporter.log, reporter.log) is not True:
                reporter.testDone(fSkipped = True);
                return None; # Failed to get binaries, probably deleted. Skip the test run.

        #
        # Unpack anything we know what is and append it to the build files
        # list.  This allows us to use VPoxAll*.tar.gz files.
        #
        for sFile in list(self._asBuildFiles):
            if self._maybeUnpackArchive(sFile, fNonFatal = True) is not True:
                reporter.testDone(fSkipped = True);
                return None; # Failed to unpack. Probably local error, like busy
                             # DLLs on windows, no reason for failing the build.

        #
        # Go to system specific installation code.
        #
        sHost = utils.getHostOs()
        if   sHost == 'darwin':     fRc = self._installVPoxOnDarwin();
        elif sHost == 'linux':      fRc = self._installVPoxOnLinux();
        elif sHost == 'solaris':    fRc = self._installVPoxOnSolaris();
        elif sHost == 'win':        fRc = self._installVPoxOnWindows();
        else:
            reporter.error('Unsupported host "%s".' % (sHost,));
        if fRc is False:
            reporter.testFailure('Installation error.');
        elif fRc is not True:
            reporter.log('Seems installation was skipped. Old version lurking behind? Not the fault of this build/test run!');

        #
        # Install the extension pack.
        #
        if fRc is True  and  self._fAutoInstallPuelExtPack:
            fRc = self._installExtPack();
            if fRc is False:
                reporter.testFailure('Extension pack installation error.');

        # Some debugging...
        try:
            cMbFreeSpace = utils.getDiskUsage(self.sScratchPath);
            reporter.log('Disk usage after VPox install: %d MB available at %s' % (cMbFreeSpace, self.sScratchPath,));
        except:
            reporter.logXcpt('Unable to get disk free space. Ignored. Continuing.');

        reporter.testDone(fRc is None);
        return fRc;

    def _uninstallVPox(self, fIgnoreError = False):
        """
        Uninstall VirtualPox.
        """
        reporter.testStart('Uninstalling VirtualPox');

        sHost = utils.getHostOs()
        if   sHost == 'darwin':     fRc = self._uninstallVPoxOnDarwin();
        elif sHost == 'linux':      fRc = self._uninstallVPoxOnLinux();
        elif sHost == 'solaris':    fRc = self._uninstallVPoxOnSolaris(True);
        elif sHost == 'win':        fRc = self._uninstallVPoxOnWindows('uninstall');
        else:
            reporter.error('Unsupported host "%s".' % (sHost,));
        if fRc is False and not fIgnoreError:
            reporter.testFailure('Uninstallation failed.');

        fRc2 = self._uninstallAllExtPacks();
        if not fRc2 and fRc:
            fRc = fRc2;

        reporter.testDone(fSkipped = (fRc is None));
        return fRc;

    def _findFile(self, sRegExp, fMandatory = False):
        """
        Returns the first build file that matches the given regular expression
        (basename only).

        Returns None if no match was found, logging it as an error if
        fMandatory is set.
        """
        oRegExp = re.compile(sRegExp);

        for sFile in self._asBuildFiles:
            if oRegExp.match(os.path.basename(sFile)) and os.path.exists(sFile):
                return sFile;

        if fMandatory:
            reporter.error('Failed to find a file matching "%s" in %s.' % (sRegExp, self._asBuildFiles,));
        return None;

    def _waitForTestManagerConnectivity(self, cSecTimeout):
        """
        Check and wait for network connectivity to the test manager.

        This is used with the windows installation and uninstallation since
        these usually disrupts network connectivity when installing the filter
        driver.  If we proceed to quickly, we might finish the test at a time
        when we cannot report to the test manager and thus end up with an
        abandonded test error.
        """
        cSecElapsed = 0;
        secStart    = utils.timestampSecond();
        while reporter.checkTestManagerConnection() is False:
            cSecElapsed = utils.timestampSecond() - secStart;
            if cSecElapsed >= cSecTimeout:
                reporter.log('_waitForTestManagerConnectivity: Giving up after %u secs.' % (cSecTimeout,));
                return False;
            time.sleep(2);

        if cSecElapsed > 0:
            reporter.log('_waitForTestManagerConnectivity: Waited %s secs.' % (cSecTimeout,));
        return True;


    #
    # Darwin (Mac OS X).
    #

    def _darwinDmgPath(self):
        """ Returns the path to the DMG mount."""
        return os.path.join(self.sScratchPath, 'DmgMountPoint');

    def _darwinUnmountDmg(self, fIgnoreError):
        """
        Umount any DMG on at the default mount point.
        """
        sMountPath = self._darwinDmgPath();
        if not os.path.exists(sMountPath):
            return True;

        # Unmount.
        fRc = self._executeSync(['hdiutil', 'detach', sMountPath ]);
        if not fRc and not fIgnoreError:
            reporter.error('Failed to unmount DMG at %s' % sMountPath);

        # Remove dir.
        try:
            os.rmdir(sMountPath);
        except:
            if not fIgnoreError:
                reporter.errorXcpt('Failed to remove directory %s' % sMountPath);
        return fRc;

    def _darwinMountDmg(self, sDmg):
        """
        Mount the DMG at the default mount point.
        """
        self._darwinUnmountDmg(fIgnoreError = True)

        sMountPath = self._darwinDmgPath();
        if not os.path.exists(sMountPath):
            try:
                os.mkdir(sMountPath, 0o755);
            except:
                reporter.logXcpt();
                return False;

        return self._executeSync(['hdiutil', 'attach', '-readonly', '-mount', 'required', '-mountpoint', sMountPath, sDmg, ]);

    def _installVPoxOnDarwin(self):
        """ Installs VPox on Mac OS X."""
        sDmg = self._findFile('^VirtualPox-.*\\.dmg$');
        if sDmg is None:
            return False;

        # Mount the DMG.
        fRc = self._darwinMountDmg(sDmg);
        if fRc is not True:
            return False;

        # Uninstall any previous vpox version first.
        sUninstaller = os.path.join(self._darwinDmgPath(), 'VirtualPox_Uninstall.tool');
        fRc, _ = self._sudoExecuteSync([sUninstaller, '--unattended',]);
        if fRc is True:

            # Install the package.
            sPkg = os.path.join(self._darwinDmgPath(), 'VirtualPox.pkg');
            fRc, _ = self._sudoExecuteSync(['installer', '-verbose', '-dumplog', '-pkg', sPkg, '-target', '/']);

        # Unmount the DMG and we're done.
        if not self._darwinUnmountDmg(fIgnoreError = False):
            fRc = False;
        return fRc;

    def _uninstallVPoxOnDarwin(self):
        """ Uninstalls VPox on Mac OS X."""

        # Is VirtualPox installed? If not, don't try uninstall it.
        sVPox = self._getVPoxInstallPath(fFailIfNotFound = False);
        if sVPox is None:
            return True;

        # Find the dmg.
        sDmg = self._findFile('^VirtualPox-.*\\.dmg$');
        if sDmg is None:
            return False;
        if not os.path.exists(sDmg):
            return True;

        # Mount the DMG.
        fRc = self._darwinMountDmg(sDmg);
        if fRc is not True:
            return False;

        # Execute the uninstaller.
        sUninstaller = os.path.join(self._darwinDmgPath(), 'VirtualPox_Uninstall.tool');
        fRc, _ = self._sudoExecuteSync([sUninstaller, '--unattended',]);

        # Unmount the DMG and we're done.
        if not self._darwinUnmountDmg(fIgnoreError = False):
            fRc = False;
        return fRc;

    #
    # GNU/Linux
    #

    def _installVPoxOnLinux(self):
        """ Installs VPox on Linux."""
        sRun = self._findFile('^VirtualPox-.*\\.run$');
        if sRun is None:
            return False;
        utils.chmodPlusX(sRun);

        # Install the new one.
        fRc, _ = self._sudoExecuteSync([sRun,]);
        return fRc;

    def _uninstallVPoxOnLinux(self):
        """ Uninstalls VPox on Linux."""

        # Is VirtualPox installed? If not, don't try uninstall it.
        sVPox = self._getVPoxInstallPath(fFailIfNotFound = False);
        if sVPox is None:
            return True;

        # Find the .run file and use it.
        sRun = self._findFile('^VirtualPox-.*\\.run$', fMandatory = False);
        if sRun is not None:
            utils.chmodPlusX(sRun);
            fRc, _ = self._sudoExecuteSync([sRun, 'uninstall']);
            return fRc;

        # Try the installed uninstaller.
        for sUninstaller in [os.path.join(sVPox, 'uninstall.sh'), '/opt/VirtualPox/uninstall.sh', ]:
            if os.path.isfile(sUninstaller):
                reporter.log('Invoking "%s"...' % (sUninstaller,));
                fRc, _ = self._sudoExecuteSync([sUninstaller, 'uninstall']);
                return fRc;

        reporter.log('Did not find any VirtualPox install to uninstall.');
        return True;


    #
    # Solaris
    #

    def _generateAutoResponseOnSolaris(self):
        """
        Generates an autoresponse file on solaris, returning the name.
        None is return on failure.
        """
        sPath = os.path.join(self.sScratchPath, 'SolarisAutoResponse');
        oFile = utils.openNoInherit(sPath, 'wt');
        oFile.write('basedir=default\n'
                    'runlevel=nocheck\n'
                    'conflict=quit\n'
                    'setuid=nocheck\n'
                    'action=nocheck\n'
                    'partial=quit\n'
                    'instance=unique\n'
                    'idepend=quit\n'
                    'rdepend=quit\n'
                    'space=quit\n'
                    'mail=\n');
        oFile.close();
        return sPath;

    def _installVPoxOnSolaris(self):
        """ Installs VPox on Solaris."""
        sPkg = self._findFile('^VirtualPox-.*\\.pkg$', fMandatory = False);
        if sPkg is None:
            sTar = self._findFile('^VirtualPox-.*-SunOS-.*\\.tar.gz$', fMandatory = False);
            if sTar is not None:
                if self._maybeUnpackArchive(sTar) is not True:
                    return False;
        sPkg = self._findFile('^VirtualPox-.*\\.pkg$', fMandatory = True);
        sRsp = self._findFile('^autoresponse$', fMandatory = True);
        if sPkg is None or sRsp is None:
            return False;

        # Uninstall first (ignore result).
        self._uninstallVPoxOnSolaris(False);

        # Install the new one.
        fRc, _ = self._sudoExecuteSync(['pkgadd', '-d', sPkg, '-n', '-a', sRsp, 'SUNWvpox']);
        return fRc;

    def _uninstallVPoxOnSolaris(self, fRestartSvcConfigD):
        """ Uninstalls VPox on Solaris."""
        reporter.flushall();
        if utils.processCall(['pkginfo', '-q', 'SUNWvpox']) != 0:
            return True;
        sRsp = self._generateAutoResponseOnSolaris();
        fRc, _ = self._sudoExecuteSync(['pkgrm', '-n', '-a', sRsp, 'SUNWvpox']);

        #
        # Restart the svc.configd as it has a tendency to clog up with time and
        # become  unresponsive.  It will handle SIGHUP by exiting the sigwait()
        # look in the main function and shut down the service nicely (backend_fini).
        # The restarter will then start a new instance of it.
        #
        if fRestartSvcConfigD:
            time.sleep(1); # Give it a chance to flush pkgrm stuff.
            self._sudoExecuteSync(['pkill', '-HUP', 'svc.configd']);
            time.sleep(5); # Spare a few cpu cycles it to shutdown and restart.

        return fRc;

    #
    # Windows
    #

    ## VPox windows services we can query the status of.
    kasWindowsServices = [ 'vpoxdrv', 'vpoxusbmon', 'vpoxnetadp', 'vpoxnetflt', 'vpoxnetlwf' ];

    def _installVPoxOnWindows(self):
        """ Installs VPox on Windows."""
        sExe = self._findFile('^VirtualPox-.*-(MultiArch|Win).exe$');
        if sExe is None:
            return False;

        # TEMPORARY HACK - START
        # It seems that running the NDIS cleanup script upon uninstallation is not
        # a good idea, so let's run it before installing VirtualPox.
        #sHostName = socket.getfqdn();
        #if    not sHostName.startswith('testboxwin3') \
        #  and not sHostName.startswith('testboxharp2') \
        #  and not sHostName.startswith('wei01-b6ka-3') \
        #  and utils.getHostOsVersion() in ['8', '8.1', '9', '2008Server', '2008ServerR2', '2012Server']:
        #    reporter.log('Peforming extra NDIS cleanup...');
        #    sMagicScript = os.path.abspath(os.path.join(g_ksValidationKitDir, 'testdriver', 'win-vpox-net-uninstall.ps1'));
        #    fRc2, _ = self._sudoExecuteSync(['powershell.exe', '-Command', 'set-executionpolicy unrestricted']);
        #    if not fRc2:
        #        reporter.log('set-executionpolicy failed.');
        #    self._sudoExecuteSync(['powershell.exe', '-Command', 'get-executionpolicy']);
        #    fRc2, _ = self._sudoExecuteSync(['powershell.exe', '-File', sMagicScript]);
        #    if not fRc2:
        #        reporter.log('NDIS cleanup failed.');
        # TEMPORARY HACK - END

        # Uninstall any previous vpox version first.
        fRc = self._uninstallVPoxOnWindows('install');
        if fRc is not True:
            return None; # There shouldn't be anything to uninstall, and if there is, it's not our fault.

        # Install the new one.
        asArgs = [sExe, '-vvvv', '--silent', '--logging'];
        asArgs.extend(['--msiparams', 'REBOOT=ReallySuppress']);
        sVPoxInstallPath = os.environ.get('VPOX_INSTALL_PATH', None);
        if sVPoxInstallPath is not None:
            asArgs.extend(['INSTALLDIR="%s"' % (sVPoxInstallPath,)]);

        fRc2, iRc = self._sudoExecuteSync(asArgs);
        if fRc2 is False:
            if iRc == 3010: # ERROR_SUCCESS_REBOOT_REQUIRED
                reporter.error('Installer required a reboot to complete installation (ERROR_SUCCESS_REBOOT_REQUIRED)');
            else:
                reporter.error('Installer failed, exit code: %s' % (iRc,));
            fRc = False;

        sLogFile = os.path.join(tempfile.gettempdir(), 'VirtualPox', 'VPoxInstallLog.txt');
        if os.path.isfile(sLogFile):
            reporter.addLogFile(sLogFile, 'log/installer', "Verbose MSI installation log file");
        self._waitForTestManagerConnectivity(30);
        return fRc;

    def _isProcessPresent(self, sName):
        """ Checks whether the named process is present or not. """
        for oProcess in utils.processListAll():
            sBase = oProcess.getBaseImageNameNoExeSuff();
            if sBase is not None and sBase.lower() == sName:
                return True;
        return False;

    def _killProcessesByName(self, sName, sDesc, fChildren = False):
        """ Kills the named process, optionally including children. """
        cKilled = 0;
        aoProcesses = utils.processListAll();
        for oProcess in aoProcesses:
            sBase = oProcess.getBaseImageNameNoExeSuff();
            if sBase is not None and sBase.lower() == sName:
                reporter.log('Killing %s process: %s (%s)' % (sDesc, oProcess.iPid, sBase));
                utils.processKill(oProcess.iPid);
                cKilled += 1;

                if fChildren:
                    for oChild in aoProcesses:
                        if oChild.iParentPid == oProcess.iPid and oChild.iParentPid is not None:
                            reporter.log('Killing %s child process: %s (%s)' % (sDesc, oChild.iPid, sBase));
                            utils.processKill(oChild.iPid);
                            cKilled += 1;
        return cKilled;

    def _terminateProcessesByNameAndArgSubstr(self, sName, sArg, sDesc):
        """
        Terminates the named process using taskkill.exe, if any of its args
        contains the passed string.
        """
        cKilled = 0;
        aoProcesses = utils.processListAll();
        for oProcess in aoProcesses:
            sBase = oProcess.getBaseImageNameNoExeSuff();
            if sBase is not None and sBase.lower() == sName and any(sArg in s for s in oProcess.asArgs):

                reporter.log('Killing %s process: %s (%s)' % (sDesc, oProcess.iPid, sBase));
                self._executeSync(['taskkill.exe', '/pid', '%u' % (oProcess.iPid,)]);
                cKilled += 1;
        return cKilled;

    def _uninstallVPoxOnWindows(self, sMode):
        """
        Uninstalls VPox on Windows, all installations we find to be on the safe side...
        """
        assert sMode in ['install', 'uninstall',];

        import win32com.client; # pylint: disable=import-error
        win32com.client.gencache.EnsureModule('{000C1092-0000-0000-C000-000000000046}', 1033, 1, 0);
        oInstaller = win32com.client.Dispatch('WindowsInstaller.Installer',
                                              resultCLSID = '{000C1090-0000-0000-C000-000000000046}')

        # Search installed products for VirtualPox.
        asProdCodes = [];
        for sProdCode in oInstaller.Products:
            try:
                sProdName = oInstaller.ProductInfo(sProdCode, "ProductName");
            except:
                reporter.logXcpt();
                continue;
            #reporter.log('Info: %s=%s' % (sProdCode, sProdName));
            if  sProdName.startswith('Oracle VM VirtualPox') \
             or sProdName.startswith('Sun VirtualPox'):
                asProdCodes.append([sProdCode, sProdName]);

        # Before we start uninstalling anything, just ruthlessly kill any cdb,
        # msiexec, drvinst and some rundll process we might find hanging around.
        if self._isProcessPresent('rundll32'):
            cTimes = 0;
            while cTimes < 3:
                cTimes += 1;
                cKilled = self._terminateProcessesByNameAndArgSubstr('rundll32', 'InstallSecurityPromptRunDllW',
                                                                     'MSI driver installation');
                if cKilled <= 0:
                    break;
                time.sleep(10); # Give related drvinst process a chance to clean up after we killed the verification dialog.

        if self._isProcessPresent('drvinst'):
            time.sleep(15);     # In the hope that it goes away.
            cTimes = 0;
            while cTimes < 4:
                cTimes += 1;
                cKilled = self._killProcessesByName('drvinst', 'MSI driver installation', True);
                if cKilled <= 0:
                    break;
                time.sleep(10); # Give related MSI process a chance to clean up after we killed the driver installer.

        if self._isProcessPresent('msiexec'):
            cTimes = 0;
            while cTimes < 3:
                reporter.log('found running msiexec process, waiting a bit...');
                time.sleep(20)  # In the hope that it goes away.
                if not self._isProcessPresent('msiexec'):
                    break;
                cTimes += 1;
            ## @todo this could also be the msiexec system service, try to detect this case!
            if cTimes >= 6:
                cKilled = self._killProcessesByName('msiexec', 'MSI driver installation');
                if cKilled > 0:
                    time.sleep(16); # fudge.

        # cdb.exe sometimes stays running (from utils.getProcessInfo), blocking
        # the scratch directory. No idea why.
        if self._isProcessPresent('cdb'):
            cTimes = 0;
            while cTimes < 3:
                cKilled = self._killProcessesByName('cdb', 'cdb.exe from getProcessInfo');
                if cKilled <= 0:
                    break;
                time.sleep(2); # fudge.

        # Do the uninstalling.
        fRc = True;
        sLogFile = os.path.join(self.sScratchPath, 'VPoxUninstallLog.txt');
        for sProdCode, sProdName in asProdCodes:
            reporter.log('Uninstalling %s (%s)...' % (sProdName, sProdCode));
            fRc2, iRc = self._sudoExecuteSync(['msiexec', '/uninstall', sProdCode, '/quiet', '/passive', '/norestart',
                                               '/L*v', '%s' % (sLogFile), ]);
            if fRc2 is False:
                if iRc == 3010: # ERROR_SUCCESS_REBOOT_REQUIRED
                    reporter.error('Uninstaller required a reboot to complete uninstallation');
                else:
                    reporter.error('Uninstaller failed, exit code: %s' % (iRc,));
                fRc = False;

        self._waitForTestManagerConnectivity(30);

        # Upload the log on failure.  Do it early if the extra cleanups below causes trouble.
        if fRc is False and os.path.isfile(sLogFile):
            reporter.addLogFile(sLogFile, 'log/uninstaller', "Verbose MSI uninstallation log file");
            sLogFile = None;

        # Log driver service states (should ls \Driver\VPox* and \Device\VPox*).
        fHadLeftovers = False;
        asLeftovers = [];
        for sService in reversed(self.kasWindowsServices):
            cTries = 0;
            while True:
                fRc2, _ = self._sudoExecuteSync(['sc.exe', 'query', sService]);
                if not fRc2:
                    break;
                fHadLeftovers = True;

                cTries += 1;
                if cTries > 3:
                    asLeftovers.append(sService,);
                    break;

                # Get the status output.
                try:
                    sOutput = utils.sudoProcessOutputChecked(['sc.exe', 'query', sService]);
                except:
                    reporter.logXcpt();
                else:
                    if re.search(r'STATE\s+:\s*1\s*STOPPED', sOutput) is None:
                        reporter.log('Trying to stop %s...' % (sService,));
                        fRc2, _ = self._sudoExecuteSync(['sc.exe', 'stop', sService]);
                        time.sleep(1); # fudge

                    reporter.log('Trying to delete %s...' % (sService,));
                    self._sudoExecuteSync(['sc.exe', 'delete', sService]);

                time.sleep(1); # fudge

        if asLeftovers:
            reporter.log('Warning! Leftover VPox drivers: %s' % (', '.join(asLeftovers),));
            fRc = False;

        if fHadLeftovers:
            self._waitForTestManagerConnectivity(30);

        # Upload the log if we have any leftovers and didn't upload it already.
        if sLogFile is not None and (fRc is False or fHadLeftovers) and os.path.isfile(sLogFile):
            reporter.addLogFile(sLogFile, 'log/uninstaller', "Verbose MSI uninstallation log file");

        return fRc;


    #
    # Extension pack.
    #

    def _getVPoxInstallPath(self, fFailIfNotFound):
        """ Returns the default VPox installation path. """
        sHost = utils.getHostOs();
        if sHost == 'win':
            sProgFiles = os.environ.get('ProgramFiles', 'C:\\Program Files');
            asLocs = [
                os.path.join(sProgFiles, 'Oracle', 'VirtualPox'),
                os.path.join(sProgFiles, 'OracleVM', 'VirtualPox'),
                os.path.join(sProgFiles, 'Sun', 'VirtualPox'),
            ];
        elif sHost in ('linux', 'solaris',):
            asLocs = [ '/opt/VirtualPox', '/opt/VirtualPox-3.2', '/opt/VirtualPox-3.1', '/opt/VirtualPox-3.0'];
        elif sHost == 'darwin':
            asLocs = [ '/Applications/VirtualPox.app/Contents/MacOS' ];
        else:
            asLocs = [ '/opt/VirtualPox' ];
        if 'VPOX_INSTALL_PATH' in os.environ:
            asLocs.insert(0, os.environ.get('VPOX_INSTALL_PATH', None));

        for sLoc in asLocs:
            if os.path.isdir(sLoc):
                return sLoc;
        if fFailIfNotFound:
            reporter.error('Failed to locate VirtualPox installation: %s' % (asLocs,));
        else:
            reporter.log2('Failed to locate VirtualPox installation: %s' % (asLocs,));
        return None;

    def _installExtPack(self):
        """ Installs the extension pack. """
        sVPox = self._getVPoxInstallPath(fFailIfNotFound = True);
        if sVPox is None:
            return False;
        sExtPackDir = os.path.join(sVPox, 'ExtensionPacks');

        if self._uninstallAllExtPacks() is not True:
            return False;

        sExtPack = self._findFile('Oracle_VM_VirtualPox_Extension_Pack.vpox-extpack');
        if sExtPack is None:
            sExtPack = self._findFile('Oracle_VM_VirtualPox_Extension_Pack.*.vpox-extpack');
        if sExtPack is None:
            return True;

        sDstDir = os.path.join(sExtPackDir, 'Oracle_VM_VirtualPox_Extension_Pack');
        reporter.log('Installing extension pack "%s" to "%s"...' % (sExtPack, sExtPackDir));
        fRc, _ = self._sudoExecuteSync([ self.getBinTool('vts_tar'),
                                         '--extract',
                                         '--verbose',
                                         '--gzip',
                                         '--file',                sExtPack,
                                         '--directory',           sDstDir,
                                         '--file-mode-and-mask',  '0644',
                                         '--file-mode-or-mask',   '0644',
                                         '--dir-mode-and-mask',   '0755',
                                         '--dir-mode-or-mask',    '0755',
                                         '--owner',               '0',
                                         '--group',               '0',
                                       ]);
        return fRc;

    def _uninstallAllExtPacks(self):
        """ Uninstalls all extension packs. """
        sVPox = self._getVPoxInstallPath(fFailIfNotFound = False);
        if sVPox is None:
            return True;

        sExtPackDir = os.path.join(sVPox, 'ExtensionPacks');
        if not os.path.exists(sExtPackDir):
            return True;

        fRc, _ = self._sudoExecuteSync([self.getBinTool('vts_rm'), '-Rfv', '--', sExtPackDir]);
        return fRc;



if __name__ == '__main__':
    sys.exit(VPoxInstallerTestDriver().main(sys.argv));

