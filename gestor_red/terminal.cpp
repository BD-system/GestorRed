// gestor_red/terminal.cpp

#include <iostream>
#include <string>
#include "terminal/style_terminal.hpp"
#include "terminal/comandos_basicos.hpp"
#include "terminal/atajos_terminal.hpp"

// --- Shell interactiva principal ---
void lanzarTerminal() {
    inicializarAtajosTerminal();
    limpiarPantalla();
    mostrarBanner();

    std::string linea;
    while (true) {
        // NO mostramos el prompt aquí, lo hace leerLineaConAtajos con color
        linea = leerLineaConAtajos();
        linea = trim_asci(linea);

        if (procesarComandoBasico(linea)) {
            if (linea == "exit" || linea == "quit")
                break;
        } else if (!linea.empty()) {
            mostrarComandoDesconocido(linea);
        }
    }
    mostrarFooter();
}
