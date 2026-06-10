#include "reseau.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    printf("=== EXECUTION SCENARIO STP (CONVERGENCE & TRANSMISSION) ===\n");
    
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
    
    // on lance un premier calcul stp sur chaque switch
    for (size_t i = 0; i < g.nb_commutateurs; i++) {
        reevalue_stp(&g, i);
    }
    
    printf("Topologie chargée :\n");
    affiche_reseau(&g);
    
    // on planifie le premier tick a t=0
    planifier_tick(&o, 0);
    
    // on laisse stp converger de t=0 a t=4
    printf("\n>> ELECTION ET DECOUVERTE STP EN ATTENTE (Date 0 à 4)\n");
    lance_simulation(&g, &o, 4, true); // true = stp actif
    
    printf("\n>> TOPOLOGIE STP CONVERGÉE ET BOUCLES COUPÉES :\n");
    affiche_reseau(&g);
    
    // on cherche deux stations pour faire le test d'envoi
    size_t src_idx = SIZE_MAX;
    size_t dest_idx = SIZE_MAX;
    if (g.nb_stations >= 2) {
        src_idx = g.nb_commutateurs;       // premiere station
        dest_idx = g.nb_commutateurs + 1; // deuxieme station
    } else if (g.nb_stations == 1) {
        src_idx = g.nb_commutateurs;
        dest_idx = 999;                   // broadcast si une seule station
    }
    
    // on planifie l'envoi du message a t=5
    if (src_idx != SIZE_MAX) {
        printf("\n>> PLANIFICATION DE L'ENVOI D'UN MESSAGE DE L'APPAREIL #%zu À L'APPAREIL #%zu (Date 5)\n", src_idx, dest_idx);
        planifier_injection(&o, 5, src_idx, dest_idx, "bonjour_via_spanning_tree");
    } else {
        printf("\n>> AUCUNE STATION DISPONIBLE POUR L'ENVOI DU MESSAGE (Date 5)\n");
    }
    
    // on fait tourner la simulation de t=5 a t=9 pour voir le trajet du message
    printf("\n>> DEBUT DE LA TRANSMISSION DU MESSAGE (Date 5 à 9)\n");
    lance_simulation(&g, &o, 9, true);
    
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
    
    printf("\nSimulation STP terminée avec succès.\n");
    return EXIT_SUCCESS;
}
