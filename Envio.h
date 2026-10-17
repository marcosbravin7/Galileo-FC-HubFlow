#pragma once
#include "Estados.h"
#include "HistorialDeMovimientos.h"
#include "BitSet.h"
#include <iostream>
#include <iomanip>
#include <string>
using namespace std;

// Representa un paquete que circula por el centro de distribucion.
// Implementada por Santiago Negreira.
//
// OWNERSHIP: un Envio es dueno de su HistorialDeMovimientos (lo tiene como
// miembro por valor), pero NO es dueno de si mismo: quien lo crea con `new`
// y lo destruye es ListaDeEnvios (ver ListaDeEnvios.h).
class Envio {
private:
    string codigo;
    string destinatario;
    string zona;
    double peso;
    NivelServicio nivel;
    Estado estado;
    int intentos;
    HistorialDeMovimientos historial;
    // RF12 — estados por los que paso el envio (un bit por estado). Guarda la
    // historia, no solo el estado actual: los bits se prenden y no se apagan.
    BitSet estadosVisitados;

public:
    Envio(string cod, string dest, string zon, double p, NivelServicio niv)
        : codigo(move(cod)), destinatario(move(dest)), zona(move(zon)), peso(p),
          nivel(niv), estado(Estado::RECIBIDO), intentos(0),
          estadosVisitados(BitSet::CANT_ESTADOS) {
        estadosVisitados.set(static_cast<int>(estado));   // todo envio arranca RECIBIDO
        historial.agregarMovimiento(estadoToString(estado), "Ingreso al centro de distribucion");
    }

    // Un Envio administra su historial: no se permite copiar (evita un
    // doble delete si dos envios terminaran compartiendo los mismos nodos).
    Envio(const Envio&) = delete;
    Envio& operator=(const Envio&) = delete;

    // ---- consultas ----
    const string& getCodigo()       const { return codigo; }
    const string& getDestinatario() const { return destinatario; }
    const string& getZona()         const { return zona; }
    double getPeso()                const { return peso; }
    NivelServicio getNivel()        const { return nivel; }
    Estado getEstado()              const { return estado; }
    int getIntentos()               const { return intentos; }
    bool estaEntregado()            const { return estado == Estado::ENTREGADO; }
    const BitSet& getEstadosVisitados() const { return estadosVisitados; }
    bool estuvoEn(Estado e)         const { return estadosVisitados.test(static_cast<int>(e)); }

    // Cambia el estado Y deja constancia en el historial en un solo paso,
    // para que sea imposible actualizar uno sin el otro.
    // RF12.3 — aca tambien se prende el bit del nuevo estado, asi el BitSet se
    // actualiza solo con cualquier cambio (cambiar estado, despachar,
    // reprogramar o entregar pasan todos por este metodo). El valor del enum
    // Estado coincide con la posicion del bit.
    void cambiarEstado(Estado nuevoEstado, const string& observacion) {
        estado = nuevoEstado;
        estadosVisitados.set(static_cast<int>(nuevoEstado));
        historial.agregarMovimiento(estadoToString(nuevoEstado), observacion);
    }

    void sumarIntento() { intentos++; }

    void mostrarHistorialCronologico() const { historial.mostrarCronologico(); }
    void mostrarHistorialInverso()     const { historial.mostrarInverso(); }

    // RF02 — Codigo | Zona | Peso | Servicio | Estado | Intentos
    void mostrar() const {
        cout << codigo
             << " | " << zona
             << " | " << fixed << setprecision(2) << peso << " kg"
             << " | " << nivelToString(nivel)
             << " | " << estadoToString(estado)
             << " | intentos: " << intentos << "\n";
    }
};
