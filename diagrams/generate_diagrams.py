#!/usr/bin/env python3
"""
Genera diagramas de clases (UML) en SVG para el proyecto HubFlow / MercadoEnviosAzul.

Salida:
  diagrams/01-sistema-completo.svg   - los 12 tipos del sistema + relaciones + panel de propiedad
  diagrams/02-grupoA-pendientes.svg  - ListaPendientes / NodoPendiente / ResumenZona (+ contexto)
  diagrams/03-grupoB-historial.svg   - HistorialDeMovimientos / NodoMovimiento / Movimiento (+ contexto)
  diagrams/04-grupoC-envios.svg      - Envio / ListaDeEnvios / NodoEnvio (+ contexto)
  diagrams/05-grupoD-centro.svg      - CentroDeDistribucion (+ colaboradores)

Refleja el diseño ACTUAL del código (todo implementado inline en los headers .h;
compila limpio con g++ -std=c++17 -Wall -Wextra). Sin dependencias externas.
Uso: python3 generate_diagrams.py
"""
import html
import math
import os

# ----------------------------------------------------------------------------
# Constantes de estilo
# ----------------------------------------------------------------------------
ROW = 18          # alto por linea de miembro
HEAD = 42         # alto del encabezado (estereotipo + nombre)
FOOT = 26         # alto del pie de pagina (nota de diseno)
MONO_CHAR = 5.9   # ancho aprox. de char en fuente mono 10.5px
SANS_NAME_CHAR = 8.8  # ancho aprox. de char en nombre bold 15px

F_SANS = "Segoe UI, Arial, Helvetica, sans-serif"
F_MONO = "Consolas, Menlo, DejaVu Sans Mono, monospace"

INK = "#334155"        # color de lineas/relaciones
TEXT = "#1f2937"

PALETTES = {
    "A":    dict(header="#1d4ed8", body="#eff6ff", border="#1e3a8a"),  # Grupo A (azul)
    "B":    dict(header="#15803d", body="#f0fdf4", border="#166534"),  # Grupo B (verde)
    "C":    dict(header="#b45309", body="#fffbeb", border="#92400e"),  # Grupo C (ambar)
    "D":    dict(header="#6d28d9", body="#f5f3ff", border="#5b21b6"),  # Grupo D (morado)
    "main": dict(header="#334155", body="#f8fafc", border="#334155"),  # punto de entrada
    "ctx":  dict(header="#64748b", body="#f1f5f9", border="#94a3b8"),  # contexto (otro grupo)
}

# ----------------------------------------------------------------------------
# Primitivas SVG
# ----------------------------------------------------------------------------

class Box:
    """Caja de clase UML. h se calcula a partir del contenido."""
    def __init__(self, key, x, y, w, stereo, name, attrs, methods, palette, footer=None):
        self.key, self.x, self.y, self.w = key, x, y, w
        self.stereo, self.name = stereo, name
        self.attrs, self.methods, self.palette = attrs, methods, palette
        self.footer = footer
        self.h = HEAD + ROW * (len(attrs) + len(methods)) + (FOOT if footer else 0)
        self.ctx = (palette == "ctx")

    # punto sobre un lado; t en [0,1] a lo largo del lado
    def pt(self, side, t):
        if side == "L": return (self.x, self.y + t * self.h)
        if side == "R": return (self.x + self.w, self.y + t * self.h)
        if side == "T": return (self.x + t * self.w, self.y)
        return (self.x + t * self.w, self.y + self.h)  # B

    def overflow(self):
        """Avisa si alguna linea de texto desborda el ancho de la caja."""
        issues = []
        for line in [self.name] + self.attrs + self.methods:
            per = SANS_NAME_CHAR if line is self.name else MONO_CHAR
            need = len(line) * per + 26
            if need > self.w:
                issues.append(f"{self.key}: '{line}' necesita ~{need:.0f}px (caja {self.w}px)")
        return issues


