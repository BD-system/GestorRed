# ========================
# CONFIGURACIÓN DEL PROYECTO
# ========================
CXX       := g++
CXXFLAGS  := -Wall -Wextra -std=c++17 -Igestor_red -Igestor_red/equipos_red -Igestor_red/BBDD -g -fsanitize=address
LDFLAGS   := -lmysqlcppconn -lreadline

# ESTRUCTURA DE DIRECTORIOS
SRCDIR    := gestor_red
OBJDIR    := build
TARGET    := gestor_red_app
TERMINAL  := terminal_app

# ========================
# DETECCIÓN AUTOMÁTICA DE FUENTES
# ========================
SRC       := $(shell find $(SRCDIR) -name '*.cpp')
OBJ       := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRC))

# ========================
# COMPILACIÓN FINAL
# ========================
all: dependencias $(TARGET)

$(TARGET): $(OBJ)
	@echo "[LD] Enlazando $(TARGET)"
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Compila solo la terminal (main en terminal.cpp, ignora inicializador.o)
$(TERMINAL): $(OBJ)
	@echo "[LD] Enlazando $(TERMINAL)"
	$(CXX) $(CXXFLAGS) $(filter-out $(OBJDIR)/inicializador.o, $(OBJ)) -o $@ $(LDFLAGS)

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
	@echo "[CLEAN] Eliminando objetos y ejecutables..."
	rm -rf $(OBJDIR) $(TARGET) $(TERMINAL)

# ========================
# EJECUCIÓN RÁPIDA
# ========================
run: all
	@echo "[RUN] Ejecutando $(TARGET)"
	./$(TARGET)

run-terminal: $(TERMINAL)
	@echo "[RUN] Ejecutando $(TERMINAL)"
	./$(TERMINAL)

# ========================
# AYUDA
# ========================
help:
	@echo "Comandos disponibles:"
	@echo "  make               Compila el proyecto principal (gestor_red_app)"
	@echo "  make -j4           Compilación paralela (4 hilos)"
	@echo "  make terminal      Compila solo la terminal (terminal_app)"
	@echo "  make run           Compila y ejecuta el inicializador (gestor_red_app)"
	@echo "  make run-terminal  Compila y ejecuta solo la terminal (terminal_app)"
	@echo "  make clean         Limpia el proyecto"
	@echo "  make help          Muestra esta ayuda"

.PHONY: all clean help run dependencias terminal run-terminal
