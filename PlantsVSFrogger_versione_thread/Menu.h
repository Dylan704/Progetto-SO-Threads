//
// Created by dylan on 28/01/24.
//

#ifndef PLANTS_VS_FROGGER_THREAD_MENU_H
#define PLANTS_VS_FROGGER_THREAD_MENU_H

#include "Struct_enum.h"

void caricamento_sprite(char nome_file[DIM_NOME_FILE], int rows, int cols, char sprite_titolo[rows][cols+1]);
void stampa_sprite(WINDOW* gioco, int rows, int cols, char sprite_titolo[rows][cols+1], int inizio_y, int inizio_x);
void pulisci_sprite(int rows, int cols, char sprite_titolo[rows][cols+1]);

WINDOW* inizializzazione_menu(WINDOW* gioco, int rows, int cols, char sprite_titolo [rows][cols+1], int* scelta);
void creazione_tasti(WINDOW* gioco, int*scelta, int posizione_y_tasto1, int posizione_y_tasto2, int lung_testo1, int lung_testo2, char testo1[], char testo2[]);
void selezione_tasto(WINDOW* gioco, WINDOW *tasto_up, WINDOW* tasto_down, int* scelta, int lung_testo1, int lung_testo2, char testo1[], char testo2[]);

void bestiario(WINDOW* gioco);
void selezione_tasti_bestiario(WINDOW* gioco, WINDOW* text, WINDOW *tasto_sx, WINDOW* tasto_cntr, WINDOW *tasto_dx, char testo_sx[], char testo_cntr[], char testo_dx[]);
void grafica_selezione_tasto(WINDOW *tasto_selezionato, WINDOW* tasto1, WINDOW* tasto2, char testo_t_selezionato[], char testo1[], char testo2[]);


void* input();

void* pos_coccodrilli_guida();
void* pos_rana_guida();
void* pos_proiettile_pianta_guida();

#endif //PLANTS_VS_FROGGER_THREAD_MENU_H
