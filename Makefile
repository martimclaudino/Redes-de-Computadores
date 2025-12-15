# Compiler and Flags
CXX = g++

# Directories (Caminhos para as pastas)
COMMON_DIR = common
SRC_DIR = src
USER_DIR = $(SRC_DIR)/UserApp
ES_DIR = $(SRC_DIR)/ESDIR

# Include Flags:
# -I.             : Procura na raiz
# -I$(COMMON_DIR) : Procura em 'common' (para common.hpp e protocols.hpp)
# -I$(USER_DIR)   : Procura em 'src/UserApp' (para utils.hpp)
CXXFLAGS = -Wall -Wextra -std=c++17 -g -I. -I$(COMMON_DIR) -I$(USER_DIR)

# Targets
all: user ES

# ------------------- Shared Objects -------------------
# protocols.o agora vive dentro da pasta common
$(COMMON_DIR)/protocols.o: $(COMMON_DIR)/protocols.cpp $(COMMON_DIR)/protocols.hpp $(COMMON_DIR)/common.hpp
	$(CXX) $(CXXFLAGS) -c $(COMMON_DIR)/protocols.cpp -o $(COMMON_DIR)/protocols.o

# ------------------- User Application -------------------
# O executável 'user' fica na raiz
user: $(USER_DIR)/user.o $(COMMON_DIR)/protocols.o $(USER_DIR)/utils.o
	$(CXX) $(CXXFLAGS) -o user $(USER_DIR)/user.o $(COMMON_DIR)/protocols.o $(USER_DIR)/utils.o

# Compilar user.cpp
$(USER_DIR)/user.o: $(USER_DIR)/user.cpp $(COMMON_DIR)/common.hpp $(COMMON_DIR)/protocols.hpp $(USER_DIR)/utils.hpp
	$(CXX) $(CXXFLAGS) -c $(USER_DIR)/user.cpp -o $(USER_DIR)/user.o

# Compilar utils.cpp
$(USER_DIR)/utils.o: $(USER_DIR)/utils.cpp $(USER_DIR)/utils.hpp $(COMMON_DIR)/protocols.hpp $(COMMON_DIR)/common.hpp
	$(CXX) $(CXXFLAGS) -c $(USER_DIR)/utils.cpp -o $(USER_DIR)/utils.o

# ------------------- Server Application (ES) -------------------
# 1. O executável precisa de ES.o E utils.o
ES: $(ES_DIR)/ES.o $(ES_DIR)/utils.o
	$(CXX) $(CXXFLAGS) -o ES $(ES_DIR)/ES.o $(ES_DIR)/utils.o

# 2. Compilar ES.cpp 
$(ES_DIR)/ES.o: $(ES_DIR)/ES.cpp $(COMMON_DIR)/common.hpp $(ES_DIR)/utils.hpp
	$(CXX) $(CXXFLAGS) -c $(ES_DIR)/ES.cpp -o $(ES_DIR)/ES.o

# 3. Compilar utils.cpp do servidor
$(ES_DIR)/utils.o: $(ES_DIR)/utils.cpp $(ES_DIR)/utils.hpp $(COMMON_DIR)/common.hpp
	$(CXX) $(CXXFLAGS) -c $(ES_DIR)/utils.cpp -o $(ES_DIR)/utils.o

# ------------------- Clean -------------------
# Limpa os .o em todas as sub-pastas
clean:
	rm -f *.o $(USER_DIR)/*.o $(ES_DIR)/*.o $(COMMON_DIR)/*.o user ES

# ------------------- Run Targets -------------------

# Truque: Entra na pasta src/ESDIR e corre o executável que está na raiz
# Nota: Como ESDIR está dentro de src, precisamos de recuar 2 vezes (../../ES)
run_server: ES
	@echo "Running Server inside $(ES_DIR)..."
	cd $(ES_DIR) && ../../ES

run_user: user
	./user