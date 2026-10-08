#pragma once
#include "Envio.h"
#include <iostream>
using namespace std;

// Nodo de la Queue de preparacion. Solo estructura: apunta al envio (sin ser
// su dueno) y al siguiente nodo.
struct NodoCola {
    Envio* envio;
    NodoCola* siguiente;

    NodoCola(Envio* e) : envio(e), siguiente(nullptr) {}
};

// RF09.3 — Queue propia (FIFO) usada como `colaPreparacion`.
// Implementada con nodos y raw pointers, sin contenedores de la STL.
//
//   FRONT -> PKG-1002 -> PKG-1005 -> PKG-1008 <- REAR
//
// Mantiene punteros al primer (frente) y al ultimo (fondo) nodo, por lo que
// enqueue y dequeue son ambas O(1): enqueue agrega detras del fondo y dequeue
// saca el frente, sin recorrer nunca la cola.
//
// OWNERSHIP (RF09.8): la cola NUNCA es duena de los Envio* que referencia
// (son de ListaDeEnvios). Su destructor libera unicamente sus propios nodos
// y jamas ejecuta `delete` sobre un Envio.
class ColaEnvios {
private:
    NodoCola* frente;
    NodoCola* fondo;
    int cantidad;

public:
    ColaEnvios() : frente(nullptr), fondo(nullptr), cantidad(0) {}

    // Administra nodos propios: no se permite copiar (evita un doble delete
    // si dos colas terminaran compartiendo la misma cadena de nodos).
    ColaEnvios(const ColaEnvios&) = delete;
    ColaEnvios& operator=(const ColaEnvios&) = delete;

    ~ColaEnvios() { vaciar(); }

    // Agrega al final de la cola. O(1).
    void enqueue(Envio* envio) {
        if (envio == nullptr) return;
        NodoCola* nuevo = new NodoCola(envio);
        if (fondo == nullptr) {
            frente = nuevo;
            fondo = nuevo;
        } else {
            fondo->siguiente = nuevo;
            fondo = nuevo;
        }
        cantidad++;
    }

    // Saca y devuelve el primer envio (FIFO) sin destruirlo. nullptr si esta
    // vacia. O(1).
    Envio* dequeue() {
        if (frente == nullptr) return nullptr;
        NodoCola* nodo = frente;
        Envio* envio = nodo->envio;
        frente = frente->siguiente;
        if (frente == nullptr) fondo = nullptr;   // la cola quedo vacia
        delete nodo;                              // solo el nodo, NUNCA el Envio
        cantidad--;
        return envio;
    }

    // Devuelve el primer envio sin sacarlo. nullptr si esta vacia. O(1).
    Envio* front() const { return frente == nullptr ? nullptr : frente->envio; }

    bool isEmpty() const { return frente == nullptr; }
    int size() const { return cantidad; }

    // Libera solo los nodos (nunca los envios) y deja la cola vacia.
    void vaciar() {
        while (frente != nullptr) {
            NodoCola* siguienteNodo = frente->siguiente;
            delete frente;
            frente = siguienteNodo;
        }
        fondo = nullptr;
        cantidad = 0;
    }

    // FRONT -> A -> B -> C <- REAR
    void mostrar() const {
        if (frente == nullptr) {
            cout << "  (cola de preparacion vacia)\n";
            return;
        }
        cout << "  FRONT";
        for (NodoCola* aux = frente; aux != nullptr; aux = aux->siguiente) {
            cout << " -> " << aux->envio->getCodigo();
        }
        cout << " <- REAR\n";
    }
};