class Diagram:
    def __init__(self, path, title, subtitle, cw, ch):
        self.path, self.title, self.subtitle = path, title, subtitle
        self.cw, self.ch = cw, ch
        self.boxes = {}
        self.rels = []      # (kind, a_box, a_side, a_t, b_box, b_side, b_t, label, points=None, lpos=None)
        self.extras = []    # elementos svg extra (notas, etc.)

    def add(self, box):
        self.boxes[box.key] = box
        return box

    def rel(self, kind, a, side_a, t_a, b, side_b, t_b, label="", points=None, lpos=None):
        """kind: composition | dcomp | assoc | dep"""
        self.rels.append(dict(kind=kind, a=a, sa=side_a, ta=t_a,
                              b=b, sb=side_b, tb=t_b, label=label,
                              points=points, lpos=lpos))

    # ------------------------------------------------------------------ emit
    def svg(self):
        s = []
        s.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.cw}" height="{self.ch}" '
                 f'viewBox="0 0 {self.cw} {self.ch}" font-family="{F_SANS}">')
        s.append(f'<rect x="0" y="0" width="{self.cw}" height="{self.ch}" fill="#ffffff"/>')
        # titulo
        s.append(txt(36, 34, self.title, 20, bold=True, fill=TEXT))
        s.append(txt(36, 56, self.subtitle, 12.5, fill="#64748b"))
        for r in self.rels:
            s.extend(self._rel_svg(r))
        for b in self.boxes.values():
            s.extend(self._box_svg(b))
        for ex in self.extras:
            s.append(ex)
        s.append("</svg>")
        return "\n".join(s)

    def _box_svg(self, b):
        p = PALETTES[b.palette]
        o = []
        dash = ' stroke-dasharray="7,4"' if b.ctx else ""
        o.append(f'<rect x="{b.x}" y="{b.y}" width="{b.w}" height="{b.h}" rx="9" '
                 f'fill="{p["body"]}" stroke="{p["border"]}" stroke-width="1.6"{dash}/>')
        # encabezado
        o.append(f'<path d="M {b.x} {b.y+24} q 0 -9 9 -9 h {b.w-18} q 9 0 9 9 v 18 h {-b.w} z" '
                 f'fill="{p["header"]}"/>')
        o.append(txt(b.x + 11, b.y + 15, "«" + b.stereo + "»", 10.5, italic=True,
                      fill="rgba(255,255,255,0.85)"))
        o.append(txt(b.x + 11, b.y + 34, b.name, 15, bold=True, fill="#ffffff"))
        # cuerpo
        yy = b.y + HEAD
        for i, a in enumerate(b.attrs):
            o.append(txt(b.x + 11, yy + i * ROW + 13, a, 10.5, mono=True, fill=TEXT))
        if b.attrs and b.methods:
            ly = yy + len(b.attrs) * ROW
            o.append(line(b.x, ly, b.x + b.w, ly, "#94a3b8", 1))
            yy = ly
        for i, m in enumerate(b.methods):
            o.append(txt(b.x + 11, yy + i * ROW + 13, m, 10.5, mono=True, fill=TEXT))
        if b.footer:
            fy = b.y + b.h - FOOT
            o.append(line(b.x, fy, b.x + b.w, fy, "#94a3b8", 1))
            o.append(txt(b.x + 11, fy + 17, b.footer, 10.5, italic=True, fill="#64748b"))
        return o

    def _rel_svg(self, r):
        A, B = self.boxes[r["a"]], self.boxes[r["b"]]
        if r["points"]:
            pts = r["points"]
        else:
            pts = [A.pt(r["sa"], r["ta"]), B.pt(r["sb"], r["tb"])]
        o = []
        dashed = r["kind"] in ("dep", "dcomp")
        dash = ' stroke-dasharray="6,4"' if dashed else ""
        path = " ".join(f"{'M' if i == 0 else 'L'} {x:.1f} {y:.1f}" for i, (x, y) in enumerate(pts))
        o.append(f'<path d="{path}" fill="none" stroke="{INK}" stroke-width="1.6"{dash}/>')
        # rombo en el extremo dueño (composicion / composicion punteada)
        if r["kind"] in ("composition", "dcomp"):
            o.append(self._diamond(pts[0], pts[1], filled=(r["kind"] == "composition")))
        # punta de flecha abierta en el extremo destino
        if r["kind"] in ("assoc", "dep"):
            o.append(self._arrow(pts[-2], pts[-1]))
        # etiqueta
        if r["label"]:
            if r["lpos"]:
                lx, ly = r["lpos"]
            else:
                lx, ly = self._path_point(pts, 0.5)
            o.append(txt(lx, ly - 6, r["label"], 11.5, bold=True, fill=TEXT, halo=True))
        return o

    def _diamond(self, p0, p1, filled):
        d = norm(p1[0] - p0[0], p1[1] - p0[1])
        n = (-d[1], d[0])
        a, b2 = 8.5, 5.0
        v = [(p0[0], p0[1]),
             (p0[0] + d[0]*a + n[0]*b2, p0[1] + d[1]*a + n[1]*b2),
             (p0[0] + d[0]*2*a, p0[1] + d[1]*2*a),
             (p0[0] + d[0]*a - n[0]*b2, p0[1] + d[1]*a - n[1]*b2)]
        pts = " ".join(f"{x:.1f},{y:.1f}" for x, y in v)
        fill = INK if filled else "#ffffff"
        return f'<polygon points="{pts}" fill="{fill}" stroke="{INK}" stroke-width="1.4"/>'

    def _arrow(self, p0, p1):
        d = norm(p1[0] - p0[0], p1[1] - p0[1])
        n = (-d[1], d[0])
        L, W = 11.0, 5.0
        bx, by = p1[0] - d[0]*L, p1[1] - d[1]*L
        l1 = (bx + n[0]*W, by + n[1]*W)
        l2 = (bx - n[0]*W, by - n[1]*W)
        return (f'<path d="M {l1[0]:.1f} {l1[1]:.1f} L {p1[0]:.1f} {p1[1]:.1f} '
                f'L {l2[0]:.1f} {l2[1]:.1f}" fill="none" stroke="{INK}" '
                f'stroke-width="1.6" stroke-linecap="round"/>')

    @staticmethod
    def _path_point(pts, t):
        segs = [dist(pts[i], pts[i+1]) for i in range(len(pts)-1)]
        total = sum(segs) or 1
        target = t * total
        acc = 0.0
        for i, L in enumerate(segs):
            if acc + L >= target:
                f = (target - acc) / L if L else 0
                x = pts[i][0] + (pts[i+1][0]-pts[i][0]) * f
                y = pts[i][1] + (pts[i+1][1]-pts[i][1]) * f
                return x, y
            acc += L
        return pts[-1]


