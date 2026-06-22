const pptxgen = require("pptxgenjs");
const path = require("path");

const pptx = new pptxgen();

// 90 x 120 cm  ->  pulgadas
const W = 90 / 2.54;  // 35.433"
const H = 120 / 2.54; // 47.244"
pptx.defineLayout({ name: "POSTER", width: W, height: H });
pptx.layout = "POSTER";

// Paleta (tema cancha de futbol)
const GREEN_DARK = "1F4D20";
const GREEN = "2C5F2D";
const CARD_BG = "EDF3ED";
const CARD_LINE = "C9D8C9";
const TEXT = "222222";
const WHITE = "FFFFFF";
const PLACE_BG = "DDE6DD";

const FONT_H = "Georgia";
const FONT_B = "Calibri";

const slide = pptx.addSlide();
slide.background = { color: WHITE };

const M = 1.2;
const GAP = 0.4;
const bottomLimit = H - M; // margen inferior

// ---------- Encabezado (titulo + autores) ----------
const headerH = 5.8;
slide.addShape(pptx.ShapeType.roundRect, {
  x: M, y: 1.0, w: W - 2 * M, h: headerH,
  fill: { color: GREEN_DARK }, line: { type: "none" }, rectRadius: 0.2,
});
slide.addText(
  "Futbolito Cliente/Servidor con paralelismo multihilo sincronizado por seccion critica",
  { x: M + 0.4, y: 1.3, w: W - 2 * M - 0.8, h: 3.4,
    fontFace: FONT_H, fontSize: 80, bold: true, color: WHITE,
    align: "center", valign: "middle" }
);
slide.addText(
  [
    { text: "[Integrante 1]  ·  [Integrante 2]  ·  [Integrante 3]  ·  [Integrante 4]\n", options: { fontSize: 40, bold: true } },
    { text: "[Universidad / Facultad] — Sistemas de Computo Paralelo y Distribuido — Ciclo 2025-2026B", options: { fontSize: 36 } },
  ],
  { x: M + 0.4, y: 4.8, w: W - 2 * M - 0.8, h: 1.8,
    fontFace: FONT_B, color: WHITE, align: "center", valign: "middle" }
);

// ---------- Geometria de columnas ----------
const colW = 16.0;
const leftX = M;            // 1.2
const rightX = M + colW + 1.0; // 18.2
const bodyTop = 1.0 + headerH + 0.6; // 6.8

// ---------- Helpers ----------
function card(x, y, w, h) {
  slide.addShape(pptx.ShapeType.roundRect, {
    x, y, w, h, fill: { color: CARD_BG },
    line: { color: CARD_LINE, width: 1 }, rectRadius: 0.12,
  });
}
function heading(x, y, w, text) {
  slide.addText(text, {
    x: x + 0.4, y: y + 0.25, w: w - 0.8, h: 0.95,
    fontFace: FONT_H, fontSize: 40, bold: true, color: GREEN,
    align: "left", valign: "middle",
  });
}
function body(x, y, w, h, runs, fontSize, bullet) {
  slide.addText(runs, {
    x: x + 0.4, y, w: w - 0.8, h,
    fontFace: FONT_B, fontSize: fontSize || 28, color: TEXT,
    align: "left", valign: "top", lineSpacingMultiple: 1.05,
    bullet: bullet ? { code: "2022", indent: 18 } : false,
    paraSpaceAfter: bullet ? 6 : 0,
  });
}
function placeholder(x, y, w, h, label) {
  slide.addShape(pptx.ShapeType.roundRect, {
    x, y, w, h, fill: { color: PLACE_BG },
    line: { color: GREEN, width: 1.5, dashType: "dash" }, rectRadius: 0.08,
  });
  slide.addText(label, {
    x, y, w, h, fontFace: FONT_B, fontSize: 24, italic: true,
    color: GREEN_DARK, align: "center", valign: "middle",
  });
}

// =================== COLUMNA IZQUIERDA ===================
let y = bodyTop;

// Resumen
let h = 5.0;
card(leftX, y, colW, h);
heading(leftX, y, colW, "Resumen");
body(leftX, y + 1.25, colW, h - 1.4,
  "Se desarrollaron dos aplicaciones nativas (Linux y Windows) de un futbolito multijugador. Ambas comparten una base en lenguaje C y usan exclusivamente APIs nativas: sockets (Winsock/BSD), hilos (CreateThread/pthread) y mutex (CRITICAL_SECTION/pthread_mutex). La arquitectura es Cliente/Servidor sobre TCP: un servidor autoritativo calcula la fisica y reparte el estado a hasta 4 jugadores. El paralelismo se sincroniza con una seccion critica con mutex que protege el estado compartido entre el hilo de fisica y los hilos de clientes, evitando condiciones de carrera.",
  28);
y += h + GAP;

