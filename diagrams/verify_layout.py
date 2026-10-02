#!/usr/bin/env python3
"""
Verificacion geometrica de los diagramas generados (sin rasterizador):
  1) que ninguna caja se solape con otra,
  2) que ningun segmento de relacion atraviese una caja ajena a sus extremos,
  3) advertencia si alguna etiqueta cae dentro de una caja.

Uso: python3 verify_layout.py   (desde diagrams/)
"""
import generate_diagrams as gd


def liang_barsky(seg, rect):
    """Longitud de la porcion del segmento que queda dentro del rectangulo."""
    x1, y1 = seg[0]
    x2, y2 = seg[1]
    xmin, ymin, xmax, ymax = rect
    dx, dy = x2 - x1, y2 - y1
    t0, t1 = 0.0, 1.0
    for p, q in ((-dx, x1 - xmin), (dx, xmax - x1), (-dy, y1 - ymin), (dy, ymax - y1)):
        if abs(p) < 1e-9:
            if q < 0:
                return 0.0
        else:
            r = q / p
            if p < 0:
                if r > t1:
                    return 0.0
                t0 = max(t0, r)
            else:
                if r < t0:
                    return 0.0
                t1 = min(t1, r)
    if t1 <= t0:
        return 0.0
    L = ((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5
    return (t1 - t0) * L


def check(d):
    boxes = list(d.boxes.values())
    problems, warnings = [], []

    # 1) solapamiento entre cajas
    for i in range(len(boxes)):
        for j in range(i + 1, len(boxes)):
            a, b = boxes[i], boxes[j]
            ox = min(a.x + a.w, b.x + b.w) - max(a.x, b.x)
            oy = min(a.y + a.h, b.y + b.h) - max(a.y, b.y)
            if ox > 1 and oy > 1:
                problems.append(f"cajas solapadas: {a.key} y {b.key} ({ox:.0f}x{oy:.0f}px)")

    # 2) segmentos de relaciones vs cajas ajenas
    for r in d.rels:
        A, B = d.boxes[r["a"]], d.boxes[r["b"]]
        if r["points"]:
            pts = r["points"]
        else:
            pts = [A.pt(r["sa"], r["ta"]), B.pt(r["sb"], r["tb"])]
        for k in range(len(pts) - 1):
            seg = (pts[k], pts[k + 1])
            for bx in boxes:
                if bx.key in (r["a"], r["b"]):
                    continue
                L = liang_barsky(seg, (bx.x, bx.y, bx.x + bx.w, bx.y + bx.h))
                if L > 1.5:
                    problems.append(
                        f"relacion {r['kind']} {r['a']}->{r['b']} atraviesa caja {bx.key} "
                        f"({L:.0f}px de interseccion)")

    # 3) etiquetas dentro de cajas (advertencia, aproximada: borde del bbox del texto)
    for r in d.rels:
        if not r["label"]:
            continue
        A, B = d.boxes[r["a"]], d.boxes[r["b"]]
        pts = r["points"] or [A.pt(r["sa"], r["ta"]), B.pt(r["sb"], r["tb"])]
        if r["lpos"]:
            lx, ly = r["lpos"]
        else:
            lx, ly = gd.Diagram._path_point(pts, 0.5)
            ly -= 6
        lw = len(r["label"]) * 6.3
        x0, x1, y0, y1 = lx - lw / 2, lx + lw / 2, ly - 10, ly + 2
        edges = [((x0, y0), (x1, y0)), ((x0, y1), (x1, y1)),
                 ((x0, y0), (x0, y1)), ((x1, y0), (x1, y1))]
        for bx in boxes:
            if bx.key in (r["a"], r["b"]):
                continue
            if any(liang_barsky(e, (bx.x, bx.y, bx.x + bx.w, bx.y + bx.h)) > 1.5
                   for e in edges):
                warnings.append(f"etiqueta '{r['label']}' cae sobre caja {bx.key}")

    return problems, warnings


def main():
    total_p = 0
    for name, fn in [("01", gd.diagram_overall), ("02", gd.diagram_grupoA),
                     ("03", gd.diagram_grupoB), ("04", gd.diagram_grupoC),
                     ("05", gd.diagram_grupoD)]:
        d = fn()
        p, w = check(d)
        total_p += len(p)
        print(f"{name} {d.path}: {'OK' if not p else 'PROBLEMAS'}"
              + (f"  ({len(w)} avisos)" if w else ""))
        for x in p:
            print("   PROBLEMA  " + x)
        for x in w:
            print("   aviso     " + x)
    print("\nResultado:", "SIN PROBLEMAS" if total_p == 0 else f"{total_p} problemas")


if __name__ == "__main__":
    main()
