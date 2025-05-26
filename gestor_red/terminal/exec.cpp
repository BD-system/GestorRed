#include "exec.hpp"
#include "style_terminal.hpp"
#include <iostream>
#include <string>
#include <map>
#include <filesystem>
#include <dlfcn.h>
#include <iomanip>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <signal.h>
extern "C" const char *strsignal(int);

// Ruta absoluta configurable para módulos
std::string modules_dir = "/root/gestor_de_red/modules"; // <-- CAMBIA AQUÍ según tu entorno

// --- Estructura del módulo ---
struct ModuleInfo {
    std::string path;           // Ruta completa al .so
    std::string description;    // Descripción del módulo
    std::string payload_dir;    // Carpeta privada (vacía si no existe)
};
static std::map<std::string, ModuleInfo> module_cache;
static bool cache_created = false;

// --- Helpers ---
std::string get_modulo(const std::string& args, std::string& modulo_args) {
    size_t space = args.find(' ');
    if (space != std::string::npos) {
        modulo_args = args.substr(space + 1);
        return args.substr(0, space);
    } else {
        modulo_args = "";
        return args;
    }
}

void print_usage() {
    std::cout << BOLD << CYAN << "\nComandos disponibles:\n" << RESET
              << GREEN << "  <modulo> [args]" << RESET << "      Ejecuta el módulo indicado con argumentos opcionales\n"
              << GREEN << "  <modulo> -h" << RESET << "           Muestra la ayuda específica de un módulo\n"
              << GREEN << "  -l, --list" << RESET << "            Lista todos los módulos disponibles\n"
              << GREEN << "  --reload" << RESET << "              Recarga la caché de módulos\n"
              << std::endl

              << BOLD << CYAN << "Gestión de payloads y archivos:\n" << RESET
              << "  Cuando un módulo necesita un archivo (payload):\n"
              << "    - Si sólo indicas el nombre del archivo (por ejemplo, " << BOLD << "SO_2.bin" << RESET << "),\n"
              << "      la terminal lo busca automáticamente en la carpeta privada del módulo,\n"
              << "      por ejemplo: " << BOLD << "modules/escaneo/.icmp_payload/SO_2.bin" << RESET << "\n"
              << "    - Si usas el prefijo " << BOLD << "modules:/" << RESET << " (por ejemplo, "
              << BOLD << "modules:/payloads_escaneo/SO_2.bin" << RESET << "),\n"
              << "      lo buscará en la ruta compartida: " << BOLD << "/root/gestor_de_red/src/modules/payloads_escaneo/SO_2.bin" << RESET << "\n"
              << std::endl

              << BOLD << CYAN << "Ejemplos de uso:\n" << RESET
              << "  " << GREEN << "exec icmp_payload execmem_send 192.168.0.33 SO_2.bin 21" << RESET << "\n"
              << "      (Busca SO_2.bin en la carpeta privada del módulo)\n"
              << "  " << GREEN << "exec icmp_payload execmem_send 192.168.0.33 modules:/payloads_escaneo/SO_2.bin 21" << RESET << "\n"
              << "      (Busca SO_2.bin en la carpeta compartida src/modules/payloads_escaneo/)\n"
              << std::endl

              << "Puedes crear tus propios módulos en C/C++ siguiendo la plantilla incluida, "
              << "y aprovechar rutas inteligentes para los payloads.\n"
              << std::endl;
}


void print_no_modulo() {
    std::cout << RED << "[!] Debes especificar un módulo válido." << RESET << std::endl;
    std::cout << "    Usa " << BOLD << "exec -l / --list" << RESET << " para listar módulos, o "
              << BOLD << "exec -h / --help" << RESET << " para ver la ayuda.\n";
}

void print_modulo_not_found(const std::string& modulo) {
    std::cout << RED << "[!] Módulo no encontrado: " << modulo << RESET << std::endl;
    std::cout << "    Usa " << BOLD << "exec -l / --list" << RESET << " para listar módulos.\n";
}

