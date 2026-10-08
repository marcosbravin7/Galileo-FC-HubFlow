#pragma once
#include "Envio.h"
#include <iostream>
using namespace std;

// Nodo de la Stack de procesados. Solo estructura: apunta al envio (sin ser
// su dueno) y al nodo que esta debajo.
struct NodoPila {
    Envio* envio;
    NodoPila* debajo;

    NodoPila(Envio* e) : envio(e), debajo(nullptr) {}
};

// RF09.4 — Stack propia (LIFO) usada como `pilaProcesados`.
// Implementada con nodos y raw pointers, sin contenedores de la STL.
//
//   TOP
//    |
//   PKG-1008
//   PKG-1005
//   PKG-1002
//
// push, pop y top son O(1): todo ocurre sobre el nodo de arriba (`tope`).
//
// Representa el historial INMEDIATO de procesamiento de la mesa. No
// reemplaza al historial permanente de movimientos de cada Envio.
//
// OWNERSHIP (RF09.8): la pila NUNCA es duena de los Envio* que referencia
// (son de ListaDeEnvios). Su destructor libera unicamente sus propios nodos
// y jamas ejecuta `delete` sobre un Envio.
class PilaEnvios {
private:
    NodoPila* tope;
    int cantidad;

public:
    PilaEnvios() : tope(nullptr), cantidad(0) {}

    // Administra nodos propios: no se permite copiar (evita un doble delete).
    PilaEnvios(const PilaEnvios&) = delete;
    PilaEnvios& operator=(const PilaEnvios&) = delete;

    ~PilaEnvios() { vaciar(); }

    // Apila un envio encima de los demas. O(1).
    void push(Envio* envio) {
        if (envio == nullptr) return;
        NodoPila* nuevo = new NodoPila(envio);
        nuevo->debajo = tope;
        tope = nuevo;
        cantidad++;
    }

    // Desapila y devuelve el envio de arriba (LIFO) sin destruirlo. nullptr
    // si esta vacia. O(1).
    Envio* pop() {
        if (tope == nullptr) return nullptr;
        NodoPila* nodo = tope;
        Envio* envio = nodo->envio;
        tope = tope->debajo;
        delete nodo;                 // solo el nodo, NUNCA el Envio
        cantidad--;
        return envio;
    }

    // Devuelve el envio de arriba sin desapilarlo. nullptr si esta vacia. O(1).
    Envio* top() const { return tope == nullptr ? nullptr : tope->envio; }

    bool isEmpty() const { return tope == nullptr; }
    int size() const { return cantidad; }

    // Libera solo los nodos (nunca los envios) y deja la pila vacia.
    void vaciar() {
        while (tope != nullptr) {
            NodoPila* nodoDebajo = tope->debajo;
            delete tope;
            tope = nodoDebajo;
        }
        cantidad = 0;
    }

    // Del tope hacia abajo.
    void mostrar() const {
        if (tope == nullptr) {
            cout << "  (pila de procesados vacia)\n";
            return;
        }
        cout << "  TOP\n";
        cout << "   |\n";
        for (NodoPila* aux = tope; aux != nullptr; aux = aux->debajo) {
            cout << "  " << aux->envio->getCodigo() << "\n";
        }
    }
};
