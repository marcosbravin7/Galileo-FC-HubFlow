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
// Extra — RF07: marcar ENTREGADO sin pasar por despacharProximo no debe
// dejar el envio colgado en pendientes.
// ============================================================
void testExtra_EntregaDirectaSacaDePendientes() {
    cout << "\nExtra - Entrega directa (sin despachar) saca de pendientes\n";
    CentroDeDistribucion cd;
    cd.registrarEnvio("E1", "Dest", "CENTRO", 1.0, NivelServicio::ESTANDAR);

    capturarSalida([&] { cd.cambiarEstado("E1", Estado::ENTREGADO, "Entrega directa"); });

    string salidaPendientes = capturarSalida([&] { cd.mostrarPendientes(); });
    chequear(salidaPendientes.find("E1") == string::npos,
              "Un envio marcado ENTREGADO ya no figura en pendientes");

    string salidaBuscar = capturarSalida([&] { cd.buscarEnvio("E1"); });
    chequear(salidaBuscar.find("ENTREGADO") != string::npos,
              "El envio sigue siendo consultable con su estado ENTREGADO");
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
    testExtra_EntregaDirectaSacaDePendientes();

    cout << "\n====================================\n";
    cout << (totalChequeos - chequeosFallidos) << "/" << totalChequeos << " checks OK\n";
    if (chequeosFallidos > 0) {
        cout << chequeosFallidos << " checks FALLIDOS\n";
        return 1;
    }
    cout << "Todos los checks pasaron correctamente.\n";
    return 0;
}