// --- Núcleo de módulos ---
void build_module_cache(const std::string& base_path) {
    module_cache.clear();
    std::string use_path = base_path.empty() ? modules_dir : base_path;
    if (!std::filesystem::exists(use_path)) {
        std::cout << RED << "[!] No se encontró el directorio de módulos: " << use_path << RESET << std::endl;
        cache_created = false;
        return;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(use_path)) {
        if (entry.path().extension() != ".so") continue;

        void* handle = dlopen(entry.path().c_str(), RTLD_LAZY);
        if (!handle) continue;

        using name_fn_t = const char* (*)();
        using desc_fn_t = const char* (*)();

        name_fn_t name_fn = (name_fn_t)dlsym(handle, "name");
        desc_fn_t desc_fn = (desc_fn_t)dlsym(handle, "description");

        std::string name = name_fn ? name_fn() : entry.path().stem().string();
        std::string desc = desc_fn ? desc_fn() : "Sin descripción";

        // Calcula la carpeta de payloads: modules/.../.nombre_modulo
        std::string payload_dir;
        std::filesystem::path candidate = entry.path().parent_path() / ("." + entry.path().stem().string());
        if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate)) {
            payload_dir = candidate.string();
        }

        module_cache[name] = { entry.path().string(), desc, payload_dir };
        dlclose(handle);
    }
    cache_created = true;
}

void list_modules() {
    if (!cache_created) {
        build_module_cache(""); 
    }
    if (module_cache.empty()) {
        std::cout << YELLOW << "[!] No se encontraron módulos en: " << modules_dir << RESET << std::endl;
        return;
    }

    std::cout << BOLD << CYAN << "\nMódulos encontrados:\n" << RESET;

    const int ancho = 14;
    for (const auto& [name, info] : module_cache) {
        std::cout << "  " << GREEN << std::left << std::setw(ancho) << name << RESET
                  << info.description << std::endl;
    }

    std::cout << std::endl; // <-- Salto de línea final para separar del prompt
}

void reload_modules() {
    std::cout << "[*] Recargando módulos..." << std::endl;
    build_module_cache(""); 
    std::cout << "[OK] Módulos recargados." << std::endl;
}

// --- Ejecución de un módulo protegido ---
void run_modulo(const std::string& modulo, const std::string& modulo_args) {
    auto it = module_cache.find(modulo);
    if (modulo.empty() || it == module_cache.end()) {
        print_no_modulo();
        return;
    }

    // PROTECCIÓN: Ejecuta el módulo en un proceso hijo (fork)
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << RED << "[ERROR] No se pudo crear el proceso para el módulo" << RESET << std::endl;
        return;
    } else if (pid == 0) {
        // HIJO: si existe carpeta de payload, la exporto
        if (!it->second.payload_dir.empty()) {
            setenv("GDR_PAYLOAD_PATH", it->second.payload_dir.c_str(), 1);
        }

        // carga y ejecuta el módulo .so
        void* handle = dlopen(it->second.path.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cout << RED << "[!] Error cargando módulo: " << it->second.path << RESET << std::endl;
            _exit(1);
        }

        if (modulo_args == "-h" || modulo_args == "--help") {
            using help_fn_t = void (*)();
            help_fn_t help_fn = (help_fn_t)dlsym(handle, "help_exec");
            if (help_fn) {
                help_fn();
            } else {
                std::cout << "[!] El módulo no implementa ayuda específica." << std::endl;
            }
            dlclose(handle);
            _exit(0);
        }

        using run_fn_t = void (*)(const char*);
        run_fn_t run_fn = (run_fn_t)dlsym(handle, "run");
        if (run_fn) {
            run_fn(modulo_args.c_str());
            dlclose(handle);
            _exit(0);
        } else {
            std::cout << "[!] El módulo no implementa función run()." << std::endl;
            dlclose(handle);
            _exit(2);
        }
    } else {
        // PADRE: espera al hijo y maneja señales/errores
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFSIGNALED(status)) {
            std::cerr << RED << "[ERROR] El módulo terminó por señal: "
                      << strsignal(WTERMSIG(status)) << RESET << std::endl;
        } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            std::cerr << RED << "[ERROR] El módulo devolvió código: "
                      << WEXITSTATUS(status) << RESET << std::endl;
        }
        // Si todo OK, nada se muestra
    }
}

// --- Entrada principal ---
void exec(const std::string& argumentos) {
    std::string args = argumentos;
    while (!args.empty() && isspace(args.front())) args.erase(args.begin());

    if (args.empty() || args == "-h" || args == "--help") {
        print_usage();
        return;
    }
    if (args == "-l" || args == "--list") {
        list_modules();
        return;
    }
    if (args == "--reload") {
        reload_modules();
        return;
    }

    std::string modulo_args;
    std::string modulo = get_modulo(args, modulo_args);

    if (!cache_created) build_module_cache(""); // usa modules_dir por defecto

    run_modulo(modulo, modulo_args);
}
