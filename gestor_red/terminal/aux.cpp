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

std::string aux_dir = "/root/gestor_de_red/aux"; // Modifica según tu entorno
std::string python_envs_dir = "/root";           // Donde están los entornos virtuales
std::string default_env_conf = "/root/gestor_de_red/config/aux_default_env.conf"; // Archivo de entorno por defecto

struct AuxInfo {
    std::string name;
    std::string path;
    std::string tipo;
    std::string description;
};
static std::vector<AuxInfo> aux_cache;
static bool aux_cache_ready = false;

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
    for (const auto& entry : std::filesystem::directory_iterator(use_path)) {
        if (entry.is_regular_file()) {
            std::string tipo = detectar_tipo(entry.path());
            if (tipo == "unk") continue;
            AuxInfo info;
            info.name = entry.path().stem().string();
            info.path = entry.path().string();
            info.tipo = tipo;
            info.description = extraer_descripcion(entry.path());
            aux_cache.push_back(info);
        }
    }
    aux_cache_ready = true;
}

// --- SOLO nombre y descripción para el usuario final ---
void list_aux() {
    if (!aux_cache_ready) build_aux_cache("");
    if (aux_cache.empty()) {
        std::cout << YELLOW << "[!] No se encontraron auxiliares en: " << aux_dir << RESET << std::endl;
        return;
    }
    std::cout << BOLD << CYAN << "\nAuxiliares disponibles:\n" << RESET;
    for (const auto& aux : aux_cache) {
        std::cout << "  " << GREEN << std::left << std::setw(14) << aux.name << RESET
                  << aux.description << std::endl;
    }
    std::cout << std::endl;
}

// --------- LISTAR ENTORNOS PYTHON DISPONIBLES ---------
void list_python_envs() {
    std::set<std::string> envs;
    if (!std::filesystem::exists(python_envs_dir)) {
        std::cout << RED << "[!] No se encontró el directorio de entornos: " << python_envs_dir << RESET << std::endl;
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(python_envs_dir)) {
        if (entry.is_directory()) {
            auto activate = entry.path() / "bin/activate";
            if (std::filesystem::exists(activate)) {
                envs.insert(entry.path().filename().string());
            }
        }
    }
    std::cout << BOLD << CYAN << "\nEntornos virtuales Python disponibles:\n" << RESET;
    if (envs.empty()) {
        std::cout << YELLOW << "  (Ningún entorno encontrado en " << python_envs_dir << ")" << RESET << std::endl;
    } else {
        for (const auto& env : envs)
            std::cout << "  " << GREEN << env << RESET << std::endl;
    }
    std::cout << std::endl;
}

// --------- DETECTAR Y EXTRAER -p:<entorno> ---------
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

// --------- LEER ENV POR DEFECTO DE .CONF ---------
std::string leer_env_por_defecto() {
    std::ifstream f(default_env_conf);
    std::string env;
    if (f.is_open() && std::getline(f, env)) {
        // Limpia espacios y saltos de línea
        while (!env.empty() && (env.back() == '\n' || env.back() == '\r' || isspace(env.back())))
            env.pop_back();
        while (!env.empty() && isspace(env.front()))
            env.erase(env.begin());
        return env;
    }
    return "";
}

