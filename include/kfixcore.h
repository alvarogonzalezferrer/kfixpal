// ---------------------------------------------------------
// KFixpal
//
// kfixcore.h
//
// By Kronoman - In loving memory of my father
// Core functions to quantize and convert images
// Revision: October-2003
// ---------------------------------------------------------

#ifndef KFIXCORE_H
#define KFIXCORE_H

#include <allegro.h>

#include "gerror.h"

// ---------------------------------------------------------
// Version
// ---------------------------------------------------------
#define KFIXPAL_VERSION "KFixpal 0.9.8 beta"

// ---------------------------------------------------------
// Estructuras de datos
// ---------------------------------------------------------
// Contenedor para la lista de colores cargados
typedef struct L_COLOR
{
  RGB rgb; // color en si
  long int c; // cantidad de veces que aparece
} L_COLOR;


// ---------------------------------------------------------
// Globales
// ---------------------------------------------------------
extern L_COLOR *l_color;
extern long int l_color_c;
extern char palette_in_file[2048];
extern BITMAP *palette_in_bmp;
extern int quiere_editar_paleta;
extern int comp_max_wh;
extern int usar_comp_max_wh;


// Prototipos

void kfixpal_liberar_l_color();
int kfixpal_qsort_helper_color(const void *c1, const void *c2);
int kfixpal_qsort_helper_paleta256(const void *c1, const void *c2);
int kfixpal_distancia_rgb(const RGB c1,const RGB c2);
int kfixpal_buscar_color_mas_proximo(const RGB c1, const PALETTE pal);
void kfixpal_agregar_color(const RGB c1, int conteo, int tolerancia_rgb);
void kfixpal_cuantificar_imagen(BITMAP *bmp, PALETTE pal, int salto_x, int salto_y, int tolerancia_rgb );
void kfixpal_calc_sup_compensacion_bmp(BITMAP *bmp);
void kfixpal_calcular_superficie_de_compensacion(char *file);
void kfixpal_convertir_a_paleta(BITMAP *bmp, PALETTE pal, PALETTE pal256out);

#endif
