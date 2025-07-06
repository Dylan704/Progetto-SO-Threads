//
// Created by dylan on 28/01/24.
//

#ifndef PLANTS_VS_FROGGER_THREAD_PERSONAGGI_H
#define PLANTS_VS_FROGGER_THREAD_PERSONAGGI_H
#include "Struct_enum.h"

void produzione(Character pg);

void consumazione(Character* pg);

void stampa_sprite_personaggio(WINDOW* gioco, int y_inizio, int x_inizio, int rows, int cols, char sprite[rows][cols+1]);

void* pos_rana();

void* pos_coccodrillo(Argument_coccodrillo* argument_coccodrillo);

void* timer_partita(int* Argument_timer);

void* pos_proiettile(Argument_proiettile* argument_proiettile);

void* pos_pianta(int *Argument);

void gen_dir_coccodrilli(int num_corsie, int dir_coccodrilli[num_corsie]);
void gen_vel_coccodrilli(int num_corsie, int possibilita, int velocita_corsia[num_corsie], int velocita_coccodrilli[possibilita]);
void gen_coccodrilli_ogni_manche(int num_corsie, int num_coccodrilli_max_corsia,
                                 bool coccodrilli_liberi[num_corsie][num_coccodrilli_max_corsia], Character coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                                 pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia], int dir_coccodrilli[num_corsie], int velocita_corsia[num_corsie],
                                 int possibilita_velocita, int velocita_coccodrilli[possibilita_velocita]);

void gen_continua_coccodrilli(int difficolta,
                              int num_corsie, int num_coccodrilli_max_corsia, int cooldown_corsie[num_corsie],
                              bool coccodrilli_liberi[num_corsie][num_coccodrilli_max_corsia], Character coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                              pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia], int dir_coccodrilli[num_corsie], int velocita_corsia[num_corsie],
                              int possibilita_velocita, int velocita_coccodrilli[possibilita_velocita], Character timer);

void gen_continua_piante(int num_max_piante, int* num_piante_in_game, int *cooldown_generazione_piante, bool piante_libere[num_max_piante],
                         Character pianta[num_max_piante], pthread_t thread_piante[num_max_piante], Character timer);

void total_kill(int num_corsie, int num_coccodrilli_max_corsia, pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia], bool coccodrilli_liberi[num_corsie][num_coccodrilli_max_corsia],
                int num_max_proiettili_rana, pthread_t thread_proiettili_rana[num_max_proiettili_rana], bool slot_liberi_proiettili_rana[num_max_proiettili_rana],
                int num_max_piante, int num_max_proiettili_piante, pthread_t thread_proiettili_pianta[num_max_piante][num_max_proiettili_piante], bool slot_liberi_proiettili_pianta[num_max_piante][num_max_proiettili_piante],
                bool piante_libere[num_max_piante], pthread_t thread_piante[num_max_piante], pthread_t thread_timer);

void reset_globale_manche(WINDOW* gioco, int num_corsie, int num_coccodrilli_max_corsia, pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia], bool coccodrilli_liberi[num_corsie][num_coccodrilli_max_corsia],
                          int num_max_proiettili_rana, pthread_t thread_proiettili_rana[num_max_proiettili_rana], bool slot_liberi_proiettili_rana[num_max_proiettili_rana],
                          int num_max_piante, int num_max_proiettili_piante, pthread_t thread_proiettili_pianta[num_max_piante][num_max_proiettili_piante], bool slot_liberi_proiettili_pianta[num_max_piante][num_max_proiettili_piante],
                          int cooldown_corsie[num_corsie], int *num_piante_in_game, bool piante_libere[num_max_piante], int* cooldown_generazione_piante,
                          pthread_t thread_timer, pthread_t thread_piante[num_max_piante]);

bool collisione_proj_piante_rana(int num_max_piante, int num_max_proiettili_piante, bool slot_liberi_proiettili_pianta[num_max_piante][num_max_proiettili_piante],
                                 Character proiettili_pianta[num_max_piante][num_max_proiettili_piante], Character rana);

Stati_rana_endgame collisione_rana_tana_o_muro(int num_tane, Character rana, Character tana[num_tane], bool tana_aperta[num_tane]);

bool rana_morte_acqua(int num_corsie, int num_coccodrilli_corsia, Character rana, bool coccodrilli_liberi[num_corsie][num_coccodrilli_corsia],
                      Character coccodrilli[num_corsie][num_coccodrilli_corsia]);

bool rana_morte_pianta(int num_max_piante, bool piante_libere[num_max_piante], Character rana, Character pianta[num_max_piante]);

void collisione_proj_pianta_proj_rana(int num_max_proiettili_rana, bool slot_liberi_proiettili_rana[num_max_proiettili_rana], Character proiettili_rana[num_max_proiettili_rana],
                                      int num_max_piante, int num_max_proiettili_pianta, Character proiettili_pianta[num_max_piante][num_max_proiettili_pianta], bool slot_liberi_proiettili_pianta[num_max_piante][num_max_proiettili_pianta],
                                      pthread_t thread_proiettili_rana[num_max_proiettili_rana],  pthread_t thread_proiettili_pianta[num_max_piante][num_max_proiettili_pianta],
                                      int *punteggio, int id1, int id2 );

void collisione_proj_rana_proj_piante(int id, int num_max_piante, int num_max_proiettili_pianta, int num_max_proiettili_rana, bool slot_liberi_proiettili_pianta[num_max_piante][num_max_proiettili_pianta],
                                      bool slot_liberi_proiettili_rana[num_max_proiettili_rana], pthread_t thread_proiettili_rana[num_max_proiettili_rana], Character proiettili_rana[num_max_proiettili_rana],
                                      Character proiettili_pianta[num_max_piante][num_max_proiettili_pianta], pthread_t thread_proiettili_pianta[num_max_piante][num_max_proiettili_pianta], int* punteggio);

void collisione_proj_rana_coccodrilli(int id, int num_corsie, int num_coccodrilli_max_corsia, bool coccodrilli_liberi[num_corsie][num_coccodrilli_max_corsia], Character coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                                      int velocita_corsia[num_corsie], int dir_coccodrilli[num_corsie], pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                                      int num_max_proiettili_rana, bool slot_liberi_proiettili_rana[num_max_proiettili_rana], Character proiettili_rana[num_max_proiettili_rana],
                                      pthread_t thread_proiettili_rana[num_max_proiettili_rana], int* punteggio);

void collisione_proj_pianta(int id, int num_max_piante, int num_max_proiettili_rana, int* punteggio, bool piante_libere[num_max_piante], Character proiettili_rana[num_max_proiettili_rana],
                            bool slot_liberi_proiettili_rana[num_max_proiettili_rana], pthread_t thread_proiettili_rana[num_max_proiettili_rana], int *num_piante_in_game, pthread_t thread_piante[num_max_piante],
                            Character pianta[num_max_piante]);

void collisione_coccodrilli_proj_rana(int id1, int id2, int num_corsie, int num_coccodrilli_max_corsia, Character coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                                      int velocita_corsia[num_corsie], int dir_coccodrilli[num_corsie], pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                                      int num_max_proiettili_rana, bool slot_liberi_proiettili_rana[num_max_proiettili_rana], Character proiettili_rana[num_max_proiettili_rana],
                                      pthread_t thread_proiettili_rana[num_max_proiettili_rana], int* punteggio);

void controllo(WINDOW* gioco, Difficolta difficolta);

void gameplay(WINDOW* gioco, Difficolta difficolta);

#endif //PLANTS_VS_FROGGER_THREAD_PERSONAGGI_H