def txt(x, y, s, size=12, bold=False, italic=False, mono=False, fill=TEXT, halo=False):
    fam = F_MONO if mono else F_SANS
    w = ' font-weight="bold"' if bold else ""
    it = ' font-style="italic"' if italic else ""
    hl = (' stroke="#ffffff" stroke-width="4" paint-order="stroke"' if halo else "")
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{fam}" font-size="{size}"{w}{it} '
            f'fill="{fill}"{hl}>{html.escape(s)}</text>')


def line(x1, y1, x2, y2, color="#94a3b8", w=1):
    return (f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
            f'stroke="{color}" stroke-width="{w}"/>')


def norm(x, y):
    L = math.hypot(x, y) or 1
    return (x / L, y / L)


def dist(a, b):
    return math.hypot(b[0]-a[0], b[1]-a[1])


# ----------------------------------------------------------------------------
# Leyenda reutilizable
# ----------------------------------------------------------------------------

def legend(x, y, w=430, items=("composition", "assoc", "dep")):
    """Caja de leyenda con muestras de relaciones. Devuelve lista de elementos."""
    o = []
    h = 26 + 24 * len(items)
    o.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" '
             f'fill="#f8fafc" stroke="#cbd5e1" stroke-width="1.2"/>')
    o.append(txt(x + 12, y + 19, "Leyenda", 12, bold=True, fill=TEXT))
    yy = y + 40
    samples = {
        "composition": ("composicion (dueno ◆)", True, False),
        "dcomp":       ("composicion punteada (intencionada / no declarada en header)", False, True),
        "assoc":       ("asociacion (puntero sin propiedad o miembro por valor)", False, False),
        "dep":         ("dependencia (uso de tipos por parametro/retorno)", False, False),
    }
    for it in items:
        label, filled, dashed = samples[it]
        x1, x2 = x + 16, x + 78
        dash = ' stroke-dasharray="5,4"' if (dashed or it == "dep") else ""
        o.append(f'<line x1="{x1}" y1="{yy-4}" x2="{x2}" y2="{yy-4}" stroke="{INK}" '
                 f'stroke-width="1.6"{dash}/>')
        if filled:
            o.append(f'<polygon points="{x1},{yy-4} {x1+9},{yy-8} {x1+18},{yy-4} {x1+9},{yy}" '
                     f'fill="{INK}" stroke="{INK}"/>')
        elif it == "dcomp":
            o.append(f'<polygon points="{x1},{yy-4} {x1+9},{yy-8} {x1+18},{yy-4} {x1+9},{yy}" '
                     f'fill="#ffffff" stroke="{INK}"/>')
        if it in ("assoc", "dep"):
            o.append(f'<path d="M {x2-9} {yy-8} L {x2} {yy-4} L {x2-9} {yy}" fill="none" '
                     f'stroke="{INK}" stroke-width="1.6"/>')
        o.append(txt(x + 92, yy, label, 11, fill=TEXT))
        yy += 24
    return o


def chip_row(x, y, chips):
    """Fila de muestras de color por grupo."""
    o = []
    xx = x
    for name, pal in chips:
        p = PALETTES[pal]
        o.append(f'<rect x="{xx}" y="{y-11}" width="14" height="14" rx="3" '
                 f'fill="{p["header"]}"/>')
        wtxt = len(name) * 6.2 + 10
        o.append(txt(xx + 19, y, name, 11, fill=TEXT))
        xx += 19 + wtxt + 18
    return o


# ----------------------------------------------------------------------------
# Datos: tipos del sistema (firmas segun los headers actuales)
# ----------------------------------------------------------------------------

