#pragma once
#include "Envio.h"
#include <iostream>
#include <string>
using namespace std;

// RF11 — Indice avanzado de envios con un arbol B+ de orden 4.
//
// Orden 4 significa:
//   * un nodo interno tiene como maximo 4 hijos y 3 claves;
//   * una hoja guarda como maximo 3 claves (cada una con su Envio*).
// Cuando un nodo llega a 4 claves "se desborda" y hay que dividirlo. Por eso
// los arreglos tienen un lugar de mas: primero se inserta y despues, si quedo
// con 4 claves, se divide en dos.
const int ORDEN_BMAS = 4;
const int MAX_CLAVES_BMAS = ORDEN_BMAS - 1;   // 3

// Nodo del arbol B+. Usamos el mismo struct para hojas y para internos, y el
// campo esHoja nos dice cuales campos se usan:
//   * interno: claves (de separacion) + hijos
//   * hoja:    claves (codigos) + envios + siguiente (enlace a la hoja de la derecha)
struct NodoBMas {
    bool esHoja;
    int cantClaves;
    string claves[ORDEN_BMAS];            // 3 + 1 lugar extra para el desborde
    NodoBMas* hijos[ORDEN_BMAS + 1];      // 4 + 1 lugar extra para el desborde
    Envio* envios[ORDEN_BMAS];            // referencias NO propietarias
    NodoBMas* siguiente;

    NodoBMas(bool hoja) : esHoja(hoja), cantClaves(0), siguiente(nullptr) {
        for (int i = 0; i < ORDEN_BMAS + 1; i++) hijos[i] = nullptr;
        for (int i = 0; i < ORDEN_BMAS; i++) envios[i] = nullptr;
    }
};

// ---------------------------------------------------------------------------
// Como esta armado
// ---------------------------------------------------------------------------
// Todas las claves (codigos de seguimiento) estan en las hojas, ordenadas, y
// las hojas estan enlazadas de izquierda a derecha con `siguiente`. Los nodos
// internos solo tienen claves de separacion para saber por que hijo bajar.
//
// Regla que usamos para las claves de separacion: la clave i de un nodo
// interno es la MENOR clave del subarbol hijos[i+1]. Entonces, para buscar un
// codigo, bajamos por el hijo i donde i = cantidad de claves <= codigo.
//
// Ejemplo con el dataset PKG-1001 ... PKG-1008 (insertados en orden; en el
// dibujo ponemos solo el numero del codigo):
/*
                       [1003 | 1005 | 1007]
                 /         /         \          \
       [1001 1002] -> [1003 1004] -> [1005 1006] -> [1007 1008]
*/
// Si despues llegan PKG-1009 y PKG-1010, la ultima hoja se divide, la raiz
// queda con 4 claves y tambien se divide, y se crea una raiz nueva:
/*
                              [1007]
                       /               \
              [1003 | 1005]             [1009]
             /      |      \            /     \
     [1001 1002] [1003 1004] [1005 1006] [1007 1008] [1009 1010]
*/
// Todas las hojas quedan a la misma profundidad porque el arbol solo crece
// "para arriba": cuando la raiz se divide se crea una raiz nueva encima.
//
// ---------------------------------------------------------------------------
// Gestion de memoria (quien libera que)
// ---------------------------------------------------------------------------
//   * Los Envio pertenecen SOLO a ListaDeEnvios (el registro del centro). El
//     arbol guarda punteros NO propietarios: nunca hace delete de un Envio*.
//   * El arbol crea con new sus propios nodos (al insertar en un arbol vacio,
//     al dividir una hoja o un interno y al crear una raiz nueva) y los libera
//     todos en el destructor, con un recorrido postorden.
//   * En CentroDeDistribucion el arbol se declara DESPUES del registro, asi se
//     destruye ANTES y nunca queda apuntando a envios ya liberados. Como los
//     envios no se borran mientras el centro existe, las referencias siguen
//     siendo validas durante toda la vida de los envios.
//   * No se puede copiar (copiar los punteros terminaria en doble delete).
//   * La eliminacion de claves no forma parte de esta entrega.
class ArbolBMas {
private:
    NodoBMas* raiz;
    int cantidad;

    // Contadores para poder mostrar (y testear) que pasaron las divisiones.
    int divisionesHoja;
    int divisionesInternas;
    int raicesCreadas;

    // Resultado de insertar en un subarbol.
    static const int DUPLICADO = 0;
    static const int SIN_DIVISION = 1;
    static const int CON_DIVISION = 2;

