makedir:=$(makedir)/Terminal

sources+=$(call List,terminal/Sourcefile)

opts+=-D__TERMINAL__ -DHAVE_TERMIOS

ifdef FREEBSD
# on FreeBSD, we have to link to libpthread explicitly
libs+=-lpthread
endif

ifdef MINGW
libs+=-mconsole
endif

NOOPENMPT=1
NOGME=1
NOHW=1
NOUPNP=1
