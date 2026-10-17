#pragma once
#include "ColaEnvios.h"
#include "PilaEnvios.h"
#include "ListaPendientes.h"
#include <iostream>
#include <string>
using namespace std;

// Criterio con el que esta ordenado el lote (RF09.2).
enum class CriterioOrden {
    NINGUNO,      // tal como salio de la lista de pendientes
    CODIGO_ASC,   // codigo de seguimiento ascendente
    PESO_DESC     // peso descendente
};

// RF09 — Mesa de clasificacion y despacho.
//
// Prepara un lote TEMPORAL con los pendientes de una zona y lo trabaja con:
//   * un arreglo dinamico de punteros (Envio**)          -> RF09.1
//   * Insertion Sort escrito a mano                      -> RF09.2
//   * una Queue propia (colaPreparacion, FIFO)           -> RF09.3
//   * una Stack propia (pilaProcesados, LIFO)            -> RF09.4
//   * busqueda lineal                                    -> RF09.5
//   * busqueda binaria (solo si el lote esta por codigo) -> RF09.6
//   * comparacion de comparaciones entre ambas busquedas -> RF09.7
//
// ---------------------------------------------------------------------------
// OWNERSHIP (RF09.8) — decision de diseno documentada
// ---------------------------------------------------------------------------
// Los objetos Envio pertenecen UNICAMENTE a ListaDeEnvios (ver
// ListaDeEnvios.h). Esta clase no cambia eso. El arreglo `lote`, la Queue y la
// Stack guardan punteros NO propietarios: son "prestamos" de envios que siguen
// vivos en el registro.
//   * ~MesaDeClasificacion libera el arreglo auxiliar con delete[] (no los
//     Envio que contiene).
//   * ~ColaEnvios y ~PilaEnvios liberan solo sus nodos.
//   * Ninguna de estas estructuras ejecuta `delete` sobre un Envio.
// Asi se evita:
//   - double free: un Envio solo se destruye en ~ListaDeEnvios;
//   - dangling pointers: el registro vive mas que la mesa, porque en
//     CentroDeDistribucion `mesa` se declara DESPUES de `registro` (los
//     miembros se destruyen en orden inverso), y un Envio nunca se borra
//     mientras el centro este vivo (despachar o entregar solo lo sacan de
//     pendientes);
//   - perdida de memoria: crearLote() libera el arreglo anterior antes de
//     reemplazarlo y vacia la cola y la pila;
//   - destruccion accidental de un envio compartido con pendientes/registro.
// El lote es una FOTO: refleja los pendientes al momento de crearlo. Si luego
// un envio se despacha, el lote conserva su referencia (sigue siendo valida).
// La mesa NO cambia el estado de ningun envio ni su historial permanente.
class MesaDeClasificacion {
private:
    string zona;
    Envio** lote;            // arreglo dinamico de punteros no propietarios
    int cantidad;
    CriterioOrden orden;
    ColaEnvios colaPreparacion;
    PilaEnvios pilaProcesados;

    // Orden estricto: true si `a` debe ir ANTES que `b` segun el criterio.
    // Al ser estricto (nunca true para iguales), el Insertion Sort resulta
    // estable: dos envios empatados conservan su orden relativo.
    static bool iraAntes(const Envio* a, const Envio* b, CriterioOrden criterio) {
        if (criterio == CriterioOrden::CODIGO_ASC) return a->getCodigo() < b->getCodigo();
        return a->getPeso() > b->getPeso();   // PESO_DESC
    }

public:
    MesaDeClasificacion() : lote(nullptr), cantidad(0), orden(CriterioOrden::NINGUNO) {}

    // Administra un arreglo propio: no se permite copiar (doble delete[]).
    MesaDeClasificacion(const MesaDeClasificacion&) = delete;
    MesaDeClasificacion& operator=(const MesaDeClasificacion&) = delete;

