#ifndef VIDEO_CONFIG_H
#define VIDEO_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include "visual.h"
#include "ffmpeg.h"
#include "text2Latex.h"

#define RECORDING 1 // 1 to record video, 0 to not record
#define AUDIO_RECORDING 0 // 1 to record audio, 0 to not record

#define FPS 50
#define WIDTH 1920 //
#define HEIGHT 1080 // 1.5



void *ffmpeg = NULL;
#define SIZEXPLOT 500
#define RATIOYPLOT 1.4
#define UNITRATIO 2
#define SPACETREERATIOX 3.0f // Espace entre les noeuds sur l'axe X
#define SPACETREERATIOY 1.5f // Espace entre les noeuds sur l'axe Y
#define BG BLACK
#define SIZETEXT 50.0f      // taille du text standar
#define ARROWSIZE 20.0f     // gives the size arrow divided by ratio i think
#define SPLIT_LEFT_ARROWS 0.2f // gives the left split for arrowsRect
#define RATIOWRITEARROWS 0.5f
#define TEST_DOUBLE_BEGIN " Essaye initiale /begin(itemize) /item Premier /item deuxieme /begin(itemize) /item troisieme /end(itemize)/end(itemize)"
#define DRAWCOLOR WHITE
#define RATIODEFAULT 4.5      // ratio entre la hauteur du rectangle et la hauteur du texte
#define SAFE_RATIO(num, den) ((den) != 0 ? (float)(num) / (float)(den) : 0.0f) // affiche 0 quand diviser par zero

RenderTexture2D screen;
RecordDevice rec;

#endif // VIDEO_CONFIG_H