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
- 🧰 **Mesa de clasificación y despacho (RF09)**: lote por zona, Insertion Sort, Queue, Stack, búsqueda lineal y binaria.
- 🌳 **Índice de envíos con BST (RF10)**: árbol binario de búsqueda propio por código, con búsqueda, in-order recursivo y altura recursiva.
- 🔒 **Máquina de estados**: cada envío solo puede seguir el ciclo de vida permitido; `ENTREGADO` es un estado final (RF04, RF06, RF07).
- 🔗 **Lista simplemente enlazada propia** → cola de prioridad estable: `EXPRESS > PRIORITARIO > ESTANDAR` (igual prioridad = orden de llegada).
- ⇄ **Lista doblemente enlazada propia** → historial de movimientos por envío, recorríble adelante y atrás con punteros previos.
- 🌀 **Recursión obligatoria** → resumen por zona (cantidad, peso total, EXPRESS) recorriendo los nodos directamente.
- 🧳 Dataset inicial: 8 envíos (`PKG-1001` a `PKG-1008`), todos en estado `RECIBIDO`.

## ⛔ Restricciones técnicas

- ❌ Prohibido: `std::list`, `std::forward_list`, `std::vector`, `std::deque`, `std::priority_queue` y smart pointers.
- ✅ Obligatorio: nodos propios, raw pointers, `new`/`delete` explícitos, destructores y **cero fugas de memoria**.

## 🛠️ Compilar y ejecutar

```bash
g++ -std=c++17 -Wall -Wextra -o hubflow main.cpp
./hubflow
```

## 📁 Estructura

Todo el sistema está implementado en headers (`.h`) incluidos desde `main.cpp`.

| Archivo | Contenido |
|---|---|
| `main.cpp` | Menú de consola y submenús de la mesa (RF09) y del índice BST (RF10) |
| `CentroDeDistribucion.h` | Fachada: registro global + pendientes + mesa + índice BST |
| `ListaDeEnvios.h` | Lista de **todos** los envíos (única dueña de los `Envio`) |
| `ListaPendientes.h` | Cola de prioridad estable (lista simple), resumen recursivo |
| `Envio.h`, `Estados.h` | Entidad, enums y **máquina de estados** (`transicionPermitida`) |
| `HistorialDeMovimientos.h`, `Movimiento.h` | Historial (lista doblemente enlazada) |
| `MesaDeClasificacion.h` | **RF09**: lote, Insertion Sort, búsquedas lineal/binaria |
| `ColaEnvios.h` | **RF09.3**: Queue propia (FIFO) |
| `PilaEnvios.h` | **RF09.4**: Stack propia (LIFO) |
| `IndiceBST.h` | **RF10**: BST propio (nodo, inserción, búsqueda, in-order, altura) |
| `tests/test_main.cpp` | Casos de la consigna + RF04/06/07, RF09 y RF10 (sin framework) |

## 🔒 Máquina de estados (RF04 / RF06 / RF07)

Antes cualquier cambio de estado era válido, lo que permitía dejar un envío duplicado en pendientes (RF06)
o sacarlo de `ENTREGADO` y devolverlo a pendientes (RF07). Ahora las reglas viven en **una sola función**
(`transicionPermitida`, en `Estados.h`) y todos los caminos pasan por el mismo punto
(`CentroDeDistribucion::aplicarTransicion`):

```
RECIBIDO ──► CLASIFICADO ──► EN_REPARTO ──► ENTREGADO   (estado final)
    │                            ▲  │
    └────────────────────────────┘  └──► REPROGRAMADO ──► EN_REPARTO
```

| Desde | Puede pasar a |
|---|---|
| `RECIBIDO` | `CLASIFICADO`, `EN_REPARTO` |
| `CLASIFICADO` | `EN_REPARTO` |
| `EN_REPARTO` | `REPROGRAMADO`, `ENTREGADO` |
| `REPROGRAMADO` | `EN_REPARTO` |
| `ENTREGADO` | ninguno (final) |

- **RF04 – Cambiar estado:** un cambio no permitido se rechaza con un mensaje y **sin** tocar el envío ni su historial. Cada cambio válido agrega un movimiento al final del historial.
- **RF06 – Reprogramar:** solo un envío `EN_REPARTO` (una entrega que falló). Suma un intento, pasa a `REPROGRAMADO`, registra el motivo y se reinserta por prioridad. Ya no puede quedar duplicado en pendientes.
- **RF07 – Finalizar entrega** (menú opción 11): `EN_REPARTO → ENTREGADO`. Al ser un estado final, el envío nunca vuelve a pendientes.
- **Invariante:** un envío figura en pendientes **si y solo si** está `RECIBIDO`, `CLASIFICADO` o `REPROGRAMADO`. Pasar a `EN_REPARTO` por «cambiar estado» también lo saca de pendientes. Como red de seguridad, `ListaPendientes::agregar` no admite el mismo envío dos veces.
- **Cambio de comportamiento:** ya no se puede marcar `ENTREGADO` un envío que nunca salió a reparto (hay que despacharlo primero). Si el equipo prefiere permitirlo, basta agregar `ENTREGADO` a las transiciones de `RECIBIDO` y `CLASIFICADO` en `transicionPermitida`.

## 🧰 RF09 — Mesa de clasificación y despacho

Menú principal → opción **10**. Prepara un lote temporal con los pendientes de
una zona (por ejemplo `NORTE`) y lo trabaja así:

