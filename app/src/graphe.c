#include "reseau.h"

void cree_reseau(graphe_reseau *g) {
    g->switchs = NULL;
    g->stations = NULL;
    g->aretes = NULL;
    g->nb_switchs = 0;
    g->nb_stations = 0;
    g->nb_arete = 0;
    g->capacite_arete = 0;
    g->ordre = 0;
}


int chargeur_reseau(const char *nom_fichier, g graphe_reseau *g)
{
FILE *f = fopen(nom_fichier,"r");
if(f == NULL)
{
    perror("erreur ouverture du fichier")
    return 0;
}
}