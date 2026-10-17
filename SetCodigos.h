#pragma once
#include <iostream>
#include <string>
using namespace std;

// Nodo del Set: un codigo de seguimiento y el puntero al siguiente.
struct NodoSet {
    string codigo;
    NodoSet* siguiente;

    NodoSet(const string& c) : codigo(c), siguiente(nullptr) {}
};

// RF13 — Set de codigos de seguimiento hecho a mano (no se usa std::set ni
// std::unordered_set).
//
// Lo implementamos como una lista simplemente enlazada ORDENADA de menor a
// mayor y sin repetidos. Elegimos mantenerla ordenada porque:
//   * insert puede cortar apenas encuentra un codigo mayor y ahi mismo
//     detecta si el elemento ya estaba (no se admiten duplicados);
//   * union, interseccion y diferencia se resuelven recorriendo las dos
//     listas a la vez una sola vez (como el "merge" de merge sort), en lugar
//     de buscar cada elemento de un conjunto dentro del otro;
//   * los resultados se muestran ordenados, como en los ejemplos.
//
// El Set guarda solo los CODIGOS (strings), no los Envio. Para ver los datos
// completos de un envio del conjunto se busca el codigo en el arbol B+, asi no
// se duplican los objetos Envio.
//
// Memoria: el Set es dueno de sus nodos. Los crea con new al insertar (o al
// copiar otro Set) y los libera con delete al eliminar un elemento y en el
// destructor. Como las operaciones devuelven un Set nuevo por valor,
// implementamos el constructor de copia y el operador = haciendo una copia
// PROFUNDA (nodos nuevos), para que dos Set nunca compartan nodos y no haya
// doble delete.
//
// Nota: "union" es una palabra reservada de C++, por eso el metodo se llama
// unionCon.
class SetCodigos {
private:
    NodoSet* primero;
    int cantidad;

    void liberar() {
        NodoSet* actual = primero;
        while (actual != nullptr) {
            NodoSet* sig = actual->siguiente;
            delete actual;
            actual = sig;
        }
        primero = nullptr;
        cantidad = 0;
    }

    // Agrega al final usando un puntero al ultimo nodo. Solo se usa en las
    // operaciones, donde los codigos ya vienen ordenados y sin repetir, asi
    // que no hace falta buscar la posicion.
    void agregarAlFinal(const string& codigo, NodoSet*& ultimo) {
        NodoSet* nuevo = new NodoSet(codigo);
        if (ultimo == nullptr) primero = nuevo;
        else ultimo->siguiente = nuevo;
        ultimo = nuevo;
        cantidad++;
    }

    void copiarDe(const SetCodigos& otro) {
        NodoSet* ultimo = nullptr;
        for (NodoSet* aux = otro.primero; aux != nullptr; aux = aux->siguiente) {
            agregarAlFinal(aux->codigo, ultimo);
        }
    }

public:
    SetCodigos() : primero(nullptr), cantidad(0) {}

    SetCodigos(const SetCodigos& otro) : primero(nullptr), cantidad(0) { copiarDe(otro); }

    SetCodigos& operator=(const SetCodigos& otro) {
        if (this != &otro) {   // si es el mismo objeto no hay que hacer nada
            liberar();
            copiarDe(otro);
        }
        return *this;
    }

    ~SetCodigos() { liberar(); }

    int size() const { return cantidad; }
    bool empty() const { return primero == nullptr; }
    const NodoSet* getPrimero() const { return primero; }

    // Inserta el codigo en su lugar (orden ascendente). Si ya estaba no lo
    // agrega y devuelve false.
    bool insert(const string& codigo) {
        NodoSet* anterior = nullptr;
        NodoSet* actual = primero;
        while (actual != nullptr && actual->codigo < codigo) {
            anterior = actual;
            actual = actual->siguiente;
        }
        if (actual != nullptr && actual->codigo == codigo) return false;   // repetido

        NodoSet* nuevo = new NodoSet(codigo);
        nuevo->siguiente = actual;
        if (anterior == nullptr) primero = nuevo;
        else anterior->siguiente = nuevo;
        cantidad++;
        return true;
    }

