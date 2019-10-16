// ---------------------------------------------------------
// KFixPal
// ---------------------------------------------------------
// Conversor de imagenes de 8 bits a una paleta unica.
// Por Kronoman - En memoria de mi querido padre
// Copyright (c) 2002-2007, Kronoman - Comenzado 28/Dic/2002
// ---------------------------------------------------------
// Modo de uso:
// kfixpal *.pcx
//
// Una vez iniciado el programa, permite seleccionar tolerancia,
// editar la paleta generada, etc, todo con un UI.
//
// NOTA: el algoritmo usado es MUY LENTO!! Usar con *PACIENCIA*!
// La paleta finalizada tiene los colores ordenados de < a >
// usando la suma de RGB...
//
// NOTA: si hay imagenes muy grandes en relacion a otras
// (por ejemplo, una de 32x32 con una de 800x600), se tiende a dar
// mas 'peso' a los colores de la imagen grande (por el conteo que se hace),
// por lo tanto, hay una opcion para compensar estas diferencias de tama~o, pero
// implica una pasada mas sobre las imagenes. (para ver cual es la mas grande...)
// Esta funcion toma en cuenta la superfice (W*H) de la imagen mas grande, y el ratio
// de la imagen actual, para aumentar el peso de los colores de las imagenes chicas.
//
// NOTA: a partir de la version 0.9.0, accedo directamente a la RAM de los bitmaps;
// complica el algoritmo, pero acelera las cosas al no usar getpixel y putpixel.
//
// ---------------------------------------------------------
// Distributed under The MIT License
//
// Permission is hereby granted, free of charge,
// to any person obtaining a copy of this software
// and associated documentation files (the "Software"),
// to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice
// shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ---------------------------------------------------------

#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>

#include "gerror.h"
#include "kfixcore.h"

// Globales de este archivo
static int tolerancia_rgb = 1; // tolerancia a colores similares
static int salto_x = 1, salto_y = 1; // salto x, y del scan de imagenes
PALETTE pal256out; // paleta de salida