def build_boxes():
    b = {}
    b["main"] = Box("main", 0, 0, 300, "entry point", "main()",
        [], ["+ main() : int",
             "+ mostrarMenu()",
             "+ cargarDataset(CentroDeDistribucion&)"], "main")
    b["Centro"] = Box("Centro", 0, 0, 560, "facade · controlador", "CentroDeDistribucion",
        ["- registro : ListaDeEnvios      (by value)",
         "- pendientes : ListaPendientes  (by value)"],
        ["+ CentroDeDistribucion() = default",
         "+ ~CentroDeDistribucion() = default   // cascada: registro libera todo",
         "+ registrarEnvio(const string&, const string&, const string&, double, NivelServicio)",
         "+ mostrarPendientes() const",
         "+ buscarEnvio(const string&) const",
         "+ cambiarEstado(const string&, Estado, const string&)",
         "+ despacharProximo()",
         "+ reprogramarEnvio(const string&, const string&)",
         "+ mostrarHistorial(const string&) const",
         "+ resumenZona(const string&) const",
         "+ envioMasPesadoDeZona(const string&) const"], "D",
        footer="Orquesta registro + pendientes; al destruirse libera todo en cascada")
    b["LP"] = Box("LP", 0, 0, 420, "class · lista simple (cola de prioridad)", "ListaPendientes",
        ["- comienzo : NodoPendiente*"],
        ["+ ListaPendientes()",
         "+ ~ListaPendientes()   // solo borra sus nodos, nunca el Envio",
         "+ ListaPendientes(const ListaPendientes&) = delete",
         "+ agregar(Envio*)   // insercion ordenada por prioridad (estable)",
         "+ despachar() : Envio*   // extrae 1er nodo sin destruir el envio",
         "+ remover(Envio*) : bool   // quita nodo sin destruir el envio",
         "+ estaVacia() const : bool",
         "+ mostrar() const",
         "+ resumenPorZona(const string&) const : ResumenZona",
         "+ envioMasPesadoDeZona(const string&) const : Envio*",
         "# resumirZonaRec(NodoPendiente*, const string&) static",
         "# masPesadoDeZonaRec(NodoPendiente*, const string&) static"], "A",
        footer="NUNCA dueña de los Envio* (los owna ListaDeEnvios)")
    b["NP"] = Box("NP", 0, 0, 250, "struct · nodo", "NodoPendiente",
        ["+ envio : Envio*",
         "+ siguiente : NodoPendiente*"],
        ["+ NodoPendiente(Envio*)"], "A")
    b["RZ"] = Box("RZ", 0, 0, 250, "struct · anidada en ListaPendientes", "ResumenZona",
        ["+ cantidad : int",
         "+ pesoTotal : double",
         "+ cantExpress : int"], [], "A")
    b["ES"] = Box("ES", 0, 0, 250, "enum class", "Estado",
        ["RECIBIDO", "CLASIFICADO", "EN_REPARTO", "REPROGRAMADO", "ENTREGADO"], [], "A",
        footer="estadoToString() / intAEstado() (funciones libres)")
    b["NS"] = Box("NS", 0, 0, 260, "enum class", "NivelServicio",
        ["EXPRESS     = 1", "PRIORITARIO = 2", "ESTANDAR    = 3"], [], "A",
        footer="el valor numerico ES la prioridad (menor = antes)")
    b["Mov"] = Box("Mov", 0, 0, 310, "struct", "Movimiento",
        ["+ numero : int",
         "+ estado : string",
         "+ observacion : string"],
        ["+ Movimiento(int, const string&, const string&)"], "B")
    b["NM"] = Box("NM", 0, 0, 300, "struct · nodo (doble enlace)", "NodoMovimiento",
        ["+ movimiento : Movimiento*",
         "+ siguiente : NodoMovimiento*",
         "+ anterior : NodoMovimiento*"],
        ["+ NodoMovimiento(Movimiento*)"], "B")
    b["HDM"] = Box("HDM", 0, 0, 440, "class · lista doblemente enlazada", "HistorialDeMovimientos",
        ["- cabeza : NodoMovimiento*",
         "- cola : NodoMovimiento*",
         "- contador : int"],
        ["+ HistorialDeMovimientos()",
         "+ ~HistorialDeMovimientos()   // delete movimiento + nodo, en cascada",
         "+ HistorialDeMovimientos(const HistorialDeMovimientos&) = delete",
         "+ agregarMovimiento(const string&, const string&)   // O(1) via cola",
         "+ mostrarCronologico() const",
         "+ mostrarInverso() const"], "B",
        footer="Dueña de sus Movimiento* y nodos (delete en cascada)")
    b["Envio"] = Box("Envio", 0, 0, 490, "class · entidad", "Envio",
        ["- codigo : string",
         "- destinatario : string",
         "- zona : string",
         "- peso : double",
         "- nivel : NivelServicio",
         "- estado : Estado",
         "- intentos : int",
         "- historial : HistorialDeMovimientos   (by value)"],
        ["+ Envio(const string&, const string&, const string&, double, NivelServicio)",
         "+ Envio(const Envio&) = delete",
         "+ operator=(const Envio&) = delete",
         "+ getCodigo() const : const string&",
         "+ getDestinatario() const : const string&",
         "+ getZona() const : const string&",
         "+ getPeso() const : double",
         "+ getNivel() const : NivelServicio",
         "+ getEstado() const : Estado",
         "+ getIntentos() const : int",
         "+ estaEntregado() const : bool",
         "+ cambiarEstado(Estado, const string&)   // estado + historial en un paso",
         "+ sumarIntento()",
         "+ mostrarHistorialCronologico() const",
         "+ mostrarHistorialInverso() const",
         "+ mostrar() const"], "C",
        footer="Dueña de su HistorialDeMovimientos (miembro por valor, no puntero)")
    b["NE"] = Box("NE", 0, 0, 270, "struct · nodo", "NodoEnvio",
        ["+ envio : Envio*",
         "+ siguiente : NodoEnvio*"],
        ["+ NodoEnvio(Envio*)"], "C")
    b["LDE"] = Box("LDE", 0, 0, 380, "class · lista simplemente enlazada", "ListaDeEnvios",
        ["- comienzo : NodoEnvio*",
         "- ultimo : NodoEnvio*   // O(1) al final",
         "- cantidad : int"],
        ["+ ListaDeEnvios()",
         "+ ~ListaDeEnvios()   // delete envio + delete nodo (cascada)",
         "+ ListaDeEnvios(const ListaDeEnvios&) = delete",
         "+ agregar(Envio*)   // toma posesion, al final (O(1))",
         "+ buscar(const string&) const : Envio*",
         "+ existeCodigo(const string&) const : bool",
         "+ mostrarTodos() const",
         "+ getCantidad() const : int",
         "+ estaVacia() const : bool"], "C",
        footer="UNICA dueña de los objetos Envio (dtor borra nodo y envio)")
    return b


