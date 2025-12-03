# Compiler and Flags
CXX = g++
# Adicionei -I. (raiz) e -IUserApp para o compilador encontrar os .hpp
CXXFLAGS = -Wall -Wextra -std=c++17 -g -I. -IUserApp

# Directories
USER_DIR = UserApp
ES_DIR = ESDIR

# Targets
all: user ES

# ------------------- Shared Objects -------------------
# protocols.o fica na raiz
protocols.o: protocols.cpp protocols.hpp common.hpp
	$(CXX) $(CXXFLAGS) -c protocols.cpp -o protocols.o

# ------------------- User Application -------------------
# O executável 'user' fica na raiz para ser fácil de correr
user: $(USER_DIR)/user.o protocols.o $(USER_DIR)/utils.o
	$(CXX) $(CXXFLAGS) -o user $(USER_DIR)/user.o protocols.o $(USER_DIR)/utils.o

# Compilar user.cpp (que está dentro de UserApp)
$(USER_DIR)/user.o: $(USER_DIR)/user.cpp common.hpp protocols.hpp $(USER_DIR)/utils.hpp
	$(CXX) $(CXXFLAGS) -c $(USER_DIR)/user.cpp -o $(USER_DIR)/user.o

# Compilar utils.cpp (que está dentro de UserApp)
$(USER_DIR)/utils.o: $(USER_DIR)/utils.cpp $(USER_DIR)/utils.hpp protocols.hpp common.hpp
	$(CXX) $(CXXFLAGS) -c $(USER_DIR)/utils.cpp -o $(USER_DIR)/utils.o

# ------------------- Server Application (ES) -------------------
# O executável 'ES' fica na raiz
ES: $(ES_DIR)/ES.o
	$(CXX) $(CXXFLAGS) -o ES $(ES_DIR)/ES.o

# Compilar ES.cpp (que está dentro de ESDIR)
$(ES_DIR)/ES.o: $(ES_DIR)/ES.cpp common.hpp
	$(CXX) $(CXXFLAGS) -c $(ES_DIR)/ES.cpp -o $(ES_DIR)/ES.o

# ------------------- Clean -------------------
# Limpa os .o dentro das pastas também
clean:
	rm -f *.o $(USER_DIR)/*.o $(ES_DIR)/*.o user ES

# ------------------- Run Targets -------------------

# Truque: Entra na pasta ESDIR e corre o executável que está na raiz (../ES)
# Isto garante que os ficheiros criados ficam dentro de ESDIR!
run_server: ES
	@echo "Running Server inside ESDIR..."
	cd $(ES_DIR) && ../ES

run_user: user
	./user