#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <map>

class EquiposRed {
private:
    // Atributos (ordenados para evitar warnings de reorden)
    std::string ip;
    std::string mac;
    std::string hostname;
    std::string metodoDescubrimiento;
    std::vector<int> puertosAbiertos;
    std::map<int, std::string> banners;
    std::string sistemaOperativo;
    std::string interfazDetectada;
    std::string estado;
    int ttl;
    int rttMs;
    std::string notas;
    std::string vendor;
    std::string timestamp;
    std::vector<std::string> vulnerabilidades;
    std::string notasDetalladas;

public:
    // Constructor principal con inicialización directa
    EquiposRed(const std::string& ip, const std::string& estado = "vacía")
        : ip(ip), estado(estado), ttl(-1), rttMs(-1)
    {
    }

    // Constructor de copia con mensaje de debug (activado solo si DEBUG está definido)
    EquiposRed(const EquiposRed& other)
        : ip(other.ip), mac(other.mac), hostname(other.hostname),
          metodoDescubrimiento(other.metodoDescubrimiento),
          puertosAbiertos(other.puertosAbiertos),
          banners(other.banners),
          sistemaOperativo(other.sistemaOperativo),
          interfazDetectada(other.interfazDetectada),
          estado(other.estado),
          ttl(other.ttl),
          rttMs(other.rttMs),
          notas(other.notas),
          vendor(other.vendor),
          timestamp(other.timestamp),
          vulnerabilidades(other.vulnerabilidades),
          notasDetalladas(other.notasDetalladas)
    {
    #ifdef DEBUG
        std::cout << "[DEBUG] Constructor de copia ejecutado para IP: " << ip << "\n";
    #endif
    }

    // Constructor por defecto
    EquiposRed() : estado("vacía"), ttl(-1), rttMs(-1) {}

    // Constructor de movimiento
    EquiposRed(EquiposRed&& other) noexcept
        : ip(std::move(other.ip)), mac(std::move(other.mac)), hostname(std::move(other.hostname)),
          metodoDescubrimiento(std::move(other.metodoDescubrimiento)),
          puertosAbiertos(std::move(other.puertosAbiertos)),
          banners(std::move(other.banners)),
          sistemaOperativo(std::move(other.sistemaOperativo)),
          interfazDetectada(std::move(other.interfazDetectada)),
          estado(std::move(other.estado)),
          ttl(other.ttl),
          rttMs(other.rttMs),
          notas(std::move(other.notas)),
          vendor(std::move(other.vendor)),
          timestamp(std::move(other.timestamp)),
          vulnerabilidades(std::move(other.vulnerabilidades)),
          notasDetalladas(std::move(other.notasDetalladas))
    {}

    // Operador de asignación por movimiento
    EquiposRed& operator=(EquiposRed&& other) noexcept {
        if (this != &other) {
            ip = std::move(other.ip);
            mac = std::move(other.mac);
            hostname = std::move(other.hostname);
            metodoDescubrimiento = std::move(other.metodoDescubrimiento);
            puertosAbiertos = std::move(other.puertosAbiertos);
            banners = std::move(other.banners);
            sistemaOperativo = std::move(other.sistemaOperativo);
            interfazDetectada = std::move(other.interfazDetectada);
            estado = std::move(other.estado);
            ttl = other.ttl;
            rttMs = other.rttMs;
            notas = std::move(other.notas);
            vendor = std::move(other.vendor);
            timestamp = std::move(other.timestamp);
            vulnerabilidades = std::move(other.vulnerabilidades);
            notasDetalladas = std::move(other.notasDetalladas);
        }
        return *this;
    }

    // Operador de asignación de copia
    EquiposRed& operator=(const EquiposRed& other) {
        if (this != &other) {
            ip = other.ip;
            mac = other.mac;
            hostname = other.hostname;
            metodoDescubrimiento = other.metodoDescubrimiento;
            puertosAbiertos = other.puertosAbiertos;
            banners = other.banners;
            sistemaOperativo = other.sistemaOperativo;
            interfazDetectada = other.interfazDetectada;
            estado = other.estado;
            ttl = other.ttl;
            rttMs = other.rttMs;
            notas = other.notas;
            vendor = other.vendor;
            timestamp = other.timestamp;
            vulnerabilidades = other.vulnerabilidades;
            notasDetalladas = other.notasDetalladas;
        }
        return *this;
    }

