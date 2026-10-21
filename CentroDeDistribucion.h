#pragma once
#include "ListaDeEnvios.h"
#include "ListaPendientes.h"
#include "MesaDeClasificacion.h"
#include "IndiceBST.h"
#include "ArbolBMas.h"
#include "BitSet.h"
#include "SetCodigos.h"
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
    // RF11 — indice B+ por codigo: tambien guarda referencias NO propietarias
    // a los envios del registro. Va declarado despues de `registro` para que
    // se destruya antes que los envios (ver ArbolBMas.h).
    ArbolBMas indiceBMas;

    // Ficha completa de un envio, con el formato del RF11.3.
    static void mostrarFicha(const Envio* e) {
        cout << "Codigo: " << e->getCodigo() << "\n";
        cout << "Destinatario: " << e->getDestinatario() << "\n";
        cout << "Zona: " << e->getZona() << "\n";
        cout << "Servicio: " << nivelToString(e->getNivel()) << "\n";
    }

    // Lista los nombres de los estados cuyo bit esta prendido. La posicion
    // del bit es el valor del enum Estado.
    static string estadosDelBitSet(const BitSet& b) {
        string texto = "";
        for (int pos = 0; pos < b.size(); pos++) {
            if (b.test(pos)) {
                if (!texto.empty()) texto += ", ";
                texto += estadoToString(static_cast<Estado>(pos));
            }
        }
        return texto.empty() ? "(ninguno)" : texto;
    }

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
        indiceBMas.insertar(e);   // RF11.2: el mismo Envio entra tambien al B+ (no se copia)
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

    // =======================================================================
    // RF14 — Consultas avanzadas (arbol B+, BitSet y Set)
    // =======================================================================

    const ArbolBMas& indiceBMasConsulta() const { return indiceBMas; }

    // Devuelve el envio usando el B+ (nullptr si no existe). Es de solo
    // lectura: sirve para consultar, no para cambiar el envio por afuera.
    const Envio* obtenerEnvio(const string& codigo) const {
        int visitados = 0;
        return indiceBMas.buscar(codigo, visitados);
    }

    // RF11.3 — busqueda por codigo en el B+, con los nodos visitados.
    void buscarEnBMas(const string& codigo) const {
        int visitados = 0;
        Envio* e = indiceBMas.buscar(codigo, visitados);
        if (e == nullptr) cout << "Envio " << codigo << " no encontrado.\n";
        else mostrarFicha(e);
        cout << "Nodos visitados: " << visitados << "\n";
    }

    // RF11.4 — envios con codigo entre `desde` y `hasta` (ambos incluidos).
    void buscarRangoBMas(const string& desde, const string& hasta) const {
        if (desde > hasta) {
            cout << "Rango invalido: 'desde' es mayor que 'hasta'.\n";
            return;
        }
        if (indiceBMas.estaVacio()) {
            cout << "No hay envios en el rango " << desde << " - " << hasta << ".\n";
            return;
        }
        // arreglo temporal para los resultados: como mucho entran todos los
        // envios. Lo reservamos aca y lo liberamos al final de la funcion.
        int maximo = indiceBMas.getCantidad();
        Envio** resultados = new Envio*[maximo];
        int visitados = 0;
        int encontrados = indiceBMas.buscarRango(desde, hasta, resultados, maximo, visitados);

        if (encontrados == 0) {
            cout << "No hay envios en el rango " << desde << " - " << hasta << ".\n";
        } else {
            cout << "Envios entre " << desde << " y " << hasta << ":\n";
            for (int i = 0; i < encontrados; i++) cout << resultados[i]->getCodigo() << "\n";
        }
        cout << "Nodos visitados: " << visitados << "\n";
        delete[] resultados;   // solo el arreglo; los Envio son del registro
    }

    // RF12.3 — estados por los que paso un envio.
    void mostrarBitSet(const string& codigo) const {
        const Envio* e = obtenerEnvio(codigo);
        if (e == nullptr) { cout << "Envio " << codigo << " no encontrado.\n"; return; }
        const BitSet& b = e->getEstadosVisitados();
        cout << "Envio: " << codigo << " (estado actual: " << estadoToString(e->getEstado()) << ")\n";
        cout << "Bit:     4 3 2 1 0\n";
        cout << "Valor:  ";
        for (int pos = b.size() - 1; pos >= 0; pos--) cout << " " << (b.test(pos) ? 1 : 0);
        cout << "\n";
        cout << "Estados por los que paso: " << estadosDelBitSet(b) << "\n";
        cout << "Cantidad de estados distintos: " << b.count() << "\n";
        cout << "Estuvo REPROGRAMADO: " << (e->estuvoEn(Estado::REPROGRAMADO) ? "si" : "no") << "\n";
    }

    // RF12.3 — compara los BitSet de dos envios:
    //   AND -> estados que tienen los dos
    //   XOR -> estados que tiene uno solo; para saber cual, se hace AND del
    //          XOR con el BitSet de cada envio.
    void compararBitSets(const string& cod1, const string& cod2) const {
        const Envio* e1 = obtenerEnvio(cod1);
        const Envio* e2 = obtenerEnvio(cod2);
        if (e1 == nullptr) { cout << "Envio " << cod1 << " no encontrado.\n"; return; }
        if (e2 == nullptr) { cout << "Envio " << cod2 << " no encontrado.\n"; return; }
        const BitSet& b1 = e1->getEstadosVisitados();
        const BitSet& b2 = e2->getEstadosVisitados();
        BitSet comunes = b1 & b2;
        BitSet distintos = b1 ^ b2;
        BitSet todos = b1 | b2;
        cout << cod1 << ": " << b1.toString() << "\n";
        cout << cod2 << ": " << b2.toString() << "\n";
        cout << "En comun (AND):        " << comunes.toString() << " -> " << estadosDelBitSet(comunes) << "\n";
        cout << "Entre los dos (OR):    " << todos.toString() << " -> " << estadosDelBitSet(todos) << "\n";
        cout << "En uno solo (XOR):     " << distintos.toString() << " -> " << estadosDelBitSet(distintos) << "\n";
        cout << "  Solo en " << cod1 << ": " << estadosDelBitSet(distintos & b1) << "\n";
        cout << "  Solo en " << cod2 << ": " << estadosDelBitSet(distintos & b2) << "\n";
    }

    // RF13.2 — conjunto con los codigos de una zona. Recorre las hojas del B+
    // de izquierda a derecha, que es pasar por todos los envios conocidos.
    SetCodigos conjuntoPorZona(const string& zona) const {
        SetCodigos conjunto;
        for (const NodoBMas* hoja = indiceBMas.primeraHoja(); hoja != nullptr; hoja = hoja->siguiente) {
            for (int i = 0; i < hoja->cantClaves; i++) {
                if (hoja->envios[i]->getZona() == zona) conjunto.insert(hoja->claves[i]);
            }
        }
        return conjunto;
    }

    // RF13.2 — conjunto con los codigos de un nivel de servicio.
    SetCodigos conjuntoPorServicio(NivelServicio nivel) const {
        SetCodigos conjunto;
        for (const NodoBMas* hoja = indiceBMas.primeraHoja(); hoja != nullptr; hoja = hoja->siguiente) {
            for (int i = 0; i < hoja->cantClaves; i++) {
                if (hoja->envios[i]->getNivel() == nivel) conjunto.insert(hoja->claves[i]);
            }
        }
        return conjunto;
    }

    // RF14 — muestra un conjunto y, para cada codigo, recupera el envio
    // completo con el arbol B+ (el Set solo guarda codigos).
    void mostrarConjunto(const string& nombre, const SetCodigos& conjunto) const {
        cout << nombre << " = " << conjunto.toString() << "  (" << conjunto.size() << " envios)\n";
        for (const NodoSet* aux = conjunto.getPrimero(); aux != nullptr; aux = aux->siguiente) {
            const Envio* e = obtenerEnvio(aux->codigo);
            cout << "  ";
            if (e != nullptr) e->mostrar();
            else cout << aux->codigo << " (no esta en el indice)\n";
        }
    }

    void mostrarEstructuraBMas() const {
        cout << "=== Arbol B+ (orden " << ORDEN_BMAS << ") ===\n";
        indiceBMas.mostrarEstructura();
    }
};
