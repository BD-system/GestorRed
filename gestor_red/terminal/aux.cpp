#include "aux.hpp"
#include "style_terminal.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <sstream>
#include <set>
#include <algorithm>

// === CONFIGURACIÓN DE RUTAS ===
std::string aux_dir = "/root/gestor_de_red/aux";
std::string python_envs_dir = "/root"; // Entornos virtuales
std::string default_env_conf = "/root/gestor_de_red/config/aux_default_env.conf";

// === ESTRUCTURA DE AUXILIAR EN CACHE ===
struct AuxInfo {
    std::string name;      // nombre base (ej: "mi_scanner")
    std::string relpath;   // ruta relativa (ej: "scripts/scan/mi_scanner")
    std::string path;      // ruta absoluta
    std::string tipo;      // py/sh/pr/c
    std::string description;
};
static std::vector<AuxInfo> aux_cache;
static bool aux_cache_ready = false;

// === MAPA DE ENTS (nombre->ruta) ===
static std::map<std::string, std::string> entornos_por_defecto;

// === FUNCIONES AUXILIARES ===

std::string detectar_tipo(const std::filesystem::path& p) {
    auto ext = p.extension().string();
    if (ext == ".py") return "py";
    if (ext == ".sh") return "sh";
    if (ext == ".pl") return "pr";
    if (ext == ".so") return "c";
    return "unk";
}

std::string extraer_descripcion(const std::filesystem::path& p) {
    std::string ext = p.extension().string();
    std::ifstream f(p);
    std::string linea;
    if ((ext == ".py" || ext == ".sh" || ext == ".pl") && f.is_open()) {
        while (std::getline(f, linea)) {
            if (!linea.empty() && (linea[0] == '#' || linea.substr(0,2) == "//")) {
                size_t pos = linea.find_first_not_of("#/ ");
                if (pos != std::string::npos)
                    return linea.substr(pos);
            }
        }
        return "Sin descripción";
    }
    if (ext == ".so") {
        return "Auxiliar en C/C++";
    }
    return "Sin descripción";
}

void build_aux_cache(const std::string& base_path) {
    aux_cache.clear();
    std::string use_path = base_path.empty() ? aux_dir : base_path;
    if (!std::filesystem::exists(use_path)) {
        std::cout << RED << "[!] No se encontró el directorio de auxiliares: " << use_path << RESET << std::endl;
        aux_cache_ready = false;
        return;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(use_path)) {
        if (entry.is_regular_file()) {
            std::string tipo = detectar_tipo(entry.path());
            if (tipo == "unk") continue;
            AuxInfo info;
            info.name = entry.path().stem().string();
            info.relpath = std::filesystem::relative(entry.path(), use_path).string();
            size_t lastdot = info.relpath.find_last_of('.');
            if (lastdot != std::string::npos) info.relpath = info.relpath.substr(0, lastdot);
            std::replace(info.relpath.begin(), info.relpath.end(), '\\', '/');
            info.path = entry.path().string();
            info.tipo = tipo;
            info.description = extraer_descripcion(entry.path());
            aux_cache.push_back(info);
        }
    }
    aux_cache_ready = true;
}

std::string resolver_ruta_auxiliares(const std::string& argumento) {
    const std::string prefix = "auxiliares:/";
    const std::string ruta_base = "/root/gestor_de_red/src/aux/";
    std::string res = argumento;
    size_t pos = 0;
    while ((pos = res.find(prefix, pos)) != std::string::npos) {
        res.replace(pos, prefix.length(), ruta_base);
        pos += ruta_base.length();
    }
    return res;
}

// === LEE ENTORNOS POR DEFECTO DEL ARCHIVO CONFIGURACIÓN ===
void cargar_entornos_por_defecto() {
    entornos_por_defecto.clear();
    std::ifstream f(default_env_conf);
    std::string linea;
    while (f.is_open() && std::getline(f, linea)) {
        if (linea.empty() || linea[0] == '#') continue;
        std::istringstream iss(linea);
        std::string nombre, ruta;
        iss >> nombre >> ruta;
        if (!nombre.empty() && !ruta.empty()) {
            entornos_por_defecto[nombre] = ruta;
        }
    }
}