    // Libera solo el arreglo auxiliar. La cola y la pila liberan sus nodos en
    // sus propios destructores. Ningun Envio se destruye aca.
    ~MesaDeClasificacion() { delete[] lote; }

    // ---- consultas ----
    int getCantidad() const { return cantidad; }
    CriterioOrden getOrden() const { return orden; }
    const string& getZona() const { return zona; }
    // Referencia (no propietaria) al envio en la posicion i, o nullptr si esta fuera de rango.
    Envio* getEnvio(int i) const { return (i >= 0 && i < cantidad) ? lote[i] : nullptr; }
    const ColaEnvios& getCola() const { return colaPreparacion; }
    const PilaEnvios& getPila() const { return pilaProcesados; }
    bool loteOrdenadoPorCodigo() const { return orden == CriterioOrden::CODIGO_ASC; }

    // -----------------------------------------------------------------------
    // RF09.1 — Crear un lote por zona
    // -----------------------------------------------------------------------
    // Recorre los pendientes, selecciona solo los de `zonaElegida` y copia sus
    // REFERENCIAS a un Envio** nuevo (no se duplica ningun objeto Envio).
    // Si la zona no tiene pendientes informa y deja intacto el lote anterior.
    // Un lote nuevo descarta el anterior: libera el arreglo y vacia cola y pila.
    bool crearLote(const ListaPendientes& pendientes, const string& zonaElegida) {
        int total = pendientes.contarDeZona(zonaElegida);
        if (total == 0) {
            cout << "No hay envios pendientes para la zona seleccionada.\n";
            return false;
        }

        Envio** nuevo = new Envio*[total];
        int copiados = pendientes.copiarDeZona(zonaElegida, nuevo, total);

        delete[] lote;                 // libera el arreglo anterior (no los Envio)
        colaPreparacion.vaciar();      // solo nodos
        pilaProcesados.vaciar();       // solo nodos
        lote = nuevo;
        cantidad = copiados;
        zona = zonaElegida;
        orden = CriterioOrden::NINGUNO;
        return true;
    }

    // -----------------------------------------------------------------------
    // RF09.2 — Ordenar el lote con Insertion Sort (implementado a mano)
    // -----------------------------------------------------------------------
    // Criterios: CODIGO_ASC o PESO_DESC. No se usa std::sort ni equivalentes.
    //
    // Analisis temporal (n = cantidad de envios del lote):
    //   * Mejor caso:  O(n)   — el lote ya esta ordenado: la condicion del
    //                           while falla de inmediato en cada iteracion
    //                           (n-1 comparaciones, ningun desplazamiento).
    //   * Peor caso:   O(n^2) — el lote esta en orden inverso: cada elemento
    //                           se desplaza hasta el inicio (n(n-1)/2
    //                           comparaciones y desplazamientos).
    //   * Caso promedio: O(n^2).
    //   * Espacio extra O(1) (se ordena en el lugar) y es ESTABLE.
    // Para los lotes por zona (pocas decenas de envios) es una buena eleccion:
    // simple, estable y casi lineal si el lote ya viene casi ordenado.
    bool ordenar(CriterioOrden criterio) {
        if (cantidad == 0) {
            cout << "No hay un lote creado. Cree primero un lote por zona.\n";
            return false;
        }
        if (criterio == CriterioOrden::NINGUNO) return false;

        for (int i = 1; i < cantidad; i++) {
            Envio* clave = lote[i];
            int j = i - 1;
            while (j >= 0 && iraAntes(clave, lote[j], criterio)) {
                lote[j + 1] = lote[j];
                j--;
            }
            lote[j + 1] = clave;
        }
        orden = criterio;
        return true;
    }

