# Makefile to build class 'helloworld' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.

# library name
lib.name = pd_waveset

# input source file (class name == source file basename)
class.sources = wavesetplayer~.c; wavesetstepper~.c

# all extra files to be included in binary distribution of the library
datafiles =

PDINCLUDEDIR =/usr/include

# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