std::string leer_ruta_entorno(const std::string& nombre_entorno) {
    if (entornos_por_defecto.empty())
        cargar_entornos_por_defecto();
    if (entornos_por_defecto.count(nombre_entorno))
        return entornos_por_defecto[nombre_entorno];
    return "";
}

std::string leer_primera_ruta_entorno() {
    if (entornos_por_defecto.empty())
        cargar_entornos_por_defecto();
    if (!entornos_por_defecto.empty())
        return entornos_por_defecto.begin()->second;
    return "";
}

// === EXTRAER Y QUITAR -p:<entorno> DE LOS ARGUMENTOS ===
std::string extraer_y_quitar_parametro_env(std::string& argumentos) {
    std::string env_prefix = "-p:";
    size_t pos = argumentos.find(env_prefix);
    if (pos == std::string::npos) return "";
    size_t start = pos + env_prefix.length();
    size_t end = argumentos.find(' ', start);
    std::string env;
    if (end != std::string::npos)
        env = argumentos.substr(start, end - start);
    else
        env = argumentos.substr(start);
    argumentos.erase(pos, (end == std::string::npos ? argumentos.length() : end - pos));
    while (!argumentos.empty() && isspace(argumentos.front())) argumentos.erase(argumentos.begin());
    return env;
}

void list_aux() {
    if (!aux_cache_ready) build_aux_cache("");
    if (aux_cache.empty()) {
        std::cout << YELLOW << "[!] No se encontraron auxiliares en: " << aux_dir << RESET << std::endl;
        return;
    }
    std::cout << BOLD << CYAN << "\nAuxiliares disponibles:\n" << RESET;
    for (const auto& aux : aux_cache) {
        std::cout << "  " << GREEN << std::left << std::setw(18) << aux.name << RESET
                  << std::setw(32) << ("(" + aux.relpath + ")")
                  << aux.description << std::endl;
    }
    std::cout << std::endl;
}

void list_envs() {
    std::ifstream f(default_env_conf);
    std::string linea;

    // Map: lenguaje -> vector de pares (nombre, ruta)
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> entornos;

    while (f.is_open() && std::getline(f, linea)) {
        if (linea.empty() || linea[0] == '#') continue;
        std::istringstream iss(linea);
        std::string lenguaje, nombre, ruta;
        iss >> lenguaje >> nombre >> ruta;
        if (!lenguaje.empty() && !nombre.empty() && !ruta.empty()) {
            entornos[lenguaje].emplace_back(nombre, ruta);
        }
    }

    std::cout << BOLD << CYAN << "\nEntornos virtuales definidos en el archivo de configuración (" << default_env_conf << "):\n" << RESET;
    if (entornos.empty()) {
        std::cout << YELLOW << "  (Archivo vacío o sin entornos definidos)" << RESET << std::endl;
    } else {
        for (const auto& par : entornos) {
            const std::string& lenguaje = par.first;
            std::cout << BOLD << lenguaje << RESET << ":" << std::endl;
            for (const auto& env : par.second) {
                std::cout << "  " << GREEN << env.first << RESET << " -> " << env.second;
                if (!std::filesystem::exists(env.second)) {
                    std::cout << RED << " [NO ENCONTRADO]" << RESET;
                }
                std::cout << std::endl;
            }
        }
    }
    std::cout << std::endl;
}