    // Indice del hijo por el que hay que bajar para encontrar `codigo`:
    // la cantidad de claves de separacion que son <= codigo.
    static int indiceHijo(const NodoBMas* nodo, const string& codigo) {
        int i = 0;
        while (i < nodo->cantClaves && codigo >= nodo->claves[i]) i++;
        return i;
    }

    // Libera los nodos en postorden: primero los hijos y despues el nodo.
    // NO toca los Envio.
    static void destruir(NodoBMas* nodo) {
        if (nodo == nullptr) return;
        if (!nodo->esHoja) {
            for (int i = 0; i <= nodo->cantClaves; i++) destruir(nodo->hijos[i]);
        }
        delete nodo;
    }

    // Divide una hoja que quedo con 4 claves: 2 quedan en la hoja original y
    // 2 pasan a una hoja nueva que se engancha a su derecha. La clave que se
    // sube al padre es la primera de la hoja nueva (en un B+ se COPIA, porque
    // la clave tiene que seguir estando en la hoja con su envio).
    void dividirHoja(NodoBMas* hoja, string& claveQueSube, NodoBMas*& nuevaHoja) {
        int mitad = hoja->cantClaves / 2;
        nuevaHoja = new NodoBMas(true);

        int j = 0;
        for (int i = mitad; i < hoja->cantClaves; i++) {
            nuevaHoja->claves[j] = hoja->claves[i];
            nuevaHoja->envios[j] = hoja->envios[i];
            hoja->claves[i] = "";
            hoja->envios[i] = nullptr;
            j++;
        }
        nuevaHoja->cantClaves = j;
        hoja->cantClaves = mitad;

        // enlace entre hojas: hoja -> nuevaHoja -> la que estaba despues
        nuevaHoja->siguiente = hoja->siguiente;
        hoja->siguiente = nuevaHoja;

        claveQueSube = nuevaHoja->claves[0];
        divisionesHoja++;
    }

    // Divide un nodo interno que quedo con 4 claves y 5 hijos. La clave del
    // medio SUBE al padre (a diferencia de la hoja, aca no se copia: en los
    // internos las claves son solo separadores).
    //   claves: k0 k1 [k2] k3   ->  izquierdo: k0 k1  | sube k2 | derecho: k3
    //   hijos:  h0 h1 h2 h3 h4  ->  izquierdo: h0 h1 h2         derecho: h3 h4
    void dividirInterno(NodoBMas* nodo, string& claveQueSube, NodoBMas*& nuevoNodo) {
        int mitad = nodo->cantClaves / 2;
        claveQueSube = nodo->claves[mitad];
        nuevoNodo = new NodoBMas(false);

        int j = 0;
        for (int i = mitad + 1; i < nodo->cantClaves; i++) {
            nuevoNodo->claves[j] = nodo->claves[i];
            nodo->claves[i] = "";
            j++;
        }
        nuevoNodo->cantClaves = j;

        j = 0;
        for (int i = mitad + 1; i <= nodo->cantClaves; i++) {
            nuevoNodo->hijos[j] = nodo->hijos[i];
            nodo->hijos[i] = nullptr;
            j++;
        }

        nodo->claves[mitad] = "";
        nodo->cantClaves = mitad;
        divisionesInternas++;
    }

    // Insercion recursiva. Baja hasta la hoja que corresponde, inserta ahi y
    // a la vuelta de la recursion va acomodando las divisiones: si un hijo se
    // dividio, el nodo actual recibe la clave que subio y el puntero al nuevo
    // hermano. Si el nodo actual tambien se desborda, se divide y le avisa a
    // su padre de la misma forma.
    int insertarEn(NodoBMas* nodo, Envio* envio, string& claveQueSube, NodoBMas*& nuevoNodo) {
        const string& codigo = envio->getCodigo();

        if (nodo->esHoja) {
            // buscar la posicion manteniendo las claves ordenadas
            int pos = 0;
            while (pos < nodo->cantClaves && nodo->claves[pos] < codigo) pos++;
            if (pos < nodo->cantClaves && nodo->claves[pos] == codigo) return DUPLICADO;

            // correr a la derecha para hacer lugar
            for (int i = nodo->cantClaves; i > pos; i--) {
                nodo->claves[i] = nodo->claves[i - 1];
                nodo->envios[i] = nodo->envios[i - 1];
            }
            nodo->claves[pos] = codigo;
            nodo->envios[pos] = envio;
            nodo->cantClaves++;

            if (nodo->cantClaves <= MAX_CLAVES_BMAS) return SIN_DIVISION;   // habia lugar
            dividirHoja(nodo, claveQueSube, nuevoNodo);
            return CON_DIVISION;
        }

        // nodo interno: bajar por el hijo que corresponde
        int i = indiceHijo(nodo, codigo);
        string claveDelHijo;
        NodoBMas* hermanoDelHijo = nullptr;
        int resultado = insertarEn(nodo->hijos[i], envio, claveDelHijo, hermanoDelHijo);
        if (resultado != CON_DIVISION) return resultado;

        // el hijo i se dividio: la clave que subio va en la posicion i y el
        // nuevo hermano queda como hijo i+1 (actualizacion de separadores)
        for (int k = nodo->cantClaves; k > i; k--) nodo->claves[k] = nodo->claves[k - 1];
        for (int k = nodo->cantClaves + 1; k > i + 1; k--) nodo->hijos[k] = nodo->hijos[k - 1];
        nodo->claves[i] = claveDelHijo;
        nodo->hijos[i + 1] = hermanoDelHijo;
        nodo->cantClaves++;

        if (nodo->cantClaves <= MAX_CLAVES_BMAS) return SIN_DIVISION;
        dividirInterno(nodo, claveQueSube, nuevoNodo);
        return CON_DIVISION;
    }

