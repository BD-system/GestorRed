// /gestor_red/BBDD/UtilsSQL.hpp

#pragma once
#include <string>
#include <vector>
#include <map>

namespace UtilsSQL {

    // Puertos abiertos <-> string (CSV)
    std::string puertosToString(const std::vector<int>& puertos);
    std::vector<int> stringToPuertos(const std::string& str);

    // Banners <-> string (JSON)
    std::string bannersToString(const std::map<int, std::string>& banners);
    std::map<int, std::string> stringToBanners(const std::string& str);

    // Vulnerabilidades <-> string (JSON)
    std::string vulnerabilidadesToString(const std::vector<std::string>& vulns);
    std::vector<std::string> stringToVulnerabilidades(const std::string& str);
}