// ---------------------------------------------------------
// Dialogo de configuracion inicial,
// Devuelve -1 si hay que salir del programa (CANCELAR) o 0 si hay que CONTINUAR
// ---------------------------------------------------------
int hacer_dialogo_inicial()
{
	char xinc[4], yinc[4], coli[4]; // vars de entrada
	int ret, el_error;
	DIALOG dlg_inicio[] =
	    {
	        /* (proc)        (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags)     (d1)                       (d2) (dp)                                    (dp2) (dp3) */
	        { d_box_proc,    0,   0,   320, 200, 0,   0,   0,    0,          0,                         0,   NULL,                                   NULL, NULL },
	        { d_text_proc,   4,   4,   208, 12,  0,   0,   0,    0,          0,                         0,   "KFixpal-(c) 2002-2007 Kronoman",       NULL, NULL },
	        { d_text_proc,   4,   12,  308, 12,  0,   0,   0,    0,          0,                         0,   "In loving memory of my father",        NULL, NULL },
	        { d_text_proc,   8,   40,  124, 8,   0,   0,   0,    0,          0,                         0,   "Color reduction",                      NULL, NULL },
	        { d_text_proc,   8,   56,  248, 8,   0,   0,   0,    0,          0,                         0,   "Read area (less=better,slower)",       NULL, NULL },
	        { d_check_proc,  4,   124, 220, 12,  0,   0,   0,    D_SELECTED, 1,                         0,   "Edit palette before apply",            NULL, NULL },
	        { d_box_proc,    136, 36,  40,  16,  0,   0,   0,    0,          0,                         0,   NULL,                                   NULL, NULL },
	        { d_edit_proc,   140, 40,  32,  8,   0,   0,   0,    0,          sizeof(coli)-1,            0,   coli,                                   NULL, NULL },
	        { d_box_proc,    52,  72,  40,  16,  0,   0,   0,    0,          0,                         0,   NULL,                                   NULL, NULL },
	        { d_edit_proc,   56,  76,  32,  12,  0,   0,   0,    0,          sizeof(xinc)-1,            0,   xinc,                                   NULL, NULL },
	        { d_box_proc,    140, 72,  40,  16,  0,   0,   0,    0,          0,                         0,   NULL,                                   NULL, NULL },
	        { d_edit_proc,   144, 76,  32,  12,  0,   0,   0,    0,          sizeof(yinc)-1,            0,   yinc,                                   NULL, NULL },
	        { d_text_proc,   16,  76,  32,  8,   0,   0,   0,    0,          0,                         0,   "X +=",                                 NULL, NULL },
	        { d_text_proc,   104, 76,  32,  8,   0,   0,   0,    0,          0,                         0,   "Y +=",                                 NULL, NULL },
	        { d_button_proc, 8,   176, 60,  20,  0,   0,   0,    D_EXIT,     0,                         0,   "START",                                NULL, NULL },
	        { d_button_proc, 252, 176, 60,  20,  0,   0,   0,    D_EXIT,     0,                         0,   "Cancel",                               NULL, NULL },
	        { d_text_proc,   4,   92,  308, 12,  0,   0,   0,    0,          0,                         0,   "Custom palette from file? (optional)", NULL, NULL },
	        { d_box_proc,    4,   104, 272, 16,  0,   0,   0,    0,          0,                         0,   NULL,                                   NULL, NULL },
	        { d_edit_proc,   8,   108, 260, 8,   0,   0,   0,    0,          sizeof(palette_in_file)-1, 0,   palette_in_file,                        NULL, NULL },
	        { d_button_proc, 284, 104, 32,  16,  0,   0,   0,    D_EXIT,     0,                         0,   "...",                                  NULL, NULL },
	        { d_check_proc,  4,   144, 220, 12,  0,   0,   0,    D_SELECTED, 1,                         0,   "Compensate colour weight",             NULL, NULL },
	        { NULL,          0,   0,   0,   0,   0,   0,   0,    0,          0,                         0,   NULL,                                   NULL, NULL }
	    };


	el_error = TRUE; // repetir hasta que salga OK
	palette_in_file[0] = '\0';

	while (el_error)
	{
		// Poner valores
		sprintf(coli, "%d", tolerancia_rgb);
		sprintf(xinc, "%d", salto_x);
		sprintf(yinc, "%d", salto_y);

		set_dialog_color(dlg_inicio, gui_fg_color, gui_bg_color);
		centre_dialog(dlg_inicio);
		ret = do_dialog(dlg_inicio,-1);
		el_error = FALSE;

		if (ret == 15) return -1; // Boton "cancelar", salir
		if (ret == 19)  // obtener path a imagen con la paleta...
		{
			file_select_ex("Pick a image to take the palette", palette_in_file, "PCX;BMP;TGA;LBM", sizeof(palette_in_file)-1, 0,0);
			el_error = TRUE; // continuar en el loop...
		}

		// Obtener valores
		tolerancia_rgb = atoi(coli);
		salto_x = atoi(xinc);
		salto_y = atoi(yinc);
		if (tolerancia_rgb < 0 || tolerancia_rgb>63) {el_error = TRUE; tolerancia_rgb = 1;}
		if (salto_x < 1 || salto_x>32) {el_error = TRUE; salto_x = 1; }
		if (salto_y < 1 || salto_y>32) {el_error = TRUE; salto_y = 1; }

		if ( dlg_inicio[5].flags & D_SELECTED )
			quiere_editar_paleta  = TRUE;
		else
			quiere_editar_paleta = FALSE;

		if ( dlg_inicio[20].flags & D_SELECTED )
			usar_comp_max_wh = TRUE;
		else
			usar_comp_max_wh = FALSE;

	}
	return 0;
}

