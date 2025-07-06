//
// Created by dylan on 28/01/24.
//

#include "Struct_enum.h"
#include "Personaggi.h"
#include "Area_gioco.h"

///SEMAFORI GLOBALI
sem_t sem_liberi, sem_occupati;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

///BUFFER GLOBALE
Queue buffer;

///ARGUMENT GLOBALI
Argument_coccodrillo argumentCoccodrillo[NUM_CORSIE][NUM_COCCODRILLI_MAX_CORSIA];
Argument_proiettile argumentProiettile_rana[NUM_MAX_PROIETTILI_RANA];
Argument_proiettile argumentProiettile_pianta[NUM_MAX_PIANTE_DIFFICILE][NUM_MAX_PROIETTILI_PIANTE];

int Argument_timer=TEMPO_MASSIMO;
int Argument_pianta[NUM_MAX_PIANTE_DIFFICILE];

//FUNZIONI PRODUZIONE E CONSUMAZIONE

/**
 * Funzione produzione nel buffer
 *
 * @param pg character da produrre
 */
void produzione(Character pg)
{
    sem_wait(&sem_liberi);

    pthread_mutex_lock(&mutex);
    buffer.coda[buffer.i_scrivi]=pg;

    buffer.i_scrivi= (buffer.i_scrivi+1) % DIM_BUFFER;
    pthread_mutex_unlock(&mutex);

    sem_post(&sem_occupati);
}

/**
 * Funzione consumazione nel buffer
 *
 * @param pg character da consumare
 */
void consumazione(Character* pg)
{

    sem_wait(&sem_occupati);

    (*pg) = buffer.coda[buffer.i_leggi];

    buffer.i_leggi = (buffer.i_leggi+1) % DIM_BUFFER;

    sem_post(&sem_liberi);

}

/**
 * Funzione che stampa gli sprite degli oggetti che usiamo in gioco
 *
 * @param gioco finestra di gioco
 * @param y_inizio coordinata iniziale stampa
 * @param x_inizio coordinata iniziale stampa
 * @param rows righe sprite
 * @param cols colonne sprite
 * @param sprite matrice di char
 */
void stampa_sprite_personaggio(WINDOW* gioco, int y_inizio, int x_inizio, int rows, int cols, char sprite[rows][cols+1])
{
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            if( (x_inizio+j>=0 && x_inizio+j<LARGHEZZA_SCHERMO-1))
            {
                mvwaddch(gioco, y_inizio+i, x_inizio+j, sprite[i][j]);
            }
        }
    }
}

/**
 * Funzione thread che genera l'aggiornamento della posizione della rana in gioco
 */
void* pos_rana()
{
    nodelay(stdscr, 1);

    Character rana;

    int cooldown=0;

    //booleano che indica la possibilita di un nuovo sparo
    _Bool nuovo_sparo=true;

    rana.id=RANA;

    //variabile di input
    int c;
    c=NO_CHOICE_INPUT;

    while(true)
    {
        c = (int)getch();

        rana.signal = CONTINUE;

        //con wasd indichiamo lo spostamento nelle 4 direzioni della rana
        switch(c)
        {
            case 'w':
                rana.x = 0;
                rana.y = - 2;
                break;
            case 's':
                rana.x = 0;
                rana.y = + 2;
                break;
            case 'a':
                rana.y = 0;
                rana.x = - 1;
                break;
            case 'd':
                rana.y = 0;
                rana.x = + 1;
                break;
            case 'q':
                rana.y=0;
                rana.x=0;
                rana.signal=USCITA;
                break;
            case ' ':
                //con spazio spariamo uno dei due proiettili disponibili in contemporanea alla rana
                if(nuovo_sparo==true)
                {
                    //mando segnale di sparo
                    rana.signal=SPARO;

                    ///reset cooldown
                    cooldown=0;
                    nuovo_sparo=false;
                }
                break;
            default:
                rana.y=0;
                rana.x=0;
                break;
        }
        //essendoci nodelay se non si legge un input la rana non esegue movimenti
        c=NO_CHOICE_INPUT;

        //ricarica proiettile ogni 20000 cicli
        cooldown++;
        if( (cooldown % 20000) == 0)
        {
            nuovo_sparo=true;
        }

        //produzione posizione rana
        produzione(rana);
    }
}

/**
 * Funzione che aggiorna le posizioni di ogni coccodrillo in base alla direzione e velocita della corsia in cui si trova
 *
 * @param argument_coccodrillo
 */
void* pos_coccodrillo(Argument_coccodrillo* argument_coccodrillo)
{
    //inizializzazione campi struttura coccodrillo
    Character coccodrillo;
    int direzione_coccodrillo=argument_coccodrillo->dir;
    int velocita=argument_coccodrillo->velocita;

    coccodrillo.id=COCCODRILLO;

    coccodrillo.id_riga = argument_coccodrillo->id_coccodrillo_corsia;
    coccodrillo.id_colonna = argument_coccodrillo->id_coccodrillo_numero;

    coccodrillo.status = argument_coccodrillo->status_coccodrillo;
    coccodrillo.signal = EMERSO;

    coccodrillo.tempo_rimasto=0;

    //counter immersione e successiva emersione coccodrilli CATTIVI
    int counter_immersione=0;
    int tempo_immersione=25 + rand()%(70-25+1);

    int counter_emersione=0;
    int tempo_emersione=25 + rand()%(70-25+1);

    while(true)
    {

        if(coccodrillo.status==CATTIVO)
        {
            //aggiornamento counter
            if(coccodrillo.signal==EMERSO || coccodrillo.signal==PROMPT_IMMERSIONE)
            {
                counter_immersione++;
            }
            else if(coccodrillo.signal==IMMERSO)
            {
                counter_emersione++;
            }
        }

        //aggiornamento spostamento coccodrilli
        coccodrillo.x=direzione_coccodrillo;
        coccodrillo.y=0;


        if(coccodrillo.status==CATTIVO)
        {
            //segnale prompt immersione 20 cicli prima che il coccodrillo si immerga
            if(coccodrillo.signal==EMERSO && counter_immersione==(tempo_immersione-20))
            {
                coccodrillo.signal=PROMPT_IMMERSIONE;
            }

            if(coccodrillo.signal==PROMPT_IMMERSIONE && counter_immersione==tempo_immersione)
            {
                coccodrillo.signal=IMMERSO;

                counter_immersione=0;
                tempo_immersione = 25 + rand()%(70-25+1);
            }
            //finito il cooldown casuale il coccodrillo riemerge
            if(coccodrillo.signal==IMMERSO && counter_emersione==tempo_emersione)
            {
                coccodrillo.signal=EMERSO;
                counter_emersione=0;
                tempo_emersione = 25 + rand()%(70-25+1);
            }
        }

        //produzione posizione coccodrillo
        produzione(coccodrillo);

        //temporizzazione velocita
        usleep(velocita);
    }
}

/**
 * Funzione thread timer partita, diminuisce il tempo ogni secondo
 *
 * @param Argument
 */
void* timer_partita(int* Argument)
{
    //creazione timer
    Character timer;
    timer.id=TIMER;
    timer.tempo_rimasto= (*Argument);
    timer.signal=CONTINUE;
    //produzione timer
    produzione(timer);

    while(true)
    {
        //aggiornamento e scrittura dopo ogni secondo
        sleep(1);
        timer.tempo_rimasto-=1;

        if(timer.tempo_rimasto==0)
        {
            timer.signal=USCITA;
        }

        produzione(timer);
    }
}

/**
 * Funzione thread che permette il movimento dei proiettili in base al personaggio che ha compiuto lo sparo
 *
 * @param argument_proiettile
 */
void* pos_proiettile(Argument_proiettile* argument_proiettile)
{
    //creazione struttura proiettile da produrre
    Character proiettile;

    int direzione_proiettile=argument_proiettile->dir_proiettile;

    proiettile.signal=CONTINUE;
    proiettile.id=PROIETTILE;
    proiettile.personaggio_signal = argument_proiettile->personaggio_sparo;
    proiettile.id_riga = argument_proiettile->id_proiettile1;
    proiettile.id_colonna = argument_proiettile->id_proiettile2;

    while(true)
    {
        //aggiornamento direzione proiettile
        proiettile.y = direzione_proiettile;
        proiettile.x = 0;

        //produzione posizione proiettile
        produzione(proiettile);

        //temporizzazione
        usleep(380000);
    }
}

/**
 * Funzione processi piante che serve per temporizzare lo sparo di ciascuna
 *
 * @param Argument
 */
void* pos_pianta(int *Argument)
{
    Character pianta;
    pianta.id=PIANTA;
    pianta.signal=CONTINUE;
    pianta.id_riga=(*Argument);

    while(true)
    {
        usleep(5000000);
        pianta.signal=SPARO;

        //produzione della poszione della pianta
        produzione(pianta);

        pianta.signal=CONTINUE;
    }
}

/**
 * Funzione che genera casualmente la direzione dei coccodrilli, fa pero' in modo che
 * 3 corsie non possano avere la stessa direzione perche' renderebbe sbilanciato il gioco
 *
 * @param num_corsie numero di corsie
 * @param dir_coccodrilli vettore direzione dei coccodrilli
 */
void gen_dir_coccodrilli(int num_corsie, int dir_coccodrilli[num_corsie])
{
    int n;

    for (int i = 0; i < num_corsie; ++i)
    {
        if(i>=2)
        {
            if(dir_coccodrilli[i-1]==dir_coccodrilli[i-2])
            {
                n=-dir_coccodrilli[i-1];
            }
            else
            {
                n=rand()%2;
                if(n==0)
                {
                    n = -1;
                }
            }
        }
        else
        {
            n=rand()%2;
            if(n==0)
            {
                n = -1;
            }
        }

        dir_coccodrilli[i]=n;

    }
}
/**
 * Funzione che genera casualmente la velocita di ogni corsia, due corsie consecutive non possono avere la stessa velocita'
 * ai fini della giocabilita'
 *
 * @param num_corsie numero corsie
 * @param possibilita numero di velocita' diverse assegnabili ad una corsia
 * @param velocita_corsia vettore che indica la velocita' di ogni corsia
 * @param velocita_coccodrilli vettore che contiene le velocita' da assegnare
 */