1. **Lote** (`Envio**`): copia las *referencias* de los pendientes de la zona; no se duplica ningún `Envio`.
2. **Ordenar** con **Insertion Sort** escrito a mano, por código ascendente o peso descendente.
   Es estable y se hace en el lugar.
   - Mejor caso **O(n)**: el lote ya está ordenado (el `while` interno no se ejecuta).
   - Peor caso **O(n²)**: el lote está en orden inverso (cada elemento recorre todo lo anterior).
3. **`colaPreparacion`** (Queue): recibe el lote ordenado; `enqueue` y `dequeue` son **O(1)** (punteros a frente y fondo).
4. **`pilaProcesados`** (Stack): cada `dequeue` apila el envío (`push`); `top()` devuelve el último procesado (LIFO). No reemplaza al historial permanente de cada envío.
5. **Búsqueda lineal**: funciona con cualquier orden. Mejor caso **O(1)**, peor caso **O(n)**.
6. **Búsqueda binaria**: solo con el lote ordenado por código. Si no lo está, **no se ejecuta** y se informa. Mejor caso **O(1)**, peor caso **O(log n)**.
7. **Comparación**: mismo código y mismo lote, con la cantidad de comparaciones de cada algoritmo
   (se cuenta una comparación de códigos por paso; en la binaria, `compare()` cuenta como una).

### Ownership (RF09.8)

Los `Envio` siguen siendo propiedad **exclusiva** de `ListaDeEnvios`.
El arreglo del lote, la Queue y la Stack guardan **punteros no propietarios**:

- `~MesaDeClasificacion` hace `delete[]` del arreglo auxiliar, no de los envíos.
- `~ColaEnvios` y `~PilaEnvios` liberan solo sus nodos.
- Ninguna ejecuta `delete` sobre un `Envio` → sin *double free*.
- En `CentroDeDistribucion` la mesa se declara **después** del registro, así se destruye antes y nunca apunta a envíos liberados → sin *dangling pointers*.
- Crear un lote nuevo libera el arreglo anterior y vacía cola y pila → sin fugas.
- El lote es una foto: si un envío se despacha después, la referencia sigue siendo válida porque el registro lo conserva.

## 🌳 RF10 — Índice de envíos con BST

Menú principal → opción **12**. Es una estructura **adicional**: no reemplaza a la lista de pendientes ni a las
búsquedas lineal y binaria del lote.

- **RF10.1 – Nodo:** `NodoBST { Envio* envio; NodoBST* izq; NodoBST* der; }`. Orden por código: menores a la izquierda, mayores a la derecha. Sin duplicados.
- **RF10.2 – Inserción (iterativa):** `registrarEnvio` inserta en el BST una referencia al **mismo** `Envio`. Nunca se reconstruye el árbol.
- **RF10.3 – Búsqueda:** muestra el camino (`PKG-1004 -> derecha`, `PKG-1007 -> izquierda`, `PKG-1005 -> encontrado`), los datos del envío y la cantidad de **nodos visitados**. Si no existe: `Envio no encontrado en el BST.`
- **RF10.4 – In-order (recursivo):** izquierdo, actual, derecho. Sale ascendente porque todo el subárbol izquierdo es menor que el nodo y todo el derecho es mayor; al aplicar la misma regla en cada subárbol, la secuencia completa queda de menor a mayor.
- **RF10.5 – Altura (recursiva):** convención = **cantidad de niveles**. Árbol vacío = 0, un único nodo = 1, varios niveles = `1 + max(altura izq, altura der)`. Se usa así en todo el sistema.
- **RF10.6 – Ownership:** el BST crea y destruye **solo sus nodos**. Nunca hace `delete` sobre un `Envio` (los posee `ListaDeEnvios`). En `CentroDeDistribucion` el índice se declara después del registro, así se destruye antes y no queda apuntando a envíos liberados. Evita *double free*, *dangling pointers* y destrucciones accidentales.
- **Alcance:** indexa todos los envíos conocidos (pendientes, en reparto y entregados). Un envío entra al registrarse y no sale.

### RF10.7 — Complejidad

| Operación | Mejor caso | Esperado / balanceado | Peor caso |
|---|---|---|---|
| Inserción | O(1) (árbol vacío) | O(log n) | O(n) |
| Búsqueda | O(1) (en la raíz) | O(log n) | O(n) |
| In-order | O(n) | O(n) | O(n) |
| Altura | O(n) | O(n) | O(n) |

**Por qué un BST común no garantiza O(log n):** la forma del árbol depende del orden de inserción y este árbol no
se rebalancea. Si los códigos llegan ya ordenados (`PKG-1001`, `PKG-1002`, `PKG-1003`, `PKG-1004`…), cada uno es mayor
que todos los anteriores y se cuelga siempre a la derecha: el árbol degenera en una lista enlazada de altura *n* y
insertar o buscar recorre hasta *n* nodos. Solo un árbol balanceado (AVL, rojo-negro) mantiene altura ≈ log₂(n).

> 💡 Con el dataset de ejemplo, que se carga en orden ascendente (`PKG-1001` a `PKG-1008`), el índice queda
> degenerado: altura 8 contra una mínima posible de 4. La opción «Altura y cantidad de nodos» lo muestra, y la
> comparación «BST vs búsqueda lineal» (opción 4 del submenú) informa los nodos visitados y las comparaciones de
> la lista sobre el mismo conjunto de envíos, más las del lote de la mesa si hay uno creado.
>
> La recursión del in-order, de la altura y del destructor tiene profundidad igual a la altura del árbol, por lo que
> en un árbol degenerado de muchísimos nodos podría agotar la pila.

## ✅ Tests

```bash
g++ -std=c++17 -Wall -Wextra -I. -o hubflow_tests tests/test_main.cpp && ./hubflow_tests
```
