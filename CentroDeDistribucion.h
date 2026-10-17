#pragma once
#include "ListaDeEnvios.h"
#include "ListaPendientes.h"
#include "MesaDeClasificacion.h"
#include "IndiceBST.h"
#include <iostream>
#include <string>
using namespace std;

// Controlador principal: coordina el registro global de envios y la cola
// de pendientes, y gestiona la memoria en cascada.
// Implementado por Zoe Corral y Maria Emilia Mocayar.
class CentroDeDistribucion {
private:
    ListaDeEnvios registro;      // duena de todos los Envio* (pendientes y despachados)
    ListaPendientes pendientes;  // cola de prioridad; solo referencia envios del registro
    // RF09 — lote temporal, cola y pila: solo referencian envios del registro.
    // IMPORTANTE: se declara DESPUES de `registro` para que se destruya ANTES
    // (los miembros se destruyen en orden inverso) y nunca apunte a envios ya
    // liberados. Ver el comentario de ownership en MesaDeClasificacion.h.
    MesaDeClasificacion mesaClasificacion;
    // RF10 — indice BST por codigo: solo referencia envios del registro. Se
    // declara DESPUES de `registro` por la misma razon que la mesa (se
    // destruye antes; ver el comentario de ownership en IndiceBST.h).
    IndiceBST indice;

    // Aplica un cambio de estado YA VALIDADO (transicionPermitida) y mantiene
    // la invariante "esta en pendientes <=> RECIBIDO/CLASIFICADO/REPROGRAMADO":
    //   * REPROGRAMADO suma un intento (RF06) y el envio vuelve a pendientes
    //     reinsertado segun su prioridad;
    //   * CLASIFICADO sigue en pendientes: agregar() lo detecta y no lo mueve,
    //     asi conserva su orden de llegada;
    //   * EN_REPARTO y ENTREGADO lo sacan de pendientes (si estaba).
    // Es el unico punto por el que cambian de estado cambiarEstado,
    // reprogramarEnvio y finalizarEntrega, de modo que las reglas no pueden
    // quedar desincronizadas entre ellos.
    void aplicarTransicion(Envio* e, Estado nuevoEstado, const string& obs) {
        if (nuevoEstado == Estado::REPROGRAMADO) e->sumarIntento();
        e->cambiarEstado(nuevoEstado, obs);
        if (estadoEsPendiente(nuevoEstado)) pendientes.agregar(e);
        else pendientes.remover(e);
    }

public:
    CentroDeDistribucion() = default;

    // Al destruirse, `registro` libera en cascada todos los Envio* (y sus
    // historiales); `pendientes` solo libera sus propios nodos.
    ~CentroDeDistribucion() = default;

    // RF01 — Registrar nuevo envio
    void registrarEnvio(const string& cod, const string& dest, const string& zona, double peso, NivelServicio nivel) {
        if (registro.existeCodigo(cod)) {
            cout << "Error: ya existe un envio con codigo " << cod << "\n";
            return;
        }
        Envio* e = new Envio(cod, dest, zona, peso, nivel);
        registro.agregar(e);
        pendientes.agregar(e);
        indice.insertar(e);   // RF10.2: misma referencia al mismo Envio, sin reconstruir el arbol
        cout << "Envio " << cod << " registrado correctamente.\n";
    }

    // RF02 — Mostrar pendientes
    void mostrarPendientes() const {
        cout << "=== ENVIOS PENDIENTES ===\n";
        pendientes.mostrar();
    }

    // RF03 — Buscar envio (entre TODOS los conocidos, no solo pendientes)
    void buscarEnvio(const string& codigo) const {
        Envio* e = registro.buscar(codigo);
        if (e == nullptr) {
            cout << "Envio no encontrado.\n";
            return;
        }
        e->mostrar();
    }

