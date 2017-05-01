#==============================================================================
# makefile


#------------------------------------------------------------------------------
# source & target

# カレントディレクトの*.cppをbuid対象にする
SRCS = $(wildcard *.cpp)

# カレントディレクト名をターゲット名称にする。
ifeq ($(OS),Windows_NT)
TARGET = $(notdir $(CURDIR)).exe
else
TARGET = $(notdir $(CURDIR))
endif

OBJS 	= $(patsubst %.cpp,%.o,$(SRCS))
DEPENDS	= $(patsubst %.cpp,%.d,$(SRCS))

#------------------------------------------------------------------------------
# build type
#BUILD_TYPE = Debug


#------------------------------------------------------------------------------
# tool chain & options

CC = clang++
CFLAGS = -Wall -std=c++14
#CFLAGS = -Wall

ifeq ($(BUILD_TYPE),Debug)
CFLAGS += -g -O0
else
CFLAGS += -O3
endif
#CFLAGS += -ferror-limit=1
#CFLAGS += -v

#ifeq ($(OS),Windows_NT)
#else
#FLAGS += -stdlib=libc++
#endif

#ifeq ($(OS),Windows_NT)
#else
#LFLAGS = -lstdc++
#endif


#------------------------------------------------------------------------------
# phony command
.PHONY: all
all:	build

.PHONY: build
build:	$(TARGET) 	

.PHONY: clean
clean:
	rm -f $(TARGET) $(DEPENDS) $(OBJS)

.PHONY: run
run:	build
	@./$(TARGET)

#------------------------------------------------------------------------------
# make rule
%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

%.d: %.cpp
	$(CC) -c $(CFLAGS) $< -MQ $(@:.d=.o) -MM > $@

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

-include $(DEPENDS)


