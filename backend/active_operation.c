#include <pthread.h>
#include <stddef.h>
#include "active_operation.h"
#include "scan.h"
#include "remediate.h"

// Jamais gardé plus que le temps d'une lecture/écriture de pointeur ou d'un
// appel à *_cancel (rapide, non bloquant, ne fait que poser un flag et
// éventuellement réveiller une connexion suspendue) — jamais gardé pendant
// xccdf_session_evaluate lui-même.
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static enum active_operation_kind g_kind = ACTIVE_OP_NONE;
static void *g_ctx = NULL;

bool active_operation_try_claim(enum active_operation_kind kind, void *ctx) {
    pthread_mutex_lock(&g_lock);
    if (g_kind != ACTIVE_OP_NONE) {
        pthread_mutex_unlock(&g_lock);
        return false;
    }
    g_kind = kind;
    g_ctx = ctx;
    pthread_mutex_unlock(&g_lock);
    return true;
}

void active_operation_release(void *ctx) {
    pthread_mutex_lock(&g_lock);
    if (g_ctx == ctx) {
        g_kind = ACTIVE_OP_NONE;
        g_ctx = NULL;
    }
    pthread_mutex_unlock(&g_lock);
}

void active_operation_cancel(void) {
    // g_lock reste tenu PENDANT l'appel à *_cancel — c'est ce qui empêche
    // active_operation_release (appelé depuis *_context_free, qui a aussi
    // besoin de g_lock) de s'exécuter en même temps et de libérer/détruire
    // ctx pendant qu'on est en train de le déréférencer ici.
    pthread_mutex_lock(&g_lock);
    if (g_kind == ACTIVE_OP_SCAN) {
        scan_cancel((struct scan_context *)g_ctx);
    } else if (g_kind == ACTIVE_OP_REMEDIATE) {
        remediate_cancel((struct remediate_context *)g_ctx);
    }
    pthread_mutex_unlock(&g_lock);
}

bool active_operation_is_busy(void) {
    pthread_mutex_lock(&g_lock);
    bool busy = (g_kind != ACTIVE_OP_NONE);
    pthread_mutex_unlock(&g_lock);
    return busy;
}