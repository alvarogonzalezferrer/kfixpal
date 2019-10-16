// ---------------------------------------------------------
// KFixpal
//
// kfixcore.c
//
// By Kronoman - In loving memory of my father
// Core functions to quantize and convert images
// Revision: October-2003
// ---------------------------------------------------------


#include <allegro.h>

#include "kfixcore.h"

// ---------------------------------------------------------
// Globales
// ---------------------------------------------------------
L_COLOR *l_color = NULL; // lista de colores cargados
long int l_color_c = 0; // cantidad de colores cargados

char palette_in_file[2048]; // path a archivo de imagen con paleta de entrada (opcional)
BITMAP *palette_in_bmp = NULL; // imagen con paleta de entrada (opciona)
int quiere_editar_paleta = 0; // quiere editar la paleta generada?

// Sistema de compensacion de color para imagenes chicas
// Superficie de la imagen mas grande (W*H); -1 indica que esta desactivada la funcion
int comp_max_wh = -1;
int usar_comp_max_wh = 0; // Utilizar esta funcion de compensacion?


// ---------------------------------------------------------
// Liberacion de lista de colores
// ---------------------------------------------------------
void kfixpal_liberar_l_color()
{
	if (l_color)
	{
		free(l_color);
		l_color_c = 0;
		l_color = NULL;
	}
}

// ---------------------------------------------------------
// Esta funcion ayuda al qsort para ordenar los colores por su aparicion
// ---------------------------------------------------------
int kfixpal_qsort_helper_color(const void *c1, const void *c2)
{
	const L_COLOR *a1 = (const L_COLOR *)c1;
	const L_COLOR *a2 = (const L_COLOR *)c2;

	if ( a1->c > a2->c ) return -1;

	return 1;
}

// ---------------------------------------------------------
// Esta funcion ayuda al qsort para ordenar los colores de la paleta finalizada
// Los ordena por sumatoria de RGB, queda bastante bien
// ---------------------------------------------------------
int kfixpal_qsort_helper_paleta256(const void *c1, const void *c2)
{
	const RGB *a1 = (const RGB *)c1;
	const RGB *a2 = (const RGB *)c2;
	int suma1, suma2;

	suma1 = a1->r + a1->g + a1->b;
	suma2 = a2->r + a2->g + a2->b;

	if (suma1 < suma2) return -1;

	return 1;
}

// ---------------------------------------------------------
// Devuelve la distancia entre 2 colores RGB
// ---------------------------------------------------------
int kfixpal_distancia_rgb(const RGB c1,const RGB c2)
{
	return ( (c1.r-c2.r)*(c1.r-c2.r) + (c1.g-c2.g)*(c1.g-c2.g) + (c1.b-c2.b)*(c1.b-c2.b));
}

// ---------------------------------------------------------
// Esto busca el color mas proximo en la paleta de salida
// y devuelve el indice 0..255
// Pasarle el color a buscar, y la paleta de busqueda
// ---------------------------------------------------------
int kfixpal_buscar_color_mas_proximo(const RGB c1, const PALETTE pal)
{
	int i;
	int dist = 65535; // distancia entre colores maxima
	int col = 0;
	// iterar buscando el color
	for (i=0; i<256; i++)
	{
		if (kfixpal_distancia_rgb(c1 , pal[i]) < dist)
		{
			dist = kfixpal_distancia_rgb(c1 , pal[i]);
			col = i;
		}
	}

	return col;
}


// ---------------------------------------------------------
// Esto coloca un nuevo color en la lista de colores.
// Lo hace solo si el color no existia.
// Si ya existe, simplemente cuenta +conteo en el lugar apropiado.
// Conteo deberia ser normalmente 1, pero es asi para
// poder compensar el color en caso de imagenes chicas combinadaas con imagenes grandes
// Utiliza 'tolerancia_rgb' para encontrar colores similares...
//
// Parametros:
// c1 = color
// conteo = contarlo por cuanto
// tolerancia_rgb = tolerancia rgb
// ---------------------------------------------------------
void kfixpal_agregar_color(const RGB c1, int conteo, int tolerancia_rgb)
{
	int i;

	// Ver si ya existe
	if (l_color != NULL) // hay algo en RAM?
	{
		for (i = 0; i < l_color_c; i++)
		{
			// Es similar?
			if (ABS(l_color[i].rgb.r - c1.r) <= tolerancia_rgb &&
			        ABS(l_color[i].rgb.g - c1.g) <= tolerancia_rgb &&
			        ABS(l_color[i].rgb.b - c1.b) <= tolerancia_rgb)
			{
				l_color[i].c += conteo;
				return; // listo...
			}
		}
	}

	// si llego aca, el color no esta en la lista.
	l_color_c++; // aumentar tama~o de lista dinamica

	if (l_color == NULL) // reservar RAM
		l_color = (L_COLOR *)malloc(sizeof(L_COLOR));
	else
		l_color = (L_COLOR *)realloc(l_color, sizeof(L_COLOR)*(l_color_c+1));

	// Se quedo sin RAM?
	if (l_color == NULL)
	{
		// ERROR
		raise_error("ERROR CATASTROFICO: Sin RAM en agregar_color()!\nNo RAM to add color to list!\n");
	}

	// Agregar
	l_color[l_color_c-1].rgb.r = c1.r;
	l_color[l_color_c-1].rgb.g = c1.g;
	l_color[l_color_c-1].rgb.b = c1.b; // color RGB
	l_color[l_color_c-1].c = conteo; // es el primero que agrego de este color...
}


