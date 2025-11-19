# Compiler and Flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g

# Targets
all: user ES

# --- User Application ---
user: user.o
	$(CXX) $(CXXFLAGS) -o user user.o

# user.o depende de user.cpp e user.hpp
user.o: user.cpp user.hpp
	$(CXX) $(CXXFLAGS) -c user.cpp

# --- Server Application (ES) ---
ES: ES.o
	$(CXX) $(CXXFLAGS) -o ES ES.o

ES.o: ES.cpp
	$(CXX) $(CXXFLAGS) -c ES.cpp

# --- Clean ---
clean:
	rm -f *.o user ES