// --------- Ejecutar auxiliar por nombre (con -p:) ---------
void run_aux(const std::string& nombre, std::string argumentos) {
    std::string entorno = extraer_y_quitar_parametro_env(argumentos);

    // Si el usuario NO especifica -p, intentamos leer el entorno por defecto
    if (entorno.empty())
        entorno = leer_env_por_defecto();

    if (!aux_cache_ready) build_aux_cache("");
    std::vector<const AuxInfo*> matches;
    for (const auto& aux : aux_cache) {
        if (aux.name == nombre) matches.push_back(&aux);
    }
    if (matches.empty()) {
        std::cout << RED << "[!] Auxiliar no encontrado: " << nombre << RESET << std::endl;
        return;
    }
    if (matches.size() > 1) {
        std::cout << YELLOW << "[!] Hay varios auxiliares llamados '" << nombre << "' con diferentes tipos:\n";
        for (const auto* aux : matches)
            std::cout << "    " << aux->name << " (" << aux->tipo << "): " << aux->description << std::endl;
        std::cout << RESET << "    Especifica el tipo: aux -run <tipo> " << nombre << " [args...]\n";
        return;
    }
    const AuxInfo& aux = *matches[0];
    std::string comando;

    if (aux.tipo == "py") comando = "python3 '" + aux.path + "' " + argumentos;
    else if (aux.tipo == "sh") comando = "bash '" + aux.path + "' " + argumentos;
    else if (aux.tipo == "pr") comando = "perl '" + aux.path + "' " + argumentos;
    else if (aux.tipo == "c")  comando = "./'" + aux.path + "' " + argumentos;
    else {
        std::cout << RED << "[!] Tipo de auxiliar no soportado: " << aux.tipo << RESET << std::endl;
        return;
    }

    if (!entorno.empty()) {
        std::filesystem::path activate = std::filesystem::path(python_envs_dir) / entorno / "bin/activate";
        if (!std::filesystem::exists(activate)) {
            std::cout << RED << "[!] Entorno virtual '" << entorno << "' no encontrado en " << activate.parent_path() << RESET << std::endl;
            std::cout << "    Usa " << BOLD << "aux -lp" << RESET << " para listar entornos disponibles.\n";
            return;
        }
        comando = "source '" + activate.string() + "' && " + comando;
    }

    std::cout << "[*] Ejecutando: " << comando << std::endl;
    int ret = system(comando.c_str());
    if (ret == -1) {
        std::cout << RED << "[!] Error al ejecutar el auxiliar." << RESET << std::endl;
    }
}

// --------- Ejecutar auxiliar por tipo (con -p:) ---------
void run_aux_tipo(const std::string& tipo, const std::string& nombre, std::string argumentos) {
    std::string entorno = extraer_y_quitar_parametro_env(argumentos);
    if (entorno.empty())
        entorno = leer_env_por_defecto();

    if (!aux_cache_ready) build_aux_cache("");
    for (const auto& aux : aux_cache) {
        if (aux.name == nombre && aux.tipo == tipo) {
            std::string comando;
            if (tipo == "py") comando = "python3 '" + aux.path + "' " + argumentos;
            else if (tipo == "sh") comando = "bash '" + aux.path + "' " + argumentos;
            else if (tipo == "pr") comando = "perl '" + aux.path + "' " + argumentos;
            else if (tipo == "c")  comando = "./'" + aux.path + "' " + argumentos;
            else {
                std::cout << RED << "[!] Tipo de auxiliar no soportado: " << tipo << RESET << std::endl;
                return;
            }
            if (!entorno.empty()) {
                std::filesystem::path activate = std::filesystem::path(python_envs_dir) / entorno / "bin/activate";
                if (!std::filesystem::exists(activate)) {
                    std::cout << RED << "[!] Entorno virtual '" << entorno << "' no encontrado en " << activate.parent_path() << RESET << std::endl;
                    std::cout << "    Usa " << BOLD << "aux -lp" << RESET << " para listar entornos disponibles.\n";
                    return;
                }
                comando = "source '" + activate.string() + "' && " + comando;
            }
            std::cout << "[*] Ejecutando: " << comando << std::endl;
            int ret = system(comando.c_str());
            if (ret == -1) {
                std::cout << RED << "[!] Error al ejecutar el auxiliar." << RESET << std::endl;
            }
            return;
        }
    }
    std::cout << RED << "[!] Auxiliar no encontrado: " << nombre << " de tipo " << tipo << RESET << std::endl;
}

void print_usage_aux() {
    std::cout << BOLD << CYAN << "\nComandos auxiliares disponibles:\n" << RESET
              << GREEN << "  aux -l, --list                    " << RESET << "    Lista los auxiliares (nombre y descripción)\n"
              << GREEN << "  aux -lp                           " << RESET << "    Lista los entornos virtuales Python detectados\n"
              << GREEN << "  aux -run <nombre> [args...]       " << RESET << "    Ejecuta un auxiliar (tipo autodetectado)\n"
              << GREEN << "  aux -run <tipo> <nombre> [args...]" << RESET << "    Forzar tipo si hay ambigüedad\n"
              << GREEN << "        -p:<entorno>                " << RESET << "    Ejecuta el auxiliar en el entorno virtual Python indicado\n"
              << "                                    Tipos soportados: "
              << BOLD << "py" << RESET << ", "
              << BOLD << "sh" << RESET << ", "
              << BOLD << "pr" << RESET << ", "
              << BOLD << "c"  << RESET << "\n"
              << GREEN << "  aux -h, --help                    " << RESET << "    Muestra esta ayuda\n"
              << std::endl;
}

// Entrada principal del comando aux
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
        list_python_envs();
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
    print_usage_aux();
}
