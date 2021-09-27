#include "fs/aio.h"
#include "kernel/errno.h"
#include <limits.h>
#include <string.h>

// Ensure a minimum capacity in the AIOCTX table.
// 
// AIOCTX must be locked before resizing the table. The lock can be elided in
// contexts where you know the table is not shared yet.
// 
// Attempts to shrink the table will be rejected silently.
// May return _ENOMEM if memory for the new table could not be allocated.
static int _aioctx_table_ensure(struct aioctx_table *tbl, unsigned int newcap) {
    if (tbl == NULL) return 0;
    if (tbl->capacity >= newcap) return 0;
    if ((INT_MAX / sizeof(struct aioctx*)) < newcap) return _ENOMEM;

    struct aioctx **new_contexts = malloc(sizeof(struct aioctx*) * newcap);
    if (new_contexts == NULL) return _ENOMEM;

    memset(new_contexts, 0, sizeof(struct aioctx*) * newcap);
    if (tbl->contexts) {
        memcpy(new_contexts, tbl->contexts, sizeof(struct aioctx*) * tbl->capacity);
        free(tbl->contexts);
    }

    tbl->contexts = new_contexts;
    tbl->capacity = newcap;

    return 0;
}

struct aioctx *aioctx_new(int events_capacity) {
    if ((INT_MAX / sizeof(struct aioctx_event)) < events_capacity) return NULL;

    struct aioctx *aioctx = malloc(sizeof(struct aioctx));
    if (aioctx == NULL) return NULL;

    struct aioctx_event *aioctx_events = malloc(sizeof(struct aioctx_event) * events_capacity);
    if (aioctx_events == NULL) {
        free(aioctx);
        return NULL;
    }

    memset(aioctx_events, 0, sizeof(struct aioctx_event) * events_capacity);

    aioctx->events_capacity = events_capacity;
    aioctx->events = aioctx_events;

    return aioctx;
}

void aioctx_retain(struct aioctx *ctx) {
    if (ctx == NULL) return;

    lock(&ctx->lock);
    ctx->refcount++;
    unlock(&ctx->lock);
}

void aioctx_release(struct aioctx *ctx) {
    if (ctx == NULL) return;

    lock(&ctx->lock);
    if (--ctx->refcount == 0) {
        free(ctx->events);
        free(ctx);
    } else {
        unlock(&ctx->lock);
    }
}

struct aioctx_table *aioctx_table_new(unsigned int capacity) {
    struct aioctx_table *tbl = malloc(sizeof(struct aioctx_table));
    if (tbl == NULL) return ERR_PTR(_ENOMEM);
    
    tbl->capacity = 0;
    tbl->contexts = NULL;
    lock_init(&tbl->lock);

    int err = _aioctx_table_ensure(tbl, capacity);
    if (err < 0) return ERR_PTR(err);

    return tbl;
}

void aioctx_table_delete(struct aioctx_table *tbl) {
    if (tbl == NULL) return;
    
    lock(&tbl->lock);
    for (int i = 0; i < tbl->capacity; i += 1) {
        if (tbl->contexts[i] != NULL) {
            aioctx_release(tbl->contexts[i]);
        }
    }
    free(tbl->contexts);
    free(tbl);
}