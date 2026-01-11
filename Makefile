CPP_FLAGS += -DLIN64

MAIN 	= main
RCNF	= readCnf
VERIFIER = verifier

ABC_PATH = ./dependencies/abc
CADICAL_PATH = ./dependencies/cadical

ifndef CXX
CXX = g++
endif

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

TARGET_MAIN  = $(BINDIR)/$(MAIN)
TARGET_RCNF  = $(BINDIR)/$(RCNF)
TARGET_VERIFIER = $(BINDIR)/$(VERIFIER)

ABC_INCLUDES = -I $(ABC_PATH) -I $(ABC_PATH)/src
CADICAL_INCLUDES = -I $(CADICAL_PATH)/src
LIB_DIRS = -L $(ABC_PATH)/ -L $(CADICAL_PATH)/build/
DIR_INCLUDES = $(ABC_INCLUDES) $(LIB_DIRS) $(CADICAL_INCLUDES)

LIB_ABC    = -Wl,-Bstatic  -labc
LIB_COMMON = -Wl,-Bdynamic -lm -ldl -lreadline -ltermcap -lpthread -fopenmp -lrt -Wl,-Bdynamic -lboost_program_options -Wl,-Bdynamic -lz
LIB_CADICAL = -Wl,-Bstatic -lcadical

CPP_FLAGS += -std=c++17 -DNO_UNIGEN
LFLAGS    = $(DIR_INCLUDES) $(LIB_ABC) $(LIB_CADICAL) $(LIB_COMMON)

# CPP_FLAGS += -O3 -g -pg
CPP_FLAGS += -O0 -g

COMMON_SOURCES  = $(SRCDIR)/helper.cpp $(SRCDIR)/nnf.cpp

MAIN_SOURCES  = $(SRCDIR)/main.cpp $(COMMON_SOURCES)
RCNF_SOURCES  = $(SRCDIR)/readCnf.cpp
VERIFIER_SOURCES = $(SRCDIR)/verifier.cpp $(COMMON_SOURCES)
MAIN_OBJECTS  = $(MAIN_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
VERIFIER_OBJECTS  = $(VERIFIER_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

ALL_OBJECTS   = $(sort $(MAIN_OBJECTS) $(VERIFIER_OBJECTS))

.PHONY: all
all: main verifier
main: directories $(TARGET_MAIN)
readCnf: directories $(TARGET_RCNF)
verifier: directories $(TARGET_VERIFIER)


directories:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

$(TARGET_MAIN): $(MAIN_OBJECTS)
	$(CXX) $(CPP_FLAGS) -o $@ $^ $(LFLAGS)
	@echo "Built Target! - main"

$(TARGET_RCNF): $(RCNF_SOURCES)
	$(CXX) $(CPP_FLAGS) $^ -o $@
	@echo "Compiled "$^" successfully!"
	@echo "Built Target! - readCnf"

$(TARGET_VERIFIER): $(VERIFIER_OBJECTS)
	$(CXX) $(CPP_FLAGS) -o $@ $^ $(LFLAGS)
	@echo "Built Target! - verifier"

$(ALL_OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	$(CXX) $(CPP_FLAGS) -c $^ -o $@  $(LFLAGS)
	@echo "Compiled "$<" successfully!"	

clean:
	@$(RM) $(MAIN_OBJECTS) $(POSTP_OBJECTS) $(VERIFIER_OBJECTS)
	@echo "Cleanup complete!"

remove: clean
	@$(RM) $(TARGET_MAIN) $(TARGET_POSTP) $(TARGET_RCNF) $(TARGET_VERIFIER)
	@echo "Executable removed!"