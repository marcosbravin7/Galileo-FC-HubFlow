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

// ---------------------------------------------------------------------------
// Maquina de estados del envio (RF04 / RF06 / RF07)
// ---------------------------------------------------------------------------
//   RECIBIDO     -> CLASIFICADO | EN_REPARTO
//   CLASIFICADO  -> EN_REPARTO
//   EN_REPARTO   -> REPROGRAMADO | ENTREGADO
//   REPROGRAMADO -> EN_REPARTO
//   ENTREGADO    -> (estado FINAL: no admite ningun cambio)
//
// Es el recorrido del ejemplo del RF08:
//   RECIBIDO, CLASIFICADO, EN_REPARTO, REPROGRAMADO, EN_REPARTO, ENTREGADO.
// (RECIBIDO -> EN_REPARTO existe porque "despachar" no obliga a clasificar.)
//
// Para cambiar las reglas basta con tocar esta unica funcion.
inline bool transicionPermitida(Estado desde, Estado hacia) {
    switch (desde) {
        case Estado::RECIBIDO:     return hacia == Estado::CLASIFICADO || hacia == Estado::EN_REPARTO;
        case Estado::CLASIFICADO:  return hacia == Estado::EN_REPARTO;
        case Estado::EN_REPARTO:   return hacia == Estado::REPROGRAMADO || hacia == Estado::ENTREGADO;
        case Estado::REPROGRAMADO: return hacia == Estado::EN_REPARTO;
        case Estado::ENTREGADO:    return false;
    }
    return false;
}

// Invariante del sistema: un envio figura en la lista de pendientes si y
// solo si su estado es RECIBIDO, CLASIFICADO o REPROGRAMADO. EN_REPARTO y
// ENTREGADO significan que ya salio de pendientes.
inline bool estadoEsPendiente(Estado e) {
    return e == Estado::RECIBIDO || e == Estado::CLASIFICADO || e == Estado::REPROGRAMADO;
}

// Texto con los estados a los que se puede pasar desde `desde` (para los mensajes de error).
inline string transicionesDesde(Estado desde) {
    switch (desde) {
        case Estado::RECIBIDO:     return "CLASIFICADO o EN_REPARTO";
        case Estado::CLASIFICADO:  return "EN_REPARTO";
        case Estado::EN_REPARTO:   return "REPROGRAMADO o ENTREGADO";
        case Estado::REPROGRAMADO: return "EN_REPARTO";
        case Estado::ENTREGADO:    return "ninguno (estado final)";
    }
    return "ninguno";
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
