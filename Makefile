CPP_FLAGS += -DLIN64

MAIN 	= main
# RCNF	= readCnf # for src folder and not src2
# VERIFIER = verifier # for src folder and not src2

ABC_PATH = ./dependencies/abc
CADICAL_PATH = ./dependencies/cadical
KISSAT_PATH = ./dependencies/kissat

ifndef CXX
CXX = g++
endif

# SRCDIR   = src # for src folder and not src2
# OBJDIR   = obj # for src folder and not src2
SRCDIR   = src2
OBJDIR   = obj2
BINDIR   = bin

TARGET_MAIN  = $(BINDIR)/$(MAIN)
# TARGET_RCNF  = $(BINDIR)/$(RCNF) # for src folder and not src2
# TARGET_VERIFIER = $(BINDIR)/$(VERIFIER) # for src folder and not src2

ABC_INCLUDES = -I $(ABC_PATH) -I $(ABC_PATH)/src
CADICAL_INCLUDES = -I $(CADICAL_PATH)/src
# LIB_DIRS = -L $(ABC_PATH)/ -L $(CADICAL_PATH)/build/ # for src folder and not src2
# DIR_INCLUDES = $(ABC_INCLUDES) $(LIB_DIRS) $(CADICAL_INCLUDES) # for src folder and not src2
KISSAT_INCLUDES = -I $(KISSAT_PATH)/src

LIB_DIRS = -L $(ABC_PATH)/ -L $(CADICAL_PATH)/build/ -L $(KISSAT_PATH)/build/
DIR_INCLUDES = $(ABC_INCLUDES) $(CADICAL_INCLUDES) $(KISSAT_INCLUDES) $(LIB_DIRS)

LIB_ABC    = -Wl,-Bstatic  -labc
LIB_COMMON = -Wl,-Bdynamic -lm -ldl -lreadline -ltermcap -lpthread -fopenmp -lrt -Wl,-Bdynamic -lboost_program_options -Wl,-Bdynamic -lz
LIB_CADICAL = -Wl,-Bstatic -lcadical
LIB_KISSAT = -Wl,-Bstatic -lkissat

CPP_FLAGS += -std=c++17 -DNO_UNIGEN
# LFLAGS    = $(DIR_INCLUDES) $(LIB_ABC) $(LIB_CADICAL) $(LIB_COMMON) # for src folder and not src2
LFLAGS    = $(DIR_INCLUDES) $(LIB_ABC) $(LIB_CADICAL) $(LIB_KISSAT) $(LIB_COMMON)

# CPP_FLAGS += -O3 -g -pg
CPP_FLAGS += -O0 -g

# COMMON_SOURCES  = $(SRCDIR)/helper.cpp $(SRCDIR)/nnf.cpp # for src folder and not src2
SOURCES := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# MAIN_SOURCES  = $(SRCDIR)/main.cpp $(COMMON_SOURCES) # for src folder and not src2
# RCNF_SOURCES  = $(SRCDIR)/readCnf.cpp # for src folder and not src2
# VERIFIER_SOURCES = $(SRCDIR)/verifier.cpp $(COMMON_SOURCES) # for src folder and not src2
# MAIN_OBJECTS  = $(MAIN_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o) # for src folder and not src2
# VERIFIER_OBJECTS  = $(VERIFIER_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o) # for src folder and not src2
.PHONY: all
all: main

# ALL_OBJECTS   = $(sort $(MAIN_OBJECTS) $(VERIFIER_OBJECTS)) # for src folder and not src2

# .PHONY: all # for src folder and not src2
# all: main verifier # for src folder and not src2
main: directories $(TARGET_MAIN)
# readCnf: directories $(TARGET_RCNF) # for src folder and not src2
# verifier: directories $(TARGET_VERIFIER) # for src folder and not src2


directories:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

# $(TARGET_MAIN): $(MAIN_OBJECTS) # for src folder and not src2
$(TARGET_MAIN): $(OBJECTS)
	$(CXX) $(CPP_FLAGS) -o $@ $^ $(LFLAGS)
	@echo "Built Target! - main"

# $(TARGET_RCNF): $(RCNF_SOURCES) # for src folder and not src2
# 	$(CXX) $(CPP_FLAGS) $^ -o $@ # for src folder and not src2
# 	@echo "Compiled "$^" successfully!" # for src folder and not src2
# 	@echo "Built Target! - readCnf" # for src folder and not src2

# $(TARGET_VERIFIER): $(VERIFIER_OBJECTS) # for src folder and not src2
# 	$(CXX) $(CPP_FLAGS) -o $@ $^ $(LFLAGS) # for src folder and not src2
# 	@echo "Built Target! - verifier" # for src folder and not src2

# $(ALL_OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp # for src folder and not src2
# 	$(CXX) $(CPP_FLAGS) -c $^ -o $@  $(LFLAGS) # for src folder and not src2
$(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	$(CXX) $(CPP_FLAGS) -c $^ -o $@  $(DIR_INCLUDES)
	@echo "Compiled "$<" successfully!"	

clean:
	# @$(RM) $(MAIN_OBJECTS) $(POSTP_OBJECTS) $(VERIFIER_OBJECTS) # for src folder and not src2
	@$(RM) $(OBJECTS)
	@echo "Cleanup complete!"

remove: clean
	# @$(RM) $(TARGET_MAIN) $(TARGET_POSTP) $(TARGET_RCNF) $(TARGET_VERIFIER) # for src folder and not src2
	@$(RM) $(TARGET_MAIN)
	@echo "Executable removed!"