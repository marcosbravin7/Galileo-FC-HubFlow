#pragma once
#include "Envio.h"
#include <iostream>
#include <string>
using namespace std;

// Nodo de la lista simplemente enlazada de pendientes.
struct NodoPendiente {
    Envio* envio;
    NodoPendiente* siguiente;

    NodoPendiente(Envio* e) : envio(e), siguiente(nullptr) {}
};

// Cola de prioridad estable, implementada a mano como lista simplemente
// enlazada ordenada por nivel de servicio: EXPRESS(1) > PRIORITARIO(2) > ESTANDAR(3).
// Implementada por Marcos Bravin y Lisandro Menendez.
//
// OWNERSHIP: esta lista NUNCA es duena de los Envio* que referencia (esos
// pertenecen a ListaDeEnvios). Su destructor solo libera sus propios nodos.
class ListaPendientes {
private:
    NodoPendiente* comienzo;

public:
    ListaPendientes() : comienzo(nullptr) {}

    // Administra nodos propios: no se permite copiar (evita un doble delete
    // si dos listas terminaran compartiendo la misma cadena de nodos).
    ListaPendientes(const ListaPendientes&) = delete;
    ListaPendientes& operator=(const ListaPendientes&) = delete;

    ~ListaPendientes() {
        NodoPendiente* actual = comienzo;
        while (actual != nullptr) {
            NodoPendiente* siguienteNodo = actual->siguiente;
            delete actual;   // solo borra el nodo, NUNCA el Envio (lo owna ListaDeEnvios)
            actual = siguienteNodo;
        }
    }

    // RF01/RF06 — insercion ordenada por prioridad estable: contempla lista
    // vacia, insercion al inicio, al medio y al final, conservando el orden
    // de llegada entre envios de igual prioridad.
    void agregar(Envio* envio) {
        NodoPendiente* nuevo = new NodoPendiente(envio);

        if (comienzo == nullptr || envio->getNivel() < comienzo->envio->getNivel()) {
            nuevo->siguiente = comienzo;
            comienzo = nuevo;
            return;
        }

        NodoPendiente* aux = comienzo;
        while (aux->siguiente != nullptr && aux->siguiente->envio->getNivel() <= envio->getNivel()) {
            aux = aux->siguiente;
        }
        nuevo->siguiente = aux->siguiente;
        aux->siguiente = nuevo;
    }

    // RF05 — el proximo envio a despachar es siempre el primer nodo.
    // Extrae ese nodo y devuelve el Envio* que contenia (sin destruirlo).
    Envio* despachar() {
        if (comienzo == nullptr) return nullptr;

        NodoPendiente* nodoAEliminar = comienzo;
        Envio* envio = nodoAEliminar->envio;
        comienzo = comienzo->siguiente;
        delete nodoAEliminar;
        return envio;
    }

    bool estaVacia() const { return comienzo == nullptr; }

    // Elimina (si existe) el nodo que referencia a `envio`, sin destruir el
    // Envio. Se usa para mantener consistencia cuando un envio pasa a un
    // estado final (ENTREGADO) sin haber pasado por despachar(), por ejemplo
    // si se lo cambia directamente con "cambiar estado". Devuelve true si
    // encontro y quito el nodo.
    bool remover(Envio* envio) {
        if (comienzo == nullptr || envio == nullptr) return false;

        if (comienzo->envio == envio) {
            NodoPendiente* aBorrar = comienzo;
            comienzo = comienzo->siguiente;
            delete aBorrar;
            return true;
        }

        NodoPendiente* aux = comienzo;
        while (aux->siguiente != nullptr && aux->siguiente->envio != envio) {
            aux = aux->siguiente;
        }
        if (aux->siguiente == nullptr) return false;

        NodoPendiente* aBorrar = aux->siguiente;
        aux->siguiente = aBorrar->siguiente;
        delete aBorrar;
        return true;
    }

    // RF02 — muestra todos los pendientes respetando el orden actual de la lista.
    void mostrar() const {
        if (comienzo == nullptr) {
            cout << "  (no hay envios pendientes)\n";
            return;
        }
        NodoPendiente* aux = comienzo;
        while (aux != nullptr) {
            cout << "  ";
            aux->envio->mostrar();
            aux = aux->siguiente;
        }
    }

    // Resumen recursivo por zona (Seccion 12 de la consigna).
    struct ResumenZona {
        int cantidad = 0;
        double pesoTotal = 0.0;
        int cantExpress = 0;
    };

    ResumenZona resumenPorZona(const string& zona) const {
        return resumirZonaRec(comienzo, zona);
    }

    // Desafio adicional (Seccion 17): envio de mayor peso de una zona,
    // resuelto tambien de forma recursiva.
    Envio* envioMasPesadoDeZona(const string& zona) const {
        return masPesadoDeZonaRec(comienzo, zona);
    }

private:
    // Caso base: lista vacia -> resumen neutro. Caso recursivo: primero se
    // resuelve el resto de la lista y despues se procesa el nodo actual,
    // combinando su aporte con lo que devolvio la llamada recursiva.
    static ResumenZona resumirZonaRec(NodoPendiente* nodo, const string& zona) {
        if (nodo == nullptr) return ResumenZona();

        ResumenZona r = resumirZonaRec(nodo->siguiente, zona);

        if (nodo->envio->getZona() == zona) {
            r.cantidad++;
            r.pesoTotal += nodo->envio->getPeso();
            if (nodo->envio->getNivel() == NivelServicio::EXPRESS) r.cantExpress++;
        }
        return r;
    }

    // Caso base: lista vacia -> no hay candidato. Caso recursivo: se compara
    // el nodo actual contra el mejor candidato del resto de la lista.
    static Envio* masPesadoDeZonaRec(NodoPendiente* nodo, const string& zona) {
        if (nodo == nullptr) return nullptr;

        Envio* mejorDelResto = masPesadoDeZonaRec(nodo->siguiente, zona);
        Envio* actual = nodo->envio;

        if (actual->getZona() != zona) return mejorDelResto;
        if (mejorDelResto == nullptr) return actual;
        return (actual->getPeso() >= mejorDelResto->getPeso()) ? actual : mejorDelResto;
    }
};