# ----------------------------------------------------------------------------
# Diagrama 1: sistema completo
# ----------------------------------------------------------------------------

def diagram_overall():
    d = Diagram("01-sistema-completo.svg",
                "HubFlow · MercadoEnviosAzul — Diagrama de clases (sistema completo)",
                "12 tipos en 4 grupos · composicion, asociacion y dependencia · C++17, sin contenedores std ni smart pointers",
                2000, 1600)
    B = build_boxes()
    pos = dict(main=(40, 120), Centro=(700, 90), LP=(60, 500), Envio=(700, 470),
               LDE=(1380, 500), NP=(70, 880), RZ=(380, 880), NE=(1400, 850),
               ES=(60, 1060), NS=(380, 1060), HDM=(760, 1060), NM=(1280, 1060),
               Mov=(1640, 1060))
    for k, (x, y) in pos.items():
        B[k].x, B[k].y = x, y
        d.add(B[k])

    r = d.rel
    r("dep",   "main", "R", .5,  "Centro", "L", .15, "usa / controla")
    r("composition", "Centro", "B", .2, "LP", "T", .5, "pendientes : 1 (by value)")
    r("composition", "Centro", "B", .8, "LDE", "T", .5, "registro : 1 (by value)")
    r("composition", "LP", "B", .3, "NP", "T", .4, "comienzo : 0..*")
    # NP -> Envio: ruta por el canal inferior (RZ esta en medio)
    NP = B["NP"]
    EV = B["Envio"]
    r("assoc", "NP", "B", .5, "Envio", "L", .44, "envio : 1 (no dueno)",
      points=[NP.pt("B", .5), (NP.pt("B", .5)[0], 1005),
              (670, 1005), (670, EV.pt("L", .44)[1]), EV.pt("L", .44)])
    r("composition", "LDE", "B", .45, "NE", "T", .5, "comienzo/ultimo : 0..*")
    r("assoc", "NE", "L", .4, "Envio", "R", .35, "envio : 1")
    r("composition", "LDE", "L", .5, "Envio", "R", .2, "duena de Envio* : * (unica duena)")
    r("composition", "Envio", "B", .4, "HDM", "T", .5, "historial : 1 (by value)")
    r("composition", "HDM", "R", .5, "NM", "L", .5, "cabeza/cola : 0..*")
    r("composition", "NM", "R", .5, "Mov", "L", .5, "movimiento : 1 (delete en dtor)")
    # auto-asociaciones
    r("assoc", "NP", "L", .25, "NP", "L", .75, "siguiente",
      points=[NP.pt("L", .25), (NP.x - 45, NP.pt("L", .25)[1]),
              (NP.x - 45, NP.pt("L", .75)[1]), NP.pt("L", .75)],
      lpos=(NP.x - 62, NP.y + NP.h + 18))
    NM = B["NM"]
    r("assoc", "NM", "T", .3, "NM", "T", .7, "siguiente / anterior",
      points=[NM.pt("T", .3), (NM.pt("T", .3)[0], NM.y - 45),
              (NM.pt("T", .7)[0], NM.y - 45), NM.pt("T", .7)],
      lpos=((NM.pt("T", .3)[0] + NM.pt("T", .7)[0]) / 2, NM.y - 52))
    # miembros por valor de enums (rutas por el canal inferior)
    r("assoc", "Envio", "B", .15, "ES", "T", .5, "estado : 1",
      points=[EV.pt("B", .15), (EV.pt("B", .15)[0], 1010),
              (B["ES"].pt("T", .5)[0], 1010), B["ES"].pt("T", .5)])
    r("assoc", "Envio", "B", .3, "NS", "T", .5, "nivel : 1",
      points=[EV.pt("B", .3), (EV.pt("B", .3)[0], 1025),
              (B["NS"].pt("T", .5)[0], 1025), B["NS"].pt("T", .5)])
    r("dep", "LP", "B", .8, "RZ", "T", .45, "resumenPorZona() : ResumenZona")

    # leyenda + chips arriba a la derecha
    d.extras.extend(legend(1450, 90, w=400))
    d.extras.extend(chip_row(1466, 90 + 26 + 24 * 3 + 18,
                             [("Grupo A", "A"), ("Grupo B", "B"),
                              ("Grupo C", "C"), ("Grupo D", "D"), ("entry", "main")]))

    # panel de notas: modelo de propiedad (verificado contra el codigo actual)
    nx, ny, nw = 60, 1340, 1880
    notas = [
        "✓ El proyecto compila limpio: g++ -std=c++17 -Wall -Wextra · sin contenedores std ni smart pointers.",
        "Cadena de propiedad: ListaDeEnvios ◆ NodoEnvio → Envio (única dueña de los Envio*; su dtor borra nodo y envío).",
        "Envio ◆ HistorialDeMovimientos por valor (no puntero): el historial vive y muere con el envío, sin new/delete.",
        "HistorialDeMovimientos ◆ NodoMovimiento ◆ Movimiento: el dtor del historial libera cada movimiento y su nodo en cascada.",
        "ListaPendientes NUNCA dueña de los Envio*: despachar()/remover() eliminan solo el nodo; el envío sobrevive en ListaDeEnvios (puede reprogramarse o entregarse).",
        "Copias prohibidas (= delete) en Envio, HistorialDeMovimientos y ambas listas: evita doble liberación de punteros crudos.",
        "CentroDeDistribucion compone por valor registro + pendientes: al salir de main, la cascada libera todo sin delete manuales.",
    ]
    h = 52 + 24 * len(notas)
    d.extras.append(f'<rect x="{nx}" y="{ny}" width="{nw}" height="{h}" rx="10" '
                    f'fill="#f0fdf4" stroke="#86efac" stroke-width="1.5"/>')
    d.extras.append(txt(nx + 16, ny + 26, "Modelo de propiedad (verificado contra el código actual)", 14, bold=True, fill="#166534"))
    for i, ntxt_ in enumerate(notas):
        d.extras.append(txt(nx + 20, ny + 50 + i * 24, ntxt_, 12, fill="#14532d"))
    return d


