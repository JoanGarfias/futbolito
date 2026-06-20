SPRITES DEL FUTBOLITO
=====================

El juego usa animaciones en PNG (con transparencia) exportadas de Aseprite.
Cada archivo es una TIRA HORIZONTAL de frames de 24x24 px:

    idle_down.png    idle_up.png    idle_side.png    (personaje quieto)
    walk_down.png    walk_up.png    walk_side.png    (personaje caminando)

- El render deduce cuantos frames hay dividiendo el ancho de la imagen / 24.
- "side" mira a la derecha; para la izquierda el juego voltea la imagen en
  espejo automaticamente.
- Los 4 jugadores comparten el mismo personaje y se distinguen por un TINTE de
  color (azul, rojo, amarillo, verde). Cuando tengan sprites con la playera ya
  pintada de cada color, basta reemplazar estos PNG y quitar el tinte en
  src/render.c (SDL_SetTextureColorMod).
- Si falta algun archivo, ese jugador se dibuja con un cuadro de color.

Como se generaron (desde el .aseprite original), por si hay que regenerarlos:

    aseprite -b --tag "Walk_Down" "16x16 All Animations.aseprite" \
        --sheet-type horizontal --sheet walk_down.png

(IMPORTANTE: --tag va ANTES del archivo, si no exporta TODAS las animaciones.)

Dependencia: el cliente ahora enlaza SDL2_image.
    - MSYS2:  pacman -S mingw-w64-ucrt-x86_64-SDL2_image
    - Linux:  sudo apt install libsdl2-image-dev