// ---------------------------------------------------------
// Este dialogo permite editar la paleta generada en pal256out
// Devuelve:
// 0 si presiono OK
// -1 si presiono Re-configure
// ---------------------------------------------------------
int editar_pal256out()
{
	char desc_rgb[256]; // linea de descripcion RGB
	char ID_R = 'R', ID_G = 'G', ID_B = 'B'; // IDs de los sliders
	int c_actual = 0; // color seleccionado 0..255
	int ret;

	// Funcion interna para actualizar el color al slider...
	int update_color(void *dp3, int dp2)
	{
		char c = *((char *)dp3); // ID del slider
		if (c == ID_R) pal256out[c_actual].r = dp2;
		if (c == ID_G) pal256out[c_actual].g = dp2;
		if (c == ID_B) pal256out[c_actual].b = dp2;
		set_palette(pal256out);
		// actualiza info
		sprintf(desc_rgb, "Index: %d - RGB: %d,%d,%d", c_actual,pal256out[c_actual].r,pal256out[c_actual].g,pal256out[c_actual].b);
		return D_REDRAW; // redibujar todo
	}

	// Procedimiento que permite editar la paleta pal256out,
	// Es un hack horrible...
	// Solo toma ancho 300 y alto 100
	// Y los cuadraditos son de 10x10
	int edit_pal_gui(int msg, DIALOG *d, int c)
	{
		int x,y,i,x2,y2;
		switch (msg)
		{
		case MSG_START: // inicializar

		case MSG_DRAW: // dibujar
			rectfill(screen, d->x, d->y, d->x+d->w-1, d->y+d->h-1, d->bg);
			x2=d->x;
			y2=d->y;
			i =0;
			for (y=0; y < 10; y++)
			{
				for (x=0; x < 30; x++)
				{
					if (i <256) rectfill(screen, x2, y2, x2+(d->w/30), y2+(d->h/10), i);
					if (i == c_actual) rect(screen, x2, y2, x2+(d->w/30), y2+(d->h/10), 255-i);
					x2 += d->w/30;
					i++;
				}
				x2 = d->x;
				y2 += d->h/10;
			}
			break;

		case MSG_LRELEASE: // cambiar seleccion
			x = mouse_x; y = mouse_y;
			x = x - d->x;
			y = y - d->y;
			if (x < 0 || x > d->w) return D_O_K;
			if (y < 0 || y > d->h) return D_O_K;
			x = x / (d->w/30);
			y = y / (d->h/10);
			c_actual = x + (y * 30);
			if (c_actual > 255) c_actual =255;
			if (c_actual<0) c_actual=0;
			// actualiza info
			sprintf(desc_rgb, "Index: %d - RGB: %d,%d,%d", c_actual,pal256out[c_actual].r,pal256out[c_actual].g,pal256out[c_actual].b);
			return D_REDRAW;
			break;


		}
		return D_O_K;
	}


	DIALOG dlg_editpal[] =
	    {
	        /* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                          (dp2) (dp3) */
	        { d_box_proc,        0,   0,   316, 200, 0,   0,   0,    0,      0,   0,   NULL,                         NULL, NULL },
	        { edit_pal_gui,      8,   8,   300, 100, 0,   0,   0,    0,      0,   0,   NULL,                         NULL, NULL },
	        { d_text_proc,       8,   120, 44,  28,  0,   0,   0,    0,      0,   0,   "RGB:",                       NULL, NULL },
	        { d_text_proc,       8,   160, 236, 12,  0,   0,   0,    0,      0,   0,   desc_rgb, NULL, NULL },
	        { d_slider_proc,     56,  116, 248, 12,  0,   0,   0,    0,      63, 0,   NULL,                         update_color, &ID_R }, // R
	        { d_slider_proc,     56,  128, 248, 12,  0,   0,   0,    0,      63, 0,   NULL,                         update_color, &ID_G }, // G
	        { d_slider_proc,     56,  140, 248, 12,  0,   0,   0,    0,      63, 0,   NULL,                         update_color, &ID_B }, // B
	        { d_button_proc,     200, 176, 108, 20,  0,   0,   0,    D_EXIT, 0,   0,   "Re-configure",               NULL, NULL },
	        { d_button_proc,     8,   176, 84,  20,  0,   0,   0,    D_EXIT, 0,   0,   "OK",                         NULL, NULL },
	        { NULL,              0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                         NULL, NULL }
	    };

	desc_rgb[0] = '\0';
	gui_fg_color = makecol(0,0,0);
	gui_bg_color = makecol(255,255,255);
	set_dialog_color(dlg_editpal, gui_fg_color, gui_bg_color);
	centre_dialog(dlg_editpal);
	ret = do_dialog(dlg_editpal,-1);

	if (ret == 7) return -1; // reconfigurar...

	return 0; // Todo OK
}