    // Revisa que todas las hojas esten en el mismo nivel. `nivelHojas` arranca
    // en -1 y se completa con el nivel de la primera hoja que encontramos.
    static bool hojasAlMismoNivel(const NodoBMas* nodo, int nivel, int& nivelHojas) {
        if (nodo->esHoja) {
            if (nivelHojas == -1) nivelHojas = nivel;
            return nivel == nivelHojas;
        }
        for (int i = 0; i <= nodo->cantClaves; i++) {
            if (!hojasAlMismoNivel(nodo->hijos[i], nivel + 1, nivelHojas)) return false;
        }
        return true;
    }

    // Muestra el arbol por niveles con sangria (para ver como quedo).
    static void imprimir(const NodoBMas* nodo, int nivel) {
        for (int i = 0; i < nivel; i++) cout << "    ";
        cout << (nodo->esHoja ? "hoja [" : "interno [");
        for (int i = 0; i < nodo->cantClaves; i++) {
            if (i > 0) cout << " | ";
            cout << nodo->claves[i];
        }
        cout << "]\n";
        if (!nodo->esHoja) {
            for (int i = 0; i <= nodo->cantClaves; i++) imprimir(nodo->hijos[i], nivel + 1);
        }
    }

    // Baja desde la raiz hasta la hoja donde estaria `codigo`, contando los
    // nodos por los que pasa (incluida la hoja).
    const NodoBMas* buscarHoja(const string& codigo, int& nodosVisitados) const {
        const NodoBMas* actual = raiz;
        while (actual != nullptr && !actual->esHoja) {
            nodosVisitados++;
            actual = actual->hijos[indiceHijo(actual, codigo)];
        }
        if (actual != nullptr) nodosVisitados++;
        return actual;
    }

public:
    ArbolBMas() : raiz(nullptr), cantidad(0), divisionesHoja(0),
                  divisionesInternas(0), raicesCreadas(0) {}

    ArbolBMas(const ArbolBMas&) = delete;
    ArbolBMas& operator=(const ArbolBMas&) = delete;

    // Libera solo los nodos del arbol; los Envio siguen siendo del registro.
    ~ArbolBMas() { destruir(raiz); }

    // ---- consultas ----
    int getCantidad() const { return cantidad; }
    bool estaVacio() const { return raiz == nullptr; }
    int getDivisionesHoja() const { return divisionesHoja; }
    int getDivisionesInternas() const { return divisionesInternas; }
    int getRaicesCreadas() const { return raicesCreadas; }
    const NodoBMas* getRaiz() const { return raiz; }

    // Hoja de mas a la izquierda (la de los codigos mas chicos). Desde ahi se
    // pueden recorrer todos los envios en orden siguiendo `siguiente`.
    const NodoBMas* primeraHoja() const {
        const NodoBMas* actual = raiz;
        while (actual != nullptr && !actual->esHoja) actual = actual->hijos[0];
        return actual;
    }

    // Cantidad de niveles (vacio = 0, solo una hoja = 1). Como todas las hojas
    // estan a la misma profundidad alcanza con bajar por el primer hijo.
    int altura() const {
        int h = 0;
        const NodoBMas* actual = raiz;
        while (actual != nullptr) {
            h++;
            actual = actual->esHoja ? nullptr : actual->hijos[0];
        }
        return h;
    }

