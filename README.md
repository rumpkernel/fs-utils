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

The project has reached basic stability and are able to modify a number
of different types of file system images on Linux and Solaris-derived
hosts.  Cygwin is also supported, but will not work out-of-the-box due
to disagreements with libtool.

Future work on fs-utils will concentrate on documentation, diagnostic
messages, increasing platform support, testing, and possibly some new
utilities.

Supported File Systems
----------------------
- block device based file systems: cd9660, efs, ext2, hfs, ffs, fat, lfs, ntfs, sysvbfs, udf, v7fs
- network based file systems: nfs, smbfs (experimental, add --enable-smbfs to the configure script to enable it)

Utilities
---------

The following operate on a file system image like their standard Unix
counterparts:

- `fsu_cat`, `fsu_chmod`, `fsu_chown`, `fsu_cp`, `fsu_df`, `fsu_diff`, `fsu_du`, `fsu_find`, `fsu_ln`, `fsu_ls`, `fsu_mkdir`, `fsu_mv`, `fsu_rm`, `fsu_rmdir`, `fsu_mknod`, `fsu_mkfifo`, `fsu_chflags`, `fsu_touch`, `fsu_stat`

Since the host kernel does not have knowledge of the image, we supply
additional tools to preserve normal work patterns:

- `fsu_ecp`, `fsu_get`, `fsu_put`: external copy; copy files between the host and image (`fsu_cp` copies within the image).
- `fsu_write`: write stdin to a file within the image.  This can be used where shell output redirection would normally be used.
- `fsu_exec`: execute a program from the image.  This is shorthand for `fsu_ecp` + execute + `rm`.

Usage examples
--------------

The example is about modifying an initially empty FFS image:

    $ fsu_ls test.ffs -la
    total 4
    drwxrwxr-x  2 0  0  512 Apr  9 12:45 .
    drwxrwxr-x  2 0  0  512 Apr  9 12:45 ..
    $ echo just a demo | fsu_write test.ffs a_file.txt
    $ fsu_ls test.ffs -l
    total 2
    -rw-r--r--  1 0  0  12 Apr  9 12:45 a_file.txt
    $ fsu_ln test.ffs -s a_file.txt a_link.txt
    $ fsu_ls test.ffs -l
    total 2
    -rw-r--r--  1 0  0  12 Apr  9 12:45 a_file.txt
    lrwxr-xr-x  1 0  0  10 Apr  9 12:49 a_link.txt -> a_file.txt
    $ fsu_cat test.ffs a_link.txt
    just a demo
    $ 

The second example is about examining the contents of a downloaded ISO image:

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
    $ fsu_du NetBSD-6.1_RC1-amd64.iso -hc amd64 lib
    22M     amd64/binary/kernel
    246M    amd64/binary/sets
    268M    amd64/binary
    14M     amd64/installation/floppy
    1.1M    amd64/installation/miniroot
    54K     amd64/installation/misc
    15M     amd64/installation
    284M    amd64
    4.9M    lib
    289M    total
    $ 

Installation Instructions
=========================

Binary Packages
---------------

If available, installing a binary package is recommended.
Known packages are:

* Void Linux: `xbps-install -S fs-utils`
* Arch Linux: [pacman](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=Arch_Core) (OBS), [AUR](https://aur.archlinux.org/packages/netbsd-fs-utils-git/)
* OpenSUSE Linux:
12.3 [RPM](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=openSUSE_12.3) (OBS)
|| Tumbleweed [RPM](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=openSUSE_Factory) (OBS)
|| Factory [RPM](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=openSUSE_Factory) (OBS)
|| SLE_11_SP2 [RPM](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=SLE_11_SP2) (OBS)
* Fedora Linux:
17 [RPM](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=Fedora_17) (OBS)
|| 18 [RPM](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=Fedora_18) (OBS)
|| RHEL 6 [RPM](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=RedHat_RHEL-6) (OBS)
|| CentOS 6 [RPM](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=CentOS_CentOS-6) (OBS)
* Mandriva Linux 2011: [RPM](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=Mandriva_2011) (OBS)
* Debian Linux:
7 [DEB](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=Debian_7.0) (OBS)
* Ubuntu Linux:
13.04 [DEB](https://build.opensuse.org/package/binaries?package=fs-utils&project=home%3Astaal1978&repository=xUbuntu_13.04) (OBS)
* NetBSD: pkgsrc/filesystems/fs-utils
* Solaris: pkgsrc/filesystems/fs-utils

The links for some of packages are provided by the
[openSUSE Build Service](https://build.opensuse.org/package/show?package=rump&project=home%3Astaal1978).
You can download and install the packages manually, but in this case you
first need to ensure that rump kernel components are installed
(available as binary packages
[here](https://github.com/anttikantee/buildrump.sh)).
It is recommended to add the OBS repositories for the right distro
and architecture to the package manager. This way, dependencies will
be automatically resolved and updates will installed when available.
Consult your distribution's documentation for further instructions.


Building from Source
--------------------

When building from source, you must first ensure the prerequisites are
available.  fs-utils uses file system drivers provided by rump kernels.
Build and install them in the following manner:

- Clone the buildrump.sh repository (http://github.com/anttikantee/buildrump.sh)
- Build and install: `./buildrump.sh -d destbase checkout fullbuild`

The destbase directory can be e.g. `/usr/local`.

Then, in the fs-utils directory:

* `./configure`
* `make`

Note: if you installed rump kernel components to a non-standard directory,
in the normal autoconf fashion you need to set `CPPFLAGS` and `LDFLAGS`
before running configure.
