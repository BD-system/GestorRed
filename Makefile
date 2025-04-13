# ========================
# CONFIGURACIÓN DEL PROYECTO
# ========================
CXX := g++
# Se eliminan los flags de AddressSanitizer (-fsanitize=address)
CXXFLAGS := -Wall -Wextra -std=c++17 -Igestor_red -g -fsanitize=address
LDFLAGS := 

# ESTRUCTURA DE DIRECTORIOS
SRCDIR := gestor_red
OBJDIR := build
TARGET := gestor_red_app

# ========================
# DETECCIÓN AUTOMÁTICA DE FUENTES
# ========================
SRC := $(shell find $(SRCDIR) -name '*.cpp')
OBJ := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRC))

# ========================
# COMPILACIÓN FINAL
# ========================
all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "[LD] Enlazando $(TARGET)"
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# ========================
# COMPILACIÓN DE OBJETOS
# ========================
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "[CC] Compilando $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ========================
# LIMPIEZA
# ========================
clean:
	@echo "[CLEAN] Eliminando objetos y ejecutable..."
	rm -rf $(OBJDIR) $(TARGET)

# ========================
# AYUDA
# ========================
help:
	@echo "Comandos disponibles:"
	@echo "  make          Compila el proyecto"
	@echo "  make -j4      Compilación paralela (4 hilos)"
	@echo "  make clean    Limpia el proyecto"
	@echo "  make help     Muestra esta ayuda"

.PHONY: all clean help
