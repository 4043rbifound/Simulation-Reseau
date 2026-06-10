#include "reseau.h"
#include <string.h>

const adr_mac STP_MULTICAST = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00};
const adr_mac BROADCAST_MAC = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool mac_egale(const adr_mac m1, const adr_mac m2) {
    return memcmp(m1, m2, 6) == 0;
}

void envoie_vers_appareil(graphe_reseau *g, size_t appareil_dest, const TrameEthernet *trame, size_t num_port_entrant, size_t appareil_source) {
    // on met la trame dans la boite de reception du destinataire
    if (appareil_dest < g->nb_commutateurs) {
        enfiler(&g->commutateurs[appareil_dest].boite_reception, trame, num_port_entrant, appareil_source);
    } else {
        size_t st_idx = appareil_dest - g->nb_commutateurs;
        enfiler(&g->stations[st_idx].boite_reception, trame, num_port_entrant, appareil_source);
    }
}

int compare_bpdu(const DonneesBPDU *b1, const DonneesBPDU *b2) {
    // on compare deux bpdu selon les priorites et mac
    if (b1->priorite_racine != b2->priorite_racine) {
        return (b1->priorite_racine < b2->priorite_racine) ? -1 : 1;
    }
    int cmp_root = memcmp(b1->mac_racine, b2->mac_racine, 6);
    if (cmp_root != 0) {
        return (cmp_root < 0) ? -1 : 1;
    }
    
    if (b1->cout_chemin_racine != b2->cout_chemin_racine) {
        return (b1->cout_chemin_racine < b2->cout_chemin_racine) ? -1 : 1;
    }
    
    if (b1->priorite_emetteur != b2->priorite_emetteur) {
        return (b1->priorite_emetteur < b2->priorite_emetteur) ? -1 : 1;
    }
    int cmp_sender = memcmp(b1->mac_emetteur, b2->mac_emetteur, 6);
    if (cmp_sender != 0) {
        return (cmp_sender < 0) ? -1 : 1;
    }
    
    if (b1->id_port_emetteur != b2->id_port_emetteur) {
        return (b1->id_port_emetteur < b2->id_port_emetteur) ? -1 : 1;
    }
    
    return 0;
}

