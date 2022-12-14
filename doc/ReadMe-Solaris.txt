
@VPOX_PRODUCT@ for Oracle Solaris (TM) Operating System
--------------------------------------------------------

Upgrading:
----------

If you have an existing VirtualPox installation and you are upgrading to
a newer version of VirtualPox, please uninstall the previous version
before installing a newer one. Please refer to the "Uninstalling" section
at the end of this document for details.


Installing:
-----------

After extracting the contents of the tar.gz file perform the following steps:

1. Login as root using the "su" command.

2. Install the VirtualPox package:

      pkgadd -d VirtualPox-@VPOX_VERSION_STRING@-SunOS-@BUILD_TARGET_ARCH@-r@VPOX_SVN_REV@.pkg

      To perform an unattended (non-interactive) installation of this
      package, add "-n -a autoresponse SUNWvpox" (without quotes)
      to the end of the above pkgadd command.

3. For each package, the installer will ask you to "Select package(s) you
   wish to process". In response, type "1".

4. Type "y" when asked about continuing the installation.

At this point, all the required files should be installed on your system.
You can launch VirtualPox by running 'VirtualPox' from the terminal.


Uninstalling:
-------------

To remove VirtualPox from your system, perform the following steps:

1. Login as root using the "su" command.

2. To remove VirtualPox, run the command:
        pkgrm SUNWvpox