    // -----------------------------------------------------------------------
    // RF09.3 / RF09.4 — Queue de preparacion y Stack de procesados
    // -----------------------------------------------------------------------
    // Ingresa el lote ORDENADO en colaPreparacion respetando el orden del
    // arreglo (el primero del lote queda en el FRONT). Solo se permite una
    // carga por lote, para que la pila refleje una unica pasada.
    bool cargarColaPreparacion() {
        if (cantidad == 0) {
            cout << "No hay un lote creado. Cree primero un lote por zona.\n";
            return false;
        }
        if (orden == CriterioOrden::NINGUNO) {
            cout << "Ordene primero el lote (por codigo o por peso) antes de cargar la cola.\n";
            return false;
        }
        if (!colaPreparacion.isEmpty() || !pilaProcesados.isEmpty()) {
            cout << "La cola de este lote ya fue cargada. Cree un nuevo lote para volver a empezar.\n";
            return false;
        }
        for (int i = 0; i < cantidad; i++) colaPreparacion.enqueue(lote[i]);
        return true;
    }

    // Saca el siguiente de colaPreparacion (dequeue, FIFO) y lo registra en
    // pilaProcesados (push, LIFO). No cambia el estado del envio. Devuelve el
    // envio procesado o nullptr si la cola estaba vacia.
    Envio* procesarSiguiente() {
        Envio* e = colaPreparacion.dequeue();
        if (e == nullptr) {
            cout << "La cola de preparacion esta vacia: no hay nada para procesar.\n";
            return nullptr;
        }
        pilaProcesados.push(e);
        return e;
    }

    // Ultimo envio procesado (top de la pila) o nullptr si no hay ninguno.
    Envio* ultimoProcesado() const { return pilaProcesados.top(); }

    // -----------------------------------------------------------------------
    // RF09.5 — Busqueda lineal
    // -----------------------------------------------------------------------
    // Recorre desde la posicion 0 hasta encontrar el codigo o llegar al final.
    // Funciona con el lote en cualquier orden. Devuelve la posicion (base 0) o
    // -1 si no esta. `comparaciones` cuenta cada comparacion de codigos.
    //   * Mejor caso: O(1) — esta en la primera posicion (1 comparacion).
    //   * Peor caso:  O(n) — esta al final o no esta (n comparaciones).
    int buscarLineal(const string& codigo, int& comparaciones) const {
        comparaciones = 0;
        for (int i = 0; i < cantidad; i++) {
            comparaciones++;
            if (lote[i]->getCodigo() == codigo) return i;
        }
        return -1;
    }

    // -----------------------------------------------------------------------
    // RF09.6 — Busqueda binaria
    // -----------------------------------------------------------------------
    // Solo es valida si el lote esta ordenado por codigo ascendente. Si no lo
    // esta NO se ejecuta (devuelve -1 con 0 comparaciones): quien la invoque
    // debe consultar antes loteOrdenadoPorCodigo(), como hace
    // mostrarBusquedaBinaria(). `comparaciones` cuenta una comparacion por
    // iteracion (compare() devuelve <0, 0 o >0 en una sola operacion).
    // Si `traza` es true imprime izq / der / mid de cada paso.
    //   * Mejor caso: O(1)     — el codigo esta justo en el medio.
    //   * Peor caso:  O(log n) — el rango se reduce a la mitad en cada paso,
    //                            como maximo floor(log2 n) + 1 comparaciones.
    int buscarBinaria(const string& codigo, int& comparaciones, bool traza = false) const {
        comparaciones = 0;
        if (!loteOrdenadoPorCodigo()) return -1;

        int izq = 0;
        int der = cantidad - 1;
        while (izq <= der) {
            int mid = izq + (der - izq) / 2;   // evita overflow de (izq + der)
            if (traza) {
                cout << "    izq = " << izq << ", der = " << der << ", mid = " << mid << "\n";
            }
            comparaciones++;
            int cmp = codigo.compare(lote[mid]->getCodigo());
            if (cmp == 0) return mid;
            if (cmp < 0) der = mid - 1;
            else izq = mid + 1;
        }
        return -1;
    }

    // ---- salida por consola ----