# ----------------------------------------------------------------------------
# Diagrama 2: Grupo A
# ----------------------------------------------------------------------------

def diagram_grupoA():
    d = Diagram("02-grupoA-pendientes.svg",
                "Grupo A — Cola de pendientes por prioridad (lista simplemente enlazada)",
                "ListaPendientes ordena EXPRESS > PRIORITARIO > ESTANDAR (estable) · recursividad para resumen por zona",
                1100, 780)
    B = build_boxes()
    # contexto: Envio (Grupo C) simplificado
    env_ctx = Box("EnvioCtx", 0, 0, 300, "contexto · Grupo C", "Envio",
                  [], ["+ getNivel() const : NivelServicio",
                       "+ getCodigo() const : string",
                       "+ getZona() const : string",
                       "+ getPeso() const : double"], "ctx")
    pos = dict(LP=(60, 90), NP=(80, 470), RZ=(410, 470), EnvioCtx=(760, 480),
               ES=(760, 90), NS=(760, 300))
    for k, (x, y) in pos.items():
        bx = B[k] if k in B else env_ctx
        bx.x, bx.y = x, y
        d.add(bx)

    r = d.rel
    r("composition", "LP", "B", .3, "NP", "T", .4, "comienzo : 0..*")
    # NP -> EnvioCtx: ruta por abajo (RZ esta en medio)
    NP = B["NP"]
    EC = env_ctx
    r("assoc", "NP", "B", .5, "EnvioCtx", "L", .8, "envio : 1 (no dueno)",
      points=[NP.pt("B", .5), (NP.pt("B", .5)[0], 630),
              (720, 630), EC.pt("L", .8)])
    r("dep", "LP", "B", .8, "RZ", "T", .45, "resumenPorZona() : ResumenZona", lpos=(430, 415))
    # LP -> NS: compara getNivel() y NivelServicio::EXPRESS (ruta por el canal inferior)
    r("dep", "LP", "B", .95, "NS", "L", .5, "compara getNivel() (prioridad)",
      points=[B["LP"].pt("B", .95), (B["LP"].pt("B", .95)[0], 436),
              (735, 436), B["NS"].pt("L", .5)])
    r("assoc", "NP", "L", .25, "NP", "L", .75, "siguiente",
      points=[NP.pt("L", .25), (NP.x - 40, NP.pt("L", .25)[1]),
              (NP.x - 40, NP.pt("L", .75)[1]), NP.pt("L", .75)],
      lpos=(NP.x - 62, NP.y + NP.h + 18))

    d.extras.extend(legend(60, 660, w=520, items=("composition", "assoc", "dep")))
    d.extras.append(txt(620, 679, "Caja gris punteada = tipo de otro grupo (contexto).",
                        11, italic=True, fill="#64748b"))
    return d


