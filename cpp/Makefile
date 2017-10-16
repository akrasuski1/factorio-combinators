PROJ:=simulator

SRC:=src
OBJ:=obj

OPT := -O2
DBG := -g3
WARN:= -Wall -Wextra

FLAGS:=-I"inc"
FLAGS+=$(OPT)
FLAGS+=$(DBG)
FLAGS+=$(WARN)
FLAGS+=-MMD -MP

C_FLAGS:=   $(FLAGS) -std=c11
CPP_FLAGS:= $(FLAGS) -std=c++14
S_FLAGS:=   $(FLAGS)

LIBS:=-lz

#----------------------------------------------------------------

C_SOURCES:=$(shell find $(SRC) -name *.c)
C_OBJECTS:=$(subst .c,.o,$(subst $(SRC),$(OBJ),$(C_SOURCES)))

CPP_SOURCES:=$(shell find $(SRC) -name *.cpp)
CPP_OBJECTS:=$(subst .cpp,.o,$(subst $(SRC),$(OBJ),$(CPP_SOURCES)))

S_SOURCES:=$(shell find $(SRC) -name *.S)
S_OBJECTS:=$(subst .S,.o,$(subst $(SRC),$(OBJ),$(S_SOURCES)))

OBJECTS:=$(C_OBJECTS) $(CPP_OBJECTS) $(S_OBJECTS)

ELFFILE:=$(OBJ)/$(PROJ).elf

SRC_DIRS:=$(shell find $(SRC) -type d)
OBJ_DIRS:=$(subst $(SRC),$(OBJ),$(SRC_DIRS))

DEPS:=$(OBJECTS:.o=.d)

all: directories $(ELFFILE)

$(C_OBJECTS): $(OBJ)/%.o: $(SRC)/%.c
	@echo "Compiling $<"
	gcc -c $< $(C_FLAGS) -o $@
	@echo
flags_c:
	@echo $(C_FLAGS)
$(CPP_OBJECTS): $(OBJ)/%.o: $(SRC)/%.cpp
	@echo "Compiling $<"
	g++ -c $< $(CPP_FLAGS) -o $@
	@echo
flags_cpp:
	@echo $(CPP_FLAGS)
$(S_OBJECTS): $(OBJ)/%.o: $(SRC)/%.S
	@echo "Compiling $<"
	gcc -c $< $(S_FLAGS) -o $@
	@echo
flags_S:
	@echo $(S_FLAGS)

directories:
	@mkdir -p $(OBJ_DIRS)

$(ELFFILE): $(OBJECTS)
	@echo "Linking final ELF..."
	g++ $(LD_FLAGS) -o $(ELFFILE) $(OBJECTS) $(LIBS)
	@echo

clean:
	@echo "Cleaning objects..."
	-rm -rf $(OBJ)/*
	@echo

.PHONY: all clean directories flags_c flags_cpp flags_S

-include $(DEPS)