void gen_vel_coccodrilli(int num_corsie, int possibilita, int velocita_corsia[num_corsie], int velocita_coccodrilli[possibilita])
{
    for (int i = 0; i < num_corsie; ++i)
    {
        if(i!=0)
        {
            do
            {
                velocita_corsia[i] = velocita_coccodrilli[rand() % possibilita];
            }while(velocita_corsia[i] == velocita_corsia[i-1]);
        }else
        {
            velocita_corsia[i] = velocita_coccodrilli[rand() % possibilita];
        }
    }
}

/**
 * Funzione che genera una serie di coccodrilli buoni all'inizio di ogni manche in posizioni casuali in modo che non si sovrappongano
 *
 * @param slot_libero_coccodrillo variabile ausiliare per scegliere lo slot della matrice coccodrilli da usare
 * @param num_corsie numero corsie in gioco
 * @param num_coccodrilli_max_corsia numero coccodrilli massimi presenti in una corsia
 * @param coccodrilli_liberi matrice booleana che indica quali slot della matrice coccodrilli non sono in uso
 * @param coccodrilli matrice di personaggi coccodrillo
 * @param thread_coccodrilli matrice di thread coccodrilli
 * @param dir_coccodrilli vettore direzione coccodrilli per ogni corsa
 * @param velocita_corsia vettore velocita per ogni corsia
 * @param possibilita_velocita diverse velocita presenti in gioco nelle corsie
 * @param velocita_coccodrilli array che contiene le possibili velocita delle corsie
 */