# ----------------------------------------------------------------------------
# Diagrama 3: Grupo B
# ----------------------------------------------------------------------------

def diagram_grupoB():
    d = Diagram("03-grupoB-historial.svg",
                "Grupo B — Historial de movimientos (lista doblemente enlazada)",
                "Cronologia por envio · recorrible adelante (siguiente) y atras (anterior)",
                1000, 740)
    B = build_boxes()
    env_ctx = Box("EnvioCtx", 0, 0, 320, "contexto · Grupo C", "Envio",
                  [], ["- historial : HistorialDeMovimientos   (by value)",
                       "+ cambiarEstado(Estado, const string&)",
                       "+ mostrarHistorialCronologico() const",
                       "+ mostrarHistorialInverso() const"], "ctx")
    pos = dict(HDM=(80, 90), NM=(80, 400), Mov=(560, 400), EnvioCtx=(560, 70))
    for k, (x, y) in pos.items():
        bx = B[k] if k in B else env_ctx
        bx.x, bx.y = x, y
        d.add(bx)

    r = d.rel
    r("composition", "HDM", "B", .4, "NM", "T", .4, "cabeza/cola : 0..*", lpos=(238, 380))
    r("composition", "NM", "R", .5, "Mov", "L", .5, "movimiento : 1 (delete en dtor)")
    r("composition", "EnvioCtx", "B", .3, "HDM", "R", .3, "historial : 1 (by value)")
    NM = B["NM"]
    r("assoc", "NM", "T", .3, "NM", "T", .7, "siguiente / anterior",
      points=[NM.pt("T", .3), (NM.pt("T", .3)[0], NM.y - 45),
              (NM.pt("T", .7)[0], NM.y - 45), NM.pt("T", .7)],
      lpos=((NM.pt("T", .3)[0] + NM.pt("T", .7)[0]) / 2, NM.y - 52))

    d.extras.extend(legend(80, 600, w=520, items=("composition", "assoc")))
    return d


# ----------------------------------------------------------------------------
# Diagrama 4: Grupo C
# ----------------------------------------------------------------------------

def diagram_grupoC():
    d = Diagram("04-grupoC-envios.svg",
                "Grupo C — Envío (entidad) y registro general de envíos",
                "Envio es dueno de su historial (by value) · ListaDeEnvios es la unica duena de los objetos Envio",
                1300, 880)
    B = build_boxes()
    ctx_hdm = Box("HDMctx", 0, 0, 340, "contexto · Grupo B", "HistorialDeMovimientos",
                  [], ["+ agregarMovimiento(const string&, const string&)",
                       "+ mostrarCronologico() const",
                       "+ mostrarInverso() const"], "ctx")
    ctx_es = Box("ESctx", 0, 0, 240, "contexto · Grupo A", "Estado",
                 [], ["RECIBIDO · CLASIFICADO · EN_REPARTO", "REPROGRAMADO · ENTREGADO"], "ctx")
    ctx_ns = Box("NSctx", 0, 0, 260, "contexto · Grupo A", "NivelServicio",
                 [], ["EXPRESS=1 · PRIORITARIO=2", "ESTANDAR=3"], "ctx")
    pos = dict(Envio=(70, 60), LDE=(640, 60), NE=(640, 420),
               HDMctx=(640, 600), ESctx=(70, 640), NSctx=(340, 640))
    for k, (x, y) in pos.items():
        bx = B[k] if k in B else {"HDMctx": ctx_hdm, "ESctx": ctx_es, "NSctx": ctx_ns}[k]
        bx.x, bx.y = x, y
        d.add(bx)

    r = d.rel
    r("composition", "LDE", "B", .5, "NE", "T", .5, "comienzo/ultimo : 0..*")
    r("assoc", "NE", "L", .4, "Envio", "R", .35, "envio : 1")
    r("composition", "LDE", "L", .5, "Envio", "R", .15, "duena de Envio* : * (unica duena)")
    r("composition", "Envio", "B", .7, "HDMctx", "T", .3, "historial : 1 (by value)")
    r("assoc", "Envio", "B", .15, "ESctx", "T", .5, "estado : 1")
    r("assoc", "Envio", "B", .35, "NSctx", "T", .5, "nivel : 1")

    d.extras.extend(legend(70, 760, w=520, items=("composition", "assoc")))
    return d


