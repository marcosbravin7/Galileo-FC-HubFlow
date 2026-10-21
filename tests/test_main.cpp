// Tests de HubFlow — cubren los "Casos de prueba obligatorios" de la consigna
// (Seccion 13). No usan ningun framework externo: cada caso arma su propio
// CentroDeDistribucion vacio, captura la salida por consola (cout) de las
// operaciones y verifica el contenido con chequear(...).
//
// Para correrlos: compilar el target HubFlowTests (ver CMakeLists.txt) y
// ejecutarlo. Termina con codigo 0 si todo paso, o 1 si algo fallo.

#include "CentroDeDistribucion.h"
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

static int totalChequeos = 0;
static int chequeosFallidos = 0;

void chequear(bool condicion, const string& descripcion) {
    totalChequeos++;
    if (condicion) {
        cout << "  [OK]   " << descripcion << "\n";
    } else {
        chequeosFallidos++;
        cout << "  [FAIL] " << descripcion << "\n";
    }
}

// Redirige cout a un buffer mientras se ejecuta `accion`, y restaura tanto
// el buffer como el formato (fixed/precision) que tenia cout antes de
// llamarla, para que un test no quede afectado por el formato que haya
// dejado seteado otro test anterior (Envio::mostrar usa fixed/setprecision).
string capturarSalida(const function<void()>& accion) {
    ostringstream buffer;
    streambuf* original = cout.rdbuf(buffer.rdbuf());
    ios formatoOriginal(nullptr);
    formatoOriginal.copyfmt(cout);

    accion();

    cout.copyfmt(formatoOriginal);
    cout.rdbuf(original);
    return buffer.str();
}

// ============================================================
// Caso 1 — Prioridades
// ============================================================
void testCaso1_Prioridades() {
    cout << "\nCaso 1 - Prioridades\n";
    CentroDeDistribucion cd;
    cd.registrarEnvio("A", "D1", "CENTRO", 1.0, NivelServicio::ESTANDAR);
    cd.registrarEnvio("B", "D2", "CENTRO", 1.0, NivelServicio::EXPRESS);
    cd.registrarEnvio("C", "D3", "CENTRO", 1.0, NivelServicio::PRIORITARIO);

    string salida = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(salida.find("B") < salida.find("C") && salida.find("C") < salida.find("A"),
             "La lista queda ordenada EXPRESS > PRIORITARIO > ESTANDAR");
}

// ============================================================
// Caso 2 — Prioridad estable
// ============================================================
void testCaso2_PrioridadEstable() {
    cout << "\nCaso 2 - Prioridad estable\n";
    CentroDeDistribucion cd;
    cd.registrarEnvio("X1", "D1", "CENTRO", 1.0, NivelServicio::PRIORITARIO);
    cd.registrarEnvio("X2", "D2", "CENTRO", 1.0, NivelServicio::PRIORITARIO);
    cd.registrarEnvio("X3", "D3", "CENTRO", 1.0, NivelServicio::PRIORITARIO);

    string salida = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(salida.find("X1") < salida.find("X2") && salida.find("X2") < salida.find("X3"),
             "Envios de igual prioridad conservan el orden de llegada");
}

// ============================================================
// Caso 3 — Despacho
// ============================================================
void testCaso3_Despacho() {
    cout << "\nCaso 3 - Despacho\n";
    CentroDeDistribucion cd;
    cd.registrarEnvio("D1", "Dest", "CENTRO", 1.0, NivelServicio::EXPRESS);

    string salidaDespacho = capturarSalida([&] { cd.despacharProximo(); });
    chequear(salidaDespacho.find("Despachado: D1") != string::npos,
              "Se informa el despacho del envio");

    string salidaBuscar = capturarSalida([&] { cd.buscarEnvio("D1"); });
    chequear(salidaBuscar.find("EN_REPARTO") != string::npos, "El estado cambio a EN_REPARTO");
    chequear(salidaBuscar.find("D1") != string::npos,
              "El objeto Envio se conserva (se lo puede seguir consultando)");

    string salidaHistorial = capturarSalida([&] { cd.mostrarHistorial("D1"); });
    chequear(salidaHistorial.find("RECIBIDO") != string::npos &&
                  salidaHistorial.find("EN_REPARTO") != string::npos,
              "Se creo el movimiento EN_REPARTO sin perder el RECIBIDO inicial");

    string salidaPendientes = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(salidaPendientes.find("D1") == string::npos,
              "El nodo se elimino de la lista de pendientes");
}

// ============================================================
// Caso 4 — Reprogramacion
// ============================================================
void testCaso4_Reprogramacion() {
    cout << "\nCaso 4 - Reprogramacion\n";
    CentroDeDistribucion cd;
    cd.registrarEnvio("R1", "Dest", "CENTRO", 1.0, NivelServicio::ESTANDAR);
    cd.registrarEnvio("R2", "Dest", "CENTRO", 1.0, NivelServicio::EXPRESS);

    capturarSalida([&] { cd.despacharProximo(); });  // despacha R2 (EXPRESS)
    capturarSalida([&] { cd.reprogramarEnvio("R2", "Destinatario ausente"); });

    string salidaBuscar = capturarSalida([&] { cd.buscarEnvio("R2"); });
    chequear(salidaBuscar.find("intentos: 1") != string::npos,
              "Aumento la cantidad de intentos de entrega");
    chequear(salidaBuscar.find("REPROGRAMADO") != string::npos, "El estado paso a REPROGRAMADO");

    string salidaHistorial = capturarSalida([&] { cd.mostrarHistorial("R2"); });
    chequear(salidaHistorial.find("Destinatario ausente") != string::npos,
              "Se registro el movimiento con la observacion indicada");

    string salidaPendientes = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(salidaPendientes.find("R2") != string::npos, "El envio volvio a la lista de pendientes");
    chequear(salidaPendientes.find("R2") < salidaPendientes.find("R1"),
              "Se reinserto respetando su prioridad (EXPRESS antes que ESTANDAR)");
}

// ============================================================
// Caso 5 — Historial (directo e inverso)
// ============================================================
void testCaso5_Historial() {
    cout << "\nCaso 5 - Historial\n";
    CentroDeDistribucion cd;
    cd.registrarEnvio("H1", "Dest", "CENTRO", 1.0, NivelServicio::ESTANDAR);
    capturarSalida([&] { cd.cambiarEstado("H1", Estado::CLASIFICADO, "Clasificado en zona"); });
    capturarSalida([&] { cd.despacharProximo(); });

    string historial = capturarSalida([&] { cd.mostrarHistorial("H1"); });

    size_t posRecibido = historial.find("RECIBIDO");
    size_t posClasificado = historial.find("CLASIFICADO");
    size_t posReparto = historial.find("EN_REPARTO");
    chequear(posRecibido < posClasificado && posClasificado < posReparto,
              "Orden cronologico: del mas antiguo al mas reciente");

    size_t inicioInverso = historial.find("Historial inverso");
    size_t posRepartoInv = historial.find("EN_REPARTO", inicioInverso);
    size_t posRecibidoInv = historial.find("RECIBIDO", inicioInverso);
    chequear(posRepartoInv < posRecibidoInv,
              "Orden inverso: del mas reciente al mas antiguo");
}

// ============================================================
// Caso 6 — Recursividad (resumen por zona + desafio opcional)
// ============================================================
void testCaso6_Recursividad() {
    cout << "\nCaso 6 - Recursividad\n";
    CentroDeDistribucion cd;
    cd.registrarEnvio("Z1", "D", "NORTE", 1.0, NivelServicio::EXPRESS);
    cd.registrarEnvio("Z2", "D", "NORTE", 2.0, NivelServicio::PRIORITARIO);
    cd.registrarEnvio("Z3", "D", "NORTE", 3.0, NivelServicio::ESTANDAR);
    cd.registrarEnvio("Z4", "D", "SUR", 9.0, NivelServicio::EXPRESS);  // otra zona: no debe contar

    string salida = capturarSalida([&] { cd.resumenZona("NORTE"); });
    chequear(salida.find("Cantidad de paquetes: 3") != string::npos,
              "Cuenta solo los paquetes de la zona pedida");
    chequear(salida.find("Peso total pendiente: 6") != string::npos,
              "Suma el peso total de la zona (1+2+3 = 6kg)");
    chequear(salida.find("Cantidad EXPRESS: 1") != string::npos,
              "Cuenta solo los EXPRESS de la zona pedida");

    string salidaPeso = capturarSalida([&] { cd.envioMasPesadoDeZona("NORTE"); });
    chequear(salidaPeso.find("Z3") != string::npos,
              "Desafio opcional: identifica el envio mas pesado de la zona");
}

// ============================================================
// Caso 7 — Casos limite
// ============================================================
void testCaso7_CasosLimite() {
    cout << "\nCaso 7 - Casos limite\n";
    CentroDeDistribucion cd;

    string salidaVacia = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(salidaVacia.find("no hay envios pendientes") != string::npos,
              "Lista de pendientes vacia se informa sin romper el programa");

    string salidaDespachoVacio = capturarSalida([&] { cd.despacharProximo(); });
    chequear(salidaDespachoVacio.find("No hay envios pendientes") != string::npos,
              "Despachar con la lista vacia no rompe el programa");

    string salidaBusquedaInexistente = capturarSalida([&] { cd.buscarEnvio("NO-EXISTE"); });
    chequear(salidaBusquedaInexistente.find("Envio no encontrado") != string::npos,
              "Busqueda de un codigo inexistente se informa correctamente");

    capturarSalida([&] { cd.registrarEnvio("DUP-1", "D", "CENTRO", 1.0, NivelServicio::ESTANDAR); });
    string salidaDuplicado =
        capturarSalida([&] { cd.registrarEnvio("DUP-1", "D", "CENTRO", 1.0, NivelServicio::ESTANDAR); });
    chequear(salidaDuplicado.find("ya existe un envio") != string::npos,
              "No permite registrar un codigo duplicado");

    string salidaHistUnico = capturarSalida([&] { cd.mostrarHistorial("DUP-1"); });
    chequear(salidaHistUnico.find("RECIBIDO") != string::npos &&
                  salidaHistUnico.find("EN_REPARTO") == string::npos &&
                  salidaHistUnico.find("CLASIFICADO") == string::npos,
              "El historial con un unico movimiento se muestra bien en ambos sentidos");

    capturarSalida([&] { cd.despacharProximo(); });  // despacha el unico pendiente (DUP-1)
    string salidaTrasEliminarUnico = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(salidaTrasEliminarUnico.find("no hay envios pendientes") != string::npos,
              "Eliminar el unico elemento deja la lista vacia otra vez");

    capturarSalida([&] { cd.registrarEnvio("NUEVO-1", "D", "CENTRO", 1.0, NivelServicio::ESTANDAR); });
    string salidaReinsercion = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(salidaReinsercion.find("NUEVO-1") != string::npos,
              "Se puede insertar de nuevo despues de vaciar la lista");
}

// ============================================================
// RF09 — Mesa de clasificacion y despacho
// ============================================================
// Los tests arman los Envio en el stack ANTES que la lista y la mesa: asi
// la mesa se destruye primero y se respeta el modelo de ownership del RF09.8.

static string codigoK(int n) {
    string d = to_string(n);
    while (d.size() < 3) d = "0" + d;
    return "K" + d;
}

