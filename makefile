# the compiler: gcc for C program, define as g++ for C++
CC = mpiCC

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -std=c++11 -fopenmp -ggdb

# Complementary .cpp files:
CPPFILES = Materials.cpp GridCreator.cpp MPI_Initializer.cpp\
			InputParser.cpp ElectromagneticSource.cpp

# the build target executable:
TARGET = main

all: $(TARGET)

# Use -DDEBUG=1, 2 or 3. The high DEBUG the more verbose the program.

$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -DDEBUG=3 -o $(TARGET) $(TARGET).cpp $(CPPFILES)

clean:
	$(RM) $(TARGET)
