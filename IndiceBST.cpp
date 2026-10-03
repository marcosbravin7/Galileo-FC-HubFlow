#include "Envio.h"
#include "HistorialDeMovimientos.h"
#include "ListaPendientes.h"
#include <iostream>
using namespace std;

struct NodoBST {
    Envio* dato;
    NodoBST* der;
    NodoBST* izq;
};

class indiceBST {
private:
    NodoBST *node{};
    NodoBST *root{nullptr};

    public:
    indiceBST(){ root = nullptr; }  //Se crea el arbol
    ~indiceBST() = default;         //tengo que ver como hacer el dest sin perder el envío.

    //Analiza alfabéticamente el codigo del paquete para buscar la rama correspondiente; true si lo encuentra, false si no
    bool search(const string& key) {
        NodoBST* temp = root;

        while (temp != nullptr) {
            if (temp->dato->getCodigo() < key) temp = temp->der;
            else if (key < temp->dato->getCodigo()) temp = temp->izq;
            else return true;
        }
        cout << key << endl;
        return false;
    }

    //insert plantea el primer nodo y que no este repetido, si esta correcto llama al recursivo para recorrer.
    void insert(Envio* envio) {
        if (root == nullptr) { root = new NodoBST{envio , nullptr, nullptr}; }
        if (search(envio->getCodigo())) return;

        insertRecursivo(envio, root);
    }

    //busca la rama que le corresponde recursivamente y se posiciona cuando llega a un nullptr
    void insertRecursivo(Envio* envio , NodoBST* nodoTemp) {
        if (nodoTemp == nullptr) {
            nodoTemp = new NodoBST{envio , nullptr, nullptr};
            return;
        }

        if (nodoTemp->dato->getCodigo() < envio->getCodigo()) {
            insertRecursivo(envio, nodoTemp->der);
            return;
        }
        if (envio->getCodigo() < nodoTemp->dato->getCodigo()) {
            insertRecursivo(envio, nodoTemp->izq);
            return;
        }
    }
};
