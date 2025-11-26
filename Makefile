# Compiler and Flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g

# Targets
all: user ES

# ------------------- User Application -------------------
# O executável 'user' junta 3 ficheiros objeto: user.o, protocols.o e utils.o
user: user.o protocols.o utils.o
	$(CXX) $(CXXFLAGS) -o user user.o protocols.o utils.o

# user.o depende de user.cpp e dos cabeçalhos que ele inclui
user.o: user.cpp common.hpp protocols.hpp utils.hpp
	$(CXX) $(CXXFLAGS) -c user.cpp

# protocols.o depende de protocols.cpp e do common.hpp
protocols.o: protocols.cpp protocols.hpp common.hpp
	$(CXX) $(CXXFLAGS) -c protocols.cpp

# utils.o depende de utils.cpp e dos cabeçalhos que ele inclui
utils.o: utils.cpp utils.hpp protocols.hpp common.hpp
	$(CXX) $(CXXFLAGS) -c utils.cpp

# ------------------- Server Application (ES) -------------------
ES: ES.o
	$(CXX) $(CXXFLAGS) -o ES ES.o

# O ES.o depende do ES.cpp e do common.hpp (se o servidor usar structs)
ES.o: ES.cpp common.hpp
	$(CXX) $(CXXFLAGS) -c ES.cpp

# ------------------- Clean -------------------
clean:
	rm -f *.o user ES

# ------------------- Run Targets (Opcional) -------------------
run_server: ES
	./ES

run_user: user
	./user