    // Elimina el codigo. Devuelve false si no estaba.
    bool remove(const string& codigo) {
        NodoSet* anterior = nullptr;
        NodoSet* actual = primero;
        while (actual != nullptr && actual->codigo < codigo) {
            anterior = actual;
            actual = actual->siguiente;
        }
        if (actual == nullptr || actual->codigo != codigo) return false;

        if (anterior == nullptr) primero = actual->siguiente;
        else anterior->siguiente = actual->siguiente;
        delete actual;
        cantidad--;
        return true;
    }

    // Como la lista esta ordenada, se puede cortar apenas aparece un codigo
    // mayor al buscado.
    bool contains(const string& codigo) const {
        NodoSet* actual = primero;
        while (actual != nullptr && actual->codigo < codigo) actual = actual->siguiente;
        return actual != nullptr && actual->codigo == codigo;
    }

    // A U B: todos los codigos que estan en A o en B. Se avanza por las dos
    // listas a la vez; si un codigo esta en las dos se agrega una sola vez.
    SetCodigos unionCon(const SetCodigos& otro) const {
        SetCodigos resultado;
        NodoSet* ultimo = nullptr;
        NodoSet* a = primero;
        NodoSet* b = otro.primero;
        while (a != nullptr && b != nullptr) {
            if (a->codigo < b->codigo) {
                resultado.agregarAlFinal(a->codigo, ultimo);
                a = a->siguiente;
            } else if (b->codigo < a->codigo) {
                resultado.agregarAlFinal(b->codigo, ultimo);
                b = b->siguiente;
            } else {   // esta en los dos
                resultado.agregarAlFinal(a->codigo, ultimo);
                a = a->siguiente;
                b = b->siguiente;
            }
        }
        // lo que sobra de cualquiera de las dos listas tambien va
        for (; a != nullptr; a = a->siguiente) resultado.agregarAlFinal(a->codigo, ultimo);
        for (; b != nullptr; b = b->siguiente) resultado.agregarAlFinal(b->codigo, ultimo);
        return resultado;
    }

    // A interseccion B: solo los codigos que estan en los dos.
    SetCodigos intersection(const SetCodigos& otro) const {
        SetCodigos resultado;
        NodoSet* ultimo = nullptr;
        NodoSet* a = primero;
        NodoSet* b = otro.primero;
        while (a != nullptr && b != nullptr) {
            if (a->codigo < b->codigo) a = a->siguiente;
            else if (b->codigo < a->codigo) b = b->siguiente;
            else {
                resultado.agregarAlFinal(a->codigo, ultimo);
                a = a->siguiente;
                b = b->siguiente;
            }
        }
        return resultado;
    }

    // A - B: los codigos de A que NO estan en B.
    SetCodigos difference(const SetCodigos& otro) const {
        SetCodigos resultado;
        NodoSet* ultimo = nullptr;
        NodoSet* a = primero;
        NodoSet* b = otro.primero;
        while (a != nullptr) {
            if (b == nullptr || a->codigo < b->codigo) {
                resultado.agregarAlFinal(a->codigo, ultimo);   // no puede estar en B
                a = a->siguiente;
            } else if (b->codigo < a->codigo) {
                b = b->siguiente;
            } else {   // esta en los dos: no va
                a = a->siguiente;
                b = b->siguiente;
            }
        }
        return resultado;
    }

    // Texto del estilo {PKG-1002, PKG-1005}. Un conjunto vacio es {}.
    string toString() const {
        string texto = "{";
        for (NodoSet* aux = primero; aux != nullptr; aux = aux->siguiente) {
            if (aux != primero) texto += ", ";
            texto += aux->codigo;
        }
        return texto + "}";
    }
};
