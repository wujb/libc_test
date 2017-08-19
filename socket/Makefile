###########################################################################
# CopyRight@junbiaow
# 2017-08-17
###########################################################################

CROSS_COMPILER ?= 

TARGET := demo
GCC := $(CROSS_COMPILER)gcc
default: all

all: $(TARGET)

CFLAGS := -Werror

$(TARGET): main.o
	$(GCC) $(CFLAGS) $< -o $@ $(LIB_INC) $(LIB) 

.c.o:
	$(GCC) -c $^ -o $@ 

clean:
	rm *.o -rf
	rm $(TARGET)

.PHONY:
	$(TARGET)