// RF09.1 — Crear un lote por zona
void testRF09_1_LoteDeZona() {
    cout << "\nRF09.1 - Crear lote por zona\n";
    Envio e2("PKG-1002", "D", "NORTE", 0.75, NivelServicio::EXPRESS);
    Envio e3("PKG-1003", "D", "SUR",   4.10, NivelServicio::PRIORITARIO);
    Envio e5("PKG-1005", "D", "NORTE", 1.90, NivelServicio::PRIORITARIO);
    Envio e8("PKG-1008", "D", "NORTE", 3.40, NivelServicio::PRIORITARIO);
    ListaPendientes pend;
    pend.agregar(&e2); pend.agregar(&e3); pend.agregar(&e5); pend.agregar(&e8);
    MesaDeClasificacion mesa;

    bool ok = mesa.crearLote(pend, "NORTE");
    chequear(ok && mesa.getCantidad() == 3, "El lote contiene solo los 3 envios de la zona NORTE");
    chequear(mesa.getEnvio(0) == &e2 && mesa.getEnvio(1) == &e5 && mesa.getEnvio(2) == &e8,
             "El lote guarda REFERENCIAS a los envios existentes (mismos punteros, sin duplicar)");
    chequear(mesa.getEnvio(0)->getZona() == "NORTE" && mesa.getEnvio(1)->getZona() == "NORTE" &&
                 mesa.getEnvio(2)->getZona() == "NORTE",
             "Ningun envio de otra zona entra al lote");
    chequear(mesa.getEnvio(-1) == nullptr && mesa.getEnvio(3) == nullptr,
             "Acceder fuera de rango devuelve nullptr");
    chequear(pend.contarDeZona("NORTE") == 3 && pend.contarDeZona("SUR") == 1,
             "Crear el lote no modifica la lista de pendientes");

    string salida = capturarSalida([&] { ok = mesa.crearLote(pend, "OESTE"); });
    chequear(!ok && salida.find("No hay envios pendientes para la zona seleccionada.") != string::npos,
             "Zona sin pendientes: se informa con el mensaje de la consigna");
    chequear(mesa.getCantidad() == 3 && mesa.getZona() == "NORTE",
             "Un intento fallido conserva el lote anterior");

    ListaPendientes vacia;
    MesaDeClasificacion mesa2;
    string salidaVacia = capturarSalida([&] { ok = mesa2.crearLote(vacia, "NORTE"); });
    chequear(!ok && mesa2.getCantidad() == 0 &&
                 salidaVacia.find("No hay envios pendientes para la zona seleccionada.") != string::npos,
             "Con la lista de pendientes vacia tampoco se crea lote");

    mesa.crearLote(pend, "SUR");
    chequear(mesa.getCantidad() == 1 && mesa.getEnvio(0) == &e3 && mesa.getOrden() == CriterioOrden::NINGUNO,
             "Crear un lote nuevo reemplaza al anterior y reinicia el orden");
}

// RF09.2 — Insertion Sort manual con los dos criterios
void testRF09_2_Ordenamiento() {
    cout << "\nRF09.2 - Ordenar el lote (Insertion Sort manual)\n";
    Envio e2("PKG-1002", "D", "NORTE", 0.75, NivelServicio::ESTANDAR);
    Envio e5("PKG-1005", "D", "NORTE", 1.90, NivelServicio::ESTANDAR);
    Envio e8("PKG-1008", "D", "NORTE", 3.40, NivelServicio::ESTANDAR);
    ListaPendientes pend;
    pend.agregar(&e5); pend.agregar(&e8); pend.agregar(&e2);   // lote: 1005, 1008, 1002
    MesaDeClasificacion mesa;
    mesa.crearLote(pend, "NORTE");

    mesa.ordenar(CriterioOrden::CODIGO_ASC);
    chequear(mesa.getEnvio(0) == &e2 && mesa.getEnvio(1) == &e5 && mesa.getEnvio(2) == &e8 &&
                 mesa.getOrden() == CriterioOrden::CODIGO_ASC,
             "Por codigo ascendente: PKG-1002, PKG-1005, PKG-1008");

    mesa.ordenar(CriterioOrden::PESO_DESC);
    chequear(mesa.getEnvio(0) == &e8 && mesa.getEnvio(1) == &e5 && mesa.getEnvio(2) == &e2 &&
                 mesa.getOrden() == CriterioOrden::PESO_DESC,
             "Por peso descendente: 3.40, 1.90, 0.75 kg");

    string salida = capturarSalida([&] { mesa.mostrarLote(); });
    chequear(salida.find("PKG-1008 | NORTE | 3.40 kg") < salida.find("PKG-1005 | NORTE | 1.90 kg") &&
                 salida.find("PKG-1005 | NORTE | 1.90 kg") < salida.find("PKG-1002 | NORTE | 0.75 kg"),
             "El lote ordenado por peso se muestra en el orden esperado");

    mesa.ordenar(CriterioOrden::PESO_DESC);
    chequear(mesa.getEnvio(0) == &e8 && mesa.getEnvio(2) == &e2,
             "Mejor caso: ordenar un lote ya ordenado no lo altera");

    // Estabilidad: A y B pesan lo mismo y deben conservar su orden relativo.
    Envio a("A", "D", "SUR", 2.0, NivelServicio::ESTANDAR);
    Envio b("B", "D", "SUR", 2.0, NivelServicio::ESTANDAR);
    Envio c("C", "D", "SUR", 5.0, NivelServicio::ESTANDAR);
    ListaPendientes pend2;
    pend2.agregar(&a); pend2.agregar(&b); pend2.agregar(&c);
    MesaDeClasificacion mesa2;
    mesa2.crearLote(pend2, "SUR");
    mesa2.ordenar(CriterioOrden::PESO_DESC);
    chequear(mesa2.getEnvio(0) == &c && mesa2.getEnvio(1) == &a && mesa2.getEnvio(2) == &b,
             "El ordenamiento es estable: los empates conservan el orden de llegada");

    // Peor caso: orden inverso.
    Envio z3("Z3", "D", "CENTRO", 1.0, NivelServicio::ESTANDAR);
    Envio z2("Z2", "D", "CENTRO", 1.0, NivelServicio::ESTANDAR);
    Envio z1("Z1", "D", "CENTRO", 1.0, NivelServicio::ESTANDAR);
    ListaPendientes pend3;
    pend3.agregar(&z3); pend3.agregar(&z2); pend3.agregar(&z1);
    MesaDeClasificacion mesa3;
    mesa3.crearLote(pend3, "CENTRO");
    mesa3.ordenar(CriterioOrden::CODIGO_ASC);
    chequear(mesa3.getEnvio(0) == &z1 && mesa3.getEnvio(1) == &z2 && mesa3.getEnvio(2) == &z3,
             "Peor caso: un lote en orden inverso queda ordenado");

    // Un solo elemento.
    Envio u("U", "D", "OESTE", 1.0, NivelServicio::ESTANDAR);
    ListaPendientes pend4;
    pend4.agregar(&u);
    MesaDeClasificacion mesa4;
    mesa4.crearLote(pend4, "OESTE");
    chequear(mesa4.ordenar(CriterioOrden::CODIGO_ASC) && mesa4.getEnvio(0) == &u,
             "Un lote de un solo envio se ordena sin problemas");

    // Sin lote.
    MesaDeClasificacion sinLote;
    bool ok = true;
    string salidaSinLote = capturarSalida([&] { ok = sinLote.ordenar(CriterioOrden::CODIGO_ASC); });
    chequear(!ok && salidaSinLote.find("No hay un lote creado") != string::npos,
             "Ordenar sin lote se informa sin romper el programa");

    // Lote grande y desordenado: debe quedar ordenado por ambos criterios.
    const int N = 50;
    Envio** arr = new Envio*[N];
    {
        ListaPendientes pend5;
        for (int i = 0; i < N; i++) {
            arr[i] = new Envio(codigoK((i * 37 + 11) % 101), "D", "NORTE",
                               ((i * 53) % 97) / 10.0 + 0.1, NivelServicio::ESTANDAR);
            pend5.agregar(arr[i]);
        }
        MesaDeClasificacion mesa5;
        mesa5.crearLote(pend5, "NORTE");

        mesa5.ordenar(CriterioOrden::CODIGO_ASC);
        bool ascendente = true;
        for (int i = 0; i + 1 < N; i++) {
            if (!(mesa5.getEnvio(i)->getCodigo() < mesa5.getEnvio(i + 1)->getCodigo())) ascendente = false;
        }
        chequear(ascendente, "50 envios desordenados quedan en codigo ascendente estricto");

        mesa5.ordenar(CriterioOrden::PESO_DESC);
        bool descendente = true;
        for (int i = 0; i + 1 < N; i++) {
            if (mesa5.getEnvio(i)->getPeso() < mesa5.getEnvio(i + 1)->getPeso()) descendente = false;
        }
        chequear(descendente, "50 envios desordenados quedan en peso descendente");
    }
    for (int i = 0; i < N; i++) delete arr[i];
    delete[] arr;
}

// RF09.3 — Queue propia (FIFO)
void testRF09_3_Queue() {
    cout << "\nRF09.3 - Queue propia (FIFO)\n";
    Envio a("PKG-1002", "D", "NORTE", 0.75, NivelServicio::EXPRESS);
    Envio b("PKG-1005", "D", "NORTE", 1.90, NivelServicio::PRIORITARIO);
    Envio c("PKG-1008", "D", "NORTE", 3.40, NivelServicio::PRIORITARIO);
    ColaEnvios q;

    chequear(q.isEmpty() && q.front() == nullptr && q.dequeue() == nullptr && q.size() == 0,
             "Una cola vacia: isEmpty, front y dequeue no rompen");
    string salidaVacia = capturarSalida([&] { q.mostrar(); });
    chequear(salidaVacia.find("vacia") != string::npos, "Una cola vacia se muestra como vacia");

    q.enqueue(nullptr);
    chequear(q.isEmpty(), "enqueue(nullptr) se ignora");

    q.enqueue(&a); q.enqueue(&b); q.enqueue(&c);
    chequear(!q.isEmpty() && q.size() == 3 && q.front() == &a,
             "Tras 3 enqueue el front es el primero que entro");
    string salida = capturarSalida([&] { q.mostrar(); });
    chequear(salida.find("FRONT -> PKG-1002 -> PKG-1005 -> PKG-1008 <- REAR") != string::npos,
             "Se muestra FRONT -> ... <- REAR");

    chequear(q.dequeue() == &a && q.front() == &b, "dequeue devuelve el primero (FIFO) y el front avanza");
    chequear(q.dequeue() == &b && q.dequeue() == &c, "El orden de salida es el de entrada");
    chequear(q.isEmpty() && q.dequeue() == nullptr, "Al vaciarse queda vacia");

    q.enqueue(&b);
    chequear(q.front() == &b && q.size() == 1, "Tras vaciarse se puede volver a encolar (el REAR se reinicio)");
    q.enqueue(&c);
    chequear(q.dequeue() == &b && q.dequeue() == &c && q.isEmpty(), "Sigue siendo FIFO tras reutilizarse");

    q.enqueue(&a); q.enqueue(&b);
    q.vaciar();
    chequear(q.isEmpty() && q.size() == 0, "vaciar() deja la cola vacia (libera solo nodos)");
    // Los envios siguen vivos (si la cola los hubiera destruido, ASAN lo detectaria).
    chequear(a.getCodigo() == "PKG-1002" && b.getCodigo() == "PKG-1005",
             "La cola no destruye los envios que referencia");
}

// RF09.4 — Stack propia (LIFO)
void testRF09_4_Stack() {
    cout << "\nRF09.4 - Stack propia (LIFO)\n";
    Envio a("PKG-1002", "D", "NORTE", 0.75, NivelServicio::EXPRESS);
    Envio b("PKG-1005", "D", "NORTE", 1.90, NivelServicio::PRIORITARIO);
    Envio c("PKG-1008", "D", "NORTE", 3.40, NivelServicio::PRIORITARIO);
    PilaEnvios p;

    chequear(p.isEmpty() && p.top() == nullptr && p.pop() == nullptr && p.size() == 0,
             "Una pila vacia: isEmpty, top y pop no rompen");
    string salidaVacia = capturarSalida([&] { p.mostrar(); });
    chequear(salidaVacia.find("vacia") != string::npos, "Una pila vacia se muestra como vacia");

    p.push(nullptr);
    chequear(p.isEmpty(), "push(nullptr) se ignora");

    p.push(&a); p.push(&b); p.push(&c);
    chequear(!p.isEmpty() && p.size() == 3 && p.top() == &c,
             "Tras 3 push el top es el ultimo que entro");
    string salida = capturarSalida([&] { p.mostrar(); });
    chequear(salida.find("TOP") < salida.find("PKG-1008") &&
                 salida.find("PKG-1008") < salida.find("PKG-1005") &&
                 salida.find("PKG-1005") < salida.find("PKG-1002"),
             "Se muestra TOP y debajo PKG-1008, PKG-1005, PKG-1002");

    chequear(p.top() == &c && p.size() == 3, "top() no desapila");
    chequear(p.pop() == &c && p.top() == &b, "pop devuelve el ultimo (LIFO) y el top baja");
    chequear(p.pop() == &b && p.pop() == &a && p.isEmpty(), "El orden de salida es el inverso al de entrada");
    chequear(p.pop() == nullptr, "pop sobre pila vacia devuelve nullptr");

    p.push(&a); p.push(&b);
    p.vaciar();
    chequear(p.isEmpty() && a.getCodigo() == "PKG-1002",
             "vaciar() deja la pila vacia y no destruye los envios");
}

