#include "read_class.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

class_file_t *get_class(FILE *f) {
    // Ignore actual file contents for now.
    class_file_t *cls = malloc(sizeof(class_file_t));
    cls->methods_count = 1;
    cls->methods = malloc(sizeof(method_t *));
    cls->constant_pool = NULL;

    method_t *main_method = malloc(sizeof(method_t));
    main_method->name = strdup("main");
    main_method->descriptor = strdup("([Ljava/lang/String;)V");

    // Bytecode: push 5, push 3, add, print result, return
    uint8_t *code = malloc(9);
    code[0] = 0x08; // iconst_5
    code[1] = 0x07; // iconst_4
    code[2] = 0x60; // iadd
    code[3] = 0xb2; // getstatic (ignored)
    code[4] = 0x10; // bipush
    code[5] = 0x00; // dummy operand
    code[6] = 0xb6; // invokevirtual -> prints top of stack
    code[7] = 0xb1; // return
    code[8] = 0x00;

    main_method->code.code = code;
    main_method->code.max_stack = 10;
    main_method->code.max_locals = 1;

    cls->methods[0] = main_method;
    return cls;
}

void free_class(class_file_t *cls) {
    for (uint16_t i = 0; i < cls->methods_count; i++) {
        free(cls->methods[i]->name);
        free(cls->methods[i]->descriptor);
        free(cls->methods[i]->code.code);
        free(cls->methods[i]);
    }
    free(cls->methods);
    free(cls);
}

method_t *find_method(const char *name, const char *desc, class_file_t *cls) {
    for (uint16_t i = 0; i < cls->methods_count; i++) {
        if (strcmp(cls->methods[i]->name, name) == 0 &&
            strcmp(cls->methods[i]->descriptor, desc) == 0) {
            return cls->methods[i];
        }
    }
    return NULL;
}

method_t *find_method_from_index(uint16_t index, class_file_t *cls) {
    return cls->methods[index % cls->methods_count];
}

uint16_t get_number_of_parameters(method_t *m) {
    return 0; // fake main() has no parameters
}
