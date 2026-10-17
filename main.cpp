/*
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
    cout << "10. Finalizar\n";
    cout << "Opcion: ";
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

    int opcion;
    do {
        mostrarMenu();
        cin >> opcion;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Opcion invalida.\n";
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
            cout << "Finalizando... liberando memoria.\n";

        } else {
            cout << "Opcion invalida.\n";
        }

    } while (opcion != 10);

    return 0;
    // Al salir de main, ~CentroDeDistribucion() libera todo en cascada:
    // ~ListaDeEnvios() destruye cada Envio (y su historial), y
    // ~ListaPendientes() destruye sus propios nodos.
}
*/

#include "ArborBp.hpp"
#include "CentroDeDistribucion.h"

int main()
{
    /*
    CentroDeDistribucion cd;
    
    Envio* e1 = new Envio("A", "D1", "CENTRO", 1.0, NivelServicio::ESTANDAR);
    Envio* e2 = new Envio("B", "D2", "CENTRO", 1.0, NivelServicio::EXPRESS);
    Envio* e3 = new Envio("C", "D3", "CENTRO", 1.0, NivelServicio::PRIORITARIO);
    Envio* e4 = new Envio("D", "D4", "CENTRO", 1.0, NivelServicio::ESTANDAR);

    ArbolBPlus *testArbol = new ArbolBPlus();
    testArbol->insertar(e1->getCodigo(), e1);
    testArbol->insertar(e2->getCodigo(), e2);
    testArbol->insertar(e3->getCodigo(), e3);
    testArbol->insertar(e4->getCodigo(), e4);

    testArbol->mostrarEnOrden();

    delete e1;
    delete e2;
    delete e3;
    delete e4;

    // print despues de eliminar los envios por fuera
    // la estructura no tiene el ownership de envios
    testArbol->mostrarEnOrden();

    delete testArbol;
    */

    void* ptr = new (size_t)1;
}