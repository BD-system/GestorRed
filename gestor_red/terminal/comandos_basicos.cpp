#include <iostream>
#include "comandos_basicos.hpp"
#include "style_terminal.hpp"
#include "exec.hpp"
#include "aux.hpp" // <--- IMPORTANTE: incluye el header de aux

bool procesarComandoBasico(const std::string& linea) {
    if (linea == "help" || linea == "?") {
        mostrarAyuda();
        return true;
    } else if (linea == "about") {
        mostrarAbout();
        return true;
    } else if (linea == "clear" || linea == "cls") {
        limpiarPantalla();
        mostrarBanner();
        return true;
    } else if (linea == "exit" || linea == "quit") {
        std::cout << BOLD << RED << "\n[*] Cerrando terminal...\n" << RESET << std::endl;
        return true;
    } else if (linea.rfind("exec ", 0) == 0) {  
        std::string argumentos = linea.substr(5);
        while (!argumentos.empty() && isspace(argumentos.front())) argumentos.erase(argumentos.begin());
        exec(argumentos);
        return true;
    } else if (linea == "exec") {
        exec("");
        return true;
    } else if (linea.rfind("aux ", 0) == 0) {
        std::string argumentos = linea.substr(4);
        while (!argumentos.empty() && isspace(argumentos.front())) argumentos.erase(argumentos.begin());
        aux(argumentos);
        return true;
    } else if (linea == "aux") {
        aux("");
        return true;
    }
    return false;
}
