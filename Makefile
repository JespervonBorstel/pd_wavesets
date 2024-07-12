# Makefile to build class 'helloworld' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.

# library name
lib.name = pd_wavesets

common.sources = src/utils.c src/analyse.c
# input source file (class name == source file basename)
class.sources = src/wavesetstepper~.c src/wavesetbuffer.c src/wavesetplayer~.c

# all extra files to be included in binary distribution of the library
datafiles = wavesetstepper~-help.pd

PDINCLUDEDIR = /usr/include

# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