    // Setters
    void setMAC(const std::string& m) { mac = m; }
    void setHostname(const std::string& h) { hostname = h; }
    void setMetodo(const std::string& m) { metodoDescubrimiento = m; }
    void setSO(const std::string& so) { sistemaOperativo = so; }
    void setInterfaz(const std::string& i) { interfazDetectada = i; }
    void setEstado(const std::string& e) { estado = e; }
    void setTTL(int t) { ttl = t; }
    void setRTT(int r) { rttMs = r; }
    void setNotas(const std::string& n) { notas = n; }
    void addPuerto(int puerto) { puertosAbiertos.push_back(puerto); }
    void addBanner(int puerto, const std::string& banner) { banners[puerto] = banner; }
    void setVendor(const std::string& v) { vendor = v; }
    void setTimestamp(const std::string& ts) { timestamp = ts; }
    void addVulnerabilidad(const std::string& vuln) { vulnerabilidades.push_back(vuln); }
    void setNotasDetalladas(const std::string& n) { notasDetalladas = n; }

    // Getters
    std::string getIP() const { return ip; }
    std::string getMAC() const { return mac; }
    std::string getHostname() const { return hostname; }
    std::string getMetodo() const { return metodoDescubrimiento; }
    std::vector<int> getPuertos() const { return puertosAbiertos; }
    std::map<int, std::string> getBanners() const { return banners; }
    std::string getSO() const { return sistemaOperativo; }
    std::string getInterfaz() const { return interfazDetectada; }
    std::string getEstado() const { return estado; }
    int getTTL() const { return ttl; }
    int getRTT() const { return rttMs; }
    std::string getNotas() const { return notas; }
    std::string getVendor() const { return vendor; }
    std::string getTimestamp() const { return timestamp; }
    std::vector<std::string> getVulnerabilidades() const { return vulnerabilidades; }
    std::string getNotasDetalladas() const { return notasDetalladas; }

    // Métodos de análisis (a implementar según se requiera)
    void realizarARPScan();
    void realizarPingSweep();

    // Métodos estáticos de análisis por fases
    static std::vector<EquiposRed> analizarRedFase1(const std::string& ipRed,
                                                    const std::string& broadcast,
                                                    const std::string& interfaz);
    static std::vector<EquiposRed> analizarRedFase2(const std::vector<EquiposRed>& dispositivos, const std::string& interfaz);
    static std::vector<EquiposRed> analizarRedFase3(...); // Pendiente

    // Controlador general, que ahora utiliza la segunda fase:
    // Controlador general, que ahora utiliza la segunda fase:
    static std::vector<EquiposRed> analizarRed_gestor(const std::string& ipRed,
                                                   const std::string& broadcast,
                                                   const std::string& interfaz);


    // Depuración / Resumen
    void imprimirResumen() const {
        std::cout << "-------------------------------\n";
        std::cout << "IP: " << ip << "  [Estado: " << estado << "]\n";
        if (!mac.empty()) std::cout << "MAC: " << mac << "\n";
        if (!hostname.empty()) std::cout << "Hostname: " << hostname << "\n";
        if (!metodoDescubrimiento.empty()) std::cout << "Método: " << metodoDescubrimiento << "\n";
        if (!interfazDetectada.empty()) std::cout << "Interfaz: " << interfazDetectada << "\n";
        if (!puertosAbiertos.empty()) {
            std::cout << "Puertos abiertos: ";
            for (auto p : puertosAbiertos) std::cout << p << " ";
            std::cout << "\n";
        }
        if (!banners.empty()) {
            std::cout << "Banners:\n";
            for (const auto& par : banners) {
                std::cout << "  Puerto " << par.first << ": " << par.second << "\n";
            }
        }
        if (!sistemaOperativo.empty()) std::cout << "Sistema operativo: " << sistemaOperativo << "\n";
        if (ttl >= 0) std::cout << "TTL: " << ttl << "\n";
        if (rttMs >= 0) std::cout << "RTT: " << rttMs << " ms\n";
        if (!notas.empty()) std::cout << "Notas: " << notas << "\n";
        if (!vendor.empty()) std::cout << "Vendor: " << vendor << "\n";
        if (!timestamp.empty()) std::cout << "Timestamp: " << timestamp << "\n";
        if (!vulnerabilidades.empty()) {
            std::cout << "Vulnerabilidades:\n";
            for (const auto& v : vulnerabilidades)
                std::cout << "  " << v << "\n";
        }
        if (!notasDetalladas.empty()) std::cout << "Notas detalladas: " << notasDetalladas << "\n";
        std::cout << "-------------------------------\n";
    }
};

