#pragma once
#include <string>
using namespace std;

// Nivel de servicio del envio. El valor numerico ES la prioridad:
// a menor valor, mayor prioridad (EXPRESS > PRIORITARIO > ESTANDAR).
enum class NivelServicio {
    EXPRESS = 1,
    PRIORITARIO = 2,
    ESTANDAR = 3
};

// Ciclo de vida de un envio dentro del centro de distribucion.
enum class Estado {
    RECIBIDO,
    CLASIFICADO,
    EN_REPARTO,
    REPROGRAMADO,
    ENTREGADO
};

inline string nivelToString(NivelServicio n) {
    switch (n) {
        case NivelServicio::EXPRESS:     return "EXPRESS";
        case NivelServicio::PRIORITARIO: return "PRIORITARIO";
        case NivelServicio::ESTANDAR:    return "ESTANDAR";
    }
    return "DESCONOCIDO";
}

inline string estadoToString(Estado e) {
    switch (e) {
        case Estado::RECIBIDO:     return "RECIBIDO";
        case Estado::CLASIFICADO:  return "CLASIFICADO";
        case Estado::EN_REPARTO:   return "EN_REPARTO";
        case Estado::REPROGRAMADO: return "REPROGRAMADO";
        case Estado::ENTREGADO:    return "ENTREGADO";
    }
    return "DESCONOCIDO";
}

// Convierte la opcion numerica que ingresa el usuario por consola
// (0=RECIBIDO ... 4=ENTREGADO) a Estado. Devuelve false si no es valida.
inline bool intAEstado(int valor, Estado& out) {
    if (valor < 0 || valor > 4) return false;
    out = static_cast<Estado>(valor);
    return true;
}

// Convierte la opcion numerica del menu (1=EXPRESS, 2=PRIORITARIO, 3=ESTANDAR)
// a NivelServicio. Devuelve false si no es valida.
inline bool intANivel(int valor, NivelServicio& out) {
    if (valor < 1 || valor > 3) return false;
    out = static_cast<NivelServicio>(valor);
    return true;
}