void reevalue_stp(graphe_reseau *g, size_t commutateur_idx) {
    // cette fonction decide du port racine et des ports a bloquer
    Commutateur *c = &g->commutateurs[commutateur_idx];
    size_t meilleur_port_idx = SIZE_MAX;
    bool a_trouve_meilleure_racine = false;
    
    // par defaut le switch croit qu'il est racine
    DonneesBPDU meilleur_bpdu;
    memcpy(meilleur_bpdu.mac_racine, c->mac, 6);
    meilleur_bpdu.priorite_racine = c->priorite;
    meilleur_bpdu.cout_chemin_racine = 0;
    memcpy(meilleur_bpdu.mac_emetteur, c->mac, 6);
    meilleur_bpdu.priorite_emetteur = c->priorite;
    meilleur_bpdu.id_port_emetteur = 0;
    
    for (size_t i = 0; i < c->nb_ports_connectes; i++) {
        Port *p = &c->ports[i];
        if (!p->a_recu_bpdu) continue;
        
        // on cree un bpdu candidat en ajoutant le cout du lien
        DonneesBPDU candidat = p->bpdu_recu;
        candidat.cout_chemin_racine += g->liaisons[p->index_liaison].poids;
        
        if (compare_bpdu(&candidat, &meilleur_bpdu) < 0) {
            meilleur_port_idx = i;
            a_trouve_meilleure_racine = true;
            meilleur_bpdu = candidat;
        }
    }
    
    if (a_trouve_meilleure_racine) {
        memcpy(c->mac_racine, meilleur_bpdu.mac_racine, 6);
        c->priorite_racine = meilleur_bpdu.priorite_racine;
        c->cout_chemin_racine = meilleur_bpdu.cout_chemin_racine;
        c->index_port_racine = meilleur_port_idx;
    } else {
        memcpy(c->mac_racine, c->mac, 6);
        c->priorite_racine = c->priorite;
        c->cout_chemin_racine = 0;
        c->index_port_racine = SIZE_MAX;
    }
    
    for (size_t i = 0; i < c->nb_ports_connectes; i++) {
        Port *p = &c->ports[i];
        EtatPort anc_etat = p->etat;
        
        if (i == c->index_port_racine) {
            p->etat = PORT_RACINE;
        } else {
            if (!p->a_recu_bpdu) {
                p->etat = PORT_DESIGNE;
            } else {
                // on compare le bpdu qu'on enverrait avec celui recu
                DonneesBPDU mon_bpdu_sortant;
                memcpy(mon_bpdu_sortant.mac_racine, c->mac_racine, 6);
                mon_bpdu_sortant.priorite_racine = c->priorite_racine;
                mon_bpdu_sortant.cout_chemin_racine = c->cout_chemin_racine;
                memcpy(mon_bpdu_sortant.mac_emetteur, c->mac, 6);
                mon_bpdu_sortant.priorite_emetteur = c->priorite;
                mon_bpdu_sortant.id_port_emetteur = (uint16_t)p->num_port;
                
                if (compare_bpdu(&mon_bpdu_sortant, &p->bpdu_recu) < 0) {
                    p->etat = PORT_DESIGNE;
                } else {
                    p->etat = PORT_BLOQUE;
                }
            }
        }
        
        if (p->etat != anc_etat) {
            printf("[Changement STP] Commutateur ");
            affiche_mac(c->mac);
            printf(" | Port %zu état : %s -> %s\n", 
                   p->num_port, 
                   (anc_etat == PORT_RACINE ? "RACINE" : (anc_etat == PORT_DESIGNE ? "DESIGNÉ" : "BLOQUÉ")),
                   (p->etat == PORT_RACINE ? "RACINE" : (p->etat == PORT_DESIGNE ? "DESIGNÉ" : "BLOQUÉ")));
        }
    }
}

void envoie_bpdu(graphe_reseau *g, size_t commutateur_idx, size_t port_idx) {
    // on prepare et on envoie un bpdu stp sur le port
    Commutateur *c = &g->commutateurs[commutateur_idx];
    Port *p = &c->ports[port_idx];
    
    DonneesBPDU bpdu;
    memcpy(bpdu.mac_racine, c->mac_racine, 6);
    bpdu.priorite_racine = c->priorite_racine;
    bpdu.cout_chemin_racine = c->cout_chemin_racine;
    memcpy(bpdu.mac_emetteur, c->mac, 6);
    bpdu.priorite_emetteur = c->priorite;
    bpdu.id_port_emetteur = (uint16_t)p->num_port;
    
    TrameEthernet trame;
    memcpy(trame.dest, STP_MULTICAST, 6);
    memcpy(trame.src, c->mac, 6);
    trame.type = 0x4242; // bpdu stp
    
    memcpy(trame.donnees, &bpdu, sizeof(DonneesBPDU));
    trame.taille_donnees = sizeof(DonneesBPDU);
    
    size_t voisin_idx = p->appareil_connecte;
    liaison *l = &g->liaisons[p->index_liaison];
    size_t num_port_entrant = 0;
    if (l->s1 == voisin_idx) num_port_entrant = l->port_s1;
    else if (l->s2 == voisin_idx) num_port_entrant = l->port_s2;
    
    envoie_vers_appareil(g, voisin_idx, &trame, num_port_entrant, commutateur_idx);
}