// Arma el escenario NORTE (1002, 1005, 1008) usado por los siguientes tests.
// RF09.3 / RF09.4 integrados: lote -> Queue -> Stack
void testRF09_5_FlujoColaYPila() {
    cout << "\nRF09.3/09.4 - Lote ordenado -> colaPreparacion -> pilaProcesados\n";
    Envio e2("PKG-1002", "D", "NORTE", 0.75, NivelServicio::EXPRESS);
    Envio e5("PKG-1005", "D", "NORTE", 1.90, NivelServicio::PRIORITARIO);
    Envio e8("PKG-1008", "D", "NORTE", 3.40, NivelServicio::PRIORITARIO);
    ListaPendientes pend;
    pend.agregar(&e2); pend.agregar(&e5); pend.agregar(&e8);
    MesaDeClasificacion mesa;
    bool ok = true;

    string salidaSinLote = capturarSalida([&] { ok = mesa.cargarColaPreparacion(); });
    chequear(!ok && salidaSinLote.find("No hay un lote creado") != string::npos,
             "No se puede cargar la cola sin lote");

    mesa.crearLote(pend, "NORTE");
    string salidaSinOrdenar = capturarSalida([&] { ok = mesa.cargarColaPreparacion(); });
    chequear(!ok && mesa.getCola().isEmpty() && salidaSinOrdenar.find("Ordene primero") != string::npos,
             "No se carga la cola hasta que el lote este ordenado");

    mesa.ordenar(CriterioOrden::CODIGO_ASC);
    ok = mesa.cargarColaPreparacion();
    chequear(ok && mesa.getCola().size() == 3 && mesa.getCola().front() == &e2,
             "El lote ordenado entra a la cola: el primero queda en el FRONT");

    string salidaDoble = capturarSalida([&] { ok = mesa.cargarColaPreparacion(); });
    chequear(!ok && mesa.getCola().size() == 3 && salidaDoble.find("ya fue cargada") != string::npos,
             "No se puede cargar dos veces el mismo lote (no duplica la cola)");

    chequear(mesa.ultimoProcesado() == nullptr, "Antes de procesar no hay ultimo procesado");
    string salidaTopVacio = capturarSalida([&] { mesa.mostrarUltimoProcesado(); });
    chequear(salidaTopVacio.find("Todavia no se proceso ningun envio") != string::npos,
             "Sin procesados se informa en lugar de fallar");

    chequear(mesa.procesarSiguiente() == &e2 && mesa.ultimoProcesado() == &e2,
             "Primer dequeue: sale PKG-1002 y pasa al top de la pila");
    chequear(mesa.procesarSiguiente() == &e5 && mesa.ultimoProcesado() == &e5,
             "Segundo dequeue: sale PKG-1005 y pasa a ser el top");
    chequear(mesa.getCola().front() == &e8 && mesa.getPila().size() == 2,
             "La cola conserva lo que falta y la pila acumula lo procesado");
    chequear(mesa.procesarSiguiente() == &e8 && mesa.ultimoProcesado() == &e8,
             "Tercer dequeue: sale PKG-1008 (FIFO) y queda en el top (LIFO)");
    chequear(mesa.getCola().isEmpty() && mesa.getPila().size() == 3,
             "Cola vacia y pila con los 3 procesados");

    string salidaPila = capturarSalida([&] { mesa.mostrarPila(); });
    chequear(salidaPila.find("PKG-1008") < salidaPila.find("PKG-1005") &&
                 salidaPila.find("PKG-1005") < salidaPila.find("PKG-1002"),
             "La pila queda TOP: PKG-1008, PKG-1005, PKG-1002");

    Envio* extra = nullptr;
    string salidaColaVacia = capturarSalida([&] { extra = mesa.procesarSiguiente(); });
    chequear(extra == nullptr && mesa.getPila().size() == 3 &&
                 salidaColaVacia.find("vacia") != string::npos,
             "Procesar con la cola vacia se informa y no altera la pila");

    chequear(e2.getEstado() == Estado::RECIBIDO && e5.getEstado() == Estado::RECIBIDO &&
                 e8.getEstado() == Estado::RECIBIDO,
             "La mesa no cambia el estado de los envios (no reemplaza al historial permanente)");

    mesa.crearLote(pend, "NORTE");
    chequear(mesa.getCola().isEmpty() && mesa.getPila().isEmpty(),
             "Crear un lote nuevo reinicia la cola y la pila");
}

// RF09.5 — Busqueda lineal
void testRF09_6_BusquedaLineal() {
    cout << "\nRF09.5 - Busqueda lineal\n";
    Envio e2("PKG-1002", "D", "NORTE", 0.75, NivelServicio::EXPRESS);
    Envio e5("PKG-1005", "D", "NORTE", 1.90, NivelServicio::PRIORITARIO);
    Envio e8("PKG-1008", "D", "NORTE", 3.40, NivelServicio::PRIORITARIO);
    ListaPendientes pend;
    pend.agregar(&e2); pend.agregar(&e5); pend.agregar(&e8);
    MesaDeClasificacion mesa;
    int comps = -1;

    chequear(mesa.buscarLineal("PKG-1005", comps) == -1 && comps == 0,
             "Sin lote la busqueda no encuentra nada y no compara");

    mesa.crearLote(pend, "NORTE");
    mesa.ordenar(CriterioOrden::PESO_DESC);   // lote: 1008, 1005, 1002 (NO ordenado por codigo)

    chequear(mesa.buscarLineal("PKG-1005", comps) == 1 && comps == 2,
             "Funciona con el lote ordenado por peso: PKG-1005 esta en la posicion 1 (2 comparaciones)");
    chequear(mesa.buscarLineal("PKG-1008", comps) == 0 && comps == 1,
             "Mejor caso: el primero se encuentra con 1 comparacion");
    chequear(mesa.buscarLineal("PKG-1002", comps) == 2 && comps == 3,
             "Peor caso (esta al final): n comparaciones");
    chequear(mesa.buscarLineal("PKG-9999", comps) == -1 && comps == 3,
             "Codigo inexistente: recorre todo el lote (n comparaciones) y devuelve -1");

    string salida = capturarSalida([&] { mesa.mostrarBusquedaLineal("PKG-1005"); });
    chequear(salida.find("Encontrado en la posicion 1") != string::npos,
             "Se informa 'Encontrado en la posicion 1'");
    string salidaNo = capturarSalida([&] { mesa.mostrarBusquedaLineal("PKG-9999"); });
    chequear(salidaNo.find("No encontrado") != string::npos, "Se informa cuando no se encuentra");
}

