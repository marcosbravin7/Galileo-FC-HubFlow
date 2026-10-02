# 🚚 HubFlow — Gestión de envíos de última milla

> 📦 Trabajo Práctico Integrador: **Estructuras de Datos y POO en C++**
> Prototipo de consola para administrar paquetes que ingresan a un centro de distribución, son clasificados y despachados para su entrega.

[![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org)
[![Standard: C++17](https://img.shields.io/badge/Standard-C%2B%2B17-4385F5?style=for-the-badge)]()
[![Compiler: g++](https://img.shields.io/badge/Compiler-g%2B%2B-A8B900?style=for-the-badge&logo=gnumake&logoColor=white)](https://gcc.gnu.org)
[![Platform: Linux](https://img.shields.io/badge/Platform-Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)]()
[![Status: En desarrollo](https://img.shields.io/badge/Status-En%20desarrollo-yellow?style=for-the-badge)]()
[![Deadline: 5 sep 23:59 ARG](https://img.shields.io/badge/Deadline-5%20sep%2023%3A59%20ARG-red?style=for-the-badge)]()

## 📌 Info del TP

| | |
|---|---|
| 🎯 **Modalidad** | Trabajo en equipos |
| ⏱️ **Duración estimada** | 2 horas |
| 🧩 **Temas integrados** | POO, recursividad, listas simple/doblemente enlazadas, Big O |
| 📤 **Entrega** | Google Docs + código fuente |

## ✨ Características

- 🍔 Menú de consola para gestionar envíos pendientes y registrar movimientos.
- 🔗 **Lista simplemente enlazada propia** → cola de prioridad estable: `EXPRESS > PRIORITARIO > ESTANDAR` (igual prioridad = orden de llegada).
- ⇄ **Lista doblemente enlazada propia** → historial de movimientos por envío, recorríble adelante y atrás con punteros previos.
- 🌀 **Recursión obligatoria** → resumen por zona (cantidad, peso total, EXPRESS) recorriendo los nodos directamente.
- 🧳 Dataset inicial: 8 envíos (`PKG-1001` a `PKG-1008`), todos en estado `RECIBIDO`.

## ⛔ Restricciones técnicas

- ❌ Prohibido: `std::list`, `std::forward_list`, `std::vector`, `std::deque`, `std::priority_queue` y smart pointers.
- ✅ Obligatorio: nodos propios, raw pointers, `new`/`delete` explícitos, destructores y **cero fugas de memoria**.

## 🛠️ Compilar y ejecutar

```bash
g++ -std=c++17 -Wall -Wextra -o hubflow *.cpp
./hubflow
```

## 📁 Estructura

- `*.cpp` / `*.hpp` — código fuente del sistema.
