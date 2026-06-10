#include "reseau.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cree_reseau(graphe_reseau *g) {
    g->commutateurs = NULL;
    g->stations = NULL;
    g->liaisons = NULL;
    g->nb_commutateurs = 0;
    g->nb_stations = 0;
    g->nb_liaisons = 0;
    g->ordre = 0;
}

void affiche_mac(const adr_mac mac) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void affiche_ip(const adr_ip ip) {
    printf("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void affiche_commutateur(const Commutateur *c) {
    printf("Commutateur [MAC: ");
    affiche_mac(c->mac);
    printf("] - Priorité: %zu - Ports: %zu connectés / %zu total\n", 
           c->priorite, c->nb_ports_connectes, c->nb_ports);
    
    // etat stp
    printf("  État STP: Racine crue est ");
    affiche_mac(c->mac_racine);
    printf(" (Priorité: %d), Coût Chemin: %d", c->priorite_racine, c->cout_chemin_racine);
    if (c->index_port_racine != SIZE_MAX) {
        printf(", Port Racine: Port %zu\n", c->ports[c->index_port_racine].num_port);
    } else {
        printf(", Port Racine: Aucun (Lui-même est Racine)\n");
    }
    
    // liste des ports
    printf("  Détail des ports:\n");
    for (size_t i = 0; i < c->nb_ports_connectes; i++) {
        Port p = c->ports[i];
        printf("    Port %zu -> Connecté à l'appareil #%zu (Liaison #%zu) | État STP: %s\n",
               p.num_port, p.appareil_connecte, p.index_liaison,
               (p.etat == PORT_RACINE ? "RACINE" : 
                (p.etat == PORT_DESIGNE ? "DESIGNÉ" : "BLOQUÉ")));
    }
    
    // table de commutation
    printf("  Table de commutation (%zu/%zu lignes):\n", c->nb_lignes, c->capacite_table);
    if (c->nb_lignes == 0) {
        printf("    (Vide)\n");
    } else {
        for (size_t i = 0; i < c->nb_lignes; i++) {
            printf("    MAC Destination: ");
            affiche_mac(c->table_commutation[i].mac_dest);
            printf(" -> Port de sortie: %zu\n", c->table_commutation[i].num_port);
        }
    }
}

void affiche_station(const Station *st) {
    printf("Station [MAC: ");
    affiche_mac(st->mac);
    printf(" | IP: ");
    affiche_ip(st->ip);
    printf("]\n");
}

void affiche_reseau(const graphe_reseau *g) {
    printf("==================== TOPOLOGIE RÉSEAU ====================\n");
    printf("Ordre (Total Appareils): %zu | Commutateurs: %zu | Stations: %zu | Liaisons: %zu\n\n",
           g->ordre, g->nb_commutateurs, g->nb_stations, g->nb_liaisons);
    
    printf("--- COMMUTATEURS ---\n");
    for (size_t i = 0; i < g->nb_commutateurs; i++) {
        printf("Appareil #%zu: ", i);
        affiche_commutateur(&g->commutateurs[i]);
        printf("\n");
    }
    
    printf("--- STATIONS ---\n");
    for (size_t i = 0; i < g->nb_stations; i++) {
        printf("Appareil #%zu (Offset station %zu): ", i + g->nb_commutateurs, i);
        affiche_station(&g->stations[i]);
    }
    printf("\n");
    
    printf("--- LIAISONS ---\n");
    for (size_t i = 0; i < g->nb_liaisons; i++) {
        liaison l = g->liaisons[i];
        printf("Liaison #%zu: Appareil #%zu", i, l.s1);
        if (l.s1 < g->nb_commutateurs) {
            printf(" (Port %zu)", l.port_s1);
        }
        printf(" <---> Appareil #%zu", l.s2);
        if (l.s2 < g->nb_commutateurs) {
            printf(" (Port %zu)", l.port_s2);
        }
        printf(" | Coût (Poids): %d\n", l.poids);
    }
    printf("==========================================================\n");
}

int chargeur_reseau(const char *nom_fichier, graphe_reseau *g) {
    FILE *f = fopen(nom_fichier, "r");
    if (f == NULL) {
        // essai dans le dossier parent et le dossier config (pratique si execute depuis app/bin ou app)
        char chemin_alt[512];
        snprintf(chemin_alt, sizeof(chemin_alt), "config/%s", nom_fichier);
        f = fopen(chemin_alt, "r");
        if (f == NULL) {
            snprintf(chemin_alt, sizeof(chemin_alt), "../%s", nom_fichier);
            f = fopen(chemin_alt, "r");
            if (f == NULL) {
                snprintf(chemin_alt, sizeof(chemin_alt), "../config/%s", nom_fichier);
                f = fopen(chemin_alt, "r");
                if (f == NULL) {
                    snprintf(chemin_alt, sizeof(chemin_alt), "../../%s", nom_fichier);
                    f = fopen(chemin_alt, "r");
                    if (f == NULL) {
                        snprintf(chemin_alt, sizeof(chemin_alt), "../../config/%s", nom_fichier);
                        f = fopen(chemin_alt, "r");
                    }
                }
            }
        }
    }
    
    if (f == NULL) {
        fprintf(stderr, "Erreur fatale : Impossible d'ouvrir le fichier de configuration '%s' (essayé aussi dans les dossiers parents).\n", nom_fichier);
        return 0; 
    }
    
    cree_reseau(g);
    
    size_t nb_equipements = 0;
    size_t nb_liens = 0;
    
    if (fscanf(f, "%zu %zu", &nb_equipements, &nb_liens) != 2) {
        fprintf(stderr, "Erreur format ligne d'entete\n");
        fclose(f);
        return 0;
    }
    
    g->ordre = nb_equipements;
    g->nb_liaisons = nb_liens;
    
    size_t count_switches = 0;
    size_t count_stations = 0;
    
    long pos_after_header = ftell(f);
    
    char line[512];
    for (size_t i = 0; i < nb_equipements; i++) {
        if (!fgets(line, sizeof(line), f)) {
            break;
        }
        if (line[0] == '\n' || line[0] == '\r') {
            i--; // ignore la ligne vide
            continue;
        }
        
        int type = 0;
        if (sscanf(line, "%d", &type) == 1) {
            if (type == 1) count_stations++;
            else if (type == 2) count_switches++;
        }
    }
    
    g->nb_commutateurs = count_switches;
    g->nb_stations = count_stations;
    
    g->commutateurs = calloc(count_switches, sizeof(Commutateur));
    g->stations = calloc(count_stations, sizeof(Station));
    g->liaisons = calloc(nb_liens, sizeof(liaison));
    
    fseek(f, pos_after_header, SEEK_SET);
    
    size_t curr_sw = 0;
    size_t curr_st = 0;
    
    for (size_t i = 0; i < nb_equipements; i++) {
        if (!fgets(line, sizeof(line), f)) break;
        if (line[0] == '\n' || line[0] == '\r') {
            i--;
            continue;
        }
        
        if (line[0] == '2') { // commutateur
            // format: 2;mac;nb_ports;priorite
            unsigned int m[6];
            size_t nb_ports = 0;
            size_t priorite = 0;
            if (sscanf(line, "2;%2x:%2x:%2x:%2x:%2x:%2x;%zu;%zu", 
                       &m[0], &m[1], &m[2], &m[3], &m[4], &m[5], &nb_ports, &priorite) == 8) {
                Commutateur *c = &g->commutateurs[curr_sw++];
                for (int j = 0; j < 6; j++) c->mac[j] = (uint8_t)m[j];
                c->nb_ports = nb_ports;
                c->priorite = priorite;
                c->nb_ports_connectes = 0;
                c->ports = calloc(nb_ports, sizeof(Port));
                for (size_t p = 0; p < nb_ports; p++) {
                    c->ports[p].num_port = p + 1; // port 1-base
                    c->ports[p].etat = PORT_BLOQUE;   // commence bloque
                    c->ports[p].appareil_connecte = SIZE_MAX;
                    c->ports[p].index_liaison = SIZE_MAX;
                    c->ports[p].a_recu_bpdu = false;
                }
                
                // table de commutation
                c->capacite_table = 10;
                c->nb_lignes = 0;
                c->table_commutation = calloc(c->capacite_table, sizeof(ligne_commutation));
                
                // stp initialise a soi-meme comme racine
                memcpy(c->mac_racine, c->mac, 6);
                c->priorite_racine = c->priorite;
                c->cout_chemin_racine = 0;
                c->index_port_racine = SIZE_MAX;

                // files locales
                init_file(&c->boite_reception);
            }
        } else if (line[0] == '1') { // station
            // format: 1;mac;ip
            unsigned int m[6];
            unsigned int ip_bytes[4];
            if (sscanf(line, "1;%2x:%2x:%2x:%2x:%2x:%2x;%u.%u.%u.%u", 
                       &m[0], &m[1], &m[2], &m[3], &m[4], &m[5],
                       &ip_bytes[0], &ip_bytes[1], &ip_bytes[2], &ip_bytes[3]) == 10) {
                Station *st = &g->stations[curr_st++];
                for (int j = 0; j < 6; j++) st->mac[j] = (uint8_t)m[j];
                for (int j = 0; j < 4; j++) st->ip[j] = (uint8_t)ip_bytes[j];

                // files locales
                init_file(&st->boite_reception);
            }
        }
    }
    
    // lecture des liens
    size_t curr_edge = 0;
    while (curr_edge < nb_liens && fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        size_t s1 = 0, s2 = 0;
        int poids = 0;
        if (sscanf(line, "%zu;%zu;%d", &s1, &s2, &poids) == 3) {
            liaison *l = &g->liaisons[curr_edge];
            l->s1 = s1;
            l->s2 = s2;
            l->poids = poids;
            
            // assignation des ports
            if (s1 < g->nb_commutateurs) {
                Commutateur *c1 = &g->commutateurs[s1];
                size_t p_idx = c1->nb_ports_connectes;
                if (p_idx < c1->nb_ports) {
                    c1->ports[p_idx].appareil_connecte = s2;
                    c1->ports[p_idx].index_liaison = curr_edge;
                    l->port_s1 = p_idx + 1; // 1-base
                    c1->nb_ports_connectes++;
                } else {
                    fprintf(stderr, "Attention: Commutateur %zu a dépassé son nombre max de ports (%zu)\n", s1, c1->nb_ports);
                }
            } else {
                l->port_s1 = 0; // station
            }
            
            if (s2 < g->nb_commutateurs) {
                Commutateur *c2 = &g->commutateurs[s2];
                size_t p_idx = c2->nb_ports_connectes;
                if (p_idx < c2->nb_ports) {
                    c2->ports[p_idx].appareil_connecte = s1;
                    c2->ports[p_idx].index_liaison = curr_edge;
                    l->port_s2 = p_idx + 1; // 1-base
                    c2->nb_ports_connectes++;
                } else {
                    fprintf(stderr, "Attention: Commutateur %zu a dépassé son nombre max de ports (%zu)\n", s2, c2->nb_ports);
                }
            } else {
                l->port_s2 = 0; // station
            }
            
            curr_edge++;
        }
    }
    
    fclose(f);
    return 1;
}

void affiche_trame_user(const TrameEthernet *t) {
    printf("Trame Ethernet [Type: 0x%04X] | Taille données: %zu octets\n", t->type, t->taille_donnees);
    printf("  Source MAC:      ");
    affiche_mac(t->src);
    printf("\n  Destination MAC: ");
    affiche_mac(t->dest);
    printf("\n");
    
    if (t->type == 0x4242) { // type bpdu
        DonneesBPDU bpdu;
        if (t->taille_donnees >= sizeof(DonneesBPDU)) {
            memcpy(&bpdu, t->donnees, sizeof(DonneesBPDU));
            printf("  -- Contenu STP BPDU --\n");
            printf("    ID Pont Racine:      Prio: %u, MAC: ", bpdu.priorite_racine);
            affiche_mac(bpdu.mac_racine);
            printf("\n    Coût Chemin Racine:  %u\n", bpdu.cout_chemin_racine);
            printf("    ID Pont Émetteur:    Prio: %u, MAC: ", bpdu.priorite_emetteur);
            affiche_mac(bpdu.mac_emetteur);
            printf("\n    ID Port Émetteur:    %u\n", bpdu.id_port_emetteur);
        }
    } else {
        printf("  Données (texte si imprimable):\n    ");
        for (size_t i = 0; i < t->taille_donnees && i < 64; i++) {
            char c = t->donnees[i];
            if (c >= 32 && c <= 126) printf("%c", c);
            else printf(".");
        }
        if (t->taille_donnees > 64) printf("...");
        printf("\n");
    }
}

void affiche_trame_hex(const TrameEthernet *t) {
    size_t total_len = 6 + 6 + 2 + t->taille_donnees;
    
    uint8_t *raw = malloc(total_len);
    if (raw == NULL) return;
    
    // Dest MAC
    memcpy(raw, t->dest, 6);
    // Src MAC
    memcpy(raw + 6, t->src, 6);
    // Type (gros boutiste)
    raw[12] = (t->type >> 8) & 0xFF;
    raw[13] = t->type & 0xFF;
    // Données
    memcpy(raw + 14, t->donnees, t->taille_donnees);
    
    printf("Dump Hexa Trame Ethernet Simplifiée (%zu octets):\n", total_len);
    for (size_t i = 0; i < total_len; i++) {
        printf("%02X ", raw[i]);
        if ((i + 1) % 16 == 0) printf("\n");
        else if ((i + 1) % 8 == 0) printf("  ");
    }
    if (total_len % 16 != 0) printf("\n");
    
    free(raw);
}