// ---------------------------------------------------------
// Cuantifica una imagen colocando los colores usados en la lista de colores
// Pasarle el nombre de archivo
// Esto es un 'wrapper' alrededor de la rutina nucleo en kfixcore.c
// ---------------------------------------------------------
void cuantificar_imagen(char *file)
{
	BITMAP *bmp;
	PALETTE pal;

	// cargar imagen en RAM
	bmp = load_bitmap(file, pal);

	// Error cargando imagen?
	if (bmp == NULL)
	{
		raise_error("ERROR CATASTROFICO: No se pudo cargar '%s'!\nCan't load the file!\n", file);
	}

	// Mostrar en pantalla (DEBUG, esto es lento!!!)
	set_palette(pal);
	clear(screen);
	blit(bmp, screen, 0,0,0,0,bmp->w,bmp->h);
	textprintf(screen, font, 0,0,makecol(255,255,255), "Reading and quantize image '%s'", file);

	kfixpal_cuantificar_imagen(bmp,pal,salto_x,salto_y,tolerancia_rgb); // aca es donde pasa la cosa realmente... =)

	destroy_bitmap(bmp);
	bmp = NULL;
}

// ---------------------------------------------------------
// Convierte una imagen a la paleta de colores fabricada en pal256out
// Pasarle el nombre de archivo entrada y de salida
// ---------------------------------------------------------
void convertir_a_paleta(char *filein, char *fileout)
{
	BITMAP *bmp;
	PALETTE pal;

	clear(screen);
	textprintf(screen, font, 0,0,makecol(255,255,255), "Converting and saving: '%s'->'%s' (Saving)", filein, fileout);

	// cargar imagen en RAM
	bmp = load_bitmap(filein, pal);

	// Error cargando imagen?
	if (bmp == NULL) raise_error ("ERROR CATASTROFICO: No se pudo cargar '%s'!\nCan't load file!\n", filein);

	// convertir usando las funciones nucleo
	kfixpal_convertir_a_paleta(bmp, pal, pal256out);

	// mostrar - DEBUG: innecesario
	blit(bmp, screen, 0,0,0,0,bmp->w,bmp->h);

	// salvar a disco
	save_bitmap(fileout, bmp, pal256out);

	// liberar RAM
	destroy_bitmap(bmp);
	bmp = NULL;
}