void apprend_mac(Commutateur *c, const adr_mac src, size_t num_port) {
    // on cherche si l'adresse mac est deja dans notre table
    for (size_t i = 0; i < c->nb_lignes; i++) {
        if (mac_egale(c->table_commutation[i].mac_dest, src)) {
            if (c->table_commutation[i].num_port != num_port) {
                printf("  [Apprentissage commutateur] MAC mise à jour ");
                affiche_mac(src);
                printf(" port %zu -> %zu\n", c->table_commutation[i].num_port, num_port);
                c->table_commutation[i].num_port = num_port;
            }
            return;
        }
    }
    
    // si la table est pleine on double sa capacite
    if (c->nb_lignes >= c->capacite_table) {
        size_t nouv_cap = c->capacite_table * 2;
        ligne_commutation *nouv_table = realloc(c->table_commutation, nouv_cap * sizeof(ligne_commutation));
        if (nouv_table != NULL) {
            c->table_commutation = nouv_table;
            c->capacite_table = nouv_cap;
        } else {
            fprintf(stderr, "Erreur fatale : table de commutation pleine !\n");
            return;
        }
    }
    
    // on ajoute l'adresse mac et le port associe
    memcpy(c->table_commutation[c->nb_lignes].mac_dest, src, 6);
    c->table_commutation[c->nb_lignes].num_port = num_port;
    c->nb_lignes++;
    
    printf("  [Apprentissage commutateur] Appris MAC ");
    affiche_mac(src);
    printf(" est sur le Port %zu\n", num_port);
}

