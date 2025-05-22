#pragma once
#include <string>

// Comando principal
void exec(const std::string& argumentos);

// Herramientas internas (si quieres usarlas en test/admin)
void build_module_cache(const std::string& base_path = "");
void list_modules();
void reload_modules();
