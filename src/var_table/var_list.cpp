#include <stdlib.h>
#include <string.h>

#include "var_list.h"
#include "base.h"

namespace varlist {

using mystr::mystr_t;

/**
 * @brief Находит позицию вставки для заданного хэша в массиве order.
 *
 * @param list Указатель на список.
 * @param hash Целевое значение хэша.
 * @return Индекс в массиве order.
 */
function size_t lower_bound(const VarList *list, unsigned long hash) {
    size_t left = 0, right = list ? list->size : 0;
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        size_t idx = list->order[mid];
        unsigned long cur = list->data[idx].hash;
        if (cur < hash) left = mid + 1; else right = mid;
    }
    return left;
}

/**
 * @brief Ищет индекс строки в списке.
 *
 * @param list Указатель на список.
 * @param name Указатель на строку.
 * @return Индекс строки или VAR_LIST_NPOS.
 */
function size_t find_internal(const VarList *list, const mystr_t *name) {
    if (!list || !name || !list->size) return NPOS;
    unsigned long hash = name->hash;
    if (!hash) return NPOS;
    size_t pos = lower_bound(list, hash);
    while (pos < list->size) {
        size_t idx = list->order[pos];
        if (list->data[idx].hash != hash) break;
        if (list->data[idx].is_same(name)) return idx;
        ++pos;
    }
    return NPOS;
}

function int ensure_capacity(VarList *list, size_t need) {
    if (!list) return -1;
    if (list->capacity >= need) return 0;
    size_t cap = list->capacity ? list->capacity : 4;
    while (cap < need) cap <<= 1;
    mystr_t *new_data = (mystr_t *) malloc(cap * sizeof(mystr_t));
    if (!new_data) return -1;
    size_t *new_order = (size_t *) malloc(cap * sizeof(size_t));
    if (!new_order) {
        free(new_data);
        return -1;
    }
    if (list->data) memcpy(new_data, list->data, list->size * sizeof(mystr_t));
    if (list->order) memcpy(new_order, list->order, list->size * sizeof(size_t));
    free(list->data);
    free(list->order);
    list->data = new_data;
    list->order = new_order;
    list->capacity = cap;
    return 0;
}

void init(VarList *list) {
    if (!list) return;
    list->data = nullptr;
    list->order = nullptr;
    list->size = 0;
    list->capacity = 0;
}

void destruct(VarList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->size; ++i)
        free(list->data[i].str);
    free(list->data);
    free(list->order);
    list->data = nullptr;
    list->order = nullptr;
    list->size = 0;
    list->capacity = 0;
}

size_t add(VarList *list, const mystr_t *name) {
    if (!list || !name || !name->hash)
        return NPOS;
    size_t existing = find_internal(list, name);
    if (existing != NPOS)
        return existing;
    if (ensure_capacity(list, list->size + 1))
        return NPOS;
    mystr_t copy = mystr::dupe(name);
    if (!copy.str || !copy.hash)
        return NPOS;
    size_t insert_pos = lower_bound(list, copy.hash);
    size_t new_idx = list->size;
    if (insert_pos < new_idx)
        memmove(list->order + insert_pos + 1, list->order + insert_pos, (new_idx - insert_pos) * sizeof(size_t));
    list->order[insert_pos] = new_idx;
    list->data[new_idx] = copy;
    list->size = new_idx + 1;
    return new_idx;
}

bool contains(const VarList *list, const mystr_t *name) {
    return find_internal(list, name) != NPOS;
}

const mystr_t *get(const VarList *list, size_t index) {
    return (list && index < list->size) ? &list->data[index] : nullptr;
}

size_t size(const VarList *list) {
    return list ? list->size : 0;
}

size_t find_index(const VarList *list, const mystr_t *name) {
    if (!list || !name) return NPOS;
    return find_internal(list, name);
}

VarList *clone(const VarList *list) {
    if (!list) return nullptr;
    VarList *copy = TYPED_CALLOC(1, VarList);
    if (!copy) return nullptr;
    init(copy);
    for (size_t i = 0; i < list->size; ++i) {
        if (add(copy, &list->data[i]) == NPOS) {
            destruct(copy);
            FREE(copy);
            return nullptr;
        }
    }
    return copy;
}

} // namespace varlist

