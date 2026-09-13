#ifndef ACTIVE_OPERATION_H
#define ACTIVE_OPERATION_H

#include <stdbool.h>

// Il ne peut jamais y avoir plus d'UN SEUL scan OU UNE SEULE remédiation
// actif à la fois dans tout le process — ce module est la seule source de
// vérité pour cette règle, partagée par les deux routes HTTP.
enum active_operation_kind {
    ACTIVE_OP_NONE,
    ACTIVE_OP_SCAN,
    ACTIVE_OP_REMEDIATE,
};

// Tente de réserver le slot unique pour ce contexte précis. Retourne true si
// la réservation a réussi. Retourne false si un scan ou une remédiation
// tourne déjà — dans ce cas l'appelant doit détruire le contexte qu'il vient
// de créer et répondre "occupé" au client, sans jamais démarrer de thread.
bool active_operation_try_claim(enum active_operation_kind kind, void *ctx);

// Libère le slot SI ET SEULEMENT SI ctx est bien celui qui le détient
// actuellement — appel sans risque même sur un ctx qui n'a jamais réussi à
// réserver le slot (no-op dans ce cas). À appeler une seule fois, au moment
// où le contexte est définitivement détruit (scan_context_free /
// remediate_context_free), jamais avant.
void active_operation_release(void *ctx);

// Annule immédiatement l'opération active, quelle qu'elle soit. Ne fait rien
// si le slot est vide. Utilisé par la route d'annulation explicite ET par
// l'arrêt du serveur (main()).
void active_operation_cancel(void);

// true tant que le slot est occupé — main() attend que ça devienne false
// avant MHD_stop_daemon.
bool active_operation_is_busy(void);

#endif