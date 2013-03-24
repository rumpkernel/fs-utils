OSNAME=	$(shell uname -s)
ENABLE_NFS?= 0
RUMPBASE?=${TOP}/../buildrump.sh/rump
COPTS=-g