# ----------------------------------------------------------------------------
# Diagrama 5: Grupo D
# ----------------------------------------------------------------------------

def diagram_grupoD():
    d = Diagram("05-grupoD-centro.svg",
                "Grupo D — Centro de distribución (fachada del sistema)",
                "Orquesta la cola de pendientes, el registro y las operaciones de gestion",
                1350, 800)
    B = build_boxes()
    ctx_lp = Box("LPctx", 0, 0, 340, "contexto · Grupo A", "ListaPendientes",
                 [], ["+ agregar(Envio*)   // prioridad estable",
                      "+ despachar() : Envio*",
                      "+ remover(Envio*) : bool",
                      "+ resumenPorZona(const string&) const : ResumenZona"], "ctx")
    ctx_lde = Box("LDEctx", 0, 0, 300, "contexto · Grupo C", "ListaDeEnvios",
                  [], ["+ agregar(Envio*)   // toma posesion",
                       "+ buscar(const string&) const : Envio*",
                       "+ existeCodigo(const string&) const : bool"], "ctx")
    ctx_env = Box("EnvCtx", 0, 0, 300, "contexto · Grupo C", "Envio",
                  [], ["+ getCodigo() const : string",
                       "+ cambiarEstado(Estado, const string&)",
                       "+ estaEntregado() const : bool",
                       "+ sumarIntento()"], "ctx")
    ctx_rz = Box("RZctx", 0, 0, 280, "contexto · Grupo A", "ResumenZona",
                 [], ["+ cantidad : int · + pesoTotal : double", "+ cantExpress : int"], "ctx")
    ctx_es = Box("ESctx", 0, 0, 260, "contexto · Grupo A", "Estado",
                 [], ["RECIBIDO · CLASIFICADO · EN_REPARTO", "REPROGRAMADO · ENTREGADO"], "ctx")
    ctx_ns = Box("NSctx", 0, 0, 270, "contexto · Grupo A", "NivelServicio",
                 [], ["EXPRESS=1 · PRIORITARIO=2", "ESTANDAR=3"], "ctx")
    pos = dict(Centro=(420, 60), LPctx=(60, 110), LDEctx=(980, 110), EnvCtx=(980, 330),
               RZctx=(450, 520), ESctx=(60, 330), NSctx=(60, 470))
    for k, (x, y) in pos.items():
        bx = B[k] if k in B else {"LPctx": ctx_lp, "LDEctx": ctx_lde, "EnvCtx": ctx_env,
                                  "RZctx": ctx_rz, "ESctx": ctx_es, "NSctx": ctx_ns}[k]
        bx.x, bx.y = x, y
        d.add(bx)

    r = d.rel
    r("composition", "Centro", "L", .25, "LPctx", "R", .5, "pendientes : 1 (by value)")
    r("composition", "Centro", "R", .25, "LDEctx", "L", .5, "registro : 1 (by value)")
    r("dep", "Centro", "R", .6, "EnvCtx", "L", .5, "opera sobre Envio* (via registro/pendientes)")
    r("dep", "Centro", "B", .35, "RZctx", "T", .5, "resumenZona() : ResumenZona")
    r("dep", "Centro", "L", .8, "ESctx", "R", .5, "cambiarEstado(..., Estado, ...)")
    # Centro -> NSctx: ruta con esquina (NS queda bajo ES)
    r("dep", "Centro", "L", .95, "NSctx", "R", .5, "registrarEnvio(..., NivelServicio)",
      points=[B["Centro"].pt("L", .95), (360, B["Centro"].pt("L", .95)[1]),
              (360, ctx_ns.pt("R", .5)[1]), ctx_ns.pt("R", .5)],
      lpos=(360, 445))

    d.extras.extend(legend(420, 640, w=520))
    return d


# ----------------------------------------------------------------------------
# main
# ----------------------------------------------------------------------------

def main():
    out = os.path.dirname(os.path.abspath(__file__))
    diags = [diagram_overall(), diagram_grupoA(), diagram_grupoB(),
             diagram_grupoC(), diagram_grupoD()]
    problems = []
    for d in diags:
        for b in d.boxes.values():
            problems += [f"{d.path}: " + p for p in b.overflow()]
        path = os.path.join(out, d.path)
        with open(path, "w", encoding="utf-8") as f:
            f.write(d.svg())
        print(f"OK  {path}  ({os.path.getsize(path)} bytes)")
    if problems:
        print("\nAVISO de desborde de texto:")
        for p in problems:
            print("  " + p)


if __name__ == "__main__":
    main()
