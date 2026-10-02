#pragma once
#include <string>
using namespace std;

// Representa un evento en el ciclo de vida de un envio.
// Implementado por Angela Contrera.
struct Movimiento {
    int numero;
    string estado;
    string observacion;

    Movimiento(int n, string est, string obs)
        : numero(n), estado(move(est)), observacion(move(obs)) {}
};
