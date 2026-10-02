#pragma once
#include "ListaDeEnvios.h"
#include "ListaPendientes.h"
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

    // RF04 — Cambiar estado (tambien cubre RF07: marcar ENTREGADO).
    // Si el nuevo estado es ENTREGADO, se lo saca de pendientes por las
    // dudas (por si todavia no habia pasado por despacharProximo): un envio
    // entregado nunca debe seguir figurando como pendiente.
    void cambiarEstado(const string& codigo, Estado nuevoEstado, const string& obs) {
        Envio* e = registro.buscar(codigo);
        if (e == nullptr) { cout << "Envio no encontrado.\n"; return; }
        e->cambiarEstado(nuevoEstado, obs);
        if (nuevoEstado == Estado::ENTREGADO) pendientes.remover(e);
        cout << "Estado actualizado.\n";
    }

    // RF05 — Despachar proximo envio (el primer nodo de pendientes)
    void despacharProximo() {
        Envio* e = pendientes.despachar();
        if (e == nullptr) { cout << "No hay envios pendientes.\n"; return; }
        e->cambiarEstado(Estado::EN_REPARTO, "Despachado del centro");
        cout << "Despachado: " << e->getCodigo() << " -> " << e->getDestinatario() << "\n";
    }

    // RF06 — Reprogramar envio: vuelve a pendientes respetando su prioridad.
    // Un envio ENTREGADO nunca puede volver a pendientes.
    void reprogramarEnvio(const string& codigo, const string& motivo) {
        Envio* e = registro.buscar(codigo);
        if (e == nullptr) { cout << "Envio no encontrado.\n"; return; }
        if (e->estaEntregado()) { cout << "El envio ya fue entregado, no puede reprogramarse.\n"; return; }
        e->sumarIntento();
        e->cambiarEstado(Estado::REPROGRAMADO, motivo);
        pendientes.agregar(e);
        cout << "Envio " << codigo << " reprogramado (intento " << e->getIntentos() << ").\n";
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

    // Desafio adicional — envio de mayor peso pendiente de una zona
    void envioMasPesadoDeZona(const string& zona) const {
        Envio* e = pendientes.envioMasPesadoDeZona(zona);
        if (e == nullptr) { cout << "No hay envios pendientes en la zona " << zona << ".\n"; return; }
        cout << "Zona consultada: " << zona << "\n";
        cout << "Envio mas pesado: " << e->getCodigo() << "\n";
        cout << "Peso: " << e->getPeso() << " kg\n";
    }
};
