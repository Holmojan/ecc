CPP      = g++.exe
CC       = gcc.exe
OBJ      = main.o
LINKOBJ  = main.o
BIN      = main.exe
CXXFLAGS = -o3 -std=c++11 -Wno-invalid-offsetof -fpermissive
CFLAGS   = -o3 -std=c++11 -Wno-invalid-offsetof -fpermissive
RM       = rm.exe

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -o $(BIN) $(LIBS)

main.o: main.cpp
	$(CPP) -c  $(CXXFLAGS) main.cpp -o main.o