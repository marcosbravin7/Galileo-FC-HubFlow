#pragma once
#include "Envio.h"
#include <iostream>
#include <string>
using namespace std;

// Nodo de la lista general de envios. Solo estructura: apunta al envio
// y al siguiente nodo.
struct NodoEnvio {
    Envio* envio;
    NodoEnvio* siguiente;

    NodoEnvio(Envio* e) : envio(e), siguiente(nullptr) {}
};

// Lista simplemente enlazada con TODOS los envios que el centro conoce:
// los pendientes y tambien los ya despachados/entregados.
// Implementada por Zoe Corral y Santiago Negreira.
//
// OWNERSHIP: esta lista es la UNICA duena de los objetos Envio. Su
// destructor hace `delete nodo->envio` ademas de `delete nodo`.
// ListaPendientes (ver ListaPendientes.h), en cambio, solo borra sus
// nodos y nunca los envios: por eso despachar un paquete elimina su nodo
// de pendientes sin destruir el Envio, que sigue vivo aca y puede
// reprogramarse, entregarse o consultarse despues.
class ListaDeEnvios {
private:
    NodoEnvio* comienzo;
    NodoEnvio* ultimo;   // permite agregar al final en O(1)
    int cantidad;

public:
    ListaDeEnvios() : comienzo(nullptr), ultimo(nullptr), cantidad(0) {}

    // Administra punteros crudos de los que es duena: no se permite copiar.
    ListaDeEnvios(const ListaDeEnvios&) = delete;
    ListaDeEnvios& operator=(const ListaDeEnvios&) = delete;

    ~ListaDeEnvios() {
        NodoEnvio* actual = comienzo;
        while (actual != nullptr) {
            NodoEnvio* siguienteNodo = actual->siguiente;
            delete actual->envio;   // destruye el envio (y su historial, en cascada)
            delete actual;          // destruye el nodo
            actual = siguienteNodo;
        }
        comienzo = nullptr;
        ultimo = nullptr;
        cantidad = 0;
    }

    // Toma posesion del envio. Se agrega al final para conservar el orden
    // de ingreso al sistema.
    void agregar(Envio* e) {
        if (e == nullptr) return;
        NodoEnvio* nuevo = new NodoEnvio(e);
        if (comienzo == nullptr) {
            comienzo = nuevo;
            ultimo = nuevo;
        } else {
            ultimo->siguiente = nuevo;
            ultimo = nuevo;
        }
        cantidad++;
    }

    // RF03 — busca en TODOS los envios conocidos, no solo en los pendientes.
    Envio* buscar(const string& codigo) const {
        NodoEnvio* actual = comienzo;
        while (actual != nullptr) {
            if (actual->envio->getCodigo() == codigo) return actual->envio;
            actual = actual->siguiente;
        }
        return nullptr;
    }

    // RF01 — verifica que el codigo no este repetido.
    bool existeCodigo(const string& codigo) const { return buscar(codigo) != nullptr; }

    void mostrarTodos() const {
        if (comienzo == nullptr) {
            cout << "No hay envios registrados.\n";
            return;
        }
        NodoEnvio* actual = comienzo;
        while (actual != nullptr) {
            actual->envio->mostrar();
            actual = actual->siguiente;
        }
    }

    int getCantidad() const { return cantidad; }
    bool estaVacia()  const { return comienzo == nullptr; }
};