// Introduccion
h = 3.8;
card(leftX, y, colW, h);
heading(leftX, y, colW, "Introduccion");
body(leftX, y + 1.25, colW, h - 1.4,
  "Los sistemas distribuidos coordinan procesos en maquinas distintas y, dentro de cada uno, multiples hilos que comparten memoria. Este proyecto materializa ambos conceptos: la comunicacion entre equipos se resuelve con sockets, y la concurrencia interna con hilos sincronizados mediante el mecanismo de seccion critica (mutex).",
  28);
y += h + GAP;

// Objetivo
h = 3.6;
card(leftX, y, colW, h);
heading(leftX, y, colW, "Objetivo");
body(leftX, y + 1.25, colW, h - 1.4,
  "Desarrollar un par de aplicaciones nativas en C, bajo arquitectura Cliente/Servidor punto a punto con conectividad heterogenea via sockets, y control del paralelismo multihilo mediante seccion critica con mutex, con una interfaz grafica que anima las mecanicas del juego de forma distribuida en hasta 4 equipos.",
  28);
y += h + GAP;

// Diseno y logica (esquemas)
h = bottomLimit - y; // hasta el margen inferior
if (h < 12) h = 12;
card(leftX, y, colW, h);
heading(leftX, y, colW, "Diseno y logica");

// Esquema 1: red
let sy = y + 1.3;
slide.addText("Esquema 1 — Arquitectura de red (Cliente/Servidor)", {
  x: leftX + 0.4, y: sy, w: colW - 0.8, h: 0.6,
  fontFace: FONT_B, fontSize: 24, bold: true, color: GREEN_DARK,
});
sy += 0.7;
// Servidor central
slide.addShape(pptx.ShapeType.roundRect, {
  x: leftX + colW / 2 - 2.4, y: sy, w: 4.8, h: 1.3,
  fill: { color: GREEN }, line: { type: "none" }, rectRadius: 0.1,
});
slide.addText("SERVIDOR\nfisica + estado", {
  x: leftX + colW / 2 - 2.4, y: sy, w: 4.8, h: 1.3,
  fontFace: FONT_B, fontSize: 22, bold: true, color: WHITE, align: "center", valign: "middle",
});
// 4 clientes
const cliY = sy + 2.6;
const cx0 = leftX + 0.8;
const cgap = (colW - 1.6 - 3.4) / 3;
for (let i = 0; i < 4; i++) {
  const cxx = cx0 + i * (3.4 + cgap);
  slide.addShape(pptx.ShapeType.roundRect, {
    x: cxx, y: cliY, w: 3.4, h: 1.1,
    fill: { color: CARD_BG }, line: { color: GREEN, width: 1.5 }, rectRadius: 0.1,
  });
  slide.addText("Cliente " + (i + 1), {
    x: cxx, y: cliY, w: 3.4, h: 1.1,
    fontFace: FONT_B, fontSize: 20, bold: true, color: GREEN_DARK, align: "center", valign: "middle",
  });
  slide.addShape(pptx.ShapeType.line, {
    x: cxx + 1.7, y: cliY, w: (leftX + colW / 2) - (cxx + 1.7), h: cliY - (sy + 1.3),
    line: { color: GREEN, width: 1.5, endArrowType: "triangle", beginArrowType: "triangle" },
    flipV: true,
  });
}
slide.addText("Conexion TCP persistente · PlayerPacket  <->  GamePacket", {
  x: leftX + 0.4, y: cliY + 1.2, w: colW - 0.8, h: 0.5,
  fontFace: FONT_B, fontSize: 20, italic: true, color: TEXT, align: "center",
});

// Esquema 2: mutex
sy = cliY + 2.0;
slide.addText("Esquema 2 — Multihilo + seccion critica (servidor)", {
  x: leftX + 0.4, y: sy, w: colW - 0.8, h: 0.6,
  fontFace: FONT_B, fontSize: 24, bold: true, color: GREEN_DARK,
});
sy += 0.7;
// hilos
const thLabels = ["Hilo fisica", "Hilo cli.1", "Hilo cli.2", "Hilo cli.N"];
const thW = 3.4;
const thGap = (colW - 1.6 - thW * 4) / 3;
for (let i = 0; i < 4; i++) {
  const txx = leftX + 0.8 + i * (thW + thGap);
  slide.addShape(pptx.ShapeType.roundRect, {
    x: txx, y: sy, w: thW, h: 1.0,
    fill: { color: WHITE }, line: { color: GREEN, width: 1.5 }, rectRadius: 0.1,
  });
  slide.addText(thLabels[i], {
    x: txx, y: sy, w: thW, h: 1.0, fontFace: FONT_B, fontSize: 18, bold: true,
    color: GREEN_DARK, align: "center", valign: "middle",
  });
}
// mutex barrier
const muY = sy + 1.4;
slide.addShape(pptx.ShapeType.roundRect, {
  x: leftX + 0.8, y: muY, w: colW - 1.6, h: 0.9,
  fill: { color: "C0392B" }, line: { type: "none" }, rectRadius: 0.08,
});
slide.addText("MUTEX  —  mutex_lock() / mutex_unlock()  (seccion critica)", {
  x: leftX + 0.8, y: muY, w: colW - 1.6, h: 0.9,
  fontFace: FONT_B, fontSize: 20, bold: true, color: WHITE, align: "center", valign: "middle",
});
// shared state
const stY = muY + 1.3;
slide.addShape(pptx.ShapeType.roundRect, {
  x: leftX + colW / 2 - 3.2, y: stY, w: 6.4, h: 1.1,
  fill: { color: GREEN }, line: { type: "none" }, rectRadius: 0.1,
});
slide.addText("GameState compartido\n(pelota, jugadores, marcador)", {
  x: leftX + colW / 2 - 3.2, y: stY, w: 6.4, h: 1.1,
  fontFace: FONT_B, fontSize: 18, bold: true, color: WHITE, align: "center", valign: "middle",
});