// RF09.6 — Busqueda binaria
void testRF09_7_BusquedaBinaria() {
    cout << "\nRF09.6 - Busqueda binaria\n";
    Envio e2("PKG-1002", "D", "NORTE", 0.75, NivelServicio::EXPRESS);
    Envio e5("PKG-1005", "D", "NORTE", 1.90, NivelServicio::PRIORITARIO);
    Envio e8("PKG-1008", "D", "NORTE", 3.40, NivelServicio::PRIORITARIO);
    ListaPendientes pend;
    pend.agregar(&e2); pend.agregar(&e5); pend.agregar(&e8);
    MesaDeClasificacion mesa;
    int comps = -1;

    mesa.crearLote(pend, "NORTE");   // sin ordenar
    chequear(mesa.buscarBinaria("PKG-1005", comps) == -1 && comps == 0,
             "Lote sin ordenar: la busqueda binaria NO se ejecuta");
    mesa.ordenar(CriterioOrden::PESO_DESC);
    chequear(mesa.buscarBinaria("PKG-1005", comps) == -1 && comps == 0,
             "Lote ordenado por peso: la busqueda binaria NO se ejecuta");
    string salidaRechazo = capturarSalida([&] { mesa.mostrarBusquedaBinaria("PKG-1005"); });
    chequear(salidaRechazo.find("requiere el lote ordenado por codigo") != string::npos &&
                 salidaRechazo.find("encontrado") == string::npos,
             "Se informa que requiere el lote ordenado por codigo");

    mesa.ordenar(CriterioOrden::CODIGO_ASC);
    chequear(mesa.buscarBinaria("PKG-1005", comps) == 1 && comps == 1,
             "Con el lote por codigo: PKG-1005 en la posicion 1 (mid = 1, mejor caso)");
    chequear(mesa.buscarBinaria("PKG-1002", comps) == 0, "Encuentra el primero");
    chequear(mesa.buscarBinaria("PKG-1008", comps) == 2 && comps == 2, "Encuentra el ultimo");
    chequear(mesa.buscarBinaria("PKG-0001", comps) == -1, "Codigo menor que todos: no encontrado");
    chequear(mesa.buscarBinaria("PKG-9999", comps) == -1, "Codigo mayor que todos: no encontrado");
    chequear(mesa.buscarBinaria("PKG-1006", comps) == -1, "Codigo intermedio inexistente: no encontrado");

    string salida = capturarSalida([&] { mesa.mostrarBusquedaBinaria("PKG-1005"); });
    chequear(salida.find("izq = 0, der = 2, mid = 1") != string::npos &&
                 salida.find("PKG-1005 encontrado") != string::npos,
             "Se muestra la traza izq = 0, der = 2, mid = 1 y 'PKG-1005 encontrado'");

    // Lote grande: toda busqueda debe acertar y respetar la cota O(log n).
    const int N = 50;
    Envio** arr = new Envio*[N];
    bool todosEncontrados = true;
    bool dentroDeCota = true;
    bool linealCuentaBien = true;
    {
        ListaPendientes pendGrande;
        for (int i = 0; i < N; i++) {
            arr[i] = new Envio(codigoK((i * 37 + 11) % 101), "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
            pendGrande.agregar(arr[i]);
        }
        MesaDeClasificacion grande;
        grande.crearLote(pendGrande, "NORTE");
        grande.ordenar(CriterioOrden::CODIGO_ASC);

        for (int i = 0; i < N; i++) {
            int cb = 0, cl = 0;
            int posB = grande.buscarBinaria(arr[i]->getCodigo(), cb);
            int posL = grande.buscarLineal(arr[i]->getCodigo(), cl);
            if (posB < 0 || grande.getEnvio(posB) != arr[i] || posB != posL) todosEncontrados = false;
            if (cb > 6) dentroDeCota = false;           // floor(log2(50)) + 1 = 6
            if (cl != posL + 1) linealCuentaBien = false;   // la lineal cuenta posicion + 1
        }
        int cb = 0;
        if (grande.buscarBinaria("K999", cb) != -1 || cb > 6) dentroDeCota = false;
        if (grande.buscarBinaria("A", cb) != -1 || cb > 6) dentroDeCota = false;
    }
    for (int i = 0; i < N; i++) delete arr[i];
    delete[] arr;
    chequear(todosEncontrados, "En un lote de 50, la binaria encuentra cada envio en su posicion correcta");
    chequear(dentroDeCota, "Peor caso O(log n): nunca mas de 6 comparaciones en un lote de 50");
    chequear(linealCuentaBien, "La lineal hace exactamente (posicion + 1) comparaciones al encontrar el codigo");
}

// RF09.7 — Comparacion de busquedas
void testRF09_8_ComparacionDeBusquedas() {
    cout << "\nRF09.7 - Comparacion lineal vs binaria\n";
    Envio e2("PKG-1002", "D", "NORTE", 0.75, NivelServicio::EXPRESS);
    Envio e5("PKG-1005", "D", "NORTE", 1.90, NivelServicio::PRIORITARIO);
    Envio e8("PKG-1008", "D", "NORTE", 3.40, NivelServicio::PRIORITARIO);
    ListaPendientes pend;
    pend.agregar(&e2); pend.agregar(&e5); pend.agregar(&e8);
    MesaDeClasificacion mesa;

    string salidaSinLote = capturarSalida([&] { mesa.compararBusquedas("PKG-1008"); });
    chequear(salidaSinLote.find("No hay un lote creado") != string::npos, "Sin lote se informa");

    mesa.crearLote(pend, "NORTE");
    string salidaSinOrden = capturarSalida([&] { mesa.compararBusquedas("PKG-1008"); });
    chequear(salidaSinOrden.find("ordenado por codigo") != string::npos &&
                 salidaSinOrden.find("Comparaciones") == string::npos,
             "Con el lote sin ordenar por codigo no compara: pide ordenarlo");

    mesa.ordenar(CriterioOrden::CODIGO_ASC);
    string salida = capturarSalida([&] { mesa.compararBusquedas("PKG-1008"); });
    size_t posLineal = salida.find("Busqueda lineal");
    size_t posBinaria = salida.find("Busqueda binaria");
    chequear(posLineal != string::npos && posBinaria != string::npos && posLineal < posBinaria,
             "Se informan ambas busquedas para el mismo codigo");
    chequear(salida.find("Comparaciones: 3", posLineal) < posBinaria,
             "Lineal: PKG-1008 al final del lote de 3 -> 3 comparaciones");
    chequear(salida.find("Comparaciones: 2", posBinaria) != string::npos,
             "Binaria: PKG-1008 -> 2 comparaciones sobre el mismo lote");
    chequear(salida.find("O(1)") != string::npos && salida.find("O(n)") != string::npos &&
                 salida.find("O(log n)") != string::npos,
             "Se justifican las complejidades: O(1), O(n) y O(log n)");

    string salidaNo = capturarSalida([&] { mesa.compararBusquedas("PKG-9999"); });
    chequear(salidaNo.find("No encontrado") != string::npos,
             "Un codigo inexistente se informa en ambas busquedas");
}

// RF09.8 — Ownership del lote temporal
void testRF09_9_Ownership() {
    cout << "\nRF09.8 - Ownership del lote temporal\n";

    // (a) Destruir la mesa con envios todavia en la cola y en la pila no
    //     destruye los envios (si lo hiciera, el acceso posterior o el delete
    //     de abajo serian un use-after-free / double free que ASAN detecta).
    Envio* a = new Envio("PKG-1002", "D", "NORTE", 0.75, NivelServicio::EXPRESS);
    Envio* b = new Envio("PKG-1005", "D", "NORTE", 1.90, NivelServicio::PRIORITARIO);
    Envio* c = new Envio("PKG-1008", "D", "NORTE", 3.40, NivelServicio::PRIORITARIO);
    {
        ListaPendientes pend;
        pend.agregar(a); pend.agregar(b); pend.agregar(c);
        MesaDeClasificacion mesa;
        mesa.crearLote(pend, "NORTE");
        mesa.ordenar(CriterioOrden::CODIGO_ASC);
        mesa.cargarColaPreparacion();
        mesa.procesarSiguiente();   // a -> pila; b y c quedan en la cola
    }   // se destruyen mesa (arreglo + nodos de cola y pila) y pend (solo nodos)
    chequear(a->getCodigo() == "PKG-1002" && b->getCodigo() == "PKG-1005" && c->getCodigo() == "PKG-1008",
             "Destruir el lote, la cola y la pila no destruye los envios");
    delete a; delete b; delete c;   // los destruye su dueno (aca, el test)

    // (b) Con el sistema completo: la mesa referencia envios del registro.
    CentroDeDistribucion cd;
    capturarSalida([&] {
        cd.registrarEnvio("N1", "D", "NORTE", 1.0, NivelServicio::EXPRESS);
        cd.registrarEnvio("N2", "D", "NORTE", 2.0, NivelServicio::ESTANDAR);
        cd.registrarEnvio("S1", "D", "SUR",   3.0, NivelServicio::ESTANDAR);
    });
    bool ok = false;
    capturarSalida([&] { ok = cd.crearLoteZona("NORTE"); });
    chequear(ok && cd.mesa().getCantidad() == 2, "El centro crea el lote NORTE con sus 2 pendientes");

    capturarSalida([&] { cd.despacharProximo(); });   // despacha N1: sale de pendientes, sigue vivo
    chequear(cd.mesa().getEnvio(0) != nullptr && cd.mesa().getEnvio(0)->getCodigo() == "N1" &&
                 cd.mesa().getEnvio(0)->getEstado() == Estado::EN_REPARTO,
             "El lote conserva una referencia valida a un envio ya despachado (el registro lo sigue poseyendo)");

    string salidaBuscar = capturarSalida([&] { cd.buscarEnvio("N1"); });
    chequear(salidaBuscar.find("N1") != string::npos, "El envio sigue consultable por codigo en el registro");

    string salidaPend = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(salidaPend.find("N2") != string::npos && salidaPend.find("S1") != string::npos,
             "Trabajar con la mesa no altera la lista de pendientes");

    cd.mesa().ordenar(CriterioOrden::CODIGO_ASC);
    cd.mesa().cargarColaPreparacion();
    cd.mesa().procesarSiguiente();
    // Al salir de la funcion se destruye `cd`: mesa primero (solo arreglo y
    // nodos), despues registro (los Envio). Sin doble free ni fugas bajo ASAN.
}

// ============================================================
// Helpers para los tests de RF04 / RF06 / RF07 / RF10
// ============================================================
static int contar(const string& texto, const string& sub) {
    int n = 0;
    size_t pos = 0;
    while ((pos = texto.find(sub, pos)) != string::npos) {
        n++;
        pos += sub.size();
    }
    return n;
}

// Extrae el codigo (lo que va antes del primer " | ") de cada linea de una salida.
static int extraerCodigos(const string& salida, string* destino, int maximo) {
    istringstream in(salida);
    string linea;
    int n = 0;
    while (n < maximo && getline(in, linea)) {
        size_t barra = linea.find(" | ");
        if (barra == string::npos) continue;
        size_t inicio = linea.find_first_not_of(' ');
        destino[n++] = linea.substr(inicio, barra - inicio);
    }
    return n;
}

// ============================================================
// Maquina de estados (RF04 / RF06 / RF07)
// ============================================================
void testEstados_MatrizDeTransiciones() {
    cout << "\nMaquina de estados - matriz completa de transiciones\n";
    // Orden de filas y columnas: RECIBIDO, CLASIFICADO, EN_REPARTO, REPROGRAMADO, ENTREGADO.
    const Estado todos[5] = {Estado::RECIBIDO, Estado::CLASIFICADO, Estado::EN_REPARTO,
                             Estado::REPROGRAMADO, Estado::ENTREGADO};
    const char* esperado[5] = {
        "01100",   // RECIBIDO     -> CLASIFICADO, EN_REPARTO
        "00100",   // CLASIFICADO  -> EN_REPARTO
        "00011",   // EN_REPARTO   -> REPROGRAMADO, ENTREGADO
        "00100",   // REPROGRAMADO -> EN_REPARTO
        "00000"    // ENTREGADO    -> (final)
    };
    for (int i = 0; i < 5; i++) {
        bool filaOk = true;
        for (int j = 0; j < 5; j++) {
            bool permitida = transicionPermitida(todos[i], todos[j]);
            if (permitida != (esperado[i][j] == '1')) filaOk = false;
        }
        chequear(filaOk, "Transiciones permitidas desde " + estadoToString(todos[i]) + " (" + esperado[i] + ")");
    }
    chequear(estadoEsPendiente(Estado::RECIBIDO) && estadoEsPendiente(Estado::CLASIFICADO) &&
                 estadoEsPendiente(Estado::REPROGRAMADO) && !estadoEsPendiente(Estado::EN_REPARTO) &&
                 !estadoEsPendiente(Estado::ENTREGADO),
             "Solo RECIBIDO, CLASIFICADO y REPROGRAMADO figuran en pendientes");
}

void testRF04_CambiarEstado() {
    cout << "\nRF04 - Cambiar estado\n";
    CentroDeDistribucion cd;
    capturarSalida([&] {
        cd.registrarEnvio("C1", "D", "CENTRO", 1.0, NivelServicio::PRIORITARIO);
        cd.registrarEnvio("C2", "D", "CENTRO", 1.0, NivelServicio::PRIORITARIO);
    });

    string s = capturarSalida([&] { cd.cambiarEstado("C1", Estado::CLASIFICADO, "Clasificado en zona"); });
    chequear(s.find("Estado actualizado") != string::npos, "RECIBIDO -> CLASIFICADO es una transicion valida");
    string hist = capturarSalida([&] { cd.mostrarHistorial("C1"); });
    chequear(contar(hist, "CLASIFICADO") == 2 && hist.find("RECIBIDO") < hist.find("CLASIFICADO"),
             "El cambio genera un movimiento nuevo, agregado al final del historial");
    string pend = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(pend.find("C1 |") < pend.find("C2 |"),
             "Clasificar no mueve al envio dentro de pendientes (conserva su orden de llegada)");

    // Cambios no permitidos: se rechazan sin tocar el envio ni su historial.
    string rechazo1 = capturarSalida([&] { cd.cambiarEstado("C1", Estado::RECIBIDO, "x"); });
    string rechazo2 = capturarSalida([&] { cd.cambiarEstado("C1", Estado::REPROGRAMADO, "x"); });
    string rechazo3 = capturarSalida([&] { cd.cambiarEstado("C1", Estado::CLASIFICADO, "x"); });
    string rechazo4 = capturarSalida([&] { cd.cambiarEstado("C1", Estado::ENTREGADO, "x"); });
    chequear(rechazo1.find("Transicion invalida") != string::npos &&
                 rechazo2.find("Transicion invalida") != string::npos &&
                 rechazo3.find("Transicion invalida") != string::npos &&
                 rechazo4.find("Transicion invalida") != string::npos,
             "Volver atras, saltar a REPROGRAMADO, repetir estado o entregar sin despachar: se rechazan");
    chequear(rechazo1.find("Desde CLASIFICADO se puede pasar a: EN_REPARTO") != string::npos,
             "El mensaje indica a que estados se puede pasar");
    string histDespues = capturarSalida([&] { cd.mostrarHistorial("C1"); });
    chequear(histDespues == hist, "Los cambios rechazados no agregan ningun movimiento");

    string noExiste = capturarSalida([&] { cd.cambiarEstado("NO-EXISTE", Estado::CLASIFICADO, "x"); });
    chequear(noExiste.find("Envio no encontrado") != string::npos, "Cambiar el estado de un codigo inexistente se informa");

    // Pasar a EN_REPARTO por "cambiar estado" lo saca de pendientes (consistencia).
    capturarSalida([&] { cd.cambiarEstado("C1", Estado::EN_REPARTO, "Sale a reparto"); });
    string pend2 = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(pend2.find("C1 |") == string::npos && pend2.find("C2 |") != string::npos,
             "Un envio pasado a EN_REPARTO deja de figurar en pendientes");
    string desp = capturarSalida([&] { cd.despacharProximo(); });
    chequear(desp.find("Despachado: C2") != string::npos, "Despachar no vuelve a sacar a C1 (ya esta en reparto)");
    string desp2 = capturarSalida([&] { cd.despacharProximo(); });
    chequear(desp2.find("No hay envios pendientes") != string::npos, "No quedan pendientes");

    string atras = capturarSalida([&] { cd.cambiarEstado("C1", Estado::CLASIFICADO, "x"); });
    chequear(atras.find("Transicion invalida") != string::npos, "EN_REPARTO no puede volver a CLASIFICADO");

    // REPROGRAMADO por "cambiar estado" aplica las reglas del RF06.
    capturarSalida([&] { cd.cambiarEstado("C1", Estado::REPROGRAMADO, "Destinatario ausente"); });
    string buscar = capturarSalida([&] { cd.buscarEnvio("C1"); });
    string pend3 = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(buscar.find("REPROGRAMADO") != string::npos && buscar.find("intentos: 1") != string::npos &&
                 contar(pend3, "C1 |") == 1,
             "EN_REPARTO -> REPROGRAMADO por cambiar estado suma un intento y vuelve a pendientes una sola vez");

    capturarSalida([&] { cd.registrarEnvio("C3", "D", "CENTRO", 1.0, NivelServicio::ESTANDAR); });
    string repro = capturarSalida([&] { cd.cambiarEstado("C3", Estado::REPROGRAMADO, "x"); });
    string buscar3 = capturarSalida([&] { cd.buscarEnvio("C3"); });
    chequear(repro.find("Transicion invalida") != string::npos && buscar3.find("intentos: 0") != string::npos,
             "REPROGRAMADO solo se acepta desde EN_REPARTO: un pendiente no suma intentos");
}

void testRF06_Reprogramar() {
    cout << "\nRF06 - Reprogramar un envio\n";
    CentroDeDistribucion cd;
    capturarSalida([&] {
        cd.registrarEnvio("PKG-1002", "D", "NORTE", 0.75, NivelServicio::EXPRESS);
        cd.registrarEnvio("PKG-1003", "D", "SUR", 4.1, NivelServicio::ESTANDAR);
    });

    // Un envio que sigue en pendientes NO se puede reprogramar (error original: quedaba duplicado).
    string s1 = capturarSalida([&] { cd.reprogramarEnvio("PKG-1002", "Destinatario ausente"); });
    string pend1 = capturarSalida([&] { cd.mostrarPendientes(); });
    string buscar1 = capturarSalida([&] { cd.buscarEnvio("PKG-1002"); });
    chequear(s1.find("Solo puede reprogramarse un envio EN_REPARTO") != string::npos,
             "Reprogramar un envio que sigue pendiente se rechaza con un mensaje claro");
    chequear(contar(pend1, "PKG-1002 |") == 1, "No queda duplicado en pendientes");
    chequear(buscar1.find("intentos: 0") != string::npos && buscar1.find("RECIBIDO") != string::npos,
             "Un reprogramado rechazado no suma intentos ni cambia el estado");

    capturarSalida([&] { cd.despacharProximo(); });   // sale PKG-1002 (EXPRESS)
    string s2 = capturarSalida([&] { cd.reprogramarEnvio("PKG-1002", "Destinatario ausente"); });
    chequear(s2.find("reprogramado (intento 1)") != string::npos, "Un envio EN_REPARTO si se puede reprogramar");
    string pend2 = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(contar(pend2, "PKG-1002 |") == 1 && pend2.find("PKG-1002 |") < pend2.find("PKG-1003 |"),
             "Vuelve a pendientes una sola vez y reinsertado por prioridad (EXPRESS primero)");

    string s3 = capturarSalida([&] { cd.reprogramarEnvio("PKG-1002", "Otra vez"); });
    string pend3 = capturarSalida([&] { cd.mostrarPendientes(); });
    string buscar3 = capturarSalida([&] { cd.buscarEnvio("PKG-1002"); });
    chequear(s3.find("Solo puede reprogramarse un envio EN_REPARTO") != string::npos &&
                 contar(pend3, "PKG-1002 |") == 1 && buscar3.find("intentos: 1") != string::npos,
             "Reprogramar dos veces seguidas se rechaza: ni duplicado ni intento de mas");

    string desp = capturarSalida([&] { cd.despacharProximo(); });
    chequear(desp.find("Despachado: PKG-1002") != string::npos, "Luego de reinsertarlo puede despacharse nuevamente");

    string noExiste = capturarSalida([&] { cd.reprogramarEnvio("NO-EXISTE", "x"); });
    chequear(noExiste.find("Envio no encontrado") != string::npos, "Reprogramar un codigo inexistente se informa");

    // Recorrido completo del ejemplo del RF08.
    CentroDeDistribucion cd2;
    capturarSalida([&] { cd2.registrarEnvio("R1", "D", "CENTRO", 1.0, NivelServicio::ESTANDAR); });
    capturarSalida([&] { cd2.cambiarEstado("R1", Estado::CLASIFICADO, "Clasificado"); });
    capturarSalida([&] { cd2.despacharProximo(); });
    capturarSalida([&] { cd2.reprogramarEnvio("R1", "Destinatario ausente"); });
    capturarSalida([&] { cd2.despacharProximo(); });
    capturarSalida([&] { cd2.finalizarEntrega("R1", "Entregado en mano"); });

    string hist = capturarSalida([&] { cd2.mostrarHistorial("R1"); });
    size_t inverso = hist.find("Historial inverso");
    size_t p1 = hist.find("RECIBIDO");
    size_t p2 = hist.find("CLASIFICADO", p1);
    size_t p3 = hist.find("EN_REPARTO", p2);
    size_t p4 = hist.find("REPROGRAMADO", p3);
    size_t p5 = hist.find("EN_REPARTO", p4);
    size_t p6 = hist.find("ENTREGADO", p5);
    chequear(p6 < inverso, "Orden cronologico: RECIBIDO, CLASIFICADO, EN_REPARTO, REPROGRAMADO, EN_REPARTO, ENTREGADO");
    size_t i1 = hist.find("ENTREGADO", inverso);
    size_t i2 = hist.find("EN_REPARTO", i1);
    size_t i3 = hist.find("REPROGRAMADO", i2);
    size_t i4 = hist.find("EN_REPARTO", i3);
    size_t i5 = hist.find("CLASIFICADO", i4);
    size_t i6 = hist.find("RECIBIDO", i5);
    chequear(i6 != string::npos, "Orden inverso: ENTREGADO, EN_REPARTO, REPROGRAMADO, EN_REPARTO, CLASIFICADO, RECIBIDO");
    chequear(contar(hist, "EN_REPARTO") == 4 && contar(hist, "REPROGRAMADO") == 2,
             "El historial tiene exactamente un movimiento por cada cambio (sin duplicados)");

    // Red de seguridad de ListaPendientes: un mismo Envio no entra dos veces.
    Envio e("X", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    ListaPendientes pend;
    chequear(pend.agregar(&e) && !pend.agregar(&e) && pend.contiene(&e) && !pend.agregar(nullptr),
             "ListaPendientes no admite el mismo envio dos veces (ni nullptr)");
    chequear(pend.contarDeZona("NORTE") == 1, "Y la lista conserva una sola copia");
}

void testRF07_FinalizarEntrega() {
    cout << "\nRF07 - Finalizar una entrega\n";
    CentroDeDistribucion cd;
    capturarSalida([&] { cd.registrarEnvio("F1", "D", "CENTRO", 1.0, NivelServicio::ESTANDAR); });

    string s0 = capturarSalida([&] { cd.finalizarEntrega("F1", "x"); });
    string pend0 = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(s0.find("Despachelo primero") != string::npos && pend0.find("F1 |") != string::npos,
             "No se puede entregar un envio que todavia no salio a reparto");

    capturarSalida([&] { cd.despacharProximo(); });
    string s1 = capturarSalida([&] { cd.finalizarEntrega("F1", "Entregado en mano"); });
    chequear(s1.find("entregado") != string::npos, "Un envio EN_REPARTO se entrega");
    string buscar = capturarSalida([&] { cd.buscarEnvio("F1"); });
    string hist = capturarSalida([&] { cd.mostrarHistorial("F1"); });
    chequear(buscar.find("ENTREGADO") != string::npos, "El estado paso a ENTREGADO");
    chequear(contar(hist, "ENTREGADO") == 2 && hist.find("Entregado en mano") != string::npos,
             "Se registro un unico movimiento ENTREGADO con su observacion");
    string pend1 = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(pend1.find("F1 |") == string::npos, "Un envio entregado no figura en pendientes");

    // ENTREGADO es final: ningun camino lo devuelve a pendientes.
    string r1 = capturarSalida([&] { cd.cambiarEstado("F1", Estado::RECIBIDO, "x"); });
    string r2 = capturarSalida([&] { cd.cambiarEstado("F1", Estado::CLASIFICADO, "x"); });
    string r3 = capturarSalida([&] { cd.cambiarEstado("F1", Estado::EN_REPARTO, "x"); });
    string r4 = capturarSalida([&] { cd.cambiarEstado("F1", Estado::REPROGRAMADO, "x"); });
    string r5 = capturarSalida([&] { cd.cambiarEstado("F1", Estado::ENTREGADO, "x"); });
    chequear(contar(r1 + r2 + r3 + r4 + r5, "su estado es final") == 5,
             "Desde ENTREGADO se rechaza cambiar a cualquier estado (incluso a ENTREGADO)");
    string r6 = capturarSalida([&] { cd.reprogramarEnvio("F1", "x"); });
    string r7 = capturarSalida([&] { cd.finalizarEntrega("F1", "x"); });
    chequear(r6.find("ya fue entregado") != string::npos && r7.find("ya fue entregado") != string::npos,
             "Un envio entregado no se puede reprogramar ni entregar de nuevo");
    string pend2 = capturarSalida([&] { cd.mostrarPendientes(); });
    string buscar2 = capturarSalida([&] { cd.buscarEnvio("F1"); });
    string hist2 = capturarSalida([&] { cd.mostrarHistorial("F1"); });
    chequear(pend2.find("F1 |") == string::npos && buscar2.find("ENTREGADO") != string::npos &&
                 buscar2.find("intentos: 0") != string::npos && hist2 == hist,
             "Tras los intentos rechazados sigue entregado, sin intentos nuevos, fuera de pendientes y con el mismo historial");

    string noExiste = capturarSalida([&] { cd.finalizarEntrega("NO-EXISTE", "x"); });
    chequear(noExiste.find("Envio no encontrado") != string::npos, "Entregar un codigo inexistente se informa");

    // Extra: la entrega directa desde RECIBIDO por "cambiar estado" tambien se rechaza.
    capturarSalida([&] { cd.registrarEnvio("F2", "D", "CENTRO", 1.0, NivelServicio::ESTANDAR); });
    string directa = capturarSalida([&] { cd.cambiarEstado("F2", Estado::ENTREGADO, "Entrega directa"); });
    string pend3 = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(directa.find("Transicion invalida") != string::npos && pend3.find("F2 |") != string::npos,
             "Marcar ENTREGADO un envio que nunca salio a reparto se rechaza y sigue pendiente");
}

// ============================================================
// RF10 — Indice de envios mediante BST
// ============================================================
// Verifica la propiedad del BST en todo el arbol: cada codigo es mayor que
// todos los de su subarbol izquierdo y menor que todos los del derecho.
static bool cumpleOrden(const NodoBST* nodo, const string* minimo, const string* maximo) {
    if (nodo == nullptr) return true;
    const string& c = nodo->envio->getCodigo();
    if (minimo != nullptr && !(c > *minimo)) return false;
    if (maximo != nullptr && !(c < *maximo)) return false;
    return cumpleOrden(nodo->izq, minimo, &c) && cumpleOrden(nodo->der, &c, maximo);
}

void testRF10_1_2_NodoEInsercion() {
    cout << "\nRF10.1/10.2 - Nodo e insercion\n";
    Envio e1("PKG-1001", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e2("PKG-1002", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e4("PKG-1004", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e5("PKG-1005", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e7("PKG-1007", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e8("PKG-1008", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);

    IndiceBST bst;
    chequear(bst.estaVacio() && bst.getCantidad() == 0 && bst.getRaiz() == nullptr,
             "Un BST nuevo esta vacio");
    chequear(!bst.insertar(nullptr) && bst.getCantidad() == 0, "insertar(nullptr) se rechaza");

    chequear(bst.insertar(&e4) && bst.getRaiz()->envio == &e4 &&
                 bst.getRaiz()->izq == nullptr && bst.getRaiz()->der == nullptr,
             "Insercion en arbol vacio: el primer envio es la raiz");

    bst.insertar(&e2); bst.insertar(&e7); bst.insertar(&e1); bst.insertar(&e5); bst.insertar(&e8);
    const NodoBST* r = bst.getRaiz();
    chequear(bst.getCantidad() == 6 && r->envio == &e4, "6 envios indexados, raiz PKG-1004");
    chequear(r->izq->envio == &e2 && r->der->envio == &e7 &&
                 r->izq->izq->envio == &e1 && r->izq->der == nullptr &&
                 r->der->izq->envio == &e5 && r->der->der->envio == &e8,
             "El arbol tiene la forma del ejemplo del enunciado");
    chequear(r->izq->izq->izq == nullptr && r->izq->izq->der == nullptr &&
                 r->der->izq->izq == nullptr && r->der->izq->der == nullptr &&
                 r->der->der->izq == nullptr && r->der->der->der == nullptr,
             "Las hojas tienen los dos hijos en nullptr");
    chequear(cumpleOrden(r, nullptr, nullptr),
             "Menores en el subarbol izquierdo y mayores en el derecho, en todo el arbol");

    Envio repetido("PKG-1005", "OTRO", "SUR", 9.0, NivelServicio::EXPRESS);
    chequear(!bst.insertar(&repetido) && bst.getCantidad() == 6 && bst.getRaiz()->der->izq->envio == &e5,
             "Un codigo duplicado no se inserta: el arbol no cambia");
    chequear(!bst.insertar(&e4) && bst.getCantidad() == 6, "Insertar dos veces el mismo envio tampoco duplica");
}

void testRF10_3_Busqueda() {
    cout << "\nRF10.3 - Busqueda por codigo\n";
    CentroDeDistribucion cd;
    string vacio = capturarSalida([&] { cd.buscarEnBST("PKG-1005"); });
    chequear(vacio.find("Envio no encontrado en el BST.") != string::npos &&
                 vacio.find("Nodos visitados: 0") != string::npos,
             "En un BST vacio: no encontrado y 0 nodos visitados");

    capturarSalida([&] {
        cd.registrarEnvio("PKG-1004", "D", "NORTE", 2.0, NivelServicio::ESTANDAR);
        cd.registrarEnvio("PKG-1002", "D", "NORTE", 2.0, NivelServicio::ESTANDAR);
        cd.registrarEnvio("PKG-1007", "D", "NORTE", 2.0, NivelServicio::ESTANDAR);
        cd.registrarEnvio("PKG-1001", "D", "NORTE", 2.0, NivelServicio::ESTANDAR);
        cd.registrarEnvio("PKG-1005", "D", "NORTE", 2.0, NivelServicio::ESTANDAR);
        cd.registrarEnvio("PKG-1008", "D", "NORTE", 2.0, NivelServicio::ESTANDAR);
    });

    string s = capturarSalida([&] { cd.buscarEnBST("PKG-1005"); });
    size_t a = s.find("PKG-1004 -> derecha");
    size_t b = s.find("PKG-1007 -> izquierda");
    size_t c = s.find("PKG-1005 -> encontrado");
    chequear(a != string::npos && b != string::npos && c != string::npos && a < b && b < c,
             "Muestra el camino: PKG-1004 -> derecha, PKG-1007 -> izquierda, PKG-1005 -> encontrado");
    chequear(s.find("Nodos visitados: 3") != string::npos, "Cuenta 3 nodos visitados");
    chequear(s.find("PKG-1005 | NORTE | 2.00 kg | ESTANDAR | RECIBIDO") != string::npos,
             "Al encontrarlo muestra los datos del envio");

    string raiz = capturarSalida([&] { cd.buscarEnBST("PKG-1004"); });
    chequear(raiz.find("Nodos visitados: 1") != string::npos, "Mejor caso: el codigo esta en la raiz (1 nodo)");

    string no1 = capturarSalida([&] { cd.buscarEnBST("PKG-1006"); });
    chequear(no1.find("Envio no encontrado en el BST.") != string::npos &&
                 no1.find("Nodos visitados: 3") != string::npos,
             "Codigo inexistente (hueco entre hojas): mensaje de la consigna y 3 nodos visitados");
    string no2 = capturarSalida([&] { cd.buscarEnBST("PKG-0001"); });
    chequear(no2.find("Envio no encontrado en el BST.") != string::npos &&
                 no2.find("Nodos visitados: 3") != string::npos,
             "Codigo menor que todos: recorre el borde izquierdo y no lo encuentra");
    string no3 = capturarSalida([&] { cd.buscarEnBST("PKG-9999"); });
    chequear(no3.find("Nodos visitados: 3") != string::npos, "Codigo mayor que todos: recorre el borde derecho");
}

void testRF10_4_InOrder() {
    cout << "\nRF10.4 - Recorrido in-order\n";
    IndiceBST vacio;
    string sVacio = capturarSalida([&] { vacio.mostrarInOrder(); });
    chequear(sVacio.find("vacio") != string::npos, "In-order de un BST vacio se informa sin romper");

    Envio e1("PKG-1001", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e2("PKG-1002", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e4("PKG-1004", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e5("PKG-1005", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e7("PKG-1007", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e8("PKG-1008", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    IndiceBST bst;
    bst.insertar(&e4); bst.insertar(&e7); bst.insertar(&e8); bst.insertar(&e1); bst.insertar(&e5); bst.insertar(&e2);
    string codigos[10];
    int n = extraerCodigos(capturarSalida([&] { bst.mostrarInOrder(); }), codigos, 10);
    chequear(n == 6 && codigos[0] == "PKG-1001" && codigos[1] == "PKG-1002" && codigos[2] == "PKG-1004" &&
                 codigos[3] == "PKG-1005" && codigos[4] == "PKG-1007" && codigos[5] == "PKG-1008",
             "In-order sobre codigos insertados desordenados: 1001, 1002, 1004, 1005, 1007, 1008");

    // Lote grande: 50 codigos distintos insertados en orden pseudoaleatorio.
    const int N = 50;
    Envio** arr = new Envio*[N];
    bool ascendente = true;
    int impresos = 0;
    {
        IndiceBST grande;
        for (int i = 0; i < N; i++) {
            arr[i] = new Envio(codigoK((i * 37 + 11) % 101), "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
            grande.insertar(arr[i]);
        }
        string* codigosGrande = new string[N];
        impresos = extraerCodigos(capturarSalida([&] { grande.mostrarInOrder(); }), codigosGrande, N);
        for (int i = 0; i + 1 < impresos; i++) {
            if (!(codigosGrande[i] < codigosGrande[i + 1])) ascendente = false;
        }
        delete[] codigosGrande;
    }
    for (int i = 0; i < N; i++) delete arr[i];
    delete[] arr;
    chequear(impresos == N && ascendente, "50 envios insertados desordenados salen en orden ascendente estricto");

    CentroDeDistribucion cd;
    capturarSalida([&] {
        cd.registrarEnvio("B", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
        cd.registrarEnvio("C", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
        cd.registrarEnvio("A", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    });
    string conTitulo = capturarSalida([&] { cd.recorrerBSTInOrder(); });
    chequear(conTitulo.find("A |") < conTitulo.find("B |") && conTitulo.find("B |") < conTitulo.find("C |"),
             "Desde el centro: los envios registrados B, C, A salen A, B, C");
}

void testRF10_5_Altura() {
    cout << "\nRF10.5 - Altura del arbol\n";
    IndiceBST bst;
    chequear(bst.altura() == 0, "Convencion: arbol vacio -> altura 0");

    Envio a("K001", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    bst.insertar(&a);
    chequear(bst.altura() == 1, "Convencion: arbol con un unico nodo -> altura 1");

    Envio b("K002", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio c("K003", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio d("K004", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    bst.insertar(&b);
    bst.insertar(&c);
    bst.insertar(&d);
    chequear(bst.altura() == 4, "K001..K004 en orden: arbol degenerado de 4 niveles -> altura 4");

    Envio e1("PKG-1001", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e2("PKG-1002", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e4("PKG-1004", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e5("PKG-1005", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e7("PKG-1007", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    Envio e8("PKG-1008", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
    IndiceBST ejemplo;
    ejemplo.insertar(&e4); ejemplo.insertar(&e2); ejemplo.insertar(&e7);
    ejemplo.insertar(&e1); ejemplo.insertar(&e5); ejemplo.insertar(&e8);
    chequear(ejemplo.altura() == 3, "El ejemplo del enunciado (6 nodos, 3 niveles) -> altura 3");

    chequear(IndiceBST::alturaMinimaPosible(0) == 0 && IndiceBST::alturaMinimaPosible(1) == 1 &&
                 IndiceBST::alturaMinimaPosible(3) == 2 && IndiceBST::alturaMinimaPosible(4) == 3 &&
                 IndiceBST::alturaMinimaPosible(7) == 3 && IndiceBST::alturaMinimaPosible(8) == 4 &&
                 IndiceBST::alturaMinimaPosible(15) == 4,
             "Altura minima posible: 0, 1, 2, 3, 3, 4 y 4 para 0, 1, 3, 4, 7, 8 y 15 nodos");

    string s = capturarSalida([&] { ejemplo.mostrarAltura(); });
    chequear(s.find("Nodos: 6") != string::npos && s.find("): 3") != string::npos,
             "mostrarAltura informa nodos y altura con la convencion elegida");
    string sVacio = capturarSalida([&] { bst.mostrarAltura(); });
    chequear(sVacio.find("Nodos: 4") != string::npos, "mostrarAltura sobre el arbol degenerado informa 4 nodos");
}

void testRF10_7_Degeneracion() {
    cout << "\nRF10.7 - Un BST comun no garantiza O(log n)\n";
    const int N = 15;
    Envio** ordenados = new Envio*[N];
    Envio** balanceados = new Envio*[N];
    const int medianaPrimero[N] = {8, 4, 12, 2, 6, 10, 14, 1, 3, 5, 7, 9, 11, 13, 15};
    bool alturaDegenerada = false, altoBalanceado = false, busquedaLenta = false, busquedaRapida = false;
    {
        IndiceBST degenerado;
        IndiceBST balanceado;
        for (int i = 0; i < N; i++) {
            ordenados[i] = new Envio(codigoK(i + 1), "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
            balanceados[i] = new Envio(codigoK(medianaPrimero[i]), "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
            degenerado.insertar(ordenados[i]);
            balanceado.insertar(balanceados[i]);
        }
        int visitadosDeg = 0, visitadosBal = 0;
        degenerado.buscar(codigoK(N), visitadosDeg);
        balanceado.buscar(codigoK(N), visitadosBal);
        alturaDegenerada = (degenerado.altura() == N);
        altoBalanceado = (balanceado.altura() == IndiceBST::alturaMinimaPosible(N));
        busquedaLenta = (visitadosDeg == N);
        busquedaRapida = (visitadosBal == IndiceBST::alturaMinimaPosible(N));
    }
    for (int i = 0; i < N; i++) { delete ordenados[i]; delete balanceados[i]; }
    delete[] ordenados;
    delete[] balanceados;
    chequear(alturaDegenerada, "15 codigos insertados ya ordenados: altura 15 (una lista enlazada)");
    chequear(busquedaLenta, "Peor caso O(n): buscar el ultimo en el arbol degenerado visita los 15 nodos");
    chequear(altoBalanceado, "Los mismos 15 codigos en otro orden (mediana primero): altura 4 = log2(15) + 1");
    chequear(busquedaRapida, "Caso balanceado O(log n): el mismo codigo se encuentra visitando 4 nodos");
}

void testRF10_6_Ownership() {
    cout << "\nRF10.6 - Ownership del BST\n";
    // El BST se destruye antes que los envios y NO los destruye: si hiciera
    // delete sobre un Envio, el acceso posterior y el delete de abajo serian un
    // use-after-free / double free que ASAN detecta.
    Envio* a = new Envio("PKG-1002", "D", "NORTE", 0.75, NivelServicio::EXPRESS);
    Envio* b = new Envio("PKG-1005", "D", "NORTE", 1.90, NivelServicio::PRIORITARIO);
    Envio* c = new Envio("PKG-1008", "D", "NORTE", 3.40, NivelServicio::PRIORITARIO);
    {
        IndiceBST bst;
        bst.insertar(b); bst.insertar(a); bst.insertar(c);
        int visitados = 0;
        chequear(bst.buscar("PKG-1002", visitados) == a, "El BST devuelve el MISMO objeto Envio (referencia, no copia)");
    }   // ~IndiceBST libera sus 3 nodos y nada mas
    chequear(a->getCodigo() == "PKG-1002" && b->getCodigo() == "PKG-1005" && c->getCodigo() == "PKG-1008",
             "Destruir el arbol completo no destruye los envios");
    delete a; delete b; delete c;

    // Con el sistema completo: el indice referencia los envios del registro.
    {
        CentroDeDistribucion cd;
        capturarSalida([&] {
            cd.registrarEnvio("N1", "D", "NORTE", 1.0, NivelServicio::EXPRESS);
            cd.registrarEnvio("N2", "D", "NORTE", 2.0, NivelServicio::ESTANDAR);
        });
        capturarSalida([&] { cd.despacharProximo(); });
        string s = capturarSalida([&] { cd.buscarEnBST("N1"); });
        chequear(s.find("EN_REPARTO") != string::npos,
                 "El BST ve el cambio de estado sin actualizarse: apunta al mismo Envio del registro");
        // Al salir se destruyen mesa e indice (solo nodos) y despues el registro (los Envio): sin double free ni fugas.
    }
    chequear(true, "El centro se destruye sin double free ni fugas (verificado con ASAN)");
}

void testRF10_2_IntegracionConElSistema() {
    cout << "\nRF10.2 - Integracion con el alta de envios\n";
    CentroDeDistribucion cd;
    chequear(cd.indiceBST().estaVacio(), "El centro arranca con el indice vacio");

    capturarSalida([&] {
        cd.registrarEnvio("A2", "D", "NORTE", 1.0, NivelServicio::EXPRESS);
        cd.registrarEnvio("A1", "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
        cd.registrarEnvio("A3", "D", "SUR", 1.0, NivelServicio::ESTANDAR);
    });
    chequear(cd.indiceBST().getCantidad() == 3, "Cada alta agrega su envio al BST");

    string dup = capturarSalida([&] { cd.registrarEnvio("A1", "OTRO", "SUR", 5.0, NivelServicio::EXPRESS); });
    chequear(dup.find("ya existe un envio") != string::npos && cd.indiceBST().getCantidad() == 3,
             "Un alta rechazada por codigo repetido no entra al BST");

    capturarSalida([&] { cd.despacharProximo(); });                       // A2 (EXPRESS)
    capturarSalida([&] { cd.finalizarEntrega("A2", "Entregado"); });
    string s = capturarSalida([&] { cd.buscarEnBST("A2"); });
    chequear(cd.indiceBST().getCantidad() == 3 && s.find("ENTREGADO") != string::npos,
             "El BST indexa todos los envios conocidos: uno entregado sigue en el indice");

    string pend = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(pend.find("A1 |") != string::npos && pend.find("A3 |") != string::npos && pend.find("A2 |") == string::npos,
             "El BST no reemplaza ni altera la lista de pendientes");
    string lineal = capturarSalida([&] { cd.buscarEnvio("A1"); });
    chequear(lineal.find("A1 | NORTE") != string::npos, "La busqueda lineal del registro (RF03) sigue funcionando");
}

void testRF10_ComparacionDeBusquedas() {
    cout << "\nRF10 - Comparacion BST vs busqueda lineal\n";
    CentroDeDistribucion cd;
    const int N = 15;
    const int medianaPrimero[N] = {8, 4, 12, 2, 6, 10, 14, 1, 3, 5, 7, 9, 11, 13, 15};
    capturarSalida([&] {
        for (int i = 0; i < N; i++) {
            cd.registrarEnvio(codigoK(medianaPrimero[i]), "D", "NORTE", 1.0, NivelServicio::ESTANDAR);
        }
    });

    string s = capturarSalida([&] { cd.compararBSTconLineal(codigoK(15)); });
    size_t posBst = s.find("Busqueda en el BST");
    size_t posLineal = s.find("Busqueda lineal en el registro");
    chequear(posBst != string::npos && posLineal != string::npos && posBst < posLineal,
             "Informa ambas busquedas para el mismo codigo y el mismo conjunto de datos");
    chequear(s.find("Nodos visitados: 4") != string::npos && s.find("Comparaciones: 15") != string::npos,
             "K015: el BST visita 4 nodos y la lista lineal compara 15 veces");
    chequear(s.find("O(log n)") != string::npos && s.find("O(n)") != string::npos && s.find("O(1)") != string::npos,
             "Se justifican las complejidades O(1), O(log n) y O(n)");
    chequear(s.find("Lote de la mesa") == string::npos, "Sin lote creado no aparece la seccion del lote");

    string noEsta = capturarSalida([&] { cd.compararBSTconLineal("K999"); });
    chequear(noEsta.find("No encontrado") != string::npos && noEsta.find("Comparaciones: 15") != string::npos,
             "Un codigo inexistente: la lineal compara los 15 y el BST informa que no esta");

    capturarSalida([&] { cd.crearLoteZona("NORTE"); });
    capturarSalida([&] { cd.mesa().ordenar(CriterioOrden::PESO_DESC); });
    string conLoteSinCodigo = capturarSalida([&] { cd.compararBSTconLineal(codigoK(15)); });
    chequear(conLoteSinCodigo.find("Lote de la mesa (zona NORTE, 15 envios") != string::npos &&
                 conLoteSinCodigo.find("Busqueda binaria: no se ejecuta") != string::npos,
             "Con el lote ordenado por peso se muestra la lineal del lote y la binaria NO se ejecuta");

    capturarSalida([&] { cd.mesa().ordenar(CriterioOrden::CODIGO_ASC); });
    string conLote = capturarSalida([&] { cd.compararBSTconLineal(codigoK(15)); });
    chequear(conLote.find("Busqueda lineal: encontrado, comparaciones: 15") != string::npos &&
                 conLote.find("Busqueda binaria: encontrado, comparaciones: 4") != string::npos,
             "Con el lote ordenado por codigo: lineal 15 comparaciones y binaria 4");
}

// ============================================================
// Ampliacion: consultas avanzadas (RF11 a RF14)
// Casos de prueba obligatorios 1 a 6 de la consigna de la ampliacion.
// ============================================================

// Codigo PKG-<1000 + n>, por ejemplo n = 5 -> "PKG-1005".
static string codigoPKG(int n) {
    return "PKG-" + to_string(1000 + n);
}

// Revisa que las hojas, recorridas con los enlaces, tengan todas las claves
// en orden estrictamente ascendente y que en total sean `esperadas`.
static bool hojasOrdenadas(const ArbolBMas& arbol, int esperadas) {
    int total = 0;
    string anterior = "";
    for (const NodoBMas* hoja = arbol.primeraHoja(); hoja != nullptr; hoja = hoja->siguiente) {
        for (int i = 0; i < hoja->cantClaves; i++) {
            if (total > 0 && !(anterior < hoja->claves[i])) return false;
            anterior = hoja->claves[i];
            total++;
        }
    }
    return total == esperadas;
}

// Caso 1 — Arbol B+: divisiones de hojas, de internos y nueva raiz.
void testAmpliacion_Caso1_ArbolBMas() {
    cout << "\nAmpliacion Caso 1 - Arbol B+\n";
    const int N = 20;
    Envio* envios[N];
    for (int i = 0; i < N; i++) {
        envios[i] = new Envio(codigoPKG(i + 1), "Dest", "CENTRO", 1.0, NivelServicio::ESTANDAR);
    }

    {
        ArbolBMas arbol;
        chequear(arbol.insertar(envios[0]) && arbol.altura() == 1,
                 "Insercion en arbol vacio: se crea una hoja raiz (altura 1)");
        arbol.insertar(envios[1]);
        arbol.insertar(envios[2]);
        chequear(arbol.getDivisionesHoja() == 0 && arbol.getCantidad() == 3,
                 "Con 3 envios la hoja todavia tiene lugar (no se divide)");

        arbol.insertar(envios[3]);
        chequear(arbol.getDivisionesHoja() == 1 && arbol.getRaicesCreadas() == 1 && arbol.altura() == 2,
                 "El 4to envio divide la hoja y se crea una raiz nueva (altura 2)");
        const NodoBMas* raiz = arbol.getRaiz();
        chequear(!raiz->esHoja && raiz->cantClaves == 1 && raiz->claves[0] == codigoPKG(3),
                 "La clave de separacion de la raiz es PKG-1003 (primera de la hoja derecha)");

        // insertamos el resto desordenado para no depender del orden
        int orden[] = {12, 5, 19, 8, 15, 4, 17, 10, 6, 13, 18, 7, 14, 9, 16, 11};
        for (int k = 0; k < 16; k++) arbol.insertar(envios[orden[k]]);

        chequear(arbol.getCantidad() == N, "Se indexaron los 20 envios");
        chequear(arbol.getDivisionesInternas() > 0, "Hubo divisiones de nodos internos");
        chequear(arbol.getRaicesCreadas() >= 2 && arbol.altura() >= 3,
                 "Se volvio a crear una raiz nueva y el arbol crecio a 3 niveles o mas");
        chequear(arbol.hojasAlMismoNivel(), "Todas las hojas estan al mismo nivel");
        chequear(hojasOrdenadas(arbol, N), "Recorriendo las hojas enlazadas los codigos salen ordenados");

        bool todosEncontrados = true;
        for (int i = 0; i < N; i++) {
            int visitados = 0;
            if (arbol.buscar(codigoPKG(i + 1), visitados) != envios[i]) todosEncontrados = false;
            if (visitados != arbol.altura()) todosEncontrados = false;
        }
        chequear(todosEncontrados,
                 "Cada busqueda devuelve el mismo Envio y visita tantos nodos como niveles tiene el arbol");
    }   // aca se destruye el arbol: libera solo sus nodos

    chequear(envios[0]->getCodigo() == codigoPKG(1),
             "Al destruir el arbol los Envio siguen vivos (referencias no propietarias)");
    for (int i = 0; i < N; i++) delete envios[i];
}

// Caso 2 — Busqueda por rango.
void testAmpliacion_Caso2_BusquedaPorRango() {
    cout << "\nAmpliacion Caso 2 - Busqueda por rango\n";
    CentroDeDistribucion cd;
    capturarSalida([&] {
        for (int i = 1; i <= 15; i++) {
            cd.registrarEnvio(codigoPKG(i), "Dest", "CENTRO", 1.0, NivelServicio::ESTANDAR);
        }
    });

    string salida = capturarSalida([&] { cd.buscarRangoBMas("PKG-1003", "PKG-1006"); });
    // buscamos cada codigo en su propia linea (el titulo tambien nombra el rango)
    size_t p3 = salida.find("\nPKG-1003\n"), p4 = salida.find("\nPKG-1004\n");
    size_t p5 = salida.find("\nPKG-1005\n"), p6 = salida.find("\nPKG-1006\n");
    chequear(p3 != string::npos && p3 < p4 && p4 < p5 && p5 < p6 && p6 != string::npos,
             "Rango PKG-1003..PKG-1006: aparecen los 4 codigos en orden ascendente");
    chequear(salida.find("PKG-1002") == string::npos && salida.find("PKG-1007") == string::npos,
             "No aparecen codigos fuera del rango");

    const ArbolBMas& arbol = cd.indiceBMasConsulta();
    Envio* resultado[15];
    int visitados = 0;
    int n = arbol.buscarRango("PKG-1005", "PKG-1012", resultado, 15, visitados);
    bool exactos = (n == 8);
    for (int i = 0; i < n && exactos; i++) {
        if (resultado[i]->getCodigo() != codigoPKG(5 + i)) exactos = false;
    }
    chequear(exactos, "Rango que cruza varias hojas: PKG-1005..PKG-1012 devuelve exactamente 8 en orden");
    chequear(visitados < arbol.altura() + 6,
             "El rango baja una sola vez y sigue por las hojas (no recorre todo el arbol)");
}

// Caso 3 — BitSet.
void testAmpliacion_Caso3_BitSet() {
    cout << "\nAmpliacion Caso 3 - BitSet\n";
    CentroDeDistribucion cd;
    capturarSalida([&] {
        cd.registrarEnvio("PKG-2001", "Dest", "NORTE", 1.0, NivelServicio::EXPRESS);
    });
    const Envio* e = cd.obtenerEnvio("PKG-2001");
    chequear(e->getEstadosVisitados().toString() == "00001", "Recien registrado: solo RECIBIDO (00001)");

    capturarSalida([&] {
        cd.cambiarEstado("PKG-2001", Estado::CLASIFICADO, "Clasificado");
        cd.cambiarEstado("PKG-2001", Estado::EN_REPARTO, "Sale a reparto");
    });
    chequear(e->getEstadosVisitados().toString() == "00111",
             "RECIBIDO, CLASIFICADO y EN_REPARTO -> 00111 (ejemplo de la consigna)");

    capturarSalida([&] {
        cd.reprogramarEnvio("PKG-2001", "Ausente");
        cd.cambiarEstado("PKG-2001", Estado::EN_REPARTO, "Segundo intento");
    });
    const BitSet& b = e->getEstadosVisitados();
    chequear(b.test(3) && !b.test(4) && b.count() == 4,
             "Despues de reprogramar: bit 3 en 1, bit 4 en 0 y count() = 4");
    chequear(e->estuvoEn(Estado::REPROGRAMADO) && e->getEstado() == Estado::EN_REPARTO,
             "Guarda la historia: estuvo REPROGRAMADO aunque ahora este EN_REPARTO");

    string salida = capturarSalida([&] { cd.mostrarBitSet("PKG-2001"); });
    chequear(salida.find("Cantidad de estados distintos: 4") != string::npos &&
                 salida.find("Estuvo REPROGRAMADO: si") != string::npos,
             "La consulta muestra la cantidad de estados y si estuvo REPROGRAMADO");

    BitSet x, y;
    x.set(0); x.set(1); x.set(2);   // 00111
    y.set(0); y.set(2); y.set(3);   // 01101
    chequear(x.any() && x.count() == 3 && x.test(1) && !x.test(3), "set/test/count/any sobre 00111");
    chequear((x & y).toString() == "00101", "AND: 00111 & 01101 = 00101");
    chequear((x | y).toString() == "01111", "OR:  00111 | 01101 = 01111");
    chequear((x ^ y).toString() == "01010", "XOR: 00111 ^ 01101 = 01010");
    x.reset(1);
    chequear(x.toString() == "00101" && x.count() == 2, "reset(1) apaga solo ese bit");
}

// Caso 4 — Set.
void testAmpliacion_Caso4_Set() {
    cout << "\nAmpliacion Caso 4 - Set\n";
    SetCodigos a, b;
    a.insert("PKG-1008"); a.insert("PKG-1002"); a.insert("PKG-1005");
    b.insert("PKG-1006"); b.insert("PKG-1002");

    chequear(a.toString() == "{PKG-1002, PKG-1005, PKG-1008}" && a.size() == 3,
             "A = {PKG-1002, PKG-1005, PKG-1008} (queda ordenado)");
    chequear(a.unionCon(b).toString() == "{PKG-1002, PKG-1005, PKG-1006, PKG-1008}",
             "A U B = {PKG-1002, PKG-1005, PKG-1006, PKG-1008}");
    chequear(a.intersection(b).toString() == "{PKG-1002}", "A n B = {PKG-1002}");
    chequear(a.difference(b).toString() == "{PKG-1005, PKG-1008}", "A - B = {PKG-1005, PKG-1008}");
    chequear(a.unionCon(b).size() == 4, "La union no repite el elemento compartido");

    chequear(!a.insert("PKG-1005") && a.size() == 3, "No se admiten duplicados (insert devuelve false)");
    chequear(a.contains("PKG-1005") && !a.contains("PKG-1006"), "contains funciona");
    chequear(a.remove("PKG-1005") && a.size() == 2 && !a.contains("PKG-1005"), "remove elimina el elemento");

    // los mismos conjuntos, pero generados desde el sistema (RF13.2)
    CentroDeDistribucion cd;
    capturarSalida([&] {
        cd.registrarEnvio("PKG-1001", "Ana Torres",    "CENTRO", 1.20, NivelServicio::ESTANDAR);
        cd.registrarEnvio("PKG-1002", "Bruno Diaz",    "NORTE",  0.75, NivelServicio::EXPRESS);
        cd.registrarEnvio("PKG-1003", "Carla Ruiz",    "SUR",    4.10, NivelServicio::PRIORITARIO);
        cd.registrarEnvio("PKG-1004", "Diego Lopez",   "CENTRO", 2.30, NivelServicio::ESTANDAR);
        cd.registrarEnvio("PKG-1005", "Elena Castro",  "NORTE",  1.90, NivelServicio::PRIORITARIO);
        cd.registrarEnvio("PKG-1006", "Franco Gomez",  "SUR",    0.50, NivelServicio::EXPRESS);
        cd.registrarEnvio("PKG-1007", "Gabriela Soto", "CENTRO", 6.20, NivelServicio::ESTANDAR);
        cd.registrarEnvio("PKG-1008", "Hugo Perez",    "NORTE",  3.40, NivelServicio::PRIORITARIO);
    });
    SetCodigos norte = cd.conjuntoPorZona("NORTE");
    SetCodigos express = cd.conjuntoPorServicio(NivelServicio::EXPRESS);
    chequear(norte.toString() == "{PKG-1002, PKG-1005, PKG-1008}", "Conjunto por zona NORTE");
    chequear(express.toString() == "{PKG-1002, PKG-1006}", "Conjunto por servicio EXPRESS");
    chequear(norte.intersection(express).toString() == "{PKG-1002}", "NORTE n EXPRESS = {PKG-1002}");
}

// Caso 5 — Casos limite.
void testAmpliacion_Caso5_CasosLimite() {
    cout << "\nAmpliacion Caso 5 - Casos limite\n";
    {
        ArbolBMas vacio;
        int visitados = -1;
        Envio* rango[1];
        chequear(vacio.estaVacio() && vacio.altura() == 0 && vacio.buscar("PKG-1001", visitados) == nullptr &&
                     visitados == 0 && vacio.buscarRango("A", "Z", rango, 1, visitados) == 0,
                 "Arbol B+ vacio: altura 0, la busqueda y el rango no encuentran nada");
    }

    CentroDeDistribucion cd;
    capturarSalida([&] {
        for (int i = 1; i <= 6; i++) cd.registrarEnvio(codigoPKG(i), "Dest", "SUR", 1.0, NivelServicio::ESTANDAR);
    });
    string noExiste = capturarSalida([&] { cd.buscarEnBMas("PKG-9999"); });
    chequear(noExiste.find("no encontrado") != string::npos, "Busqueda de un codigo inexistente");

    string dup = capturarSalida([&] {
        cd.registrarEnvio("PKG-1003", "Otro", "NORTE", 2.0, NivelServicio::EXPRESS);
    });
    chequear(dup.find("ya existe") != string::npos && cd.indiceBMasConsulta().getCantidad() == 6,
             "Insercion duplicada en el sistema: se rechaza y el B+ sigue con 6");
    Envio otro("PKG-1003", "Otro", "NORTE", 2.0, NivelServicio::EXPRESS);
    {
        ArbolBMas arbol;
        arbol.insertar(&otro);
        chequear(!arbol.insertar(&otro) && arbol.getCantidad() == 1,
                 "Insercion duplicada directa en el B+: insertar devuelve false");
    }

    string sinResultados = capturarSalida([&] { cd.buscarRangoBMas("PKG-5000", "PKG-6000"); });
    chequear(sinResultados.find("No hay envios en el rango") != string::npos, "Busqueda por rango sin resultados");

    BitSet vacioBits;
    chequear(!vacioBits.any() && vacioBits.count() == 0 && vacioBits.toString() == "00000",
             "BitSet sin bits activos: any() false y count() 0");
    chequear(!vacioBits.set(5) && !vacioBits.set(-1) && !vacioBits.reset(9) && !vacioBits.test(7) &&
                 vacioBits.count() == 0,
             "Posicion de bit invalida: se rechaza sin modificar el BitSet");

    SetCodigos vacio, c;
    c.insert("PKG-1001"); c.insert("PKG-1002");
    chequear(vacio.empty() && vacio.size() == 0 && !vacio.contains("PKG-1001") && vacio.toString() == "{}",
             "Set vacio");
    chequear(c.unionCon(vacio).toString() == c.toString() && vacio.unionCon(c).size() == 2,
             "Union con un conjunto vacio da el mismo conjunto");
    SetCodigos d;
    d.insert("PKG-1003"); d.insert("PKG-1004");
    chequear(c.intersection(d).empty(), "Interseccion de conjuntos disjuntos es vacia");
    chequear(!c.remove("PKG-9999") && c.size() == 2, "Eliminar un elemento inexistente devuelve false");

    SetCodigos copia = c;   // copia profunda: se pueden modificar por separado
    copia.remove("PKG-1001");
    chequear(c.size() == 2 && copia.size() == 1, "Copiar un Set no comparte nodos con el original");
}

// Caso 6 — Integracion.
void testAmpliacion_Caso6_Integracion() {
    cout << "\nAmpliacion Caso 6 - Integracion\n";
    CentroDeDistribucion cd;
    capturarSalida([&] {
        cd.registrarEnvio("PKG-1002", "Bruno Diaz",   "NORTE", 0.75, NivelServicio::EXPRESS);
        cd.registrarEnvio("PKG-1005", "Elena Castro", "NORTE", 1.90, NivelServicio::PRIORITARIO);
        cd.registrarEnvio("PKG-1006", "Franco Gomez", "SUR",   0.50, NivelServicio::EXPRESS);
    });

    string alta = capturarSalida([&] {
        cd.registrarEnvio("PKG-1009", "Ivana Mendez", "NORTE", 2.10, NivelServicio::EXPRESS);
    });
    chequear(alta.find("registrado") != string::npos, "Se registra un envio nuevo");

    string busqueda = capturarSalida([&] { cd.buscarEnBMas("PKG-1009"); });
    chequear(busqueda.find("Codigo: PKG-1009") != string::npos &&
                 busqueda.find("Destinatario: Ivana Mendez") != string::npos &&
                 busqueda.find("Zona: NORTE") != string::npos &&
                 busqueda.find("Servicio: EXPRESS") != string::npos &&
                 busqueda.find("Nodos visitados:") != string::npos,
             "Se lo encuentra con el arbol B+ y se muestran sus datos y los nodos visitados");

    capturarSalida([&] { cd.despacharProximo(); });   // el primero de pendientes es PKG-1002 (EXPRESS)
    capturarSalida([&] { cd.cambiarEstado("PKG-1009", Estado::EN_REPARTO, "Sale"); });
    const Envio* e = cd.obtenerEnvio("PKG-1009");
    chequear(e != nullptr && e->getEstadosVisitados().toString() == "00101",
             "Su BitSet se actualiza solo al cambiar de estado (RECIBIDO + EN_REPARTO)");

    string comparacion = capturarSalida([&] { cd.compararBitSets("PKG-1009", "PKG-1005"); });
    chequear(comparacion.find("En comun (AND):        00001") != string::npos &&
                 comparacion.find("Solo en PKG-1009: EN_REPARTO") != string::npos,
             "Comparar BitSets: estados en comun y estados de uno solo");

    SetCodigos norte = cd.conjuntoPorZona("NORTE");
    SetCodigos express = cd.conjuntoPorServicio(NivelServicio::EXPRESS);
    SetCodigos ambos = norte.intersection(express);
    chequear(ambos.contains("PKG-1009") && ambos.toString() == "{PKG-1002, PKG-1009}",
             "El envio nuevo aparece en NORTE n EXPRESS");

    string detalle = capturarSalida([&] { cd.mostrarConjunto("A n B", ambos); });
    chequear(detalle.find("PKG-1009 | NORTE | 2.10 kg | EXPRESS | EN_REPARTO") != string::npos,
             "Con los codigos del Set se recuperan los datos completos usando el B+");
    chequear(cd.obtenerEnvio("PKG-1009") == e,
             "El B+ apunta al mismo objeto Envio (no se duplico)");
}

int main() {
    cout << "========== TESTS HUBFLOW ==========\n";

    testCaso1_Prioridades();
    testCaso2_PrioridadEstable();
    testCaso3_Despacho();
    testCaso4_Reprogramacion();
    testCaso5_Historial();
    testCaso6_Recursividad();
    testCaso7_CasosLimite();

    testRF09_1_LoteDeZona();
    testRF09_2_Ordenamiento();
    testRF09_3_Queue();
    testRF09_4_Stack();
    testRF09_5_FlujoColaYPila();
    testRF09_6_BusquedaLineal();
    testRF09_7_BusquedaBinaria();
    testRF09_8_ComparacionDeBusquedas();
    testRF09_9_Ownership();

    testEstados_MatrizDeTransiciones();
    testRF04_CambiarEstado();
    testRF06_Reprogramar();
    testRF07_FinalizarEntrega();

    testRF10_1_2_NodoEInsercion();
    testRF10_3_Busqueda();
    testRF10_4_InOrder();
    testRF10_5_Altura();
    testRF10_6_Ownership();
    testRF10_7_Degeneracion();
    testRF10_2_IntegracionConElSistema();
    testRF10_ComparacionDeBusquedas();

    testAmpliacion_Caso1_ArbolBMas();
    testAmpliacion_Caso2_BusquedaPorRango();
    testAmpliacion_Caso3_BitSet();
    testAmpliacion_Caso4_Set();
    testAmpliacion_Caso5_CasosLimite();
    testAmpliacion_Caso6_Integracion();

    cout << "\n====================================\n";
    cout << (totalChequeos - chequeosFallidos) << "/" << totalChequeos << " checks OK\n";
    if (chequeosFallidos > 0) {
        cout << chequeosFallidos << " checks FALLIDOS\n";
        return 1;
    }
    cout << "Todos los checks pasaron correctamente.\n";
    return 0;
}
