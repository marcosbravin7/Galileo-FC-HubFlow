# Diagramas de clases — HubFlow / MercadoEnviosAzul

Diagramas UML (SVG) generados a partir del código fuente del proyecto.
Reflejan el **diseño actual de los headers** (todo implementado inline en `.h`;
el proyecto compila limpio con `g++ -std=c++17 -Wall -Wextra`).

> Regenerados el 8-sep tras el refactor a headers inline: la versión anterior
> describía el estado intermedio (`.hpp`/`.cpp` separados, `Estados`, miembro
> fantasma `listaEnvios`) que ya no existe en el código.

## Archivos

| Archivo | Contenido |
|---|---|
| `01-sistema-completo.svg` | Los 12 tipos del sistema + `main()`, todas las relaciones y panel del modelo de propiedad |
| `02-grupoA-pendientes.svg` | `ListaPendientes` / `NodoPendiente` / `ResumenZona` / `Estado` / `NivelServicio` (+ contexto `Envio`) |
| `03-grupoB-historial.svg` | `HistorialDeMovimientos` / `NodoMovimiento` / `Movimiento` (+ contexto `Envio`) |
| `04-grupoC-envios.svg` | `Envio` / `ListaDeEnvios` / `NodoEnvio` (+ contextos de otros grupos) |
| `05-grupoD-centro.svg` | `CentroDeDistribucion` (fachada) y todos sus colaboradores |

Previsualizaciones PNG en `preview/`.

## Cómo regenerar / verificar

```bash
python3 diagrams/generate_diagrams.py   # regenera los 5 SVG
python3 diagrams/verify_layout.py       # verificacion geometrica (solapes, cruces, etiquetas)
```

Sin dependencias externas (solo stdlib). Las previsualizaciones PNG opcionales
se generan con `GdkPixbuf` (`python3-gi`) si está disponible. Abrir cualquier
SVG directamente en el navegador.

## Leyenda de relaciones

- **Composición** — línea sólida con rombo relleno en el extremo dueño:
  el objeto destino se crea y destruye junto con el dueño (miembro por valor,
  o puntero propio que el destructor libera).
- **Asociación** — línea sólida con flecha abierta: puntero sin propiedad
  (el destino lo gestiona otro objeto) o referencia por valor.
- **Dependencia** — línea punteada con flecha abierta: uso puntual de un tipo
  como parámetro o retorno.
- **Auto-asociación** — lazo sobre sí misma: punteros `siguiente` /
  `anterior` dentro del propio nodo.

## Código de colores (grupos del TP)

| Color | Grupo | Tipos |
|---|---|---|
| Azul | A | `ListaPendientes`, `NodoPendiente`, `ResumenZona`, `Estado`, `NivelServicio` |
| Verde | B | `HistorialDeMovimientos`, `NodoMovimiento`, `Movimiento` |
| Ámbar | C | `Envio`, `ListaDeEnvios`, `NodoEnvio` |
| Morado | D | `CentroDeDistribucion` (fachada) |
| Gris oscuro | — | `main()` (punto de entrada) |
| Gris punteado | contexto | tipo de otro grupo, mostrado solo con sus miembros relevantes |

## Modelo de propiedad (el corazón del diseño)

Restricción del TP: sin contenedores std ni smart pointers; todo nodo propio
con `new`/`delete` explícitos y destructores que no fuguen memoria. La cadena
de propiedad resultante:

```
ListaDeEnvios ─◆─ NodoEnvio ── Envio ─◆─ HistorialDeMovimientos ─◆─ NodoMovimiento ─◆─ Movimiento
      (única dueña de Envio)                (miembro por valor)        (delete en dtor)   (delete en dtor)
```

- `ListaDeEnvios` es la **única dueña** de los objetos `Envio` (su destructor
  borra nodo *y* envío).
- `Envio` posee su `HistorialDeMovimientos` **por valor** (no puntero): el
  historial vive y muere con el envío, sin `new`/`delete`.
- `ListaPendientes` solo posee sus `NodoPendiente`; el puntero `envio` es
  **no propietario** (el envío sobrevive al despachar porque vive en
  `ListaDeEnvios`).
- `HistorialDeMovimientos` libera cada `Movimiento*` y su nodo en cascada en
  su destructor.
- Copias prohibidas (`= delete`) en `Envio`, `HistorialDeMovimientos` y ambas
  listas: evita doble liberación de punteros crudos.
- `CentroDeDistribucion` compone por valor `registro` + `pendientes`: al salir
  de `main`, la cascada libera todo sin `delete` manuales.

## Verificación

- `verify_layout.py` confirma en los 5 diagramas: ninguna caja se solapa,
  ningún segmento de relación atraviesa una caja ajena y ninguna etiqueta cae
  sobre una caja.
- Comprobación adicional (fuera del verificador): cero colisiones entre
  etiquetas y clearance mínimo > 6 px entre líneas y cajas.
