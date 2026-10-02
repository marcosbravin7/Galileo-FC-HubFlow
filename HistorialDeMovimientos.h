#pragma once
#include "Movimiento.h"
#include <iostream>
using namespace std;

// Nodo de la lista doblemente enlazada de movimientos.
struct NodoMovimiento {
    Movimiento* movimiento;
    NodoMovimiento* siguiente;
    NodoMovimiento* anterior;

    NodoMovimiento(Movimiento* m)
        : movimiento(m), siguiente(nullptr), anterior(nullptr) {}
};

// Historial de un envio, implementado con una lista doblemente enlazada.
// Implementada por Angela Contrera.
// OWNERSHIP: es duena de sus Movimiento* y de sus nodos; los libera en cascada
// en su destructor.
class HistorialDeMovimientos {
private:
    NodoMovimiento* cabeza;
    NodoMovimiento* cola;
    int contador;

public:
    HistorialDeMovimientos() : cabeza(nullptr), cola(nullptr), contador(0) {}

    // Administra punteros crudos de los que es duena: no se permite copiar
    // (evita un doble delete si dos historiales terminaran apuntando a los
    // mismos nodos).
    HistorialDeMovimientos(const HistorialDeMovimientos&) = delete;
    HistorialDeMovimientos& operator=(const HistorialDeMovimientos&) = delete;

    ~HistorialDeMovimientos() {
        NodoMovimiento* actual = cabeza;
        while (actual != nullptr) {
            NodoMovimiento* siguienteNodo = actual->siguiente;
            delete actual->movimiento;
            delete actual;
            actual = siguienteNodo;
        }
    }

    // RF04 / RF07 — agrega un nuevo movimiento al final del historial (O(1)
    // gracias al puntero cola).
    void agregarMovimiento(const string& estado, const string& observacion) {
        contador++;
        Movimiento* m = new Movimiento(contador, estado, observacion);
        NodoMovimiento* nodo = new NodoMovimiento(m);

        if (cabeza == nullptr) {
            cabeza = nodo;
            cola = nodo;
        } else {
            cola->siguiente = nodo;
            nodo->anterior = cola;
            cola = nodo;
        }
    }

    // RF08 — recorre del mas antiguo al mas reciente.
    void mostrarCronologico() const {
        if (cabeza == nullptr) {
            cout << "  (sin movimientos registrados)\n";
            return;
        }
        NodoMovimiento* aux = cabeza;
        while (aux != nullptr) {
            cout << "  " << aux->movimiento->numero
                 << " | " << aux->movimiento->estado
                 << " | " << aux->movimiento->observacion << "\n";
            aux = aux->siguiente;
        }
    }

    // RF08 — recorre del mas reciente al mas antiguo, usando el puntero anterior.
    void mostrarInverso() const {
        if (cola == nullptr) {
            cout << "  (sin movimientos registrados)\n";
            return;
        }
        NodoMovimiento* aux = cola;
        while (aux != nullptr) {
            cout << "  " << aux->movimiento->numero
                 << " | " << aux->movimiento->estado
                 << " | " << aux->movimiento->observacion << "\n";
            aux = aux->anterior;
        }
    }
};