// ---------------------------------------------------------
// Rock 'n Roll
// ---------------------------------------------------------
int main(int argc, char *argv[] )
{
	int i,c;
	int todo_ok = FALSE;

	allegro_init();
	install_keyboard();
	install_mouse(); // Para el  GUI

	if (argc < 2)
	{
		allegro_message("%s\nBy Kronoman - In loving memory of my father\nUnder the MIT license\nCopyrigth (C) 2002, 2003, Kronoman\nUsage: kfixpal [images PCX,BMP,TGA]\nExample: kfixpal *.pcx", KFIXPAL_VERSION);
		return 0; // no hay parametros - DEBUG! mostrar ayuda!!!
	}


	set_color_depth(8);
	if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0))
		if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0))
			if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0))
				raise_error("ERROR CATASTROFICO: No puedo setear video a 640x480x8bits!\nI need a 8 bpp screen!\n");

	while(todo_ok == FALSE) // esto es para permitir ajustar varias veces los parametros si no se esta conforme...
	{

		// --------------------- aca esta lo interesante en si... ------------------

		kfixpal_liberar_l_color(); // liberar lista de color

		clear(screen);
		set_palette(default_palette);
		gui_fg_color = makecol(0,0,0);
		gui_bg_color = makecol(255,255,255);

		if (hacer_dialogo_inicial()) return -1; // dialogo de inicio, para configurar

		// Ver si estoy usando paleta custom...
		palette_in_bmp = load_bitmap(palette_in_file, pal256out);

		if (palette_in_bmp == NULL) // no la estoy usando, generar la paleta
		{
			// Si hay que compensar por imagenes peque~as, primero debo ver las superficies involucradas
			if (usar_comp_max_wh)
			{
				comp_max_wh = -1; // valor inicial minimo...

				for (i = 1; i < argc; i++) // recorrer cada archivo de la linea de comandos...
				{
					textprintf(screen, font, 0,0,makecol(255,255,255), "Mesuring colour weight of %s..", argv[i]);
					kfixpal_calcular_superficie_de_compensacion(argv[i]);
				}
			}

			// Cuantificar para generar paleta optima
			for (i = 1; i < argc; i++) // recorrer cada archivo de la linea de comandos...
			{
				cuantificar_imagen(argv[i]);
			}

			// Ordenar paleta por los mas usados...
			qsort(l_color,l_color_c,sizeof(L_COLOR), kfixpal_qsort_helper_color);

			// Fabricar la paleta de salida
			for (i=0; i < 256; i++) pal256out[i].r = pal256out[i].g = pal256out[i].b = 0; // limpiar paleta

			pal256out[255].r = pal256out[255].g = pal256out[255].b = 63; // ultimo color = blanco

			c = l_color_c;
			if (c > 256) c = 256; // tomar los primeros 256 colores

			for (i=0; i < c; i++) // copiar los colores 1 a 1
			{
				pal256out[i].r = l_color[i].rgb.r;
				pal256out[i].g = l_color[i].rgb.g;
				pal256out[i].b = l_color[i].rgb.b;
			}

			// Ordenar la paleta - NOTA: esto es opcional, pero queda mejor... :P
			qsort(pal256out,256, sizeof(RGB), kfixpal_qsort_helper_paleta256);

			todo_ok = TRUE; // listo...
			// --------------------- aca esta lo interesante en si... ------------------

		}
		else
		{
			// ya no preciso mas la imagen, solo la paleta...
			destroy_bitmap(palette_in_bmp);
			palette_in_bmp = NULL;
			todo_ok = TRUE;
		}


		// permitir editar la paleta generada!
		if (quiere_editar_paleta)
		{

			set_palette(pal256out);
			gui_fg_color = makecol(0,0,0);
			gui_bg_color = makecol(255,255,255);
			clear(screen);

			if (editar_pal256out() != 0)
				todo_ok = FALSE;
			else
				todo_ok = TRUE;
		}
	} // fin del while principal del todo_ok

	// Convertir las imagenes a la nueva paleta, e ir salvando...
	set_palette(pal256out);


	for (i = 1; i < argc; i++) // recorrer cada archivo de la linea de comandos...
		convertir_a_paleta(argv[i], argv[i]); // esto la abre, la convierte, y la salva a disco...

	// Listo macho...

	// DEBUG: algo de info de que carajo hizo...
	set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	allegro_message("Kfixpal Finished\n%d files where processed and converted.\n %ld total colors readed from file\n\n", argc-1, l_color_c);

	// Liberar RAM usada
	kfixpal_liberar_l_color();



	return 0;
}
END_OF_MAIN();
