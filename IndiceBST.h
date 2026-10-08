#pragma once
#include "Envio.h"
#include <iostream>
#include <string>
using namespace std;

// RF10.1 — Nodo del BST. Guarda una referencia NO propietaria al envio y los
// punteros a sus dos hijos. El criterio de orden es el codigo de seguimiento:
//   * codigos menores  -> subarbol izquierdo (izq)
//   * codigos mayores  -> subarbol derecho   (der)
struct NodoBST {
    Envio* envio;
    NodoBST* izq;
    NodoBST* der;

    NodoBST(Envio* e) : envio(e), izq(nullptr), der(nullptr) {}
};

// RF10 — Indice de envios mediante un arbol binario de busqueda (BST/ABB)
// implementado a mano, ordenado por codigo de seguimiento.
//
// Es una estructura ADICIONAL: no reemplaza a la lista de pendientes, ni a la
// busqueda lineal ni a la binaria del lote (RF09). Indexa todos los envios
// conocidos (pendientes, en reparto y entregados): un envio entra al BST
// cuando se registra y nunca sale, igual que ocurre con el registro.
//
/*
              PKG-1004
             /        \
       PKG-1002      PKG-1007
          /          /      \
    PKG-1001   PKG-1005   PKG-1008
*/
//
// ---------------------------------------------------------------------------
// RF10.6 — OWNERSHIP (decision de diseno documentada)
// ---------------------------------------------------------------------------
// Los objetos Envio pertenecen UNICAMENTE a ListaDeEnvios (ver
// ListaDeEnvios.h). El BST guarda punteros NO propietarios:
//   * el BST crea (new) y destruye (delete) SUS PROPIOS nodos y nada mas;
//   * destruir un nodo o el arbol completo NUNCA ejecuta delete sobre un
//     Envio*, de modo que no hay double free (el Envio solo se libera en
//     ~ListaDeEnvios);
//   * no hay dangling pointers porque en CentroDeDistribucion el indice se
//     declara DESPUES del registro: se destruye antes y nunca queda
//     apuntando a envios liberados; ademas un Envio no se borra mientras el
//     centro este vivo (despachar o entregar solo lo sacan de pendientes);
//   * ~IndiceBST libera todos los nodos (recorrido postorden), sin fugas;
//   * por contener punteros crudos de los que es dueno, no se puede copiar.
class IndiceBST {
private:
    NodoBST* raiz;
    int cantidad;

    // Libera los nodos en postorden (izq, der, nodo): el nodo se borra recien
    // cuando sus dos hijos ya fueron liberados. NO toca los Envio.
    static void destruir(NodoBST* nodo) {
        if (nodo == nullptr) return;
        destruir(nodo->izq);
        destruir(nodo->der);
        delete nodo;
    }

    // RF10.4 — recorrido in-order, RECURSIVO: izquierdo, actual, derecho.
    static void inOrder(const NodoBST* nodo) {
        if (nodo == nullptr) return;          // caso base: subarbol vacio
        inOrder(nodo->izq);                   // 1) todos los menores
        cout << "  ";
        nodo->envio->mostrar();               // 2) el nodo actual
        inOrder(nodo->der);                   // 3) todos los mayores
    }

    // RF10.5 — altura RECURSIVA.
    static int altura(const NodoBST* nodo) {
        if (nodo == nullptr) return 0;        // caso base: arbol vacio
        int alturaIzq = altura(nodo->izq);
        int alturaDer = altura(nodo->der);
        return 1 + (alturaIzq > alturaDer ? alturaIzq : alturaDer);
    }

public:
    IndiceBST() : raiz(nullptr), cantidad(0) {}

    IndiceBST(const IndiceBST&) = delete;
    IndiceBST& operator=(const IndiceBST&) = delete;

    // Libera solo los nodos del arbol; los Envio siguen siendo del registro.
    ~IndiceBST() { destruir(raiz); }

    // ---- consultas ----
    int getCantidad() const { return cantidad; }
    bool estaVacio() const { return raiz == nullptr; }
    const NodoBST* getRaiz() const { return raiz; }

    // -----------------------------------------------------------------------
    // RF10.2 — Insercion (iterativa)
    // -----------------------------------------------------------------------
    // Desciende desde la raiz comparando codigos hasta encontrar el hueco
    // donde corresponde el nuevo nodo y lo cuelga ahi. No se reconstruye el
    // arbol: cada alta toca un solo camino de la raiz a una hoja.
    // Los codigos son unicos: si el codigo ya esta (o el envio es nullptr) no
    // inserta y devuelve false.
    //
    // Complejidad (h = altura del arbol):
    //   * Mejor caso: O(1) si esta vacio; O(log n) en un arbol balanceado,
    //     porque h ~ log2(n) y solo se recorre un camino.
    //   * Peor caso: O(n) si el arbol esta degenerado (h = n).
    bool insertar(Envio* envio) {
        if (envio == nullptr) return false;

        NodoBST* padre = nullptr;
        NodoBST* actual = raiz;
        int cmp = 0;
        while (actual != nullptr) {
            cmp = envio->getCodigo().compare(actual->envio->getCodigo());
            if (cmp == 0) return false;               // codigo duplicado: no se admite
            padre = actual;
            actual = (cmp < 0) ? actual->izq : actual->der;
        }

        NodoBST* nuevo = new NodoBST(envio);
        if (padre == nullptr) raiz = nuevo;           // arbol vacio
        else if (cmp < 0) padre->izq = nuevo;
        else padre->der = nuevo;
        cantidad++;
        return true;
    }

