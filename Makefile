CPP_FLAGS += -DLIN64

MAIN 	= main

ABC_PATH = ./dependencies/abc

ifndef CXX
CXX = g++
endif

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

TARGET_MAIN  = $(BINDIR)/$(MAIN)

ABC_INCLUDES = -I $(ABC_PATH) -I $(ABC_PATH)/src
LIB_DIRS = -L $(ABC_PATH)/
DIR_INCLUDES = $(ABC_INCLUDES) $(LIB_DIRS)

LIB_ABC    = -Wl,-Bstatic  -labc
LIB_COMMON = -Wl,-Bdynamic -lm -ldl -lreadline -ltermcap -lpthread -fopenmp -lrt -Wl,-Bdynamic -lboost_program_options -Wl,-Bdynamic -lz

CPP_FLAGS += -std=c++17 -DNO_UNIGEN
LFLAGS    = $(DIR_INCLUDES) $(LIB_ABC) $(LIB_COMMON)

CPP_FLAGS += -O3 -g -pg

COMMON_SOURCES  = $(SRCDIR)/helper.cpp

MAIN_SOURCES  = $(SRCDIR)/main.cpp $(COMMON_SOURCES)
MAIN_OBJECTS  = $(MAIN_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

ALL_OBJECTS   = $(sort $(MAIN_OBJECTS))

.PHONY: all
all: main
main: directories $(TARGET_MAIN)


directories:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

$(TARGET_MAIN): $(MAIN_OBJECTS)
	$(CXX) $(CPP_FLAGS) -o $@ $^ $(LFLAGS)
	@echo "Built Target! - main"

$(ALL_OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	$(CXX) $(CPP_FLAGS) -c $^ -o $@  $(LFLAGS)
	@echo "Compiled "$<" successfully!"	

clean:
	@$(RM) $(MAIN_OBJECTS) $(POSTP_OBJECTS)
	@echo "Cleanup complete!"

remove: clean
	@$(RM) $(TARGET_MAIN) $(TARGET_POSTP)
	@echo "Executable removed!"