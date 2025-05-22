// style_terminal.hpp
#pragma once
#include <string>

// Definición de los colores y estilos ANSI (extern para variables en el cpp)
extern const char* RESET;
extern const char* CYAN;
extern const char* GREEN;
extern const char* YELLOW;
extern const char* RED;
extern const char* BLUE;
extern const char* MAGENTA;
extern const char* BOLD;
extern const char* UNDERLINE;

// Presentación visual
void mostrarBanner();
void mostrarPrompt();
void mostrarFooter();          // Despedida

// Información y ayuda
void mostrarAyuda();
void mostrarAbout();
void mostrarComandoDesconocido(const std::string& cmd);

// Limpieza y helpers
void limpiarPantalla();

// Utilidad de formato
std::string trim_asci(const std::string& str);
