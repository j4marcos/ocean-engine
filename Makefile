# Makefile para Ocean Engine
# Alternativa ao CMake para compilação simples

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include
LDFLAGS = -lGL -lGLU -lglut -lm

# Diretórios
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# Arquivos
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
TARGET = $(BUILD_DIR)/ocean_engine

# Cores para output
GREEN = \033[0;32m
YELLOW = \033[1;33m
NC = \033[0m

.PHONY: all clean run debug release

# Build padrão (debug)
all: $(TARGET)
	@echo "$(GREEN)Build complete!$(NC)"

# Cria diretório de build
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compila objetos
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "$(YELLOW)Compiling $<...$(NC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Linka executável
$(TARGET): $(OBJECTS)
	@echo "$(YELLOW)Linking $(TARGET)...$(NC)"
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Build de debug
debug: CXXFLAGS += -g -O0 -DDEBUG
debug: clean all

# Build de release
release: CXXFLAGS += -O3 -DNDEBUG
release: clean all

# Executa
run: all
	@echo "$(GREEN)Running Ocean Engine...$(NC)"
	./$(TARGET)

# Limpa
clean:
	@echo "$(YELLOW)Cleaning...$(NC)"
	rm -rf $(BUILD_DIR)

# Ajuda
help:
	@echo "Ocean Engine - Makefile"
	@echo ""
	@echo "Comandos disponíveis:"
	@echo "  make        - Compila o projeto (debug)"
	@echo "  make debug  - Compila com símbolos de debug"
	@echo "  make release- Compila otimizado para release"
	@echo "  make run    - Compila e executa"
	@echo "  make clean  - Remove arquivos de build"
	@echo "  make help   - Mostra esta ajuda"
