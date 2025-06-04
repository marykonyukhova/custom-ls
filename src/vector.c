#include "vector.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

struct GenericVector {
    void** arr_;
    size_t len_;
    size_t capacity_;
};

// Создание нового вектора
GenericVector* NewGenericVector(size_t capacity) {
    GenericVector* vector = (GenericVector*)malloc(sizeof(GenericVector));
    if (!vector) return NULL;

    vector->arr_ = malloc(capacity * sizeof(void*));
    if (!vector->arr_) {
        free(vector);
        return NULL;
    }
    vector->len_ = 0;
    vector->capacity_ = capacity;

    return vector;
}

// Освобождение памяти
void FreeGenericVector(GenericVector* vector) {
    if (!vector) return;

    // Освобождение каждого элемента, если он был выделен на куче
    for (size_t i = 0; i < vector->len_; i++) {
        // Если предполагается, что элементы вектора выделены на куче, можно их освободить
        free(vector->arr_[i]);  // Здесь стоит убедиться, что элементы были динамически выделены
    }
    free(vector->arr_);
    free(vector);
}

// Добавление элемента в конец вектора
void Append(GenericVector* vector, void* elem) {
    if (vector->len_ == vector->capacity_) {
        // Используем промежуточную переменную для безопасности realloc
        void** new_arr = realloc(vector->arr_, vector->capacity_ * 2 * sizeof(void*));
        if (!new_arr) return;  // Если realloc не удался, возвращаемся без изменений
        vector->arr_ = new_arr;  // Теперь обновляем указатель на новый массив
        vector->capacity_ *= 2;
    }

    vector->arr_[(vector->len_++)] = elem;
}

// // Перемещение всех элементов из source в vector
// void Extend(GenericVector* vector, GenericVector* source) {
//     for (size_t i = 0; i < source->len_; i++) {
//         Append(vector, source->arr_[i]);
//         source->arr_[i] = NULL; // Обнуляем ссылку на перенесенный элемент
//     }
//     source->len_ = 0;
// }

// Получение элемента по индексу
void* GetElement(const GenericVector* vector, size_t idx) {
    return (idx < vector->len_) ? vector->arr_[idx] : NULL;
}

// // Получение указателя на массив данных
// void** GetData(GenericVector* vector) {
//     return vector->arr_;
// }

// Получение текущей длины
size_t GetLength(const GenericVector* vector) {
    return vector->len_;
}

// // Проверка, пуст ли вектор
// bool IsEmpty(const GenericVector* vector) {
//     return (vector->len_ == 0);
// }
