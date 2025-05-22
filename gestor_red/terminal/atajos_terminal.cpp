// /gestor_red/equipos_red/atajos_terminal.cpp

#include "atajos_terminal.hpp"
#include "style_terminal.hpp"
#include <iostream>
#include <string>
#include <readline/readline.h>
#include <readline/history.h>

// --- Hook para Ctrl+L en readline (limpiar pantalla y banner) ---
int hook_limpiar_pantalla() {
    rl_bind_keyseq("\\C-l", [](int /*count*/, int /*key*/) -> int {
        limpiarPantalla();
        mostrarBanner();
        // Muestra el prompt igual que readline lo pinta
        // (No hace falta mostrarPrompt() aquí porque readline lo hace)
        rl_on_new_line();            // Ajusta internamente readline
        rl_replace_line("", 0);      // Borra la línea de entrada actual
        rl_redisplay();
        return 0;
    });
    return 0;
}

void inicializarAtajosTerminal() {
    rl_startup_hook = hook_limpiar_pantalla;
}

std::string prompt_coloreado() {
    // \001 y \002 son delimitadores para que readline ignore el tamaño de los códigos ANSI
    return "\001\033[1m\033[32m\002gdr\001\033[0m\002\001\033[1m\033[33m\002 > \001\033[0m\002";
}

// Lee línea con histórico y Ctrl+L (el prompt se escribe aquí)
std::string leerLineaConAtajos() {
    char* input = readline(prompt_coloreado().c_str());
    if (!input) return "";
    std::string linea(input);
    if (!linea.empty())
        add_history(input); // Añade al histórico solo si no está vacía
    free(input);
    return linea;
}