    // -----------------------------------------------------------------------
    // RF10.3 — Busqueda por codigo
    // -----------------------------------------------------------------------
    // En cada nodo se decide: igual -> encontrado; menor -> izquierda; mayor
    // -> derecha. `nodosVisitados` cuenta los nodos examinados. Con `traza`
    // imprime el camino:   PKG-1004 -> derecha ... PKG-1005 -> encontrado
    //
    // Complejidad:
    //   * Mejor caso: O(1)     — el codigo buscado esta en la raiz.
    //   * Esperado:   O(log n) — arbol razonablemente balanceado.
    //   * Peor caso:  O(n)     — arbol degenerado: hay que bajar por todos.
    Envio* buscar(const string& codigo, int& nodosVisitados, bool traza = false) const {
        nodosVisitados = 0;
        const NodoBST* actual = raiz;
        while (actual != nullptr) {
            nodosVisitados++;
            int cmp = codigo.compare(actual->envio->getCodigo());
            if (cmp == 0) {
                if (traza) cout << "    " << actual->envio->getCodigo() << " -> encontrado\n";
                return actual->envio;
            }
            if (traza) {
                cout << "    " << actual->envio->getCodigo() << " -> "
                     << (cmp < 0 ? "izquierda" : "derecha") << "\n";
            }
            actual = (cmp < 0) ? actual->izq : actual->der;
        }
        return nullptr;
    }

    // -----------------------------------------------------------------------
    // RF10.4 — Recorrido in-order (recursivo)
    // -----------------------------------------------------------------------
    // Por que sale ordenado ascendentemente: por la propiedad del BST, todos
    // los codigos del subarbol izquierdo son MENORES que el del nodo, y todos
    // los del derecho MAYORES. In-order visita primero TODO el subarbol
    // izquierdo, despues el nodo y por ultimo TODO el derecho; es decir,
    // primero los menores, luego el actual y luego los mayores. Como la misma
    // regla se cumple recursivamente en cada subarbol, la secuencia completa
    // queda de menor a mayor.
    // Complejidad: O(n) — cada nodo se visita exactamente una vez.
    void mostrarInOrder() const {
        if (raiz == nullptr) {
            cout << "  (el BST esta vacio)\n";
            return;
        }
        inOrder(raiz);
    }

    // -----------------------------------------------------------------------
    // RF10.5 — Altura (recursiva)
    // -----------------------------------------------------------------------
    // CONVENCION ELEGIDA (se usa en todo el sistema): la altura es la CANTIDAD
    // DE NIVELES, o sea la cantidad de nodos del camino mas largo desde la
    // raiz hasta una hoja.
    //   * arbol vacio                  -> altura 0
    //   * arbol con un unico nodo      -> altura 1
    //   * arbol con varios niveles     -> 1 + max(altura izq, altura der)
    //     (el ejemplo del enunciado, con 6 nodos, tiene altura 3; el
    //     degenerado PKG-1001..PKG-1004 tiene altura 4)
    // Complejidad: O(n) — visita todos los nodos una vez.
    int altura() const { return altura(raiz); }

    // Menor altura posible para n nodos: el menor h con 2^h - 1 >= n. Es lo
    // que lograria un arbol perfectamente balanceado (~ log2(n) + 1).
    static int alturaMinimaPosible(int n) {
        int h = 0;
        long capacidad = 0;         // nodos que caben en h niveles: 2^h - 1
        while (capacidad < n) {
            h++;
            capacidad = capacidad * 2 + 1;
        }
        return h;
    }

    // -----------------------------------------------------------------------
    // RF10.7 — Por que un BST comun NO garantiza O(log n)
    // -----------------------------------------------------------------------
    // La FORMA del arbol depende del orden de insercion y este BST no se
    // rebalancea. Si los codigos llegan ya ordenados (PKG-1001, 1002, 1003,
    // 1004...), cada codigo nuevo es mayor que todos los anteriores y se
    // cuelga siempre a la derecha:
    //
    /*
       PKG-1001
            \
           PKG-1002
                \
               PKG-1003
                    \
                   PKG-1004
    */
    //
    // El arbol se degenera en una lista enlazada de altura n: insertar y
    // buscar recorren hasta n nodos, O(n). Solo un arbol balanceado (como un
    // AVL o un rojo-negro, que reacomodan nodos al insertar) mantiene h ~
    // log2(n) y garantiza O(log n).
    //
    // Resumen: insercion O(log n) mejor/esperado y O(n) peor; busqueda O(1)
    // mejor, O(log n) esperado y O(n) peor; in-order O(n); altura O(n).
    void mostrarAltura() const {
        int h = altura();
        cout << "  Nodos: " << cantidad << "\n";
        cout << "  Altura (cantidad de niveles; vacio = 0, un nodo = 1): " << h << "\n";
        if (cantidad > 0) {
            cout << "  Altura minima posible con " << cantidad << " nodos: "
                 << alturaMinimaPosible(cantidad) << "\n";
            cout << "  Altura maxima posible (arbol degenerado): " << cantidad << "\n";
        }
    }
};