    // RF04 — Cambiar estado.
    // Solo se aceptan las transiciones de la maquina de estados (Estados.h):
    // un cambio no permitido se rechaza SIN tocar el envio ni su historial.
    // Cada cambio valido agrega un movimiento al final del historial y deja a
    // pendientes consistente (aplicarTransicion). ENTREGADO es un estado
    // final: de ahi no se puede volver a ningun otro (RF07).
    void cambiarEstado(const string& codigo, Estado nuevoEstado, const string& obs) {
        Envio* e = registro.buscar(codigo);
        if (e == nullptr) { cout << "Envio no encontrado.\n"; return; }
        Estado actual = e->getEstado();
        if (!transicionPermitida(actual, nuevoEstado)) {
            if (actual == Estado::ENTREGADO) {
                cout << "Transicion invalida: el envio ya fue ENTREGADO y su estado es final.\n";
            } else {
                cout << "Transicion invalida: de " << estadoToString(actual) << " a "
                     << estadoToString(nuevoEstado) << " no esta permitido. Desde "
                     << estadoToString(actual) << " se puede pasar a: " << transicionesDesde(actual) << ".\n";
            }
            return;
        }
        aplicarTransicion(e, nuevoEstado, obs);
        cout << "Estado actualizado.\n";
    }

    // RF05 — Despachar proximo envio (el primer nodo de pendientes)
    void despacharProximo() {
        Envio* e = pendientes.despachar();
        if (e == nullptr) { cout << "No hay envios pendientes.\n"; return; }
        e->cambiarEstado(Estado::EN_REPARTO, "Despachado del centro");
        cout << "Despachado: " << e->getCodigo() << " -> " << e->getDestinatario() << "\n";
    }

    // RF06 — Reprogramar envio: una entrega que no pudo realizarse.
    // Solo se puede reprogramar un envio EN_REPARTO (es el unico que esta
    // "en la calle" intentando entregar). Eso tambien impide meter dos veces
    // al mismo envio en pendientes. Incrementa intentos, pasa a REPROGRAMADO,
    // agrega el movimiento con el motivo y lo reinserta segun su prioridad.
    void reprogramarEnvio(const string& codigo, const string& motivo) {
        Envio* e = registro.buscar(codigo);
        if (e == nullptr) { cout << "Envio no encontrado.\n"; return; }
        if (e->estaEntregado()) { cout << "El envio ya fue entregado, no puede reprogramarse.\n"; return; }
        if (e->getEstado() != Estado::EN_REPARTO) {
            cout << "Solo puede reprogramarse un envio EN_REPARTO (estado actual: "
                 << estadoToString(e->getEstado()) << ").\n";
            return;
        }
        aplicarTransicion(e, Estado::REPROGRAMADO, motivo);
        cout << "Envio " << codigo << " reprogramado (intento " << e->getIntentos() << ").\n";
    }

    // RF07 — Finalizar una entrega: EN_REPARTO -> ENTREGADO. Registra el
    // movimiento y, como ENTREGADO es un estado final, el envio nunca vuelve a
    // pendientes (ni por reprogramar ni por cambiar estado).
    void finalizarEntrega(const string& codigo, const string& obs) {
        Envio* e = registro.buscar(codigo);
        if (e == nullptr) { cout << "Envio no encontrado.\n"; return; }
        if (e->estaEntregado()) { cout << "El envio ya fue entregado.\n"; return; }
        if (e->getEstado() != Estado::EN_REPARTO) {
            cout << "Solo puede entregarse un envio EN_REPARTO (estado actual: "
                 << estadoToString(e->getEstado()) << "). Despachelo primero.\n";
            return;
        }
        aplicarTransicion(e, Estado::ENTREGADO, obs.empty() ? "Entrega realizada" : obs);
        cout << "Envio " << codigo << " entregado.\n";
    }

    // RF08 — Mostrar historial bidireccional
    void mostrarHistorial(const string& codigo) const {
        Envio* e = registro.buscar(codigo);
        if (e == nullptr) { cout << "Envio no encontrado.\n"; return; }
        cout << "--- Historial cronologico (antiguo -> reciente) ---\n";
        e->mostrarHistorialCronologico();
        cout << "--- Historial inverso (reciente -> antiguo) ---\n";
        e->mostrarHistorialInverso();
    }

    // Resumen recursivo por zona (sobre los envios pendientes)
    void resumenZona(const string& zona) const {
        ListaPendientes::ResumenZona r = pendientes.resumenPorZona(zona);
        cout << "Zona: " << zona << "\n";
        cout << "  Cantidad de paquetes: " << r.cantidad << "\n";
        cout << "  Peso total pendiente: " << r.pesoTotal << " kg\n";
        cout << "  Cantidad EXPRESS: " << r.cantExpress << "\n";
    }

    // RF09 — acceso a la mesa de clasificacion y despacho (cola, pila,
    // ordenamiento y busquedas sobre el lote).
    MesaDeClasificacion& mesa() { return mesaClasificacion; }
    const MesaDeClasificacion& mesa() const { return mesaClasificacion; }

