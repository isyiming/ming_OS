#ifndef ORDERED_ARRAY_H
#define ORDERED_ARRAY_H

#include <stdint.h>
#include <stddef.h>
#include "mem.h"
#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../libc/common.h"

// sorted_array.h -- Interface for creating, inserting and deleting
// from ordered arrays.
// Written for JamesM's kernel development tutorials.

typedef void* type_t;

/*
 * return an integer greater than, equal to, or less than 0, according as the
 * first argument is greater than, equal to, or less than the second argument.
 */
typedef uint8_t (*lessthan_predicate_t)(type_t,type_t);

typedef struct {
	type_t	*array;
	uint32_t	size;
	uint32_t	max_size;
	lessthan_predicate_t less_than;
}ordered_array_t;

/*
A standard less than predicate.
*/
int8_t standard_lessthan_predicate(type_t a, type_t b);

/**
  Create an ordered array.
**/
ordered_array_t create_ordered_array(uint32_t max_size, lessthan_predicate_t less_than);
ordered_array_t place_ordered_array(void *addr, uint32_t max_size, lessthan_predicate_t less_than);

/**
  Destroy an ordered array.
**/
void destroy_ordered_array(ordered_array_t *array);

/**
  Add an item into the array.
**/
void insert_ordered_array(type_t item, ordered_array_t *array);

/**
  Lookup the item at index i.
**/
type_t lookup_ordered_array(uint32_t i, ordered_array_t *array);

/**
  Deletes the item at location i from the array.
**/
void remove_ordered_array(uint32_t i, ordered_array_t *array);

#endif /* ndef SORTED_ARRAY_H */
