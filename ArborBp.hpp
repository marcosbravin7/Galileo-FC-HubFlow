#pragma once

#include "Envio.h"
#include <cstddef>
#include <string>
#include <iostream>
#include <stdexcept>

// Arbol B+ indexado por el codigo del envio.
//
// OWNERSHIP:
// - ListaDeEnvios es la unica estructura propietaria de cada Envio y lo libera.
// - ArbolBPlus solo conserva referencias no propietarias mediante Envio*.
// - ArbolBPlus es propietario de sus Nodo y libera todos con delete.
//
// No se copian Envio ni se eliminan claves en esta ampliacion. Al eliminar el
// arbol, solo se destruyen sus nodos; los Envio permanecen en ListaDeEnvios.
class ArbolBPlus
{
public:
    explicit ArbolBPlus(std::size_t orden = 4);
    ~ArbolBPlus();

    ArbolBPlus(const ArbolBPlus&) = delete;
    ArbolBPlus& operator=(const ArbolBPlus&) = delete;

    // Rechaza una clave duplicada o un envio nulo.
    bool insertar(const std::string& clave, Envio* envio);
    Envio* buscar(const std::string& clave) const;

    // Recorre las hojas enlazadas en orden ascendente de clave.
    void mostrarEnOrden() const;

    std::size_t cantidad() const;
    bool estaVacio() const;

private:
    struct Nodo
    {
        explicit Nodo(bool hoja, std::size_t orden);
        ~Nodo();

        bool esHoja;
        std::size_t cantidadClaves;
        std::string* claves;
        Envio** envios;
        Nodo** hijos;
        Nodo* siguiente;
    };

    struct Division
    {
        Division();

        bool ocurrio;
        std::string claveSeparadora;
        Nodo* hijoDerecho;
    };

    std::size_t orden;
    std::size_t cantidadEnvios;
    Nodo* raiz;

    Division insertarEnNodo(Nodo* nodo, const std::string& clave, Envio* envio);
    Division insertarEnHoja(Nodo* hoja, const std::string& clave, Envio* envio);
    Division insertarEnInterno(Nodo* interno, const std::string& clave, Envio* envio);

    Nodo* hojaInicial() const;
    void liberarNodo(Nodo* nodo);
};
