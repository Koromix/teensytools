/* Teensy Tools - public domain
   Niels Martignène <niels.martignene@gmail.com>
   https://neodd.com/teensytools

   This software is in the public domain. Where that dedication is not
   recognized, you are granted a perpetual, irrevocable license to copy,
   distribute, and modify this file as you see fit.

   See the LICENSE file for more details. */

#include "common_priv.h"
#include "htable.h"

int ty_htable_init(ty_htable *table, unsigned int size)
{
    table->heads = malloc(size * sizeof(*table->heads));
    if (!table->heads)
        return ty_error(TY_ERROR_MEMORY, NULL);
    table->size = size;

    ty_htable_clear(table);

    return 0;
}

void ty_htable_release(ty_htable *table)
{
    free(table->heads);
}

ty_htable_head *ty_htable_get_head(ty_htable *table, uint32_t key)
{
    return (ty_htable_head *)&table->heads[key % table->size];
}

void ty_htable_add(ty_htable *table, uint32_t key, ty_htable_head *n)
{
    ty_htable_head *head = ty_htable_get_head(table, key);

    n->key = key;

    n->next = head->next;
    head->next = n;
}

void ty_htable_insert(ty_htable_head *prev, ty_htable_head *n)
{
    n->key = prev->key;

    n->next = prev->next;
    prev->next = n;
}

void ty_htable_remove(ty_htable_head *head)
{
    for (ty_htable_head *prev = head->next; prev != head; prev = prev->next) {
        if (prev->next == head) {
            prev->next = head->next;
            head->next = NULL;

            break;
        }
    }
}

void ty_htable_clear(ty_htable *table)
{
    for (unsigned int i = 0; i < table->size; i++)
        table->heads[i] = (ty_htable_head *)&table->heads[i];
}
