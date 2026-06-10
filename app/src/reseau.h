#pragma once
#include <stdint.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

typedef uint8_t adr_ip[4];   // adresse ip de 32 bits
typedef uint8_t adr_mac[6];  // adresse mac de 48 bits

#define TAILLE_MAX_DONNEES_ETHERNET 1500

// format de trame ethernet simplifiee
typedef struct {
    adr_mac dest;        // mac destination
    adr_mac src;         // mac source
    uint16_t type;       // type (ex: 0x0800 pour ipv4, 0x4242 pour bpdu)
    uint8_t donnees[TAILLE_MAX_DONNEES_ETHERNET];
    size_t taille_donnees;
} TrameEthernet;

// format des donnees bpdu
typedef struct {
    adr_mac mac_racine;         // mac du pont racine (root bridge)
    uint32_t priorite_racine;   // priorite du pont racine
    uint32_t cout_chemin_racine;// cout du chemin vers la racine
    adr_mac mac_emetteur;       // mac du commutateur emetteur
    uint32_t priorite_emetteur; // priorite du commutateur emetteur
    uint16_t id_port_emetteur;  // id du port emetteur
} DonneesBPDU;

// element de la file d'attente
typedef struct NoeudTrame {
    TrameEthernet trame;
    size_t num_port_entrant; // port de reception (1-base, 0 si direct depuis station)
    size_t appareil_source;  // index de l'appareil emetteur dans le graphe
    struct NoeudTrame *suivant;
} NoeudTrame;

typedef struct FileTrame {
    NoeudTrame *tete;
    NoeudTrame *queue;
    size_t taille;
} FileTrame;

static inline void init_file(FileTrame *f) {
    f->tete = NULL;
    f->queue = NULL;
    f->taille = 0;
}

static inline void enfiler(FileTrame *f, const TrameEthernet *trame, size_t num_port_entrant, size_t appareil_source) {
    NoeudTrame *noeud = malloc(sizeof(NoeudTrame));
    if (!noeud) {
        perror("malloc noeud trame");
        exit(EXIT_FAILURE);
    }
    noeud->trame = *trame;
    noeud->num_port_entrant = num_port_entrant;
    noeud->appareil_source = appareil_source;
    noeud->suivant = NULL;
    if (f->queue == NULL) {
        f->tete = noeud;
        f->queue = noeud;
    } else {
        f->queue->suivant = noeud;
        f->queue = noeud;
    }
    f->taille++;
}

static inline bool defiler(FileTrame *f, TrameEthernet *trame, size_t *num_port_entrant, size_t *appareil_source) {
    if (f->tete == NULL) return false;
    NoeudTrame *noeud = f->tete;
    if (trame) *trame = noeud->trame;
    if (num_port_entrant) *num_port_entrant = noeud->num_port_entrant;
    if (appareil_source) *appareil_source = noeud->appareil_source;
    f->tete = noeud->suivant;
    if (f->tete == NULL) {
        f->queue = NULL;
    }
    free(noeud);
    f->taille--;
    return true;
}

static inline void vider_file(FileTrame *f) {
    TrameEthernet t;
    size_t pe, as;
    while (defiler(f, &t, &pe, &as));
}

typedef struct Station {
    adr_mac mac;
    adr_ip ip;
    FileTrame boite_reception;
} Station;

typedef struct ligne_commutation {
    adr_mac mac_dest;
    size_t num_port;
} ligne_commutation;

// etats des ports stp
typedef enum {
    PORT_BLOQUE,
    PORT_RACINE,
    PORT_DESIGNE
} EtatPort;

// structure detaillee d'un port sur un commutateur
typedef struct Port {
    size_t num_port;             // index 1-base du port
    EtatPort etat;               // etat stp (PORT_BLOQUE, PORT_RACINE, PORT_DESIGNE)
    size_t appareil_connecte;    // index de l'appareil connecte dans le graphe (SIZE_MAX si non connecte)
    size_t index_liaison;        // index de la liaison correspondante dans le graphe (SIZE_MAX si non connecte)
    
    // dernier bpdu recu sur ce port (pour comparaisons stp)
    bool a_recu_bpdu;
    DonneesBPDU bpdu_recu;       // tout est regroupe ici
} Port;

typedef struct Commutateur {
    adr_mac mac;
    size_t nb_ports;
    size_t priorite;
    ligne_commutation *table_commutation;
    size_t capacite_table;
    size_t nb_lignes;
    
    Port *ports;
    size_t nb_ports_connectes; // ports configures par les liaisons
    
    // etat stp propre au commutateur
    adr_mac mac_racine;
    uint32_t priorite_racine;
    uint32_t cout_chemin_racine;
    size_t index_port_racine; // index 0-base dans le tableau de ports, SIZE_MAX si aucun (elu racine)

    FileTrame boite_reception;
} Commutateur;

typedef struct liaison {
    size_t s1;  
    size_t s2; 
    int poids; 
    size_t port_s1; // port affecte sur s1 (si s1 est un commutateur)
    size_t port_s2; // port affecte sur s2 (si s2 est un commutateur)
} liaison;

typedef struct graphe_reseau {
    Commutateur *commutateurs;          
    size_t nb_commutateurs;        
    
    Station *stations;        
    size_t nb_stations;       
    
    size_t ordre;  

    liaison *liaisons;             
    size_t nb_liaisons;          
} graphe_reseau;

// systeme d'evenements discrets
typedef enum {
    TYPE_EVENEMENT_TICK,
    TYPE_EVENEMENT_INJECTION
} TypeEvenement;

typedef struct Evenement {
    TypeEvenement type;
    size_t date;
    size_t appareil_source;
    size_t appareil_dest;
    char message[256];
    struct Evenement *suivant;
} Evenement;

typedef struct Ordonnanceur {
    size_t temps_courant;
    Evenement *file_evenements;
} Ordonnanceur;

// prototypes des fonctions
void cree_reseau(graphe_reseau *g);
int chargeur_reseau(const char *nom_fichier, graphe_reseau *g);
void affiche_mac(const adr_mac mac);
void affiche_ip(const adr_ip ip);
void affiche_commutateur(const Commutateur *c);
void affiche_station(const Station *st);
void affiche_reseau(const graphe_reseau *g);
void affiche_trame_user(const TrameEthernet *t);
void affiche_trame_hex(const TrameEthernet *t);

bool mac_egale(const adr_mac m1, const adr_mac m2);
void envoie_vers_appareil(graphe_reseau *g, size_t appareil_dest, const TrameEthernet *trame, size_t num_port_entrant, size_t appareil_source);
void reevalue_stp(graphe_reseau *g, size_t commutateur_idx);
void envoie_bpdu(graphe_reseau *g, size_t commutateur_idx, size_t port_idx);
void apprend_mac(Commutateur *c, const adr_mac src, size_t num_port);
void traite_tick_interne(graphe_reseau *g, size_t temps_courant, bool active_stp);
void injecte_trame_interne(graphe_reseau *g, size_t temps_courant, size_t src_idx, size_t dest_idx, const char *message);
void affiche_transit(const graphe_reseau *g);

// ordonnanceur evenementiel discret
void init_ordonnanceur(Ordonnanceur *o);
void planifier_tick(Ordonnanceur *o, size_t date);
void planifier_injection(Ordonnanceur *o, size_t date, size_t src, size_t dest, const char *msg);
void lance_simulation(graphe_reseau *g, Ordonnanceur *o, size_t date_limite, bool active_stp);
void vider_ordonnanceur(Ordonnanceur *o);