#ifndef VAR_LIST_H
#define VAR_LIST_H

#include <stddef.h>
#include <unistd.h>

#include "stringNthong.h"

namespace varlist {

const size_t NPOS = (size_t) -1;

/**
 * @brief Структура для хранения уникальных имен переменных.
 *
 * Хранит копии строк и упорядоченные по хэшу индексы, что позволяет выполнять
 * бинпоиск по хэшам с проверкой строк. Значение 0 зарезервировано как poison,
 * поэтому внутри списка хэш всегда >= 1.
 */
typedef struct {
    mystr::mystr_t *data;           /**< Массив строк с именами переменных. */
    size_t         *order;          /**< Индексы в data, отсортированные по хэшам. */
    size_t          size;           /**< Количество записанных имен. */
    size_t          capacity;       /**< Общая емкость массивов data и order. */
} VarList;

/**
 * @brief Инициализирует пустой список переменных.
 *
 * @param list Указатель на структуру VarList. Не может быть NULL.
 */
void init(VarList *list);

/**
 * @brief Освобождает все ресурсы списка и обнуляет его.
 *
 * @param list Указатель на список VarList. Допускается NULL.
 */
void destruct(VarList *list);

/**
 * @brief Добавляет имя переменной, возвращая ее индекс.
 *
 * При повторном добавлении уже существующего имени возвращает прежний индекс.
 *
 * @param list Указатель на список VarList. Не может быть NULL.
 * @param name Указатель на строку mystr_t с именем переменной. Не может быть NULL.
 * @return Индекс имени в списке или NPOS при ошибке.
 */
size_t add(VarList *list, const mystr::mystr_t *name);

/**
 * @brief Проверяет наличие имени в списке.
 *
 * @param list Указатель на список VarList.
 * @param name Указатель на строку mystr_t с именем переменной.
 * @return true, если имя присутствует, иначе false.
 */
bool contains(const VarList *list, const mystr::mystr_t *name);

/**
 * @brief Возвращает имя по индексу.
 *
 * @param list Указатель на список VarList.
 * @param index Индекс строки.
 * @return Указатель на строку или NULL, если индекс вне диапазона.
 */
const mystr::mystr_t *get(const VarList *list, size_t index);

/**
 * @brief Возвращает количество имен в списке.
 *
 * @param list Указатель на список VarList.
 * @return Число элементов или 0, если list == NULL.
 */
size_t size(const VarList *list);

/**
 * @brief Ищет индекс имени по строке mystr.
 */
size_t find_index(const VarList *list, const mystr::mystr_t *name);

/**
 * @brief Создает копию списка переменных.
 */
VarList *clone(const VarList *list);

} // namespace varlist

#endif //VAR_LIST_H