#include "CentroDeDistribucion.h"
#include <iostream>
#include <string>
#include <limits>
using namespace std;

void mostrarMenu() {
    cout << "\n========== HUBFLOW ==========\n";
    cout << "1. Mostrar envios pendientes\n";
    cout << "2. Registrar nuevo envio\n";
    cout << "3. Buscar envio\n";
    cout << "4. Cambiar estado\n";
    cout << "5. Despachar proximo envio\n";
    cout << "6. Reprogramar envio\n";
    cout << "7. Mostrar historial\n";
    cout << "8. Resumen recursivo por zona\n";
    cout << "9. Envio mas pesado por zona (desafio)\n";
    cout << "10. Mesa de clasificacion y despacho (RF09)\n";
    cout << "11. Registrar entrega (RF07)\n";
    cout << "12. Indice de envios BST (RF10)\n";
    cout << "0.  Finalizar\n";
    cout << "Opcion: ";
}

void mostrarMenuBST() {
    cout << "\n===== INDICE DE ENVIOS (BST) =====\n";
    cout << "1. Buscar envio por codigo en el BST\n";
    cout << "2. Recorrido in-order (codigo ascendente)\n";
    cout << "3. Altura y cantidad de nodos\n";
    cout << "4. Comparar BST vs busqueda lineal\n";
    cout << "0. Volver\n";
    cout << "Opcion: ";
}

// Submenu del RF10. El BST es de solo lectura desde aca: se actualiza solo
// al registrar envios y no modifica los envios ni las demas estructuras.
void menuBST(CentroDeDistribucion& cd) {
    int op = -1;
    while (op != 0) {
        mostrarMenuBST();
        cin >> op;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Opcion invalida.\n";
            op = -1;   // una lectura fallida deja op en 0: evita salir sin querer
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (op == 1) {
            string cod;
            cout << "Codigo a buscar: "; getline(cin, cod);
            cd.buscarEnBST(cod);

        } else if (op == 2) {
            cd.recorrerBSTInOrder();

        } else if (op == 3) {
            cd.mostrarAlturaBST();

        } else if (op == 4) {
            string cod;
            cout << "Codigo a buscar: "; getline(cin, cod);
            cd.compararBSTconLineal(cod);

        } else if (op != 0) {
            cout << "Opcion invalida.\n";
        }
    }
}

void mostrarMenuMesa() {
    cout << "\n===== MESA DE CLASIFICACION Y DESPACHO =====\n";
    cout << "1.  Crear lote por zona\n";
    cout << "2.  Ordenar lote por codigo (ascendente)\n";
    cout << "3.  Ordenar lote por peso (descendente)\n";
    cout << "4.  Ver lote\n";
    cout << "5.  Cargar lote en la cola de preparacion\n";
    cout << "6.  Procesar siguiente (dequeue -> push)\n";
    cout << "7.  Ver cola de preparacion\n";
    cout << "8.  Ver pila de procesados\n";
    cout << "9.  Ver ultimo procesado (top)\n";
    cout << "10. Busqueda lineal en el lote\n";
    cout << "11. Busqueda binaria en el lote\n";
    cout << "12. Comparar busqueda lineal vs binaria\n";
    cout << "0.  Volver\n";
    cout << "Opcion: ";
}

// Submenu del RF09. Trabaja sobre el lote temporal de la mesa; no modifica
// los envios ni la lista de pendientes.
void menuMesa(CentroDeDistribucion& cd) {
    MesaDeClasificacion& mesa = cd.mesa();
    int op = -1;
    while (op != 0) {
        mostrarMenuMesa();
        cin >> op;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Opcion invalida.\n";
            op = -1;   // una lectura fallida deja op en 0: evita salir sin querer
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (op == 1) {
            string zona;
            cout << "Zona (NORTE/SUR/CENTRO): "; getline(cin, zona);
            if (cd.crearLoteZona(zona)) {
                cout << "Lote creado.\n";
                mesa.mostrarLote();
            }

        } else if (op == 2) {
            if (mesa.ordenar(CriterioOrden::CODIGO_ASC)) {
                cout << "Lote ordenado por codigo (Insertion Sort).\n";
                mesa.mostrarLote();
            }

        } else if (op == 3) {
            if (mesa.ordenar(CriterioOrden::PESO_DESC)) {
                cout << "Lote ordenado por peso descendente (Insertion Sort).\n";
                mesa.mostrarLote();
            }

        } else if (op == 4) {
            mesa.mostrarLote();

        } else if (op == 5) {
            if (mesa.cargarColaPreparacion()) {
                cout << "Lote cargado en la cola de preparacion.\n";
                mesa.mostrarCola();
            }

        } else if (op == 6) {
            Envio* e = mesa.procesarSiguiente();
            if (e != nullptr) {
                cout << "Procesado: " << e->getCodigo() << "\n";
                mesa.mostrarCola();
                mesa.mostrarPila();
            }

        } else if (op == 7) {
            mesa.mostrarCola();

        } else if (op == 8) {
            mesa.mostrarPila();

        } else if (op == 9) {
            mesa.mostrarUltimoProcesado();

        } else if (op == 10) {
            string cod;
            cout << "Codigo a buscar: "; getline(cin, cod);
            mesa.mostrarBusquedaLineal(cod);

        } else if (op == 11) {
            string cod;
            cout << "Codigo a buscar: "; getline(cin, cod);
            mesa.mostrarBusquedaBinaria(cod);

        } else if (op == 12) {
            string cod;
            cout << "Codigo a buscar: "; getline(cin, cod);
            mesa.compararBusquedas(cod);

        } else if (op != 0) {
            cout << "Opcion invalida.\n";
        }
    }
}