    bool hojasAlMismoNivel() const {
        if (raiz == nullptr) return true;
        int nivelHojas = -1;
        return hojasAlMismoNivel(raiz, 0, nivelHojas);
    }

    // -----------------------------------------------------------------------
    // RF11.2 — Insercion
    // -----------------------------------------------------------------------
    // Casos que se contemplan:
    //   * arbol vacio: se crea una hoja que pasa a ser la raiz;
    //   * hoja con lugar: se inserta ordenado y listo;
    //   * hoja llena: se divide y se sube una clave al padre;
    //   * el padre recibe la clave (se actualizan los separadores) y si se
    //     llena tambien se divide, y asi hacia arriba;
    //   * si se divide la raiz, se crea una raiz nueva con dos hijos.
    // No se reconstruye el arbol: solo se toca el camino de la raiz a la hoja.
    // Devuelve false si el envio es nullptr o si el codigo ya estaba.
    bool insertar(Envio* envio) {
        if (envio == nullptr) return false;

        if (raiz == nullptr) {
            raiz = new NodoBMas(true);
            raiz->claves[0] = envio->getCodigo();
            raiz->envios[0] = envio;
            raiz->cantClaves = 1;
            cantidad = 1;
            return true;
        }

        string claveQueSube;
        NodoBMas* nuevoNodo = nullptr;
        int resultado = insertarEn(raiz, envio, claveQueSube, nuevoNodo);
        if (resultado == DUPLICADO) return false;

        if (resultado == CON_DIVISION) {
            // se dividio la raiz: el arbol crece un nivel
            NodoBMas* nuevaRaiz = new NodoBMas(false);
            nuevaRaiz->claves[0] = claveQueSube;
            nuevaRaiz->hijos[0] = raiz;
            nuevaRaiz->hijos[1] = nuevoNodo;
            nuevaRaiz->cantClaves = 1;
            raiz = nuevaRaiz;
            raicesCreadas++;
        }
        cantidad++;
        return true;
    }

    // -----------------------------------------------------------------------
    // RF11.3 — Busqueda por codigo
    // -----------------------------------------------------------------------
    // Baja por los internos usando los separadores y al llegar a la hoja
    // busca el codigo. Cuenta los nodos visitados (internos + la hoja).
    // Devuelve nullptr si no esta.
    Envio* buscar(const string& codigo, int& nodosVisitados) const {
        nodosVisitados = 0;
        const NodoBMas* hoja = buscarHoja(codigo, nodosVisitados);
        if (hoja == nullptr) return nullptr;
        for (int i = 0; i < hoja->cantClaves; i++) {
            if (hoja->claves[i] == codigo) return hoja->envios[i];
        }
        return nullptr;
    }

    // -----------------------------------------------------------------------
    // RF11.4 — Busqueda por rango [desde, hasta]
    // -----------------------------------------------------------------------
    // Se baja UNA sola vez hasta la hoja donde estaria `desde` y despues se
    // avanza por los enlaces entre hojas, hasta pasar `hasta`. Asi no se
    // recorre todo el arbol, solo las hojas del rango.
    // Los resultados se guardan en `destino` (lo reserva quien llama, con
    // lugar para `maximo` envios) en orden ascendente. Devuelve cuantos hay.
    int buscarRango(const string& desde, const string& hasta, Envio** destino, int maximo,
                    int& nodosVisitados) const {
        nodosVisitados = 0;
        if (raiz == nullptr || desde > hasta) return 0;

        int encontrados = 0;
        const NodoBMas* hoja = buscarHoja(desde, nodosVisitados);
        bool primera = true;
        while (hoja != nullptr) {
            if (!primera) nodosVisitados++;   // la primera hoja ya la conto buscarHoja
            primera = false;
            for (int i = 0; i < hoja->cantClaves; i++) {
                if (hoja->claves[i] < desde) continue;
                if (hoja->claves[i] > hasta) return encontrados;   // ya nos pasamos
                if (encontrados < maximo) destino[encontrados] = hoja->envios[i];
                encontrados++;
            }
            hoja = hoja->siguiente;
        }
        return encontrados;
    }

    void mostrarEstructura() const {
        if (raiz == nullptr) {
            cout << "  (el arbol B+ esta vacio)\n";
            return;
        }
        imprimir(raiz, 1);
        cout << "  Envios indexados: " << cantidad << " | Altura: " << altura()
             << " | Hojas al mismo nivel: " << (hojasAlMismoNivel() ? "si" : "no") << "\n";
    }
};