// =================== COLUMNA DERECHA ===================
y = bodyTop;

// Esquema 3: tabla portabilidad
h = 7.2;
card(rightX, y, colW, h);
heading(rightX, y, colW, "Esquema 3 — Portabilidad nativa");
const tblRows = [
  [
    { text: "Recurso", options: { bold: true, color: WHITE, fill: { color: GREEN } } },
    { text: "Windows", options: { bold: true, color: WHITE, fill: { color: GREEN } } },
    { text: "Linux", options: { bold: true, color: WHITE, fill: { color: GREEN } } },
  ],
  ["Sockets", "Winsock2 (ws2_32)", "Sockets BSD"],
  ["Hilos", "CreateThread", "pthread_create"],
  ["Mutex", "CRITICAL_SECTION", "pthread_mutex_t"],
  ["Graficos", "SDL2", "SDL2"],
];
slide.addTable(tblRows, {
  x: rightX + 0.4, y: y + 1.4, w: colW - 0.8, h: h - 1.7,
  fontFace: FONT_B, fontSize: 26, color: TEXT, valign: "middle", align: "center",
  border: { type: "solid", color: CARD_LINE, pt: 1 },
  fill: { color: WHITE },
});
y += h + GAP;

// Resultados
h = 13.0;
card(rightX, y, colW, h);
heading(rightX, y, colW, "Resultados");
const imgY = y + 1.4;
const imgW = (colW - 0.8 - 0.4) / 2;
placeholder(rightX + 0.4, imgY, imgW, 4.6, "[Captura — Windows]");
placeholder(rightX + 0.4 + imgW + 0.4, imgY, imgW, 4.6, "[Captura — Linux]");
placeholder(rightX + 0.4, imgY + 5.0, colW - 0.8, 3.4, "[Consola del servidor — 4 clientes conectados]");
body(rightX, imgY + 8.7, colW, 3.0,
  [
    { text: "Hasta 4 jugadores simultaneos." , options:{} },
    { text: "Marcador y fisica consistentes en todas las pantallas.", options:{ breakLine:true } },
    { text: "Sin corrupcion de estado gracias al mutex.", options:{ breakLine:true } },
  ], 24, true);
y += h + GAP;

// Conclusiones
h = 4.2;
card(rightX, y, colW, h);
heading(rightX, y, colW, "Conclusiones");
body(rightX, y + 1.25, colW, h - 1.4,
  "Combinar sockets para la comunicacion distribuida con hilos sincronizados por seccion critica permite un videojuego multijugador estable y portable entre Linux y Windows usando solo C y APIs nativas. El mutex resulto indispensable: sin el, el acceso concurrente al estado producia inconsistencias visibles.",
  28);
y += h + GAP;

// Referencias
h = 5.0;
card(rightX, y, colW, h);
heading(rightX, y, colW, "Referencias");
body(rightX, y + 1.25, colW, h - 1.4,
  [
    { text: "1. Stevens, Fenner, Rudoff. UNIX Network Programming, Vol. 1. Addison-Wesley." },
    { text: "2. Microsoft. Winsock (Windows Sockets 2) Reference. Microsoft Learn.", options:{ breakLine:true } },
    { text: "3. Microsoft. Critical Section Objects. Microsoft Learn.", options:{ breakLine:true } },
    { text: "4. The Open Group. POSIX Threads (pthreads) Specification.", options:{ breakLine:true } },
    { text: "5. SDL2. Simple DirectMedia Layer — API Reference. libsdl.org.", options:{ breakLine:true } },
  ], 20);
y += h + GAP;

// QR
h = bottomLimit - y;
if (h < 6) h = 6;
card(rightX, y, colW, h);
heading(rightX, y, colW, "Video del proyecto");
const qrSize = Math.min(h - 2.2, colW - 6);
placeholder(rightX + colW / 2 - qrSize / 2, y + 1.4, qrSize, qrSize, "[QR al video]");
slide.addText("Escanea para ver el video explicativo (YouTube)", {
  x: rightX + 0.4, y: y + 1.4 + qrSize + 0.1, w: colW - 0.8, h: 0.7,
  fontFace: FONT_B, fontSize: 24, italic: true, color: GREEN_DARK, align: "center",
});

const out = path.join(__dirname, "..", "cartel_futbolito.pptx");
pptx.writeFile({ fileName: out }).then(() => console.log("OK -> " + out));