void traite_tick_interne(graphe_reseau *g, size_t temps_courant, bool active_stp) {
    if (active_stp) {
        printf("\n--- TICK %zu DEBUT (STP ACTIF) ---\n", temps_courant);
    } else {
        printf("\n--- TICK %zu DEBUT (SANS STP) ---\n", temps_courant);
    }
    
    // on fige la taille des files au debut du tick
    size_t *nb_trames_sw = malloc(g->nb_commutateurs * sizeof(size_t));
    for (size_t i = 0; i < g->nb_commutateurs; i++) {
        nb_trames_sw[i] = g->commutateurs[i].boite_reception.taille;
    }
    size_t *nb_trames_st = malloc(g->nb_stations * sizeof(size_t));
    for (size_t i = 0; i < g->nb_stations; i++) {
        nb_trames_st[i] = g->stations[i].boite_reception.taille;
    }

    // 1. envoi des bpdu si stp est active
    if (active_stp) {
        for (size_t i = 0; i < g->nb_commutateurs; i++) {
            Commutateur *c = &g->commutateurs[i];
            for (size_t p = 0; p < c->nb_ports_connectes; p++) {
                if (c->index_port_racine == SIZE_MAX || c->ports[p].etat == PORT_DESIGNE) {
                    envoie_bpdu(g, i, p);
                }
            }
        }
    }
    
    // on compte le total des trames a traiter
    size_t total_trames = 0;
    for (size_t i = 0; i < g->nb_commutateurs; i++) {
        total_trames += nb_trames_sw[i];
    }
    printf("Nombre total de trames à traiter dans les commutateurs : %zu\n", total_trames);
    
    // 2. traitement des trames recues par les commutateurs
    for (size_t i = 0; i < g->nb_commutateurs; i++) {
        Commutateur *c = &g->commutateurs[i];
        size_t count = nb_trames_sw[i];
        
        for (size_t k = 0; k < count; k++) {
            TrameEthernet trame;
            size_t num_port_entrant;
            size_t appareil_source;
            
            if (defiler(&c->boite_reception, &trame, &num_port_entrant, &appareil_source)) {
                size_t in_port_idx = num_port_entrant - 1;
                Port *p = &c->ports[in_port_idx];
                
                printf("[Tick %zu] Commutateur #%zu a reçu trame de l'appareil #%zu sur le Port %zu\n",
                       temps_courant, i, appareil_source, num_port_entrant);
                
                if (trame.type == 0x4242) {
                    if (active_stp) {
                        DonneesBPDU bpdu;
                        memcpy(&bpdu, trame.donnees, sizeof(DonneesBPDU));
                        
                        p->bpdu_recu = bpdu;
                        p->a_recu_bpdu = true;
                        
                        reevalue_stp(g, i);
                    } else {
                        printf("  [Sans STP] BPDU ignoré.\n");
                    }
                } else {
                    // trame de donnees
                    if (active_stp && p->etat == PORT_BLOQUE) {
                        printf("  [Bloqué] Trame jetée ! Le Port %zu est BLOQUÉ.\n", p->num_port);
                    } else {
                        apprend_mac(c, trame.src, p->num_port);
                        
                        bool est_bcast = mac_egale(trame.dest, BROADCAST_MAC);
                        bool a_achemine = false;
                        
                        if (!est_bcast) {
                            size_t num_port_sortie = 0;
                            bool trouve = false;
                            for (size_t l = 0; l < c->nb_lignes; l++) {
                                if (mac_egale(c->table_commutation[l].mac_dest, trame.dest)) {
                                    num_port_sortie = c->table_commutation[l].num_port;
                                    trouve = true;
                                    break;
                                }
                            }
                            
                            if (trouve) {
                                size_t out_port_idx = num_port_sortie - 1;
                                Port *out_p = &c->ports[out_port_idx];
                                if (!active_stp || out_p->etat != PORT_BLOQUE) {
                                    if (out_p->num_port != p->num_port) {
                                        size_t voisin_idx = out_p->appareil_connecte;
                                        liaison *li = &g->liaisons[out_p->index_liaison];
                                        size_t port_entrant_voisin = (li->s1 == voisin_idx) ? li->port_s1 : li->port_s2;
                                        
                                        printf("  [Unicast Acheminement] Envoi trame via Port %zu -> Appareil #%zu\n", 
                                               out_p->num_port, voisin_idx);
                                        
                                        envoie_vers_appareil(g, voisin_idx, &trame, port_entrant_voisin, i);
                                        a_achemine = true;
                                    }
                                } else {
                                    printf("  [Bloqué] Port de sortie %zu est BLOQUÉ. Trame jetée.\n", out_p->num_port);
                                    a_achemine = true;
                                }
                            }
                        }
                        
                        if (est_bcast || !a_achemine) {
                            if (est_bcast) {
                                printf("  [Inondation] Trame broadcast reçue.\n");
                            } else {
                                printf("  [Inondation] Destination MAC inconnue, inondation.\n");
                            }
                            
                            for (size_t out_p_idx = 0; out_p_idx < c->nb_ports_connectes; out_p_idx++) {
                                Port *out_p = &c->ports[out_p_idx];
                                if (out_p_idx != in_port_idx && (!active_stp || out_p->etat != PORT_BLOQUE)) {
                                    size_t voisin_idx = out_p->appareil_connecte;
                                    liaison *li = &g->liaisons[out_p->index_liaison];
                                    size_t port_entrant_voisin = (li->s1 == voisin_idx) ? li->port_s1 : li->port_s2;
                                    
                                    printf("    -> Inondé vers Port %zu -> Appareil #%zu\n", 
                                           out_p->num_port, voisin_idx);
                                    
                                    envoie_vers_appareil(g, voisin_idx, &trame, port_entrant_voisin, i);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 3. traiter les boites de reception pour les stations
    for (size_t i = 0; i < g->nb_stations; i++) {
        Station *st = &g->stations[i];
        size_t global_idx = i + g->nb_commutateurs;
        size_t count = nb_trames_st[i];
        
        for (size_t k = 0; k < count; k++) {
            TrameEthernet trame;
            size_t num_port_entrant;
            size_t appareil_source;
            
            if (defiler(&st->boite_reception, &trame, &num_port_entrant, &appareil_source)) {
                printf("[Tick %zu] Station #%zu [IP: ", temps_courant, global_idx);
                affiche_ip(st->ip);
                printf("] a reçu trame de l'appareil #%zu\n", appareil_source);
                
                if (mac_egale(trame.dest, st->mac) || mac_egale(trame.dest, BROADCAST_MAC)) {
                    printf("  >> MATCH! La Station a traité le message : ");
                    for (size_t ch = 0; ch < trame.taille_donnees; ch++) {
                        putchar(trame.donnees[ch]);
                    }
                    putchar('\n');
                } else {
                    printf("  >> MAC mismatch (Attendu : ");
                    affiche_mac(st->mac);
                    printf(", Reçu : ");
                    affiche_mac(trame.dest);
                    printf("). Trame ignorée.\n");
                }
            }
        }
    }
    
    // les trames en transit pour le prochain tick sont celles restees dans les files
    size_t total_transit = 0;
    for (size_t i = 0; i < g->nb_commutateurs; i++) {
        total_transit += g->commutateurs[i].boite_reception.taille;
    }
    printf("Nombre total de trames EN TRANSIT pour le tick suivant : %zu\n", total_transit);
    if (!active_stp && total_transit > 12) {
        printf("  >>> [ALERTE TEMPÊTE DE BROADCAST] Le nombre de trames augmente de façon incontrôlée !\n");
    }
    
    free(nb_trames_sw);
    free(nb_trames_st);
    
    printf("--- TICK %zu FIN ---\n", temps_courant);
}

void injecte_trame_interne(graphe_reseau *g, size_t temps_courant, size_t src_idx, size_t dest_idx, const char *message) {
    if (src_idx < g->nb_commutateurs || src_idx >= g->ordre) {
        printf("[ERREUR] Index source invalide : %zu\n", src_idx);
        return;
    }

    TrameEthernet trame;
    memcpy(trame.src, g->stations[src_idx - g->nb_commutateurs].mac, 6);

    if (dest_idx == 999) {
        memcpy(trame.dest, BROADCAST_MAC, 6);
    } else {
        if (dest_idx < g->nb_commutateurs || dest_idx >= g->ordre) {
            printf("[ERREUR] Index destination invalide : %zu\n", dest_idx);
            return;
        }
        memcpy(trame.dest, g->stations[dest_idx - g->nb_commutateurs].mac, 6);
    }

    trame.type = 0x0800; // ipv4
    trame.taille_donnees = strlen(message);
    if (trame.taille_donnees > TAILLE_MAX_DONNEES_ETHERNET) trame.taille_donnees = TAILLE_MAX_DONNEES_ETHERNET;
    memcpy(trame.donnees, message, trame.taille_donnees);

    printf("\n=== TRAME INJECTÉE (Temps %zu) ===\n", temps_courant);
    affiche_trame_user(&trame);
    affiche_trame_hex(&trame);
    printf("======================\n");

    bool trouve = false;
    for (size_t l = 0; l < g->nb_liaisons; l++) {
        liaison *li = &g->liaisons[l];
        if (li->s1 == src_idx || li->s2 == src_idx) {
            size_t voisin = (li->s1 == src_idx) ? li->s2 : li->s1;
            size_t port_entrant = (li->s1 == src_idx) ? li->port_s2 : li->port_s1;

            printf("[Station #%zu] -> trame mise en file vers Appareil #%zu (arrivée temps %zu)\n",
                   src_idx, voisin, temps_courant + 1);

            envoie_vers_appareil(g, voisin, &trame, port_entrant, src_idx);
            trouve = true;
            break;
        }
    }
    if (!trouve)
        printf("[ERREUR] Station #%zu n'est connectée à aucune liaison !\n", src_idx);
}

void affiche_transit(const graphe_reseau *g) {
    printf("\n=== TRAMES EN TRANSIT (POUR LE PROCHAIN TICK) ===\n");
    bool vide = true;
    
    for (size_t i = 0; i < g->nb_commutateurs; i++) {
        Commutateur *c = &g->commutateurs[i];
        NoeudTrame *curr = c->boite_reception.tete;
        while (curr != NULL) {
            vide = false;
            printf("  [Vers Commutateur #%zu] de #%zu | Type: 0x%04X | Src: ",
                   i, curr->appareil_source, curr->trame.type);
            affiche_mac(curr->trame.src);
            printf(" | Dst: ");
            affiche_mac(curr->trame.dest);
            printf("\n");
            curr = curr->suivant;
        }
    }
    for (size_t i = 0; i < g->nb_stations; i++) {
        Station *st = &g->stations[i];
        size_t global_idx = i + g->nb_commutateurs;
        NoeudTrame *curr = st->boite_reception.tete;
        while (curr != NULL) {
            vide = false;
            printf("  [Vers Station #%zu] de #%zu | Type: 0x%04X | Src: ",
                   global_idx, curr->appareil_source, curr->trame.type);
            affiche_mac(curr->trame.src);
            printf(" | Dst: ");
            affiche_mac(curr->trame.dest);
            printf("\n");
            curr = curr->suivant;
        }
    }
    
    if (vide) {
        printf("  (vide)\n");
    }
    printf("=========================================================\n");
}

// ──────────────────────────────────────────────────────────────────
//  ordonnanceur evenementiel discret
// ──────────────────────────────────────────────────────────────────
void init_ordonnanceur(Ordonnanceur *o) {
    o->temps_courant = 0;
    o->file_evenements = NULL;
}

void planifier_tick(Ordonnanceur *o, size_t date) {
    Evenement *ev = malloc(sizeof(Evenement));
    if (!ev) {
        perror("malloc evenement tick");
        exit(EXIT_FAILURE);
    }
    ev->type = TYPE_EVENEMENT_TICK;
    ev->date = date;
    ev->appareil_source = 0;
    ev->appareil_dest = 0;
    ev->suivant = NULL;
    
    // insertion triee par date croissante
    if (o->file_evenements == NULL || o->file_evenements->date > date) {
        ev->suivant = o->file_evenements;
        o->file_evenements = ev;
    } else {
        Evenement *curr = o->file_evenements;
        while (curr->suivant != NULL && curr->suivant->date <= date) {
            curr = curr->suivant;
        }
        ev->suivant = curr->suivant;
        curr->suivant = ev;
    }
}

void planifier_injection(Ordonnanceur *o, size_t date, size_t src, size_t dest, const char *msg) {
    Evenement *ev = malloc(sizeof(Evenement));
    if (!ev) {
        perror("malloc evenement injection");
        exit(EXIT_FAILURE);
    }
    ev->type = TYPE_EVENEMENT_INJECTION;
    ev->date = date;
    ev->appareil_source = src;
    ev->appareil_dest = dest;
    strncpy(ev->message, msg, sizeof(ev->message) - 1);
    ev->message[sizeof(ev->message) - 1] = '\0';
    ev->suivant = NULL;
    
    // insertion triee par date croissante
    if (o->file_evenements == NULL || o->file_evenements->date > date) {
        ev->suivant = o->file_evenements;
        o->file_evenements = ev;
    } else {
        Evenement *curr = o->file_evenements;
        while (curr->suivant != NULL && curr->suivant->date <= date) {
            curr = curr->suivant;
        }
        ev->suivant = curr->suivant;
        curr->suivant = ev;
    }
}

void lance_simulation(graphe_reseau *g, Ordonnanceur *o, size_t date_limite, bool active_stp) {
    while (o->file_evenements != NULL && o->file_evenements->date <= date_limite) {
        // retirer l'evenement de la tete
        Evenement *ev = o->file_evenements;
        o->file_evenements = ev->suivant;
        
        // mettre a jour le temps de l'ordonnanceur au temps de l'evenement
        o->temps_courant = ev->date;
        
        if (ev->type == TYPE_EVENEMENT_TICK) {
            // traite le tick interne
            traite_tick_interne(g, o->temps_courant, active_stp);
            
            // planifie automatiquement le prochain tick a temps_courant + 1
            planifier_tick(o, o->temps_courant + 1);
        }
        else if (ev->type == TYPE_EVENEMENT_INJECTION) {
            // injecte le message a la date de l'evenement
            injecte_trame_interne(g, o->temps_courant, ev->appareil_source, ev->appareil_dest, ev->message);
        }
        
        free(ev);
    }
}

void vider_ordonnanceur(Ordonnanceur *o) {
    Evenement *curr = o->file_evenements;
    while (curr != NULL) {
        Evenement *tmp = curr->suivant;
        free(curr);
        curr = tmp;
    }
    o->file_evenements = NULL;
}
