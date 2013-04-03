fs-utils: File System Access Utilities in Userland
==================================================

About
-----
The aim of this project is to have a set of utilities to access and
modify a file system image without having to mount it.  To use fs-utils
you do not have to be root, you just need read/write access to the
image or device.  The advantage of fs-utils over similar projects such
as mtools is supporting the usage of familiar Unix tools (`ls`, `cp`,
`mv`, etc.) for a large number of file systems.

Supported File Systems
----------------------
- block device based file systems: cd9660, efs, ext2, hfs, ffs, fat, lfs, ntfs, sysvbfs, udf, v7fs
- network based file systems: nfs

Instructions
------------

fs-utils uses file system drivers provided by rump kernels.  If your
system does not supply rump kernels, build and install them in the
following manner:

- Clone the buildrump.sh repository (http://github.com/anttikantee/buildrump.sh)
- Build and install: `./buildrump.sh -d destbase checkout fullbuild`

The destbase directory can be e.g. `/usr/local`.

Then, in the fs-utils directory:

* `./configure`
* `make`

Note: if you installed rump kernel components to a non-standard directory,
in the normal autoconf fashion you need to set `CPPFLAGS` and `LDFLAGS`
before running configure.

Usage examples
--------------

    $ fsu_ls ~/NetBSD-6.1_RC1-amd64.iso -l
    total 12584
    drwxr-xr-x   4 611  0     2048 Feb 19 20:26 amd64
    drwxr-xr-x   2 611  0     6144 Feb 19 20:13 bin
    -r--r--r--   1 611  0    73276 Feb 19 20:29 boot
    -rw-r--r--   1 611  0      555 Feb 19 20:29 boot.cfg
    drwxr-xr-x   2 611  0     2048 Feb 19 20:29 dev
    drwxr-xr-x  26 611  0    12288 Feb 19 20:29 etc
    -r-xr-xr-x   1 611  0     3084 Feb 19 20:29 install.sh
    drwxr-xr-x   2 611  0    10240 Feb 19 20:13 lib
    drwxr-xr-x   3 611  0     2048 Feb 19 19:51 libdata
    drwxr-xr-x   4 611  0     2048 Feb 19 20:29 libexec
    drwxr-xr-x   2 611  0     2048 Feb 19 19:51 mnt
    drwxr-xr-x   2 611  0     2048 Feb 19 20:29 mnt2
    -rw-r--r--   1 611  0  5979454 Feb 19 20:29 netbsd
    drwxr-xr-x   2 611  0    18432 Feb 19 20:13 sbin
    drwxr-xr-x   3 611  0     2048 Feb 19 19:59 stand
    -rwxr-xr-x   1 611  0   195255 Feb 19 20:29 sysinst
    -rwxr-xr-x   1 611  0    32253 Feb 19 20:29 sysinstmsgs.de
    -rwxr-xr-x   1 611  0    31278 Feb 19 20:29 sysinstmsgs.es
    -rwxr-xr-x   1 611  0    32005 Feb 19 20:29 sysinstmsgs.fr
    -rwxr-xr-x   1 611  0    27959 Feb 19 20:29 sysinstmsgs.pl
    drwxr-xr-x   2 611  0     2048 Feb 19 20:29 targetroot
    drwxr-xr-x   2 611  0     2048 Feb 19 19:51 tmp
    drwxr-xr-x   8 611  0     2048 Feb 19 20:29 usr
    drwxr-xr-x   2 611  0     2048 Feb 19 20:29 var

    $ fsu_cat ~/NetBSD-6.1_RC1-amd64.iso /boot.cfg
    banner=Welcome to the NetBSD 6.1_RC1 installation CD
    banner================================================================================
    banner=
    banner=ACPI (Advanced Configuration and Power Interface) should work on all modern
    banner=and legacy hardware.  However if you do encounter a problem while booting,
    banner=try disabling it and report a bug at http://www.NetBSD.org/.
    menu=Install NetBSD:boot netbsd
    menu=Install NetBSD (no ACPI):boot netbsd -2
    menu=Install NetBSD (no ACPI, no SMP):boot netbsd -12
    menu=Drop to boot prompt:prompt
    timeout=30

