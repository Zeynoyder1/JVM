// read_class.h
#ifndef READ_CLASS_H
#define READ_CLASS_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint8_t *code;
    uint16_t max_stack;
    uint16_t max_locals;
} code_attribute_t;

typedef struct {
    char *name;
    char *descriptor;
    code_attribute_t code;
} method_t;

typedef struct {
    void *info;
} cp_info_t;

typedef struct {
    cp_info_t *constant_pool;
    method_t **methods;
    uint16_t methods_count;
} class_file_t;

class_file_t *get_class(FILE *f);
void free_class(class_file_t *c);
method_t *find_method(const char *name, const char *desc, class_file_t *cls);
method_t *find_method_from_index(uint16_t index, class_file_t *cls);
uint16_t get_number_of_parameters(method_t *m);

#endif
