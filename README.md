fs-utils: File System Access Utilities in Userland [![Build Status](https://travis-ci.org/rumpkernel/fs-utils.png?branch=master)](https://travis-ci.org/rumpkernel/fs-utils)
==================================================

The goal of fs-utils is to provide a set of utilities for accessing and
modifying file system images without having to use host kernel mounts.
To use fs-utils you do not have to be root, you just need read/write
access to the image or device.  The advantage of fs-utils over similar
projects such as mtools is supporting the usage of familiar Unix tools
(`ls`, `cp`, `mv`, etc.) for a large number of file systems.

For information on installing/building and using, see
the [the wiki](http://wiki.rumpkernel.org/Repo:-fs-utils).
