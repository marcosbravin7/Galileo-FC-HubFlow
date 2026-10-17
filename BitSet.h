#pragma once
#include <string>
using namespace std;

// RF12 — BitSet hecho a mano (no se usa std::bitset).
//
// Guardamos todos los bits en un solo entero sin signo: el bit de la posicion
// `pos` es el bit numero `pos` del entero. Para tocar un bit usamos una
// "mascara" (1u << pos), que es un entero con un unico 1 en esa posicion:
//   * set:   bits | mascara   -> prende ese bit y deja los demas igual
//   * reset: bits & ~mascara  -> apaga ese bit y deja los demas igual
//   * test:  bits & mascara   -> distinto de 0 si el bit esta prendido
//
// Para los envios se usan 5 bits, uno por estado (mismo orden que el enum
// Estado):  bit 0 RECIBIDO, 1 CLASIFICADO, 2 EN_REPARTO, 3 REPROGRAMADO,
// 4 ENTREGADO.
//
// Memoria: no usa memoria dinamica (es solo un entero), asi que no hay nada
// que liberar y se puede copiar sin problema. Cada Envio tiene el suyo como
// miembro por valor.
class BitSet {
private:
    unsigned int bits;
    int tamanio;   // cantidad de posiciones validas (entre 1 y 32)

    bool posicionValida(int pos) const { return pos >= 0 && pos < tamanio; }

    // Mascara con los `tamanio` bits mas bajos en 1 (para no "ensuciar" bits
    // que estan fuera del rango del BitSet, por ejemplo despues de un XOR).
    unsigned int mascaraTotal() const {
        if (tamanio >= 32) return ~0u;
        return (1u << tamanio) - 1u;
    }

public:
    static const int CANT_ESTADOS = 5;

    explicit BitSet(int tam = CANT_ESTADOS) : bits(0), tamanio(tam) {
        if (tamanio < 1) tamanio = 1;
        if (tamanio > 32) tamanio = 32;
    }

    int size() const { return tamanio; }
    unsigned int getValor() const { return bits; }

    // Activa el bit `pos`. Si la posicion no existe no hace nada y devuelve false.
    bool set(int pos) {
        if (!posicionValida(pos)) return false;
        bits = bits | (1u << pos);
        return true;
    }

    // Desactiva el bit `pos`. Si la posicion no existe devuelve false.
    bool reset(int pos) {
        if (!posicionValida(pos)) return false;
        bits = bits & ~(1u << pos);
        return true;
    }

    // Consulta el bit `pos`. Una posicion invalida se toma como 0 (false).
    bool test(int pos) const {
        if (!posicionValida(pos)) return false;
        return (bits & (1u << pos)) != 0;
    }

    // Cuenta los bits en 1 recorriendo el entero bit por bit.
    int count() const {
        int total = 0;
        unsigned int aux = bits;
        while (aux != 0) {
            total += aux & 1u;   // miramos el bit mas bajo
            aux = aux >> 1;      // y corremos todo un lugar a la derecha
        }
        return total;
    }

    // Hay algun bit prendido si el entero no es 0.
    bool any() const { return bits != 0; }
    bool none() const { return bits == 0; }

    // Operaciones bit a bit entre dos BitSet. Se hacen directamente sobre los
    // enteros, asi que se resuelven en una sola operacion del procesador.
    BitSet operator&(const BitSet& otro) const {
        BitSet resultado(tamanio);
        resultado.bits = (bits & otro.bits) & mascaraTotal();
        return resultado;
    }

    BitSet operator|(const BitSet& otro) const {
        BitSet resultado(tamanio);
        resultado.bits = (bits | otro.bits) & mascaraTotal();
        return resultado;
    }

    BitSet operator^(const BitSet& otro) const {
        BitSet resultado(tamanio);
        resultado.bits = (bits ^ otro.bits) & mascaraTotal();
        return resultado;
    }

    bool operator==(const BitSet& otro) const {
        return tamanio == otro.tamanio && bits == otro.bits;
    }

    // Texto con el bit mas alto a la izquierda, como en la consigna:
    // RECIBIDO + CLASIFICADO + EN_REPARTO -> "00111"
    string toString() const {
        string texto = "";
        for (int pos = tamanio - 1; pos >= 0; pos--) texto += test(pos) ? '1' : '0';
        return texto;
    }
};
