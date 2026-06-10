#include "reseau.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    printf("=== EXECUTION SCENARIO SANS STP (TEMPÊTE DE BROADCAST) ===\n");
    
    // fichier de config par defaut
    const char *fichier_config = "config2.txt";
    if (argc > 1) {
        fichier_config = argv[1];
    }
    
    graphe_reseau g;
    Ordonnanceur o;
    init_ordonnanceur(&o);
    
    // on charge le reseau depuis le fichier
    printf("Chargement de '%s'...\n", fichier_config);
    if (!chargeur_reseau(fichier_config, &g)) {
        fprintf(stderr, "Échec chargement !\n");
        return EXIT_FAILURE;
    }
    
    // on active de force tous les ports sans blocage (pas de stp)
    printf("\n[SANS STP] Configuration forcée de tous les ports en DESIGNÉ (pas de blocage)\n");
    for (size_t i = 0; i < g.nb_commutateurs; i++) {
        Commutateur *c = &g.commutateurs[i];
        for (size_t p = 0; p < c->nb_ports_connectes; p++) {
            c->ports[p].etat = PORT_DESIGNE;
        }
    }
    
    printf("Topologie chargée :\n");
    affiche_reseau(&g);
    
    // on planifie le premier tick a t=0
    planifier_tick(&o, 0);
    
    // on cherche la premiere station pour lancer le broadcast
    size_t src_idx = SIZE_MAX;
    if (g.nb_stations >= 1) {
        src_idx = g.nb_commutateurs; // index de la premiere station
    }
    
    // on planifie un envoi de broadcast a t=0 pour faire la tempete
    if (src_idx != SIZE_MAX) {
        printf("\n>> PLANIFICATION DE L'INJECTION BROADCAST DEPUIS LA STATION #%zu (Date 0)\n", src_idx);
        planifier_injection(&o, 0, src_idx, 999, "Alerte_Storm");
    } else {
        printf("\n>> AUCUNE STATION DISPONIBLE POUR L'INJECTION (Date 0)\n");
    }
    
    // on lance la simulation de t=0 a t=4 pour observer l'inondation infinie
    lance_simulation(&g, &o, 4, false); // false = stp desactive
    
    // on libere la memoire a la fin du programme
    vider_ordonnanceur(&o);
    for (size_t i = 0; i < g.nb_commutateurs; i++) {
        vider_file(&g.commutateurs[i].boite_reception);
        free(g.commutateurs[i].ports);
        free(g.commutateurs[i].table_commutation);
    }
    for (size_t i = 0; i < g.nb_stations; i++) {
        vider_file(&g.stations[i].boite_reception);
    }
    free(g.commutateurs);
    free(g.stations);
    free(g.liaisons);
    
    printf("\nFin du scénario sans STP (Tempête observée).\n");
    return EXIT_SUCCESS;
}
