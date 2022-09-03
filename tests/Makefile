###################################################
# Project definition
#
PROJECT = Zipper
TARGET = $(PROJECT)-UnitTest
DESCRIPTION = Unit tests for $(PROJECT)
BUILD_TYPE = debug
USE_COVERAGE = 1

###################################################
# Location of the project directory and Makefiles
#
P := ..
M := $(P)/.makefile
include $(M)/Makefile.header

###################################################
# Inform Makefile where to find *.cpp and *.o files
#
VPATH += $(P) $(P)/src $(P)/src/utils

###################################################
# Inform Makefile where to find header files
#
INCLUDES += -I$(P) -I$(P)/include -I$(P)/src

###################################################
# Project defines.
#
DEFINES += -DPWD=\"$(shell pwd)\"
DEFINES += -DHAVE_AES
ifeq ($(ARCHI),Windows)
DEFINES += -DUSE_WINDOWS
else
DEFINES += -UUSE_WINDOWS
endif

###################################################
# Compiled files
#
OBJS += TestPath.o TestZip.o main.o

###################################################
# Libraries.
#
PKG_LIBS += gtest gmock
THIRDPART_LIBS += $(abspath $(P)/build/libzipper.a.$(shell cat $(P)/VERSION.txt))

###################################################
# Compile unit tests
all: $(TARGET)

###################################################
# Run unit tests (better to call: make coverage)
.PHONY: unit-tests
.PHONY: check
unit-tests check: $(TARGET)
	@$(call print-to,"Running","$(TARGET)","$(RAPPORT)","")
	$(SANITIZER) ./$(BUILD)/$(TARGET) || (cat $(TARGET).log; return 1)

###################################################
# Sharable informations between all Makefiles
include $(M)/Makefile.footer
