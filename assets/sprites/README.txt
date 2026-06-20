SPRITES DEL FUTBOLITO
=====================

Coloca aqui los sprites en formato BMP con estos nombres EXACTOS:

    player1.bmp   -> jugador 1 (izquierda, azul)
    player2.bmp   -> jugador 2 (derecha, rojo)
    player3.bmp   -> jugador 3 (arriba, amarillo)
    player4.bmp   -> jugador 4 (abajo, verde)
    ball.bmp      -> pelota

Reglas:
- Formato: BMP (24 bits). El juego los carga con SDL_LoadBMP (SDL puro,
  sin librerias extra).
- Transparencia: usa MAGENTA puro (R=255, G=0, B=255) para las zonas que
  quieres que sean transparentes. El motor lo trata como color transparente
  (color key).
- Tamano sugerido: jugadores 30x30 px, pelota 20x20 px. Si usas otro tamano,
  la imagen se escala automaticamente al tamano del personaje en el campo.

Si falta algun archivo, ese elemento se dibuja con su color solido de
respaldo, asi que el juego nunca se rompe por falta de sprites.

Para usar PNG con transparencia real (canal alfa) en lugar de BMP, avisame:
hay que enlazar la libreria SDL2_image (-lSDL2_image) y cambiar SDL_LoadBMP
por IMG_Load en src/render.c.