    // RF09.1 — crea el lote temporal con los pendientes de una zona.
    bool crearLoteZona(const string& zona) {
        return mesaClasificacion.crearLote(pendientes, zona);
    }

    // RF10 — acceso de solo lectura al indice BST (altura, in-order, etc.).
    const IndiceBST& indiceBST() const { return indice; }

    // RF10.3 — busca un envio por codigo usando el BST y muestra el camino y
    // la cantidad de nodos visitados.
    void buscarEnBST(const string& codigo) const {
        int visitados = 0;
        cout << "Busqueda en el BST de " << codigo << ":\n";
        Envio* e = indice.buscar(codigo, visitados, true);
        if (e == nullptr) cout << "Envio no encontrado en el BST.\n";
        else e->mostrar();
        cout << "Nodos visitados: " << visitados << "\n";
    }

    // RF10.4 — in-order: todos los envios ordenados por codigo ascendente.
    void recorrerBSTInOrder() const {
        cout << "=== BST in-order (codigo ascendente) ===\n";
        indice.mostrarInOrder();
    }

    // RF10.5 — altura y cantidad de nodos del BST.
    void mostrarAlturaBST() const {
        cout << "=== BST: altura ===\n";
        indice.mostrarAltura();
    }

    // RF10 — compara BST contra busqueda lineal sobre el MISMO conjunto: todos
    // los envios conocidos (el registro). La busqueda lineal y la binaria del
    // lote (RF09) siguen existiendo aparte y trabajan solo sobre el lote.
    void compararBSTconLineal(const string& codigo) const {
        int nodos = 0;
        int comparaciones = 0;
        Envio* enBst = indice.buscar(codigo, nodos);
        Envio* enLista = registro.buscarContando(codigo, comparaciones);
        cout << "Codigo buscado: " << codigo << " (" << indice.getCantidad() << " envios conocidos)\n";
        cout << "Busqueda en el BST:\n";
        cout << "  " << (enBst != nullptr ? "Encontrado" : "No encontrado") << "\n";
        cout << "  Nodos visitados: " << nodos << "\n";
        cout << "Busqueda lineal en el registro:\n";
        cout << "  " << (enLista != nullptr ? "Encontrado" : "No encontrado") << "\n";
        cout << "  Comparaciones: " << comparaciones << "\n";
        // Referencia: las estrategias de la mesa (RF09) trabajan sobre el lote,
        // que es OTRO conjunto de datos (solo los pendientes de una zona).
        if (mesaClasificacion.getCantidad() > 0) {
            int cl = 0;
            int pl = mesaClasificacion.buscarLineal(codigo, cl);
            cout << "Lote de la mesa (zona " << mesaClasificacion.getZona() << ", "
                 << mesaClasificacion.getCantidad() << " envios; otro conjunto de datos):\n";
            cout << "  Busqueda lineal: " << (pl >= 0 ? "encontrado" : "no esta en el lote")
                 << ", comparaciones: " << cl << "\n";
            if (mesaClasificacion.loteOrdenadoPorCodigo()) {
                int cb = 0;
                int pb = mesaClasificacion.buscarBinaria(codigo, cb);
                cout << "  Busqueda binaria: " << (pb >= 0 ? "encontrado" : "no esta en el lote")
                     << ", comparaciones: " << cb << "\n";
            } else {
                cout << "  Busqueda binaria: no se ejecuta (requiere el lote ordenado por codigo)\n";
            }
        }
        cout << "Complejidad:\n";
        cout << "  BST     - mejor caso O(1) (esta en la raiz), esperado O(log n) con el arbol balanceado,\n"
                "            peor caso O(n) si esta degenerado (codigos insertados ya ordenados).\n";
        cout << "  Lineal  - mejor caso O(1), peor caso O(n).\n";
    }

    // Desafio adicional — envio de mayor peso pendiente de una zona
    void envioMasPesadoDeZona(const string& zona) const {
        Envio* e = pendientes.envioMasPesadoDeZona(zona);
        if (e == nullptr) { cout << "No hay envios pendientes en la zona " << zona << ".\n"; return; }
        cout << "Zona consultada: " << zona << "\n";
        cout << "Envio mas pesado: " << e->getCodigo() << "\n";
        cout << "Peso: " << e->getPeso() << " kg\n";
    }
};
