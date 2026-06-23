#ifndef AUDIO_H
#define AUDIO_H

/* Sonidos del juego con SDL2_mixer. Si el dispositivo de audio no abre o
 * algun archivo no carga, el juego sigue sin sonido (no es fatal). */

/* Llamar una vez, despues de SDL_Init. */
void initAudio(void);
void freeAudio(void);

/* Llamar una vez por frame con los contadores actuales (goalCount y
 * ball.kickCount de GameState). Si cambiaron desde el frame anterior, suena
 * el gol o la patada; ademas, de vez en cuando dispara un sonido ambiental
 * al azar. */
void audioUpdate(int goalCount, int kickCount);

#endif
