PROJECT = Zipper
TARGET = $(PROJECT)-UnitTest
DESCRIPTION = Unit tests for $(PROJECT)
BUILD_TYPE = debug
USE_COVERAGE = 1

P := ..
M := $(P)/.makefile
include $(M)/Makefile.header

DEFINES += -DPWD=\"$(shell pwd)\"

VPATH += $(P) $(P)/src $(P)/src/utils
INCLUDES += -I$(P) -I$(P)/src

OBJS += TestPath.o Path.o
OBJS += main.o

PKG_LIBS += gtest gmock

all: $(TARGET)

.PHONY: unit-tests
.PHONY: check
unit-tests check: $(TARGET)
	@$(call print-to,"Running","$(TARGET)","$(RAPPORT)","")
	$(SANITIZER) ./$(BUILD)/$(TARGET) || (cat $(TARGET).log; return 1)

include $(M)/Makefile.footer
