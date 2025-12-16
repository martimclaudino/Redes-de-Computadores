# Compiler and Flags
CXX = g++

# Directories
COMMON_DIR = common
SRC_DIR = src
USER_DIR = $(SRC_DIR)/UserApp
ES_DIR = $(SRC_DIR)/ESDIR

# Include Flags
CXXFLAGS = -Wall -Wextra -std=c++17 -g -I. -I$(COMMON_DIR) -I$(USER_DIR) -I$(ES_DIR)

# Targets
all: user ES

# ------------------- Shared Objects (Common) -------------------

# 1. Compilar common.o (CORREÇÃO: Regra separada)
$(COMMON_DIR)/common.o: $(COMMON_DIR)/common.cpp $(COMMON_DIR)/common.hpp
	$(CXX) $(CXXFLAGS) -c $(COMMON_DIR)/common.cpp -o $(COMMON_DIR)/common.o

# 2. Compilar protocols.o
$(COMMON_DIR)/protocols.o: $(COMMON_DIR)/protocols.cpp $(COMMON_DIR)/protocols.hpp $(COMMON_DIR)/common.hpp
	$(CXX) $(CXXFLAGS) -c $(COMMON_DIR)/protocols.cpp -o $(COMMON_DIR)/protocols.o

# ------------------- User Application -------------------

# O executável precisa de common.o e protocols.o
user: $(USER_DIR)/user.o $(COMMON_DIR)/protocols.o $(USER_DIR)/utils.o $(COMMON_DIR)/common.o
	$(CXX) $(CXXFLAGS) -o user $(USER_DIR)/user.o $(COMMON_DIR)/protocols.o $(USER_DIR)/utils.o $(COMMON_DIR)/common.o

# Compilar user.cpp
$(USER_DIR)/user.o: $(USER_DIR)/user.cpp $(COMMON_DIR)/common.hpp $(COMMON_DIR)/protocols.hpp $(USER_DIR)/utils.hpp
	$(CXX) $(CXXFLAGS) -c $(USER_DIR)/user.cpp -o $(USER_DIR)/user.o

# Compilar utils.cpp (do User)
$(USER_DIR)/utils.o: $(USER_DIR)/utils.cpp $(USER_DIR)/utils.hpp $(COMMON_DIR)/protocols.hpp $(COMMON_DIR)/common.hpp
	$(CXX) $(CXXFLAGS) -c $(USER_DIR)/utils.cpp -o $(USER_DIR)/utils.o

# ------------------- Server Application (ES) -------------------

# O executável ES precisa de TUDO: ES.o, utils do server, protocols e common
ES: $(ES_DIR)/ES.o $(ES_DIR)/utils.o $(COMMON_DIR)/protocols.o $(COMMON_DIR)/common.o
	$(CXX) $(CXXFLAGS) -o ES $(ES_DIR)/ES.o $(ES_DIR)/utils.o $(COMMON_DIR)/protocols.o $(COMMON_DIR)/common.o

# Compilar ES.cpp
$(ES_DIR)/ES.o: $(ES_DIR)/ES.cpp $(COMMON_DIR)/common.hpp $(ES_DIR)/utils.hpp $(COMMON_DIR)/protocols.hpp
	$(CXX) $(CXXFLAGS) -c $(ES_DIR)/ES.cpp -o $(ES_DIR)/ES.o

# Compilar utils.cpp (do Server)
$(ES_DIR)/utils.o: $(ES_DIR)/utils.cpp $(ES_DIR)/utils.hpp $(COMMON_DIR)/common.hpp
	$(CXX) $(CXXFLAGS) -c $(ES_DIR)/utils.cpp -o $(ES_DIR)/utils.o

# ------------------- Clean -------------------
clean:
	rm -f user ES
	rm -f $(USER_DIR)/*.o
	rm -f $(ES_DIR)/*.o
	rm -f $(COMMON_DIR)/*.o

# ------------------- Run Targets -------------------
run_server: ES
	@echo "Running Server inside $(ES_DIR)..."
	cd $(ES_DIR) && ../../ES

run_user: user
	./user