    void mostrarLote() const {
        if (cantidad == 0) {
            cout << "  (no hay lote creado)\n";
            return;
        }
        cout << "  Zona: " << zona << " | " << cantidad << " envio(s) | orden: ";
        if (orden == CriterioOrden::CODIGO_ASC) cout << "codigo ascendente\n";
        else if (orden == CriterioOrden::PESO_DESC) cout << "peso descendente\n";
        else cout << "sin ordenar\n";
        for (int i = 0; i < cantidad; i++) {
            cout << "  [" << i << "] ";
            lote[i]->mostrar();
        }
    }

    void mostrarCola() const {
        cout << "=== COLA DE PREPARACION ===\n";
        colaPreparacion.mostrar();
    }

    void mostrarPila() const {
        cout << "=== PILA DE PROCESADOS ===\n";
        pilaProcesados.mostrar();
    }

    void mostrarUltimoProcesado() const {
        Envio* e = pilaProcesados.top();
        if (e == nullptr) {
            cout << "Todavia no se proceso ningun envio.\n";
            return;
        }
        cout << "Ultimo procesado (top): ";
        e->mostrar();
    }

    void mostrarBusquedaLineal(const string& codigo) const {
        if (cantidad == 0) {
            cout << "No hay un lote creado. Cree primero un lote por zona.\n";
            return;
        }
        int comps = 0;
        int pos = buscarLineal(codigo, comps);
        cout << "Busqueda lineal de " << codigo << ":\n";
        if (pos >= 0) cout << "  Encontrado en la posicion " << pos << "\n";
        else cout << "  No encontrado en el lote\n";
        cout << "  Comparaciones: " << comps << "\n";
    }

    void mostrarBusquedaBinaria(const string& codigo) const {
        if (cantidad == 0) {
            cout << "No hay un lote creado. Cree primero un lote por zona.\n";
            return;
        }
        if (!loteOrdenadoPorCodigo()) {
            cout << "La busqueda binaria requiere el lote ordenado por codigo ascendente. "
                    "Ordene primero el lote por codigo.\n";
            return;
        }
        int comps = 0;
        int pos = buscarBinaria(codigo, comps, true);
        cout << "Busqueda binaria de " << codigo << ":\n";
        if (pos >= 0) cout << "  " << codigo << " encontrado (posicion " << pos << ")\n";
        else cout << "  " << codigo << " no encontrado en el lote\n";
        cout << "  Comparaciones: " << comps << "\n";
    }

    // RF09.7 — mismo codigo, mismo lote, ambas busquedas y sus comparaciones.
    void compararBusquedas(const string& codigo) const {
        if (cantidad == 0) {
            cout << "No hay un lote creado. Cree primero un lote por zona.\n";
            return;
        }
        if (!loteOrdenadoPorCodigo()) {
            cout << "Para comparar ambas busquedas el lote debe estar ordenado por codigo "
                    "(la busqueda binaria lo requiere). Ordene primero el lote por codigo.\n";
            return;
        }
        int compsLineal = 0;
        int compsBinaria = 0;
        int posLineal = buscarLineal(codigo, compsLineal);
        int posBinaria = buscarBinaria(codigo, compsBinaria);

        cout << "Codigo buscado: " << codigo << " (lote de " << cantidad << " envio(s))\n";
        cout << "Busqueda lineal:\n";
        if (posLineal >= 0) cout << "  Encontrado en la posicion " << posLineal << "\n";
        else cout << "  No encontrado\n";
        cout << "  Comparaciones: " << compsLineal << "\n";
        cout << "Busqueda binaria:\n";
        if (posBinaria >= 0) cout << "  Encontrado en la posicion " << posBinaria << "\n";
        else cout << "  No encontrado\n";
        cout << "  Comparaciones: " << compsBinaria << "\n";
        cout << "Complejidad:\n";
        cout << "  Lineal  - mejor caso O(1) (esta en la primera posicion), "
                "peor caso O(n) (esta al final o no esta).\n";
        cout << "  Binaria - mejor caso O(1) (esta justo en el medio), "
                "peor caso O(log n) (el rango se divide a la mitad en cada paso).\n";
    }
};