// === EJECUTA AUXILIAR POR NOMBRE BASE O RUTA RELATIVA ===
void run_aux(const std::string& nombre, std::string argumentos) {
    argumentos = resolver_ruta_auxiliares(argumentos);
    std::string entorno = extraer_y_quitar_parametro_env(argumentos);

    // Lee la ruta de activación del entorno desde el archivo config (nuevo formato)
    std::string ruta_activacion;
    if (!entorno.empty())
        ruta_activacion = leer_ruta_entorno(entorno);
    else
        ruta_activacion = leer_primera_ruta_entorno();

    if (!aux_cache_ready) build_aux_cache("");
    const AuxInfo* match = nullptr;

    // Busca por nombre base
    for (const auto& aux : aux_cache) {
        if (aux.name == nombre) {
            match = &aux;
            break;
        }
    }
    // Si no encuentra, busca por ruta relativa
    if (!match) {
        for (const auto& aux : aux_cache) {
            if (aux.relpath == nombre) {
                match = &aux;
                break;
            }
        }
    }

    if (!match) {
        std::cout << RED << "[!] Auxiliar no encontrado: " << nombre << RESET << std::endl;
        return;
    }

    std::string comando;
    if (match->tipo == "py") comando = "python3 '" + match->path + "' " + argumentos;
    else if (match->tipo == "sh") comando = "bash '" + match->path + "' " + argumentos;
    else if (match->tipo == "pr") comando = "perl '" + match->path + "' " + argumentos;
    else if (match->tipo == "c")  comando = "./'" + match->path + "' " + argumentos;
    else {
        std::cout << RED << "[!] Tipo de auxiliar no soportado: " << match->tipo << RESET << std::endl;
        return;
    }

    if (!ruta_activacion.empty()) {
        if (!std::filesystem::exists(ruta_activacion)) {
            std::cout << RED << "[!] El entorno virtual especificado no existe en: " << ruta_activacion << RESET << std::endl;
            std::cout << "    Revisa el archivo de configuración de entornos por defecto (" << default_env_conf << ")\n";
            return;
        }
        comando = "source '" + ruta_activacion + "' && " + comando;
    }

    std::cout << "[*] Ejecutando: " << comando << std::endl;
    int ret = system(comando.c_str());
    if (ret == -1) {
        std::cout << RED << "[!] Error al ejecutar el auxiliar." << RESET << std::endl;
    }
}

// === EJECUTA AUXILIAR FORZANDO TIPO Y RELPATH (igual lógica que run_aux) ===
void run_aux_tipo(const std::string& tipo, const std::string& nombre, std::string argumentos) {
    argumentos = resolver_ruta_auxiliares(argumentos);
    std::string entorno = extraer_y_quitar_parametro_env(argumentos);

    std::string ruta_activacion;
    if (!entorno.empty())
        ruta_activacion = leer_ruta_entorno(entorno);
    else
        ruta_activacion = leer_primera_ruta_entorno();

    if (!aux_cache_ready) build_aux_cache("");
    const AuxInfo* match = nullptr;

    // Busca por nombre base y tipo
    for (const auto& aux : aux_cache) {
        if (aux.name == nombre && aux.tipo == tipo) {
            match = &aux;
            break;
        }
    }
    // Si no encuentra, busca por ruta relativa y tipo
    if (!match) {
        for (const auto& aux : aux_cache) {
            if (aux.relpath == nombre && aux.tipo == tipo) {
                match = &aux;
                break;
            }
        }
    }

    if (!match) {
        std::cout << RED << "[!] Auxiliar no encontrado: " << nombre << " de tipo " << tipo << RESET << std::endl;
        return;
    }

    std::string comando;
    if (tipo == "py") comando = "python3 '" + match->path + "' " + argumentos;
    else if (tipo == "sh") comando = "bash '" + match->path + "' " + argumentos;
    else if (tipo == "pr") comando = "perl '" + match->path + "' " + argumentos;
    else if (tipo == "c")  comando = "./'" + match->path + "' " + argumentos;
    else {
        std::cout << RED << "[!] Tipo de auxiliar no soportado: " << tipo << RESET << std::endl;
        return;
    }

    if (!ruta_activacion.empty()) {
        if (!std::filesystem::exists(ruta_activacion)) {
            std::cout << RED << "[!] El entorno virtual especificado no existe en: " << ruta_activacion << RESET << std::endl;
            std::cout << "    Revisa el archivo de configuración de entornos por defecto (" << default_env_conf << ")\n";
            return;
        }
        comando = "source '" + ruta_activacion + "' && " + comando;
    }

    std::cout << "[*] Ejecutando: " << comando << std::endl;
    int ret = system(comando.c_str());
    if (ret == -1) {
        std::cout << RED << "[!] Error al ejecutar el auxiliar." << RESET << std::endl;
    }
}

