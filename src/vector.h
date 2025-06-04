#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct GenericVector GenericVector;

// Allocate an array of specified capacity and zero length
GenericVector* NewGenericVector(size_t capacity);
// Free allocated memory
// Each array element should also be freed in case it was previously allocated on heap
void FreeGenericVector(GenericVector* vector);

// Append an element to the array
// Assume the element was allocated on heap
void Append(GenericVector* vector, void* elem);
// Move all the elements from the source vector to the destination vector
// Source vector length is assumed to be zero afterwards
void Extend(GenericVector* vector, GenericVector* source);
// Get an array element by its index
void* GetElement(const GenericVector* vector, size_t idx);

// Get raw pointer on the underlying array of elements
void** GetData(GenericVector* vector);
// Get vector length
size_t GetLength(const GenericVector* vector);
// Check whether the vector is empty
bool IsEmpty(const GenericVector* vector);
