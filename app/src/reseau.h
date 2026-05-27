#pragma once
#include <stdint.h> 
#include <stdlib.h>
#include <stdbool.h>

typedef uint8_t ip_addr[4];   // tableau de 32 bits 
typedef uint8_t mac_addr[6];  // tableau de 48 bits 

typedef struct Station {
    mac_addr mac;
    ip_addr ip;
} Station;

typedef struct ligne_commutation {
    mac_addr mac_destination;
    size_t num_port;
} ligne_commutation;

typedef struct Switch {
    mac_addr mac;
    size_t nb_ports;
    size_t priorite;
    ligne_commutation *table_commutation;
    size_t capacite_table;
    size_t nb_lignes;
} Switch;

typedef struct arete {
    size_t s1;  
    size_t s2; 
    int poids; 
    // bool bloquer;  en prévision de protocol STP 
} arete;

typedef struct graphe_reseau {
    Switch *switchs;          
    size_t nb_switchs;        
    
    Station *stations;        
    size_t nb_stations;       
    
    
    size_t ordre;  

    arete *aretes;             
    size_t nb_arete;          
    size_t capacite_arete;    
} graphe_reseau;