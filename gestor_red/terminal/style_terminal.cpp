// gestor_red/terminal/style_terminal.cpp
#include "style_terminal.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>

// Definición de los colores y estilos ANSI
const char* RESET     = "\033[0m";
const char* CYAN      = "\033[36m";
const char* GREEN     = "\033[32m";
const char* YELLOW    = "\033[33m";
const char* RED       = "\033[31m";
const char* BLUE      = "\033[34m";
const char* MAGENTA   = "\033[35m";
const char* BOLD      = "\033[1m";
const char* UNDERLINE = "\033[4m";

// Banner ASCII
void mostrarBanner() {
    std::cout << BOLD << BLUE;
    std::cout <<
        "════════════════════════════════════════════════════════════\n"
        "  ██████╗ ███████╗███████╗████████╗ ██████╗ ██████╗      \n"
        "  ██╔════╝ ██╔════╝██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗     \n"
        "   ██║  ███╗█████╗  ███████╗   ██║   ██║   ██║██████╔╝     \n"
        "    ██║   ██║██╔══╝  ╚════██║   ██║   ██║   ██║██╔══██╗     \n"
        "     ╚██████╔╝███████╗███████║   ██║   ╚██████╔╝██║  ██║     \n"
        "      ╚═════╝ ╚══════╝╚══════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝     \n"
        "════════════════ TERMINAL MODULAR ══════════════════════════\n"
        " Proyecto: Gestor de Red                 Autor: Borja Diaz  \n"
        "════════════════════════════════════════════════════════════\n"
    << RESET;
}


void mostrarPrompt() {
    std::cout << BOLD << GREEN << "gdr" << RESET
              << BOLD << YELLOW << " > " << RESET;
}


void mostrarFooter() {
    std::cout << BOLD << BLUE
        << "════════════════════════════════════════════════════════════\n"
        << RESET;
}


void mostrarAyuda() {
    std::cout << BOLD << CYAN << "\nComandos disponibles:\n" << RESET
              << GREEN << "  help" << RESET << "      Muestra esta ayuda\n"
              << GREEN << "  exec" << RESET << "      Trabajar con modulos\n"
              << GREEN << "  aux" << RESET << "       Trabajar con auxiliares\n"
              << GREEN << "  exit" << RESET << "      Salir de la terminal\n"
              << GREEN << "  clear" << RESET << "     Limpia la pantalla\n"
              << GREEN << "  about" << RESET << "     Información del proyecto\n"
              << std::endl;
}


void mostrarAbout() {
    std::cout << MAGENTA
              << "\nProyecto: Gestor de Red\n"
              << "Terminal modular para gestión avanzada y ejecución de módulos.\n"
              << "Desarrollado por Borja Diaz\n"
              << RESET << std::endl;
}


void mostrarComandoDesconocido(const std::string& cmd) {
    std::cout << YELLOW << "[!] Comando desconocido: " << RESET << cmd << std::endl;
    std::cout << "    Escribe " << BOLD << "help" << RESET << " para ver los comandos disponibles.\n";
}


void limpiarPantalla() {
    std::cout << "\033[2J\033[1;1H";
}


std::string trim_asci(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}