// === AYUDA PROFESIONAL Y EXPLICATIVA (ESTILO EXEC) ===
void print_usage_aux() {
    std::cout << BOLD << CYAN << "\nComandos disponibles:\n" << RESET
              << GREEN << "  <auxiliar> [args]" << RESET << "             Ejecuta el auxiliar por nombre (esté donde esté)\n"
              << GREEN << "  <ruta/relativa/auxiliar> [args]" << RESET << "   (Para nombres en subcarpetas)\n"
              << GREEN << "  -l, --list" << RESET << "                    Lista todos los auxiliares disponibles\n"
              << GREEN << "  -lp" << RESET << "                           Lista los entornos virtuales Python detectados\n"
              << GREEN << "  --reload" << RESET << "                      Recarga la caché de auxiliares\n"
              << std::endl

              << BOLD << CYAN << "Gestión de payloads y archivos:\n" << RESET
              << "  Cuando un auxiliar necesita un archivo (payload):\n"
              << "    - Si sólo indicas el nombre del archivo (por ejemplo, " << BOLD << "mi_config.json" << RESET << "),\n"
              << "      el auxiliar lo buscará en su carpeta privada (donde reside el script).\n"
              << "    - Si usas el prefijo " << BOLD << "auxiliares:/" << RESET << " (por ejemplo, "
              << BOLD << "auxiliares:/payloads/scan.bin" << RESET << "),\n"
              << "      la terminal lo buscará automáticamente en la carpeta global: "
              << BOLD << "/root/gestor_de_red/src/aux/payloads/scan.bin" << RESET << "\n"
              << std::endl

              << BOLD << CYAN << "Archivo de entornos por defecto:\n" << RESET
              << "  Formato: " << BOLD << "<libreria> <ruta_de_activacion>" << RESET << "\n"
              << "  Ejemplo:\n"
              << "    venv_gdr /root/venv_gdr/bin/activate\n"
              << "    otra_venv /root/otra_venv/bin/activate\n"
              << "  Si no usas -p:<libreria>, se usará la primera del archivo.\n"
              << std::endl

              << BOLD << CYAN << "Ejemplos de uso:\n" << RESET
              << "  " << GREEN << "aux -run mi_scanner argumentos" << RESET << "\n"
              << "      (Ejecuta cualquier 'mi_scanner' encontrado en la jerarquía de auxiliares)\n"
              << "  " << GREEN << "aux -run scripts/utilidades/limpiar_logs argumentos" << RESET << "\n"
              << "      (Especifica ruta relativa para casos de nombres en subcarpetas)\n"
              << "  " << GREEN << "aux -run mi_scanner -p:venv_gdr argumentos" << RESET << "\n"
              << "      (Ejecuta con el entorno virtual llamado venv_gdr)\n"
              << "  " << GREEN << "aux -run scripts/scan/mi_scanner auxiliares:/payloads/scan.bin" << RESET << "\n"
              << "      (El script recibe la ruta absoluta de scan.bin en la carpeta global de auxiliares)\n"
              << std::endl

              << "Puedes organizar tus scripts auxiliares en subcarpetas. Basta con poner el nombre, salvo que haya duplicados.\n"
              << std::endl;
}

// === ENTRADA PRINCIPAL DEL COMANDO AUXILIAR ===
void aux(const std::string& argumentos) {
    std::string args = argumentos;
    while (!args.empty() && isspace(args.front())) args.erase(args.begin());

    if (args.empty() || args == "-h" || args == "--help") {
        print_usage_aux();
        return;
    }
    if (args == "-l" || args == "--list") {
        list_aux();
        return;
    }
    if (args == "-lp") {
        list_envs();
        return;
    }
    if (args == "--reload") {
        build_aux_cache("");
        std::cout << "[OK] Auxiliares recargados." << std::endl;
        return;
    }

    // aux -run <nombre> [args...]
    if (args.rfind("-run ", 0) == 0) {
        std::string resto = args.substr(5);
        while (!resto.empty() && isspace(resto.front())) resto.erase(resto.begin());
        std::istringstream iss(resto);
        std::string primer, segundo, argstr;
        iss >> primer >> segundo;
        std::getline(iss, argstr);
        while (!argstr.empty() && isspace(argstr.front())) argstr.erase(argstr.begin());

        if ((primer == "py" || primer == "sh" || primer == "pr" || primer == "c") && !segundo.empty()) {
            run_aux_tipo(primer, segundo, argstr);
        } else {
            run_aux(primer, segundo + argstr);
        }
        return;
    }

    // Permitir también "aux mi_scanner args"
    if (args.find(' ') != std::string::npos && args[0] != '-') {
        std::string primer, argstr;
        std::istringstream iss(args);
        iss >> primer;
        std::getline(iss, argstr);
        while (!argstr.empty() && isspace(argstr.front())) argstr.erase(argstr.begin());
        run_aux(primer, argstr);
        return;
    }

    print_usage_aux();
}