void gen_coccodrilli_ogni_manche(int num_corsie, int num_coccodrilli_max_corsia,
                                 bool coccodrilli_liberi[num_corsie][num_coccodrilli_max_corsia], Character coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                                 pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia], int dir_coccodrilli[num_corsie], int velocita_corsia[num_corsie],
                                 int possibilita_velocita, int velocita_coccodrilli[possibilita_velocita])
{
    int slot_libero_coccodrillo;
    int num_coccodrilli_da_generare;


    for (int i = 0; i < num_corsie; ++i)
    {
        //per ogni corsia genero un tot di coccodrilli in base alla velocita
        if(velocita_corsia[i]==velocita_coccodrilli[0])
        {
            num_coccodrilli_da_generare = 7;
        }
        else if(velocita_corsia[i]==velocita_coccodrilli[1])
        {
            num_coccodrilli_da_generare = 5;
        }
        else
        {
            num_coccodrilli_da_generare = 4;
        }


        for (int h = 0; h < num_coccodrilli_da_generare; ++h)
        {
            //controllo tramite la variabile ausiliare quale slot della matrice coccodrilli non e' in uso e la scelgo
            slot_libero_coccodrillo = -1;

            for (int j = 0; j < num_coccodrilli_max_corsia; ++j)
            {
                if (coccodrilli_liberi[i][j] == true)
                {
                    slot_libero_coccodrillo = j;
                    break;
                }
            }

            //se la trovo
            if (slot_libero_coccodrillo != -1)
            {
                //lo slot diventa occupato
                coccodrilli_liberi[i][slot_libero_coccodrillo] = false;

                //e genero le sue coordinate in modo che non si sovrappongano piu' coccodrilli
                int in_range;

                coccodrilli[i][slot_libero_coccodrillo].y = ALTEZZA_SCHERMO - ALTEZZA_MARCIAPIEDE_RANA - (ALTEZZA_COCCODRILLO * (i + 1)) - 1;
                coccodrilli[i][slot_libero_coccodrillo].signal = EMERSO;
                coccodrilli[i][slot_libero_coccodrillo].status = BUONO;

                do {
                    in_range = false;
                    coccodrilli[i][slot_libero_coccodrillo].x = 1 + (rand() % (LARGHEZZA_SCHERMO - 1));

                    for (int k = 0; k < slot_libero_coccodrillo; ++k)
                    {
                        if ((coccodrilli[i][k].x > coccodrilli[i][slot_libero_coccodrillo].x - LARGHEZZA_COCCODRILLO - 4) &&
                            (coccodrilli[i][k].x < coccodrilli[i][slot_libero_coccodrillo].x + LARGHEZZA_COCCODRILLO + 4))
                        {
                            in_range = true;
                        }
                    }
                } while (in_range);

                //genero il thread coccodrillo
                argumentCoccodrillo[i][slot_libero_coccodrillo].dir = dir_coccodrilli[i];
                argumentCoccodrillo[i][slot_libero_coccodrillo].id_coccodrillo_corsia = i;
                argumentCoccodrillo[i][slot_libero_coccodrillo].id_coccodrillo_numero = slot_libero_coccodrillo;
                argumentCoccodrillo[i][slot_libero_coccodrillo].velocita = velocita_corsia[i];
                argumentCoccodrillo[i][slot_libero_coccodrillo].status_coccodrillo = coccodrilli[i][slot_libero_coccodrillo].status;

                if (pthread_create(&thread_coccodrilli[i][slot_libero_coccodrillo], NULL,(void *(*)(void *)) &pos_coccodrillo, &argumentCoccodrillo[i][slot_libero_coccodrillo]) != 0)
                {
                    perror("Errore durante la creazione del thread");
                    endwin();
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

/**
 *
 * @param difficolta difficolta scelta dal player
 * @param num_corsie numero corsie in gioco
 * @param num_coccodrilli_max_corsia numero coccodrilli massimi presenti in una corsia
 * @param cooldown_corsie tempo da attendere per far spawnare un altro coccodrillo
 * @param coccodrilli_liberi matrice booleana che indica quali slot della matrice coccodrilli non sono in uso
 * @param coccodrilli matrice di personaggi coccodrillo
 * @param thread_coccodrilli matrice di thread coccodrill
 * @param dir_coccodrilli vettore direzione coccodrilli per ogni corsa
 * @param velocita_corsia vettore velocita per ogni corsia
 * @param possibilita_velocita diverse velocita presenti in gioco nelle corsie
 * @param velocita_coccodrilli array che contiene le possibili velocita delle corsie
 * @param timer timer di gioco, utile per il cooldown coccodrilli
 */
void gen_continua_coccodrilli(int difficolta,
                              int num_corsie, int num_coccodrilli_max_corsia, int cooldown_corsie[num_corsie],
                              bool coccodrilli_liberi[num_corsie][num_coccodrilli_max_corsia], Character coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                              pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia], int dir_coccodrilli[num_corsie], int velocita_corsia[num_corsie],
                              int possibilita_velocita, int velocita_coccodrilli[possibilita_velocita], Character timer)
{
    //valore di probabilita' spawn coccodrilli verdi, dipende dalla difficolta'
    int probabilita_coccodrilli_verdi;
    int slot_libero_coccodrillo;
    int status_coccodrillo;

    if(difficolta == NORMALE)
    {
        probabilita_coccodrilli_verdi = PROBABILITA_COCCODRILLI_VERDI_NORMALE;
    }
    else
    {
        probabilita_coccodrilli_verdi = PROBABILITA_COCCODRILLI_VERDI_DIFFICILE;
    }

    //ciclo per ogni corsia
    for (int i = 0; i < num_corsie; ++i)
    {
        //per ogni corsia controllo se il cooldown e' rispettato
        if(cooldown_corsie[i]==(TEMPO_MASSIMO-timer.tempo_rimasto))
        {
            //seleziono uno slot libero
            slot_libero_coccodrillo=-1;
            for (int j = 0; j < num_coccodrilli_max_corsia; ++j)
            {
                if( coccodrilli_liberi[i][j] == true)
                {
                    slot_libero_coccodrillo=j;
                }
            }

            if(slot_libero_coccodrillo!=-1)
            {
                //occupazione slot
                coccodrilli_liberi[i][slot_libero_coccodrillo]=false;

                //generazione status coccodrillo
                status_coccodrillo = rand()%100;
                if( status_coccodrillo<=probabilita_coccodrilli_verdi)
                {
                    status_coccodrillo=BUONO;
                }else
                {
                    status_coccodrillo=CATTIVO;
                }

                //generazione y coccodrillo
                coccodrilli[i][slot_libero_coccodrillo].y=ALTEZZA_SCHERMO-ALTEZZA_MARCIAPIEDE_RANA-(ALTEZZA_COCCODRILLO*(i+1))-1;

                //check direzione
                if(dir_coccodrilli[i]==-1)
                {
                    coccodrilli[i][slot_libero_coccodrillo].x = LARGHEZZA_SCHERMO - 1;
                }
                else if(dir_coccodrilli[i]==+1)
                {
                    coccodrilli[i][slot_libero_coccodrillo].x = 1 - LARGHEZZA_COCCODRILLO + 1;;
                }

                //inizializzazione argument
                argumentCoccodrillo[i][slot_libero_coccodrillo].dir = dir_coccodrilli[i];
                argumentCoccodrillo[i][slot_libero_coccodrillo].id_coccodrillo_corsia = i;
                argumentCoccodrillo[i][slot_libero_coccodrillo].id_coccodrillo_numero = slot_libero_coccodrillo;
                argumentCoccodrillo[i][slot_libero_coccodrillo].velocita = velocita_corsia[i];
                argumentCoccodrillo[i][slot_libero_coccodrillo].status_coccodrillo = status_coccodrillo;


                //creazione thread coccodrilli
                if (pthread_create(&thread_coccodrilli[i][slot_libero_coccodrillo], NULL, (void *(*)(void *)) &pos_coccodrillo, &argumentCoccodrillo[i][slot_libero_coccodrillo]) != 0)
                {
                    perror("Errore durante la creazione del thread");
                    endwin();
                    exit(EXIT_FAILURE);
                }
            }

            //aggiornamento cooldown in base alla velocita', piu' i coccodrilli sono veloci meno frequentemente spawnano
            if(velocita_corsia[i]==velocita_coccodrilli[0])
            {
                cooldown_corsie[i] += 7 + rand()%(10-7+1);
            }
            else if(velocita_corsia[i]==velocita_coccodrilli[1])
            {
                cooldown_corsie[i] += 4 + rand()%(7-4+1);
            }
            else
            {
                cooldown_corsie[i] += 3 + rand()%(5-3+1);
            }
        }
    }
}

/**
 * Funzione che permette il continuo spawn di piante fino al numero massimo permesso in gioco
 *
 * @param num_max_piante numero massimo di piante in gioco, dipende dalla difficolta'
 * @param num_piante_in_game numero di piante attualmente in gioco
 * @param cooldown_generazione_piante cooldown dopo il quale si puo' generare una nuova pianta
 * @param piante_libere vettore booleano che indica quali slot sono liberi da utilizzare
 * @param pianta vettore personaggi pianta
 * @param thread_piante vettore thread piante
 */
void gen_continua_piante(int num_max_piante, int* num_piante_in_game, int *cooldown_generazione_piante, bool piante_libere[num_max_piante],
                         Character pianta[num_max_piante], pthread_t thread_piante[num_max_piante], Character timer)
{
    int slot_pianta_libera;

    //check cooldown
    if ((*cooldown_generazione_piante) == (TEMPO_MASSIMO-timer.tempo_rimasto))
    {
        //check numero di piante in game
        if ((*num_piante_in_game) < num_max_piante)
        {
            //selezione slot libero
            slot_pianta_libera = -1;

            for (int i = 0; i < num_max_piante; ++i)
            {
                if (piante_libere[i] == true)
                {
                    slot_pianta_libera = i;
                    break;
                }
            }

            if (slot_pianta_libera != -1)
            {
                //generazione coordinate casuali in modo che le piante non si sovrappongano
                piante_libere[slot_pianta_libera] = false;

                bool in_range_piante;
                pianta[slot_pianta_libera].y = ALTEZZA_SCHERMO - ALTEZZA_MARCIAPIEDE_RANA - ALTEZZA_LAGHETTO - ALTEZZA_MARCIAPIEDE_PIANTE + 1;
                pianta[slot_pianta_libera].signal = CONTINUE;

                do {
                    in_range_piante = false;
                    pianta[slot_pianta_libera].x = LARGHEZZA_PIANTA + (rand() % ((LARGHEZZA_SCHERMO - LARGHEZZA_PIANTA) - 1 - LARGHEZZA_PIANTA));

                    for (int k = 0; k < slot_pianta_libera; ++k)
                    {
                        if (piante_libere[k] == false)
                        {
                            if ((pianta[slot_pianta_libera].x > pianta[k].x - LARGHEZZA_PIANTA - LARGHEZZA_RANA) &&
                                (pianta[slot_pianta_libera].x < pianta[k].x + LARGHEZZA_PIANTA + LARGHEZZA_RANA))
                            {
                                in_range_piante = true;
                            }
                        }
                    }
                } while (in_range_piante);

                //creazione thread piante
                Argument_pianta[slot_pianta_libera] = slot_pianta_libera;

                if (pthread_create(&thread_piante[slot_pianta_libera], NULL, (void *(*)(void *))&pos_pianta, &Argument_pianta[slot_pianta_libera]) != 0)
                {
                    perror("Errore durante la creazione del thread");
                    endwin();
                    exit(EXIT_FAILURE);
                }

            }
            (*num_piante_in_game)++;
        }
        (*cooldown_generazione_piante) += 4 + rand()%(7-4+1);
    }

}
/**
 * Funzione che killa tutti i processi attivi nel programma
 * @param num_corsie numero corsie in gioco
 * @param num_coccodrilli_max_corsia numero coccdrilli massimo per ogni corsia
 * @param thread_coccodrilli matrice thread coccodrilli
 * @param coccodrilli_liberi schermo booleano che indica gli slot coccodrilli
 * @param num_max_proiettili_rana numero massimo proiettili che la rana puo sparare
 * @param thread_proiettili_rana vettore thread proiettili rana
 * @param slot_liberi_proiettili_rana schermo booleano proiettili rana
 * @param num_max_piante numero massimo piante in gioco
 * @param num_max_proiettili_piante numero massimo proiettili per ogni pianta
 * @param thread_proiettili_pianta matrice thread proiettili pianta
 * @param slot_liberi_proiettili_pianta schermo booleano per i proiettili pianta
 * @param piante_libere array che indica slot liberi piante
 * @param thread_piante array thread piante
 * @param thread_timer thread del timer unico di gioco
*/
void total_kill(int num_corsie, int num_coccodrilli_max_corsia, pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia], bool coccodrilli_liberi[num_corsie][num_coccodrilli_max_corsia],
                int num_max_proiettili_rana, pthread_t thread_proiettili_rana[num_max_proiettili_rana], bool slot_liberi_proiettili_rana[num_max_proiettili_rana],
                int num_max_piante, int num_max_proiettili_piante, pthread_t thread_proiettili_pianta[num_max_piante][num_max_proiettili_piante], bool slot_liberi_proiettili_pianta[num_max_piante][num_max_proiettili_piante],
                bool piante_libere[num_max_piante], pthread_t thread_piante[num_max_piante], pthread_t thread_timer)
{
    //kill  coccodrilli
    for (int i = 0; i < num_corsie; ++i)
    {
        for (int j = 0; j < num_coccodrilli_max_corsia; ++j)
        {
            if(coccodrilli_liberi[i][j]==false)
            {
                pthread_cancel(thread_coccodrilli[i][j]);
                pthread_join(thread_coccodrilli[i][j], NULL);
            }
        }
    }

    //kill proiettili rana
    for (int i = 0; i < num_max_proiettili_rana; ++i)
    {
        if(slot_liberi_proiettili_rana[i]==false)
        {
            pthread_cancel(thread_proiettili_rana[i]);
            pthread_join(thread_proiettili_rana[i], NULL);
        }
    }


    //kill piante e proiettili
    for (int i = 0; i < num_max_piante; ++i)
    {
        if(piante_libere[i]==false)
        {
            pthread_cancel(thread_piante[i]);
            pthread_join(thread_piante[i], NULL);
        }


        for (int j = 0; j < num_max_proiettili_piante; ++j)
        {
            if(slot_liberi_proiettili_pianta[i][j]==false)
            {
                pthread_cancel(thread_proiettili_pianta[i][j]);
                pthread_join(thread_proiettili_pianta[i][j], NULL);
            }
        }

    }

    //kill timer ogni manche
    pthread_cancel(thread_timer);
    pthread_join(thread_timer, NULL);

}

/**
 * Funzione che viene chiamata al termine di ogni manche che esegue la total kill e resetta tutti gli schermi booleani
 * insieme agli eventuali cooldown
 *
 * @param num_corsie numero corsie in gioco
 * @param num_coccodrilli_max_corsia numero coccdrilli massimo per ogni corsia
 * @param thread_coccodrilli matrice thread coccodrilli
 * @param coccodrilli_liberi schermo booleano che indica gli slot coccodrilli
 * @param num_max_proiettili_rana numero massimo proiettili che la rana puo sparare
 * @param thread_proiettili_rana vettore thread proiettili rana
 * @param slot_liberi_proiettili_rana schermo booleano proiettili rana
 * @param num_max_piante numero massimo piante in gioco
 * @param num_max_proiettili_piante numero massimo proiettili per ogni pianta
 * @param thread_proiettili_pianta matrice thread proiettili pianta
 * @param slot_liberi_proiettili_pianta schermo booleano per i proiettili pianta
 * @param cooldown_corsie array che indica il cooldown dello spawn dei coccodrilli per ogni corsia
 * @param num_piante_in_game numero di piante attualmente in gioco
 * @param piante_libere array che indica slot liberi piante
 * @param cooldown_generazione_piante intero che indica il cooldown per la generazione di una nuova pianta
 * @param thread_timer thread del timer unico di gioco
 * @param thread_piante array thread piante
*/
void reset_globale_manche(WINDOW* gioco, int num_corsie, int num_coccodrilli_max_corsia, pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia], bool coccodrilli_liberi[num_corsie][num_coccodrilli_max_corsia],
                          int num_max_proiettili_rana, pthread_t thread_proiettili_rana[num_max_proiettili_rana], bool slot_liberi_proiettili_rana[num_max_proiettili_rana],
                          int num_max_piante, int num_max_proiettili_piante, pthread_t thread_proiettili_pianta[num_max_piante][num_max_proiettili_piante], bool slot_liberi_proiettili_pianta[num_max_piante][num_max_proiettili_piante],
                          int cooldown_corsie[num_corsie], int *num_piante_in_game, bool piante_libere[num_max_piante], int* cooldown_generazione_piante,
                          pthread_t thread_timer, pthread_t thread_piante[num_max_piante])
{

    //kill di tutti i processi in esecuzione
    total_kill(num_corsie, num_coccodrilli_max_corsia, thread_coccodrilli, coccodrilli_liberi,
               num_max_proiettili_rana, thread_proiettili_rana, slot_liberi_proiettili_rana,
               num_max_piante, num_max_proiettili_piante, thread_proiettili_pianta, slot_liberi_proiettili_pianta, piante_libere, thread_piante, thread_timer);

    //-----------------------------------reset slot-----------------------------------

    //reset slot coccodrilli
    for (int j = 0; j < num_corsie; ++j)
    {
        for (int k = 0; k < num_coccodrilli_max_corsia; ++k)
        {
            cooldown_corsie[j]=0;
            coccodrilli_liberi[j][k]=true;
        }
    }
    //reset slot proiettili pianta
    for (int j = 0; j < num_max_piante; ++j)
    {
        for (int k = 0; k < num_max_proiettili_piante; ++k)
        {
            slot_liberi_proiettili_pianta[j][k]=true;
        }
    }

    //reset cooldown pianta
    (*cooldown_generazione_piante)=0;

    //reset slot liberi proiettili rana
    for (int j = 0; j < num_max_proiettili_rana; ++j)
    {
        slot_liberi_proiettili_rana[j]=true;
    }
    //reset slot liberi proiettili pianta
    for (int j = 0; j < num_max_piante; ++j)
    {
        (*num_piante_in_game)=0;
        piante_libere[j]=true;
    }


}

/**
 * Funzione che controlla se la rana è stata colpita da un proiettile di una pianta
 *
 * @param num_max_piante numero massimo di piante in partita
 * @param num_max_proiettili_piante numero massimo di proiettili che può sparare ogni pianta
 * @param slot_liberi_proiettili_pianta matrice di booleani che indicano se lo slot i j è in gioco (false) o meno (true) per i proiettili delle piante
 * @param proiettili_pianta matrice di proiettili di tutte le piante
 * @param rana character dell'utente
 * @return true se la rana rana è stata colpita, false se la rana non è stata colpita
 */
bool collisione_proj_piante_rana(int num_max_piante, int num_max_proiettili_piante, bool slot_liberi_proiettili_pianta[num_max_piante][num_max_proiettili_piante],
                                 Character proiettili_pianta[num_max_piante][num_max_proiettili_piante], Character rana)
{
    //per ogni proittile si controlla se colpisce la rana
    for(int i = 0; i < num_max_piante; ++i)
    {
        for(int j = 0; j < num_max_proiettili_piante; ++j)
        {
            if(slot_liberi_proiettili_pianta[i][j]==false)
            { if((proiettili_pianta[i][j].x>=rana.x-1 && proiettili_pianta[i][j].x<=rana.x+1) &&
                 (proiettili_pianta[i][j].y>=rana.y &&  proiettili_pianta[i][j].y<=rana.y+1))
                {
                    //la rana è stata colpita
                    return true;
                }
            }
        }
    }

    //la rana non è stata colpita
    return false;
}

/**
 * Funzione che controlla se la rana è ancora in gioco o se si deve resettare la manche
 *
 * @param num_tane numero di tane totali
 * @param rana character dell'utente
 * @param tana array di tane
 * @param tana_aperta array di booleani che indicano se la porta i è aperta (true) o chiusa (false)
 * @return restitusce se la rana è in gioco quando si trova nella zona delle tane, se colpisce una tana o se colpisce il muro
 */
Stati_rana_endgame collisione_rana_tana_o_muro(int num_tane, Character rana, Character tana[num_tane], bool tana_aperta[num_tane])
{
    //controllo che la rana sia nella zona delle tane
    if(rana.y<=ALTEZZA_STATS+ALTEZZA_TANE)
    {
        for (int i = 0; i < num_tane; ++i)
        {
            //rana colpisce una tana aperta
            if (tana_aperta[i] == true)
            {
                if ((rana.y >= tana[i].y && rana.y <= tana[i].y + ALTEZZA_TANA - 1) &&
                    (rana.x >= tana[i].x - 1 && rana.x <= tana[i].x + LARGHEZZA_TANA))
                {
                    tana_aperta[i]=false;
                    return RANA_IN_TANA;
                }
            }
        }

        //rana colpisce muro
        return RANA_CONTRO_MURO;
    }

    return RANA_IN_GAME;
}

/**
 * Funzione che controlla se la rana colpisce l'acqua del laghetto o meno
 *
 * @param num_corsie numero di corsie del laghetto
 * @param num_coccodrilli_max_corsia numero massimo di coccodrilli per corsia
 * @param rana character dell'utente
 * @param coccodrilli_liberi matrice di booleani che indicano se lo slot i j è in gioco (false) o meno (true) per i coccodrilli
 * @param coccodrilli matrice di coccodrilli che contiene tutti i coccodrilli per ogni corsia
 * @return true se la rana è morta per l'acqua, false se la rana non è morta per l'acqua
 */
bool rana_morte_acqua(int num_corsie, int num_coccodrilli_max_corsia, Character rana, bool coccodrilli_liberi[num_corsie][num_coccodrilli_max_corsia],
                      Character coccodrilli[num_corsie][num_coccodrilli_max_corsia])
{
    bool morte_acqua=true;

    //controllo che la rana si trovi nella zona Laghetto
    if(rana.y<=ALTEZZA_SCHERMO-ALTEZZA_MARCIAPIEDE_RANA-2 && rana.y>=ALTEZZA_SCHERMO-ALTEZZA_MARCIAPIEDE_RANA-ALTEZZA_LAGHETTO-2)
    {
        //si controlla se la rana si trova su almeno un coccodrillo
        for (int i = 0; i < num_corsie; ++i)
        {
            for (int p = 0; p < num_coccodrilli_max_corsia; ++p)
            {
                if (coccodrilli_liberi[i][p] == false && rana.y == coccodrilli[i][p].y && rana.x > coccodrilli[i][p].x &&
                    rana.x + 1 <= coccodrilli[i][p].x + LARGHEZZA_COCCODRILLO - 1 && (coccodrilli[i][p].signal == EMERSO || coccodrilli[i][p].signal == PROMPT_IMMERSIONE))
                {
                    morte_acqua = false;
                }
            }
        }
        //controllo se la rana viene trasportata fuori mappa da un coccodrillo
        if (rana.x == 0 || rana.x == LARGHEZZA_SCHERMO - 1)
        {
            morte_acqua = true;
        }

        return morte_acqua;
    }
    return false;
}

/**
 *Funzione che controlla se la rana va in contatto con una delle piante in gioco
 *
 * @param num_max_piante numero massimo di piante in partita
 * @param piante_libere array di booleani che indicano se lo slot i è in gioco (false) o meno (true) per le piante
 * @param rana character dell'utente
 * @param pianta array di pianta
 * @return true se la rana è morta per il contatto con la pianta, false se la rana non è morta per il contatto con la pianta
 */
bool rana_morte_pianta(int num_max_piante, bool piante_libere[num_max_piante], Character rana, Character pianta[num_max_piante])
{
    for (int i = 0; i < num_max_piante; ++i)
    {
        if(piante_libere[i]==false)
        {
            if( (rana.y>=pianta[i].y && rana.y<=pianta[i].y+ALTEZZA_PIANTA-1) &&
                (rana.x>=pianta[i].x-LARGHEZZA_PIANTA && rana.x<=pianta[i].x+1) )
            {
                return true;
            }
        }
    }
    return false;
}

/**
 * Funzione che controlla se un proiettile di una determinata pianta colpisce uno dei proittili della rana
 *
 * @param num_max_proiettili_rana numero massimo di proiettili che può sparare la rana
 * @param slot_liberi_proiettili_rana array di booleani che indicano se lo slot i è in gioco (false) o meno (true) per i proiettili della rana
 * @param proiettili_rana array di proiettili della rana
 * @param num_max_piante numero massimo di piante in partita
 * @param num_max_proiettili_pianta numero massimo di proiettili che può sparare ogni pianta
 * @param proiettili_pianta matrice di proiettili di tutte le piante
 * @param slot_liberi_proiettili_pianta matrice di booleani che indicano se lo slot i j è in gioco (false) o meno (true) per i proiettili delle piante
 * @param thread_proiettili_rana array di thread dei proiettili della rana
 * @param thread_proiettili_pianta matrice di thread dei proiettili di tutte le piante
 * @param punteggio punteggio corrente del giocatore
 * @param id1 id per identificare la pianta
 * @param id2 id per identificare il proiettile della pianta id1
 */
void collisione_proj_pianta_proj_rana(int num_max_proiettili_rana, bool slot_liberi_proiettili_rana[num_max_proiettili_rana], Character proiettili_rana[num_max_proiettili_rana],
                                      int num_max_piante, int num_max_proiettili_pianta, Character proiettili_pianta[num_max_piante][num_max_proiettili_pianta], bool slot_liberi_proiettili_pianta[num_max_piante][num_max_proiettili_pianta],
                                      pthread_t thread_proiettili_rana[num_max_proiettili_rana],  pthread_t thread_proiettili_pianta[num_max_piante][num_max_proiettili_pianta],
                                      int *punteggio, int id1, int id2 )
{
    for (int k = 0; k < num_max_proiettili_rana; ++k)
    {
        if(slot_liberi_proiettili_rana[k]==false)
        {
            //se un proittile della rana e un proittile di una pianta si scontrano il punteggio viene incrementanto e i due processi vengono uccisi
            if(proiettili_rana[k].x==proiettili_pianta[id1][id2].x &&
               proiettili_rana[k].y==proiettili_pianta[id1][id2].y &&
               proiettili_rana[k].x!=UNDEFINED_X_PROIETTILE_RANA &&
               proiettili_pianta[id1][id2].x!=UNDEFINED_X_PROIETTILE_PIANTA)
            {
                //incremento punteggio
                (*punteggio)+=PUNTEGGIO_PROITTILE;

                if((*punteggio)>MAX_PUNTEGGIO)
                {
                    (*punteggio) = MAX_PUNTEGGIO;
                }

                //distruzione proiettili
                slot_liberi_proiettili_rana[k]=true;
                pthread_cancel(thread_proiettili_rana[k]);
                pthread_join(thread_proiettili_rana[k], NULL);

                slot_liberi_proiettili_pianta[id1][id2]=true;
                pthread_cancel(thread_proiettili_pianta[id1][id2]);
                pthread_join(thread_proiettili_pianta[id1][id2], NULL);

                proiettili_rana[k].x=UNDEFINED_X_PROIETTILE_RANA;
                proiettili_pianta[id1][id2].x=UNDEFINED_X_PROIETTILE_PIANTA;
            }
        }
    }

}

/**
 * Funzione che controlla se uno dei proittili della rana colpisce un proiettile di una determinata pianta
 *
 * @param id id per identificare il rpoittile della rana
 * @param num_max_piante numero massimo di piante in partita
 * @param num_max_proiettili_pianta numero massimo di proiettili che può sparare ogni pianta
 * @param num_max_proiettili_rana numero massimo di proiettili che può sparare la rana
 * @param slot_liberi_proiettili_pianta matrice di booleani che indicano se lo slot i j è in gioco (false) o meno (true) per i proiettili delle piante
 * @param slot_liberi_proiettili_rana array di booleani che indicano se lo slot i è in gioco (false) o meno (true) per i proiettili della rana
 * @param thread_proiettili_rana array di thread dei proiettili della rana
 * @param proiettili_rana array di proiettili della rana
 * @param proiettili_pianta matrice di proiettili di tutte le piante
 * @param thread_proiettili_pianta matrice di thread dei proiettili di tutte le piante
 * @param punteggio punteggio corrente del giocatore
 */
void collisione_proj_rana_proj_piante(int id, int num_max_piante, int num_max_proiettili_pianta, int num_max_proiettili_rana, bool slot_liberi_proiettili_pianta[num_max_piante][num_max_proiettili_pianta],
                                      bool slot_liberi_proiettili_rana[num_max_proiettili_rana], pthread_t thread_proiettili_rana[num_max_proiettili_rana], Character proiettili_rana[num_max_proiettili_rana],
                                      Character proiettili_pianta[num_max_piante][num_max_proiettili_pianta], pthread_t thread_proiettili_pianta[num_max_piante][num_max_proiettili_pianta], int* punteggio)
{
    for (int j = 0; j < num_max_piante; ++j)
    {
        for (int k = 0; k < num_max_proiettili_pianta; ++k)
        {
            if(slot_liberi_proiettili_pianta[j][k]==false)
            {
                //se un proittile della rana e un proittile di una pianta si scontrano il punteggio viene incrementanto e i due processi vengono uccisi
                if(proiettili_rana[id].x==proiettili_pianta[j][k].x &&
                   proiettili_rana[id].y==proiettili_pianta[j][k].y &&
                   proiettili_rana[id].x!=UNDEFINED_X_PROIETTILE_RANA &&
                   proiettili_pianta[j][k].x!=UNDEFINED_X_PROIETTILE_PIANTA)
                {
                    //incremento punteggio
                    (*punteggio)+=PUNTEGGIO_PROITTILE;

                    if((*punteggio)>MAX_PUNTEGGIO)
                    {
                        (*punteggio) = MAX_PUNTEGGIO;
                    }

                    //distruzione proiettili
                    slot_liberi_proiettili_rana[id]=true;
                    pthread_cancel(thread_proiettili_rana[id]);
                    pthread_join(thread_proiettili_rana[id], NULL);

                    slot_liberi_proiettili_pianta[j][k]=true;
                    pthread_cancel(thread_proiettili_pianta[j][k]);
                    pthread_join(thread_proiettili_pianta[j][k], NULL);

                    proiettili_rana[id].x=UNDEFINED_X_PROIETTILE_RANA;
                    proiettili_pianta[j][k].x=UNDEFINED_X_PROIETTILE_PIANTA;
                }
            }
        }
    }
}

/**
 * Funzione che controlla il comportamento in caso di collsione tra coccodrillo e un proittile della rana
 *
 * @param id id per identificare il proiettile della rana
 * @param num_corsie numero di corsie del laghetto
 * @param num_coccodrilli_max_corsia numero massimo di coccodrilli per corsia
 * @param coccodrilli_liberi matrice di booleani che indicano se lo slot i j è in gioco (false) o meno (true) per i coccodrilli
 * @param coccodrilli matrice di coccodrilli che contiene tutti i coccodrilli per ogni corsia
 * @param velocita_corsia array che indica la velocità di ogni corsia
 * @param dir_coccodrilli array che indica la direzione di ogni corsia
 * @param thread_coccodrilli matrice di thread di tutti i coccodrilli per ogni corsia
 * @param num_max_proiettili_rana numero massimo di proiettili che può sparare la rana
 * @param slot_liberi_proiettili_rana array di booleani che indicano se lo slot i è in gioco (false) o meno (true) per i proiettili della rana
 * @param proiettili_rana array di proiettili della rana
 * @param thread_proiettili_rana array di thread dei proiettili della rana
 * @param punteggio punteggio corrente del giocatore
 */
void collisione_proj_rana_coccodrilli(int id, int num_corsie, int num_coccodrilli_max_corsia, bool coccodrilli_liberi[num_corsie][num_coccodrilli_max_corsia], Character coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                                      int velocita_corsia[num_corsie], int dir_coccodrilli[num_corsie], pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                                      int num_max_proiettili_rana, bool slot_liberi_proiettili_rana[num_max_proiettili_rana], Character proiettili_rana[num_max_proiettili_rana],
                                      pthread_t thread_proiettili_rana[num_max_proiettili_rana], int* punteggio)
{
    //per ogni coccodrillo si controlla se il proittile compisce un coccodrillo
    for (int j = 0; j < num_corsie; ++j)
    {
        for (int k = 0; k < num_coccodrilli_max_corsia; ++k)
        {
            if (coccodrilli_liberi[j][k] == false)
            {
                //se il coccodrillo è buono viene ucciso solo il proiettile
                if (coccodrilli[j][k].status == BUONO &&
                    (proiettili_rana[id].x >= coccodrilli[j][k].x &&
                     proiettili_rana[id].x <= coccodrilli[j][k].x + LARGHEZZA_COCCODRILLO - 1) &&
                    (proiettili_rana[id].y >= coccodrilli[j][k].y &&
                     proiettili_rana[id].y <= coccodrilli[j][k].y + ALTEZZA_COCCODRILLO - 1))
                {
                    //kill proiettile schiantato
                    slot_liberi_proiettili_rana[id] = true;
                    pthread_cancel(thread_proiettili_rana[id]);
                    pthread_join(thread_proiettili_rana[id], NULL);

                    proiettili_rana[id].x = UNDEFINED_X_PROIETTILE_RANA;
                    proiettili_rana[id].y = UNDEFINED_X_PROIETTILE_RANA;

                }
                    //se il coccodrillo è cattivo viene ucciso il proiettile, se il coccodrillo è emerso o si sta per immergere il coccodrillo diventa buono
                else if (coccodrilli[j][k].status == CATTIVO &&
                         (coccodrilli[j][k].signal == EMERSO || coccodrilli[j][k].signal == PROMPT_IMMERSIONE) &&
                         (proiettili_rana[id].x >= coccodrilli[j][k].x &&
                          proiettili_rana[id].x <= coccodrilli[j][k].x + LARGHEZZA_COCCODRILLO - 1) &&
                         (proiettili_rana[id].y >= coccodrilli[j][k].y &&
                          proiettili_rana[id].y <= coccodrilli[j][k].y + ALTEZZA_COCCODRILLO - 1))
                {

                    //aggiornamento punteggio quando trasformo un coccodrillo
                    (*punteggio) += PUNTEGGIO_COCCODRILLI;

                    if ((*punteggio) > MAX_PUNTEGGIO)
                    {
                        (*punteggio) = MAX_PUNTEGGIO;
                    }


                    //kill vecchi thread
                    slot_liberi_proiettili_rana[id] = true;
                    pthread_cancel(thread_proiettili_rana[id]);
                    pthread_join(thread_proiettili_rana[id], NULL);

                    proiettili_rana[id].x = UNDEFINED_X_PROIETTILE_RANA;
                    proiettili_rana[id].y = UNDEFINED_X_PROIETTILE_RANA;

                    pthread_cancel(thread_coccodrilli[j][k]);
                    pthread_join(thread_coccodrilli[j][k], NULL);

                    //inizializzazione argument
                    argumentCoccodrillo[j][k].dir = dir_coccodrilli[j];
                    argumentCoccodrillo[j][k].id_coccodrillo_corsia = j;
                    argumentCoccodrillo[j][k].id_coccodrillo_numero = k;
                    argumentCoccodrillo[j][k].velocita = velocita_corsia[j];
                    argumentCoccodrillo[j][k].status_coccodrillo = BUONO;

                    //creazione thread coccodrilli
                    if (pthread_create(&thread_coccodrilli[j][k], NULL, (void *(*)(void *)) &pos_coccodrillo, &argumentCoccodrillo[j][k]) != 0)
                    {
                        perror("Errore durante la creazione del thread");
                        endwin();
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
}

/**
 * Funzione che controlla il comportamento in caso di collisione tra proiettile sparato dalla rana
 * e una delle piante in gioco
 *
 * @param id per identificare il proiettile della rana
 * @param num_max_piante numero massimo di piante in partita
 * @param num_max_proiettili_rana numero massimo di proiettili che può sparare la rana
 * @param punteggio punteggio corrente del giocatore
 * @param piante_libere array di booleani che indicano se lo slot i è in gioco (false) o meno (true) per le piante
 * @param proiettili_rana array di proiettili della rana
 * @param slot_liberi_proiettili_rana array di booleani che indicano se lo slot i è in gioco (false) o meno (true) per i proiettili della rana
 * @param thread_proiettili_rana array di thread dei proiettili della rana
 * @param num_piante_in_game numero di piante attualmente presenti in gioco
 * @param thread_piante array di thread per i processi pianta
 * @param pianta array di pianta
 */
void collisione_proj_pianta(int id, int num_max_piante, int num_max_proiettili_rana, int* punteggio, bool piante_libere[num_max_piante], Character proiettili_rana[num_max_proiettili_rana],
                            bool slot_liberi_proiettili_rana[num_max_proiettili_rana], pthread_t thread_proiettili_rana[num_max_proiettili_rana], int *num_piante_in_game, pthread_t thread_piante[num_max_piante],
                            Character pianta[num_max_piante])
{
    //scorro tutte le piante e controllo tra quelle attive se collidono con il proiettile sparato dalla rana
    for (int j = 0; j < num_max_piante; ++j)
    {
        if(piante_libere[j]==false)
        {
            if( (proiettili_rana[id].x>=pianta[j].x-LARGHEZZA_PIANTA+1 && proiettili_rana[id].x<=pianta[j].x ) &&
                (proiettili_rana[id].y>=pianta[j].y && proiettili_rana[id].y<=pianta[j].y + ALTEZZA_PIANTA-1))
            {
                //aggiornamento punteggio

                (*punteggio)+=PUNTEGGIO_PIANTE;

                if((*punteggio)>MAX_PUNTEGGIO)
                {
                    (*punteggio) = MAX_PUNTEGGIO;
                }

                //distruzione proiettile pianta
                slot_liberi_proiettili_rana[id]=true;
                pthread_cancel(thread_proiettili_rana[id]);
                pthread_join(thread_proiettili_rana[id], NULL);

                //distruzione pianta
                (*num_piante_in_game)--;
                piante_libere[j]=true;
                pthread_cancel(thread_piante[j]);
                pthread_join(thread_piante[j], NULL);

            }
        }
    }
}

/**
 * Funzione che controlla il comportamento in caso di collisione tra coccodrilli e un proiettile sparato dalla rana,
 * funziona allo stesso modo della funzione 'collisione_proj_rana_coccodrilli' ma scorre l'inverso
 *
 * @param id1
 * @param id2
 * @param num_corsie numero di corsie del laghetto
 * @param num_coccodrilli_max_corsia numero massimo di coccodrilli per corsia
 * @param coccodrilli matrice di coccodrilli che contiene tutti i coccodrilli per ogni corsia
 * @param velocita_corsia array che indica la velocità di ogni corsia
 * @param dir_coccodrilli array che indica la direzione di ogni corsia
 * @param thread_coccodrilli matrice di thread di tutti i coccodrilli per ogni corsia
 * @param num_max_proiettili_rana numero massimo di proiettili che può sparare la rana
 * @param slot_liberi_proiettili_rana array di booleani che indicano se lo slot i è in gioco (false) o meno (true) per i proiettili della rana
 * @param proiettili_rana array di proiettili della rana
 * @param thread_proiettili_rana array di thread dei proiettili della rana
 * @param punteggio punteggio corrente del giocatore
 */
void collisione_coccodrilli_proj_rana(int id1, int id2, int num_corsie, int num_coccodrilli_max_corsia, Character coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                                      int velocita_corsia[num_corsie], int dir_coccodrilli[num_corsie], pthread_t thread_coccodrilli[num_corsie][num_coccodrilli_max_corsia],
                                      int num_max_proiettili_rana, bool slot_liberi_proiettili_rana[num_max_proiettili_rana], Character proiettili_rana[num_max_proiettili_rana],
                                      pthread_t thread_proiettili_rana[num_max_proiettili_rana], int* punteggio)
{
    //anziche scorrere i coccodrilli si scorrono i proiettili della rana
    for (int k = 0; k < num_max_proiettili_rana; ++k)
    {
        if(slot_liberi_proiettili_rana[k] == false)
        {
            if (coccodrilli[id1][id2].status == BUONO &&
                (proiettili_rana[k].x >= coccodrilli[id1][id2].x &&
                 proiettili_rana[k].x <= coccodrilli[id1][id2].x + LARGHEZZA_COCCODRILLO - 1) &&
                (proiettili_rana[k].y >= coccodrilli[id1][id2].y &&
                 proiettili_rana[k].y <= coccodrilli[id1][id2].y + ALTEZZA_COCCODRILLO - 1))
            {
                //kill proiettile schiantato
                slot_liberi_proiettili_rana[k] = true;
                pthread_cancel(thread_proiettili_rana[k]);
                pthread_join(thread_proiettili_rana[k], NULL);

                proiettili_rana[k].x = UNDEFINED_X_PROIETTILE_RANA;
                proiettili_rana[k].y = UNDEFINED_X_PROIETTILE_RANA;

            } else if (coccodrilli[id1][id2].status == CATTIVO &&
                       (coccodrilli[id1][id2].signal == EMERSO || coccodrilli[id1][id2].signal == PROMPT_IMMERSIONE) &&
                       (proiettili_rana[k].x >= coccodrilli[id1][id2].x &&
                        proiettili_rana[k].x <= coccodrilli[id1][id2].x + LARGHEZZA_COCCODRILLO - 1) &&
                       (proiettili_rana[k].y >= coccodrilli[id1][id2].y &&
                        proiettili_rana[k].y <= coccodrilli[id1][id2].y + ALTEZZA_COCCODRILLO - 1))
            {

                //aggiornamento punteggio quando trasformo un coccodrillo
                (*punteggio) += PUNTEGGIO_COCCODRILLI;

                if ((*punteggio) > MAX_PUNTEGGIO)
                {
                    (*punteggio) = MAX_PUNTEGGIO;
                }


                //kill vecchi processi
                slot_liberi_proiettili_rana[k] = true;
                pthread_cancel(thread_proiettili_rana[k]);
                pthread_join(thread_proiettili_rana[k], NULL);

                proiettili_rana[k].x = UNDEFINED_X_PROIETTILE_RANA;
                proiettili_rana[k].y = UNDEFINED_X_PROIETTILE_RANA;

                //rigenerazione coccodrillo buono
                pthread_cancel(thread_coccodrilli[id1][id2]);
                pthread_join(thread_coccodrilli[id1][id2], NULL);

                //inizializzazione argument
                argumentCoccodrillo[id1][id2].dir = dir_coccodrilli[id1];
                argumentCoccodrillo[id1][id2].id_coccodrillo_corsia = id1;
                argumentCoccodrillo[id1][id2].id_coccodrillo_numero = id2;
                argumentCoccodrillo[id1][id2].velocita = velocita_corsia[id1];
                argumentCoccodrillo[id1][id2].status_coccodrillo = BUONO;

                if (pthread_create(&thread_coccodrilli[id1][id2], NULL, (void *(*)(void *)) &pos_coccodrillo, &argumentCoccodrillo[id1][id2]) != 0)
                {
                    perror("Errore durante la creazione del thread");
                    endwin();
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

/**
 * Funzione principale di gioco, consiste in un ciclo infinito che legge continuamente dalla pipe, e termina solo in caso di
 * avvenimenti di gioco specifici (vittoria, sconfitta oppure quit del giocatore).
 * La variabile aux viene utilizzata come contenitore dei dati letti dalla pipe per poi aggiornare i vari oggetti di gioco proprio
 * in base a che valori contiene aux, in particolare il campo ID che indica il personaggio che si vuole aggiornare
 *
 * @param gioco finestra di gioco
 * @param difficolta difficolta scelta dal player
 */
void controllo(WINDOW* gioco, Difficolta difficolta)
{
    ///DICHIARAZIONE FINESTRE INTERNE AREA DI GIOCO
    WINDOW* marciapiede_rana=NULL, *laghetto=NULL, *marciapiede_piante=NULL, *tane=NULL, *stats=NULL;

    ///OGGETTI DI GIOCO
    Character aux;
    aux.signal=CONTINUE;

    //oggetto timer di gioco
    Character timer;
    timer.signal=CONTINUE;
    timer.tempo_rimasto=TEMPO_MASSIMO;

    //oggetti riguardanti rana
    Character rana;
    rana.y=ALTEZZA_SCHERMO-3;
    rana.x=LARGHEZZA_SCHERMO/2;
    rana.signal=CONTINUE;

    Character proiettili_rana[NUM_MAX_PROIETTILI_RANA];
    bool slot_liberi_proiettili_rana[NUM_MAX_PROIETTILI_RANA];
    for (int i = 0; i < NUM_MAX_PROIETTILI_RANA; ++i)
    {
        slot_liberi_proiettili_rana[i]=true;
    }
    int slot_pr_rana;

    //oggetti riguardanti le tane
    Character tana[NUM_TANE];
    bool tana_aperta[NUM_TANE];
    for (int i = 0; i < NUM_TANE; ++i)
    {
        tana_aperta[i]=true;
    }
    //inizializzazione coordinate tane
    for (int i = 0; i < NUM_TANE; ++i)
    {
        tana[i].y=ALTEZZA_STATS+1;
        tana[i].x=( LARGHEZZA_SCHERMO-(NUM_TANE*LARGHEZZA_TANA) ) / 6 * (i+1) + i*LARGHEZZA_TANA;
    }

    //oggetti riguardanti coccodrilli
    Character coccodrilli[NUM_CORSIE][NUM_COCCODRILLI_MAX_CORSIA];

    int dir_coccodrilli[NUM_CORSIE];
    int max_coccodrilli_corsia[NUM_CORSIE];
    bool coccodrilli_liberi[NUM_CORSIE][NUM_COCCODRILLI_MAX_CORSIA];
    for (int i = 0; i < NUM_CORSIE; ++i)
    {
        for (int j = 0; j < NUM_COCCODRILLI_MAX_CORSIA; ++j)
        {
            coccodrilli_liberi[i][j]=true;
        }
    }

    int velocita_corsia[NUM_CORSIE];
    int velocita_coccodrilli[POSSIBILITA_VELOCITA]= {500000, 250000, 100000};

    int cooldown_corsie[NUM_CORSIE];
    for (int i = 0; i < NUM_CORSIE; ++i)
    {
        cooldown_corsie[i]=0;
    }

    //oggetti riguardanti piante
    int num_max_piante;
    if(difficolta==DIFFICILE)
    {
        num_max_piante=NUM_MAX_PIANTE_DIFFICILE;
    }else
    {
        num_max_piante=NUM_MAX_PIANTE_NORMALE;
    }


    Character pianta[num_max_piante];
    Character proiettili_pianta[num_max_piante][NUM_MAX_PROIETTILI_PIANTE];
    bool slot_liberi_proiettili_pianta[num_max_piante][NUM_MAX_PROIETTILI_PIANTE];
    for (int i = 0; i < num_max_piante; ++i)
    {
        for (int j = 0; j < NUM_MAX_PROIETTILI_PIANTE; ++j)
        {
            slot_liberi_proiettili_pianta[i][j]=true;
        }
    }

    bool piante_libere[num_max_piante];
    for (int i = 0; i < num_max_piante; ++i)
    {
        piante_libere[i]=true;
    }

    int num_piante_in_game=0;

    int cooldown_generazione_piante=4 + rand()%(7-4+1);
    int cooldown_spari_piante[num_max_piante];
    for (int i = 0; i < num_max_piante; ++i)
    {
        cooldown_spari_piante[i]=0;
    }

    int slot_pr_pianta;

    ///THREAD
    pthread_t thread_timer;
    pthread_t thread_coccodrilli[NUM_CORSIE][NUM_COCCODRILLI_MAX_CORSIA];
    pthread_t thread_proiettili_rana[NUM_MAX_PROIETTILI_RANA];
    pthread_t thread_proiettili_pianta[num_max_piante][NUM_MAX_PROIETTILI_PIANTE];
    pthread_t thread_piante[num_max_piante];

    ///VARIABILI PARTITA
    int hp_rana;
    int hp_max;

    if(difficolta == NORMALE)
    {
        hp_max=HP_MAX_NORMALE;
    }
    else
    {
        hp_max=HP_MAX_DIFFICILE;
    }
    hp_rana=hp_max;

    int punti_vittoria=0;

    bool nuova_manche=true;
    Stati_rana_endgame status_rana;
    bool rana_morta=false;

    //proiettile contro proiettile=1 punti
    //coccodrillo trasformato=10
    //pianta uccisa=15
    //tana chiusa=25
    //punteggio massimo=999;
    int punteggio_precedente=0;
    int punteggio=0;
    int modulo_hp_up=PUNTEGGIO_HPUP;

    Stati_fine_partita fine_partita;


    ///CICLO PRINCIPALE DI LETTURA E STAMPA
    do {
        werase(gioco);

        ///CONSUMAZIONE
        consumazione(&aux);

        ///----------------------------------------GENERAZIONE COCCODRILLI----------------------------------------

        ///GENERAZIONE COCCODRILLI OGNI MANCHE
        if (nuova_manche == true)
        {

            //creazione timer
            if ( pthread_create(&thread_timer, NULL, (void *(*)(void *)) &timer_partita, &Argument_timer) != 0)
            {
                perror("Errore durante la creazione del thread");
                endwin();
                exit(EXIT_FAILURE);
            }

            ///GENERAZIONI STATS COCCODRILLI
            //generazione velocita dei coccodrilli
            gen_vel_coccodrilli(NUM_CORSIE, POSSIBILITA_VELOCITA, velocita_corsia, velocita_coccodrilli);

            //generazione direzione di ogni corsia
            gen_dir_coccodrilli(NUM_CORSIE, dir_coccodrilli);

            //generazione cooldown
            for (int i = 0; i < NUM_CORSIE; ++i)
            {
                if(velocita_corsia[i]==velocita_coccodrilli[0])
                {
                    cooldown_corsie[i] += 7 + rand()%(10-7+1);
                }
                else if(velocita_corsia[i]==velocita_coccodrilli[1])
                {
                    cooldown_corsie[i] += 4 + rand()%(7-4+1);
                }
                else
                {
                    cooldown_corsie[i] += 3 + rand()%(5-3+1);
                }

            }

            ///GENERAZIONE COCCODRILLI OGNI MANCHE
            gen_coccodrilli_ogni_manche(NUM_CORSIE, NUM_COCCODRILLI_MAX_CORSIA,
                                        coccodrilli_liberi, coccodrilli, thread_coccodrilli, dir_coccodrilli,
                                        velocita_corsia, POSSIBILITA_VELOCITA, velocita_coccodrilli);

            nuova_manche = false;
        }


        //aggiornamento punteggio precedente
        punteggio_precedente=punteggio;

        ///GENERAZIONE CONTINUA COCCODRILLI
        gen_continua_coccodrilli(difficolta,NUM_CORSIE, NUM_COCCODRILLI_MAX_CORSIA, cooldown_corsie,
                                 coccodrilli_liberi, coccodrilli, thread_coccodrilli, dir_coccodrilli, velocita_corsia,
                                 POSSIBILITA_VELOCITA, velocita_coccodrilli, timer);

        ///----------------------------------------GENERAZIONE PIANTE----------------------------------------

        ///GENERAZIONE X PIANTE

        gen_continua_piante(num_max_piante, &num_piante_in_game, &cooldown_generazione_piante, piante_libere, pianta, thread_piante, timer);

        ///----------------------------------------CONTROLLO LETTURA----------------------------------------
        if(aux.id==RANA)
        {

            ///aggiornamento valori rana
            rana.x+=aux.x;
            rana.y+=aux.y;

            ///aggiornamento signal rana
            rana.signal=aux.signal;

            ///COLLISIONI RANA
            if(rana.x < 2 || rana.x > LARGHEZZA_SCHERMO - 3)
            {
                rana.x-=aux.x;
            }
            if(rana.y + ALTEZZA_RANA >= ALTEZZA_SCHERMO)
            {
                rana.y-=aux.y;
            }

            //generazione processo sparo rana
            if(rana.signal==SPARO)
            {
                //seleziono slot libero
                slot_pr_rana=-1;

                for (int i = 0; i < NUM_MAX_PROIETTILI_RANA; ++i)
                {
                    if(slot_liberi_proiettili_rana[i]==true)
                    {
                        slot_pr_rana=i;
                    }
                }


                if(slot_pr_rana!=-1)
                {
                    //genero il proiettile
                    slot_liberi_proiettili_rana[slot_pr_rana]=false;

                    proiettili_rana[slot_pr_rana].y=rana.y-1;
                    proiettili_rana[slot_pr_rana].x=rana.x;

                    argumentProiettile_rana[slot_pr_rana].id_proiettile1=slot_pr_rana;
                    argumentProiettile_rana[slot_pr_rana].personaggio_sparo=RANA;
                    argumentProiettile_rana[slot_pr_rana].dir_proiettile=-1;
                    argumentProiettile_rana[slot_pr_rana].id_proiettile2=NOCHOICE;

                    //creazione thread proiettili della rana
                    if (pthread_create(&thread_proiettili_rana[slot_pr_rana], NULL, (void *(*)(void *)) &pos_proiettile, &argumentProiettile_rana[slot_pr_rana]) != 0)
                    {
                        perror("Errore durante la creazione del thread");
                        endwin();
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        if(aux.id==COCCODRILLO)
        {
            for (int i = 0; i < NUM_CORSIE; ++i)
            {
                for (int j = 0; j < NUM_COCCODRILLI_MAX_CORSIA; ++j)
                {
                    if (i == aux.id_riga && j == aux.id_colonna && coccodrilli_liberi[i][j]==false)
                    {

                        ///aggiornamenti posizioni coccodrillo
                        coccodrilli[i][j].x += aux.x;
                        coccodrilli[i][j].y += aux.y;

                        ///aggiornamenti coccodrillo
                        coccodrilli[i][j].status = aux.status;
                        coccodrilli[i][j].signal = aux.signal;

                        ///AGGIORNAMENTO POSIZIONE RANA QUANDO SI TROVA SOPRA UN COCCODRILLO
                        if(coccodrilli_liberi[i][j]==false && rana.y==coccodrilli[i][j].y &&
                           rana.x >= coccodrilli[i][j].x && rana.x +1 <= coccodrilli[i][j].x + LARGHEZZA_COCCODRILLO && (coccodrilli[i][j].signal==EMERSO || coccodrilli[i][j].signal==PROMPT_IMMERSIONE))
                        {
                            rana.x += dir_coccodrilli[i];
                        }


                        //collisioni coccodrillo
                        if(dir_coccodrilli[i] == -1)
                        {
                            if (coccodrilli[i][j].x == 1 - LARGHEZZA_COCCODRILLO)
                            {
                                coccodrilli_liberi[i][j]=true;
                                pthread_cancel(thread_coccodrilli[i][j]);
                                pthread_join(thread_coccodrilli[i][j], NULL);

                                coccodrilli[i][j].x=UNDEFINED_X_COCCODRILLO;
                                coccodrilli[i][j].y=UNDEFINED_X_COCCODRILLO;
                            }
                        }

                        if(dir_coccodrilli[i] == +1)
                        {
                            if (coccodrilli[i][j].x == LARGHEZZA_SCHERMO - 1)
                            {
                                coccodrilli_liberi[i][j]=true;
                                pthread_cancel(thread_coccodrilli[i][j]);
                                pthread_join(thread_coccodrilli[i][j], NULL);

                                coccodrilli[i][j].x=UNDEFINED_X_COCCODRILLO;
                                coccodrilli[i][j].y=UNDEFINED_X_COCCODRILLO;
                            }
                        }

                        //collisione con proiettile
                        collisione_coccodrilli_proj_rana(i, j, NUM_CORSIE, NUM_COCCODRILLI_MAX_CORSIA, coccodrilli,
                                                         velocita_corsia,  dir_coccodrilli, thread_coccodrilli,
                                                         NUM_MAX_PROIETTILI_RANA, slot_liberi_proiettili_rana, proiettili_rana,
                                                         thread_proiettili_rana, &punteggio);

                    }
                }
            }
        }
        if(aux.id==PROIETTILE)
        {
            //controllo personaggio che ha compiuto lo sparo
            if(aux.personaggio_signal==RANA)
            {
                for (int i = 0; i < NUM_MAX_PROIETTILI_RANA; ++i)
                {
                    //aggiornamento posizione proiettile letto
                    if(aux.id_riga==i && slot_liberi_proiettili_rana[i]==false)
                    {

                        proiettili_rana[i].y += aux.y;
                        proiettili_rana[i].x += aux.x;

                        //kill e liberazione dello slot quando sbatte contro muro
                        if (proiettili_rana[i].y == ALTEZZA_STATS + ALTEZZA_MARCIAPIEDE_PIANTE)
                        {
                            pthread_cancel(thread_proiettili_rana[i]);
                            pthread_join(thread_proiettili_rana[i], NULL);

                            slot_liberi_proiettili_rana[i]=true;

                            proiettili_rana[i].y=UNDEFINED_X_PROIETTILE_RANA;
                            proiettili_rana[i].x=UNDEFINED_X_PROIETTILE_RANA;
                        }

                        //collisione proiettili
                        collisione_proj_rana_proj_piante(i, num_max_piante, NUM_MAX_PROIETTILI_PIANTE, NUM_MAX_PROIETTILI_RANA, slot_liberi_proiettili_pianta, slot_liberi_proiettili_rana,
                                                         thread_proiettili_rana, proiettili_rana, proiettili_pianta, thread_proiettili_pianta, &punteggio);

                        //collisione proiettile rana con coccodrilli
                        collisione_proj_rana_coccodrilli(i, NUM_CORSIE, NUM_COCCODRILLI_MAX_CORSIA, coccodrilli_liberi, coccodrilli,
                                                         velocita_corsia,  dir_coccodrilli, thread_coccodrilli,
                                                         NUM_MAX_PROIETTILI_RANA, slot_liberi_proiettili_rana, proiettili_rana,
                                                         thread_proiettili_rana, &punteggio);

                        //collisione proiettile con pianta nemica
                        collisione_proj_pianta(i, num_max_piante, NUM_MAX_PROIETTILI_RANA, &punteggio, piante_libere, proiettili_rana,
                                               slot_liberi_proiettili_rana, thread_proiettili_rana, &num_piante_in_game, thread_piante, pianta);

                    }
                }
            }
            if(aux.personaggio_signal==PIANTA)
            {
                for (int i = 0; i < num_max_piante; ++i)
                {
                    for (int j = 0; j < NUM_MAX_PROIETTILI_PIANTE; ++j)
                    {
                        //aggiornamento posizione proiettile letto
                        if(aux.id_riga==i && aux.id_colonna==j && slot_liberi_proiettili_pianta[i][j]==false)
                        {
                            proiettili_pianta[i][j].y += aux.y;
                            proiettili_pianta[i][j].x += aux.x;

                            //kill e liberazione dello slot quando sbatte contro muro
                            if (proiettili_pianta[i][j].y == ALTEZZA_SCHERMO-1)
                            {
                                pthread_cancel(thread_proiettili_pianta[i][j]);
                                pthread_join(thread_proiettili_pianta[i][j], NULL);

                                slot_liberi_proiettili_pianta[i][j]=true;

                                proiettili_pianta[i][j].y=UNDEFINED_X_PROIETTILE_PIANTA;
                                proiettili_pianta[i][j].x=UNDEFINED_X_PROIETTILE_PIANTA;
                            }


                            //collisione proiettili pianta con proiettili rana
                            collisione_proj_pianta_proj_rana(NUM_MAX_PROIETTILI_RANA, slot_liberi_proiettili_rana, proiettili_rana,
                                                             num_max_piante, NUM_MAX_PROIETTILI_PIANTE, proiettili_pianta, slot_liberi_proiettili_pianta,
                                                             thread_proiettili_rana, thread_proiettili_pianta,
                                                             &punteggio, i, j );
                        }
                    }
                }
            }
        }
        if(aux.id==TIMER)
        {
            timer.tempo_rimasto = aux.tempo_rimasto;
            timer.signal = aux.signal;
        }
        if(aux.id==PIANTA)
        {
            for (int i = 0; i < num_piante_in_game; ++i)
            {
                if(i==aux.id_riga)
                {
                    ///SPARO CONTINU0 PIANTE
                    if(piante_libere[i]==false && aux.signal==SPARO)
                    {
                        //seleziono lo slot proiettile libero
                        slot_pr_pianta=-1;

                        for (int j = 0; j < NUM_MAX_PROIETTILI_PIANTE; ++j)
                        {
                            if( slot_liberi_proiettili_pianta[i][j] == true)
                            {
                                slot_pr_pianta=j;
                            }
                        }

                        if(slot_pr_pianta!=-1)
                        {
                            //lo occupo e genero il proiettile
                            slot_liberi_proiettili_pianta[i][slot_pr_pianta]=false;

                            proiettili_pianta[i][slot_pr_pianta].y=pianta[i].y+ALTEZZA_PIANTA;
                            proiettili_pianta[i][slot_pr_pianta].x=pianta[i].x;

                            //inizializzazione argument proiettili pianta
                            argumentProiettile_pianta[i][slot_pr_pianta].id_proiettile1=i;
                            argumentProiettile_pianta[i][slot_pr_pianta].id_proiettile2=slot_pr_pianta;

                            argumentProiettile_pianta[i][slot_pr_pianta].personaggio_sparo=PIANTA;
                            argumentProiettile_pianta[i][slot_pr_pianta].dir_proiettile=1;

                            if (pthread_create(&thread_proiettili_pianta[i][slot_pr_pianta], NULL, (void *(*)(void *))&pos_proiettile, &argumentProiettile_pianta[i][slot_pr_pianta]) != 0)
                            {
                                perror("Errore durante la creazione del thread");
                                endwin();
                                exit(EXIT_FAILURE);
                            }
                        }
                    }

                }
            }
        }


        ///----------------------------------------COLLISIONI GENERALI----------------------------------------

        //rana entra in una tana aperta oppure muore quando sbatte contro il muro
        status_rana = collisione_rana_tana_o_muro(NUM_TANE, rana, tana, tana_aperta);

        //se la rana e' ancora in game controllo tutte le collisioni e la scadenza del timer a ogni ciclo
        if(status_rana == RANA_IN_GAME)
        {
            //proiettili piante con la rana
            rana_morta = collisione_proj_piante_rana(num_max_piante, NUM_MAX_PROIETTILI_PIANTE, slot_liberi_proiettili_pianta, proiettili_pianta, rana);

            if(rana_morta == false)
            {
                //rana muore in acqua tranne quando e' su un coccodrillo
                rana_morta = rana_morte_acqua(NUM_CORSIE, NUM_COCCODRILLI_MAX_CORSIA, rana,  coccodrilli_liberi, coccodrilli);

                if(rana_morta==false)
                {
                    //rana muore quando tocca una pianta
                    rana_morta = rana_morte_pianta( num_max_piante, piante_libere, rana, pianta);

                    if(rana_morta == false)
                    {
                        //timer della manche scade
                        if(timer.signal==USCITA)
                        {
                            timer.signal=CONTINUE;

                            rana_morta=true;
                        }
                    }
                }
            }
        }

        //se la manche viene interrotta attivo 'nuova manche' e in caso aggiorno il punteggio
        if(status_rana == RANA_CONTRO_MURO || status_rana == RANA_IN_TANA || rana_morta == true)
        {
            if(status_rana==RANA_IN_TANA)
            {
                punteggio+=PUNTEGGIO_TANA;

                if(punteggio>MAX_PUNTEGGIO)
                {
                    punteggio = MAX_PUNTEGGIO;
                }

            }
            nuova_manche = true;
        }

        //hp up in caso di difficolta normale
        if(hp_rana < hp_max && difficolta==NORMALE && punteggio >= modulo_hp_up)
        {
            hp_rana++;
            modulo_hp_up+=PUNTEGGIO_HPUP;
        }

        //aggiorno le stats partita, la posizione della rana, killo tutti i processi e resetto tutti gli schermi booleani
        if(nuova_manche==true)
        {

            //reset posizione rana ogni manche
            rana.y=ALTEZZA_SCHERMO-3;
            rana.x=LARGHEZZA_SCHERMO/2;
            rana.signal=CONTINUE;

            //decremento vita quando colpita
            if(status_rana == RANA_CONTRO_MURO || rana_morta == true)
            {
                hp_rana--;
            }
            else if(status_rana == RANA_IN_TANA)
            {
                punti_vittoria++;
            }

            ///----------------------------------RESET GLOBALE------------------------------------

            reset_globale_manche(gioco, NUM_CORSIE, NUM_COCCODRILLI_MAX_CORSIA, thread_coccodrilli, coccodrilli_liberi,
                                 NUM_MAX_PROIETTILI_RANA, thread_proiettili_rana, slot_liberi_proiettili_rana,
                                 num_max_piante,NUM_MAX_PROIETTILI_PIANTE, thread_proiettili_pianta, slot_liberi_proiettili_pianta,
                                 cooldown_corsie, &num_piante_in_game, piante_libere, &cooldown_generazione_piante, thread_timer, thread_piante);

            ///----------------------------------RESET GLOBALE-----------------------------------
        }



        ///----------------------------------------STAMPE----------------------------------------
        //ad ogni ciclo ristampo tutti gli oggetti in gioco
        stampa_oggetti(gioco, num_max_piante, piante_libere, pianta,
                       NUM_CORSIE, NUM_COCCODRILLI_MAX_CORSIA, coccodrilli, coccodrilli_liberi,
                       dir_coccodrilli,
                       rana, NUM_MAX_PROIETTILI_RANA, slot_liberi_proiettili_rana, proiettili_rana,
                       NUM_MAX_PROIETTILI_PIANTE, proiettili_pianta, slot_liberi_proiettili_pianta,
                       NUM_TANE, tana, tana_aperta,TEMPO_MASSIMO, timer, hp_max, hp_rana, punteggio, punteggio_precedente);




        if (rana.signal==USCITA)		//uscita cliccando q
        {
            fine_partita=QUIT;
            break;
        }

        if (hp_rana==0)         //uscita morendo
        {
            fine_partita=SCONFITTA;
            break;
        }

        if (punti_vittoria==5)  //uscita vincendo
        {
            fine_partita=VITTORIA;
            break;
        }

        //stampa colori di gioco
        area_gioco(gioco, marciapiede_rana, laghetto, marciapiede_piante, tane, stats);


    }while(true);

    ///ELIMINAZIONE THREAD ANCORA ATTIVI IN END-GAME
    total_kill(NUM_CORSIE, NUM_COCCODRILLI_MAX_CORSIA,  thread_coccodrilli, coccodrilli_liberi,
               NUM_MAX_PROIETTILI_RANA, thread_proiettili_rana, slot_liberi_proiettili_rana,
               num_max_piante, NUM_MAX_PROIETTILI_PIANTE, thread_proiettili_pianta, slot_liberi_proiettili_pianta, piante_libere, thread_piante, thread_timer);

    //clear schermo
    werase(gioco);
    box(gioco, ACS_VLINE, ACS_HLINE);

    //visualizzazione schermata relativa all'obiettivo raggiunto giocando
    if(fine_partita==VITTORIA)
    {
        schermata_vittoria(gioco, punteggio);
    }else if(fine_partita==SCONFITTA)
    {
        schermata_sconfitta(gioco, punteggio);
    }else
    {
        schermata_quit(gioco, punteggio);
    }
    wrefresh(gioco);

}

/**
 * Funzione principale di gioco
 * creazione processo rana
 *
 * @param gioco finestra di gioco dove si stampa
 * @param difficolta difficoltà scelta dal giocatore
 */
void gameplay(WINDOW* gioco, Difficolta difficolta)
{
    wbkgd(gioco, COLOR_PAIR(BG_BLACK));
    //linea che divide l'area di gioco dalle stats
    for (int i = 1; i < LARGHEZZA_SCHERMO-1; ++i)
    {
        mvwaddch(gioco, ALTEZZA_STATS, i, ACS_HLINE);
    }

    //inizializzazione buffer
    buffer.i_scrivi=0;
    buffer.i_leggi=0;

    //inizializzazione semafori
    sem_init(&sem_liberi, 0, DIM_BUFFER);
    sem_init(&sem_occupati, 0, 0);


    //generazione thread rana
    pthread_t thread_rana;
    if ( pthread_create(&thread_rana, NULL, (void *(*)(void *)) &pos_rana, NULL) != 0)
    {
        perror("Errore durante la creazione del thread");
        endwin();
        exit(EXIT_FAILURE);
    }

    controllo(gioco, difficolta);

    //kill thread
    pthread_cancel(thread_rana);
    pthread_join(thread_rana, NULL);

}