// ---------------------------------------------------------
// Cuantifica una imagen colocando los colores usados en la lista de colores
//
// Parametros:
// bmp = bitmap de la imagen
// pal = paleta de colores de la imagen
// salto_x = cada cuantos pixeles cuantificar (muy recomendado: dejar en 1)
// salto_y = cada cuantos pixeles cuantificar (muy recomendado: dejar en 1)
// tolerancia_rgb = como considerar colores 'parecidos' (amplitud)
//
// NOTA: usar_comp_max_wh afecta como cuantifica la imagen (compensa o no el tama~o de la imagen)
//
// ---------------------------------------------------------
void kfixpal_cuantificar_imagen(BITMAP *bmp, PALETTE pal, int salto_x, int salto_y, int tolerancia_rgb )
{
	int x,y,s; // contadores varios


	// agregar colores a lista look-up...
	// Para esto, recorro la imagen, porque a veces hay colores en la paleta
	// que no se usan en la imagen... ademas, de esta manera, cuantifico realmente
	// que tanto se usan los colores en la imagen...
	// Incorpora ademas, un sistema de compensacion para imagenes chicas en relacion a grandes

	// Se debe compensar?
	if (usar_comp_max_wh)
	{
		s = bmp->w * bmp->h; // superficie (para compensacion)
		s = comp_max_wh / s; // valor de compensacion
		if (s < 1) s = 1;
	}
	else
	{
		s = 1;
	}


	for (x =0; x < bmp->w; x += salto_x)
	{
		for (y =0; y < bmp->h; y += salto_y)
		{
			// agregar_color(imagenes.pal[getpixel(imagenes.bmp, x, y)], s);
			kfixpal_agregar_color(pal[bmp->line[y][x]], s, tolerancia_rgb); // version con acceso a RAM directa, solo 8 bpp
		}
	}
}



// ---------------------------------------------------------
// Algoritmo de compensacion
//
// DEBE ser llamado antes de cuantificar, con TODAS las imagenes que van a ser cuantificadas
// No es necesario hacerlo si no importa la correcion de color (las imagenes no seran tan precisas en el color)
//
// Obtiene una imagen, calcula su superficie, y la descarga
// Si la superficie es > que comp_max_wh, reemplaza comp_max_wh  con el nuevo valor
//
// Recibe el bitmap como parametro
// ---------------------------------------------------------
void kfixpal_calc_sup_compensacion_bmp(BITMAP *bmp)
{
	int s = 0;
	s = bmp->w * bmp->h; // superficie
	if (s > comp_max_wh) comp_max_wh = s;
}

// ---------------------------------------------------------
// Helper para el algoritmo de compensacion
// Obtiene una imagen, calcula su superficie, y la descarga
// Si la superficie es > que comp_max_wh, reemplaza comp_max_wh  con el nuevo valor
//
// esta funciona con una imagen desde disco
// ---------------------------------------------------------
void kfixpal_calcular_superficie_de_compensacion(char *file)
{
	BITMAP *bmp = NULL;


	bmp = load_bitmap(file, NULL);
	if (bmp == NULL) raise_error("ERROR CATASTROFICO: No se pudo cargar '%s'!\nCan't load the file!\n", file);

	kfixpal_calc_sup_compensacion_bmp(bmp);

	destroy_bitmap(bmp);
	bmp = NULL;
}

// ---------------------------------------------------------
// Convierte una imagen a la paleta de colores fabricada en pal256out
// Pasarle el bitmap y la paleta origen, destino
// EL BITMAP SERA MODIFICADO PARA REFLEJAR LOS CAMBIOS!!
// ---------------------------------------------------------
void kfixpal_convertir_a_paleta(BITMAP *bmp, PALETTE pal, PALETTE pal256out)
{
	int x,y; // contadores varios
	int newcol; // cache de colores

	for (x =0; x < bmp->w; x++)
	{
		for (y =0; y < bmp->h; y++)
		{
			// obtener color de la paleta propia del bitmap

			// col = getpixel(bmp, x, y);

			// buscar el mas parecido en el destino
			newcol = kfixpal_buscar_color_mas_proximo(pal[bmp->line[y][x]], pal256out);

			// putpixel(bmp, x,y, newcol);
			bmp->line[y][x] = newcol;

		}
		// feedback visual... porcentaje...
		//	textprintf(screen, font, 0,text_height(font),white,"%%%3d", (x*100)/bmp->w);
	}
}