void cargarDataset(CentroDeDistribucion& cd) {
    cd.registrarEnvio("PKG-1001", "Ana Torres",    "CENTRO", 1.20, NivelServicio::ESTANDAR);
    cd.registrarEnvio("PKG-1002", "Bruno Diaz",    "NORTE",  0.75, NivelServicio::EXPRESS);
    cd.registrarEnvio("PKG-1003", "Carla Ruiz",    "SUR",    4.10, NivelServicio::PRIORITARIO);
    cd.registrarEnvio("PKG-1004", "Diego Lopez",   "CENTRO", 2.30, NivelServicio::ESTANDAR);
    cd.registrarEnvio("PKG-1005", "Elena Castro",  "NORTE",  1.90, NivelServicio::PRIORITARIO);
    cd.registrarEnvio("PKG-1006", "Franco Gomez",  "SUR",    0.50, NivelServicio::EXPRESS);
    cd.registrarEnvio("PKG-1007", "Gabriela Soto", "CENTRO", 6.20, NivelServicio::ESTANDAR);
    cd.registrarEnvio("PKG-1008", "Hugo Perez",    "NORTE",  3.40, NivelServicio::PRIORITARIO);
}

int main() {
    CentroDeDistribucion cd;

    cout << "Cargando dataset inicial...\n";
    cargarDataset(cd);

    int opcion = -1;
    do {
        mostrarMenu();
        cin >> opcion;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Opcion invalida.\n";
            opcion = -1;   // una lectura fallida deja opcion en 0 (= salir): se evita
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (opcion == 1) {
            cd.mostrarPendientes();

        } else if (opcion == 2) {
            string cod, dest, zona;
            double peso;
            int nivelOpcion;
            NivelServicio nivel;
            cout << "Codigo: "; getline(cin, cod);
            cout << "Destinatario: "; getline(cin, dest);
            cout << "Zona: "; getline(cin, zona);
            cout << "Peso (kg): "; cin >> peso;
            cout << "Nivel (1=EXPRESS, 2=PRIORITARIO, 3=ESTANDAR): "; cin >> nivelOpcion;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (!intANivel(nivelOpcion, nivel)) {
                cout << "Nivel invalido.\n";
                continue;
            }
            cd.registrarEnvio(cod, dest, zona, peso, nivel);

        } else if (opcion == 3) {
            string cod;
            cout << "Codigo: "; getline(cin, cod);
            cd.buscarEnvio(cod);

        } else if (opcion == 4) {
            string cod, obs;
            int estOpcion;
            Estado est;
            cout << "Codigo: "; getline(cin, cod);
            cout << "Nuevo estado (0=RECIBIDO 1=CLASIFICADO 2=EN_REPARTO 3=REPROGRAMADO 4=ENTREGADO): ";
            cin >> estOpcion;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (!intAEstado(estOpcion, est)) {
                cout << "Estado invalido.\n";
                continue;
            }
            cout << "Observacion: "; getline(cin, obs);
            cd.cambiarEstado(cod, est, obs);

        } else if (opcion == 5) {
            cd.despacharProximo();

        } else if (opcion == 6) {
            string cod, motivo;
            cout << "Codigo: "; getline(cin, cod);
            cout << "Motivo: "; getline(cin, motivo);
            cd.reprogramarEnvio(cod, motivo);

        } else if (opcion == 7) {
            string cod;
            cout << "Codigo: "; getline(cin, cod);
            cd.mostrarHistorial(cod);

        } else if (opcion == 8) {
            string zona;
            cout << "Zona (NORTE/SUR/CENTRO): "; getline(cin, zona);
            cd.resumenZona(zona);

        } else if (opcion == 9) {
            string zona;
            cout << "Zona (NORTE/SUR/CENTRO): "; getline(cin, zona);
            cd.envioMasPesadoDeZona(zona);

        } else if (opcion == 10) {
            menuMesa(cd);

        } else if (opcion == 11) {
            string cod, obs;
            cout << "Codigo: "; getline(cin, cod);
            cout << "Observacion: "; getline(cin, obs);
            cd.finalizarEntrega(cod, obs);

        } else if (opcion == 12) {
            menuBST(cd);

        } else if (opcion == 0) {
            cout << "Finalizando... liberando memoria.\n";

        } else {
            cout << "Opcion invalida.\n";
        }

    } while (opcion != 0);

    return 0;
    // Al salir de main, ~CentroDeDistribucion() libera todo en cascada:
    // primero la mesa y el indice BST (solo sus nodos/arreglos), despues
    // ~ListaPendientes() (solo sus nodos) y por ultimo ~ListaDeEnvios(), que
    // destruye cada Envio (y su historial). Solo ListaDeEnvios hace delete
    // sobre los Envio.
}
