#include "jvm.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.h"
#include "read_class.h"

const int ERROR = 99;
/** The name of the method to invoke to run the class file */
const char MAIN_METHOD[] = "main";
/**
 * The "descriptor" string for main(). The descriptor encodes main()'s signature,
 * i.e. main() takes a String[] and returns void.
 * If you're interested, the descriptor string is explained at
 * https://docs.oracle.com/javase/specs/jvms/se12/html/jvms-4.html#jvms-4.3.2.
 */
const char MAIN_DESCRIPTOR[] = "([Ljava/lang/String;)V";

/**
 * Represents the return value of a Java method: either void or an int or a reference.
 * For simplification, we represent a reference as an index into a heap-allocated array.
 * (In a real JVM, methods could also return object references or other primitives.)
 */
typedef struct {
    /** Whether this returned value is an int */
    bool has_value;
    /** The returned value (only valid if `has_value` is true) */
    int32_t value;
} optional_value_t;

void binary_arithmetic(uint8_t op, int32_t *stack, uint32_t *top) {
    if (*top < 2) {
        exit(ERROR);
    }
    int32_t b = stack[--(*top)];
    int32_t a = stack[--(*top)];
    int32_t result;
    switch (op) {
        case i_iadd:
            result = a + b;
            break;
        case i_isub:
            result = a - b;
            break;
        case i_imul:
            result = a * b;
            break;
        case i_idiv:
            if (b == 0) {
                exit(ERROR);
            }
            result = a / b;
            break;
        case i_irem:
            if (b == 0) {
                exit(ERROR);
            }
            result = a % b;
            break;
        case i_iand:
            result = a & b;
            break;
        case i_ior:
            result = a | b;
            break;
        case i_ixor:
            result = a ^ b;
            break;
        default:
            exit(ERROR);
    }
    stack[(*top)++] = result;
}

/**
 * Runs a method's instructions until the method returns.
 *
 * @param method the method to run
 * @param locals the array of local variables, including the method parameters.
 *   Except for parameters, the locals are uninitialized.
 * @param class the class file the method belongs to
 * @param heap an array of heap-allocated pointers, useful for references
 * @return an optional int containing the method's return value
 */
optional_value_t execute(method_t *method, int32_t *locals, class_file_t *class,
                         heap_t *heap) {
    uint8_t *code = method->code.code;
    int32_t *stack = (int32_t *) malloc(method->code.max_stack * sizeof(int32_t));
    if (!stack) {
        exit(ERROR);
    }

    uint32_t top = 0;
    uint32_t counter = 0;
    optional_value_t result = {.has_value = false};
    while (1) {
        uint8_t op = (uint8_t) code[counter];
        switch (op) {
            case i_bipush:;
                int8_t temp = (int8_t) code[counter + 1];
                stack[top++] = (int32_t) temp;
                counter += 2;
                break;
            case i_return:;
                free(stack);
                return result;
                break;
            case i_getstatic:
                counter += 3;
                break;
            case i_invokevirtual:;
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t t = stack[--top];
                printf("%d\n", t);
                counter += 3;
                break;
            case i_iconst_m1:
            case i_iconst_0:
            case i_iconst_1:
            case i_iconst_2:
            case i_iconst_3:
            case i_iconst_4:
            case i_iconst_5: {
                int32_t val = (int32_t) op - 0x03;
                stack[top++] = val;
                counter += 1;
                break;
            }
            case i_sipush:;
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                int16_t s = (int16_t)(b1 << 8 | b2);
                stack[top++] = s;
                counter += 3;
                break;
            case i_iadd:
            case i_isub:
            case i_imul:
            case i_idiv:
            case i_irem:
            case i_iand:
            case i_ior:
            case i_ixor:
                binary_arithmetic(op, stack, &top);
                counter += 1;
                break;
            case i_ineg: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t a = stack[--top];
                stack[top++] = a * -1;
                counter += 1;
                break;
            }
            case i_ishl: {
                if (top < 2) {
                    exit(ERROR);
                }
                int32_t b = stack[--top];
                int32_t a = stack[--top];
                int32_t s = a << b;
                stack[top++] = s;
                counter += 1;
                break;
            }
            case i_ishr: {
                if (top < 2) {
                    exit(ERROR);
                }
                int32_t b = stack[--top];
                int32_t a = stack[--top];
                int32_t s = a >> b;
                stack[top++] = s;
                counter += 1;
                break;
            }
            case i_iushr: {
                if (top < 2) {
                    exit(ERROR);
                }
                int32_t b = stack[--top];
                int32_t a = stack[--top];
                int32_t s = ((uint32_t) a) >> b;
                stack[top++] = s;
                counter += 1;
                break;
            }
            case i_iload: {
                uint32_t i = code[counter + 1];
                stack[top++] = locals[i];
                counter += 2;
                break;
            }
            case i_iload_0:
            case i_iload_1:
            case i_iload_2:
            case i_iload_3: {
                uint8_t i = (uint8_t)(op - i_iload_0);
                stack[top++] = locals[i];
                counter += 1;
                break;
            }
            case i_istore: {
                if (top < 1) {
                    exit(ERROR);
                }
                uint32_t i = code[counter + 1];
                int32_t a = stack[--top];
                locals[i] = a;
                counter += 2;
                break;
            }
            case i_istore_0:
            case i_istore_1:
            case i_istore_2:
            case i_istore_3: {
                if (top < 1) {
                    exit(ERROR);
                }
                uint8_t i = (uint8_t)(op - i_istore_0);
                int32_t a = stack[--top];
                locals[i] = a;
                counter += 1;
                break;
            }
            case i_iinc: {
                uint32_t i = code[counter + 1];
                int8_t b = (int8_t) code[counter + 2];
                locals[i] += (int32_t) b;
                counter += 3;
                break;
            }
            case i_ldc: {
                uint8_t b = code[counter + 1];
                stack[top++] = *(int32_t *) class->constant_pool[b - 1].info;
                counter += 2;
                break;
            }
            case i_ifeq: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a == 0) {
                    counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_ifne: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a != 0) {
                    counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_iflt: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a < 0) {
                    counter += (int16_t)((uint16_t) b1 << 8) |
                               (uint16_t) b2; 
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_ifge: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a >= 0) {
                    counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_ifgt: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a > 0) {
                    counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_ifle: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a <= 0) {
                    counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_if_icmpeq: {
                if (top < 2) {
                    exit(ERROR);
                }
                int32_t b = stack[--top];
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a == b) {
                    counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_if_icmpne: {
                if (top < 2) {
                    exit(ERROR);
                }
                int32_t b = stack[--top];
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a != b) {
                    counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_if_icmplt: {
                if (top < 2) {
                    exit(ERROR);
                }
                int32_t b = stack[--top];
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a < b) {
                    counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_if_icmpge: {
                if (top < 2) {
                    exit(ERROR);
                }
                int32_t b = stack[--top];
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a >= b) {
                    counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_if_icmpgt: {
                if (top < 2) {
                    exit(ERROR);
                }
                int32_t b = stack[--top];
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a > b) {
                    counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_if_icmple: {
                if (top < 2) {
                    exit(ERROR);
                }
                int32_t b = stack[--top];
                int32_t a = stack[--top];
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                if (a <= b) {
                    counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                }
                else {
                    counter += 3;
                }
                break;
            }
            case i_goto: {
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                counter += (int16_t)((uint16_t) b1 << 8) | (uint16_t) b2;
                break;
            }
            case i_ireturn: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t a = stack[--top];
                result.value = a;
                result.has_value = true;
                free(stack);
                return result;
                break;
            }
            case i_invokestatic: {
                uint8_t b1 = code[counter + 1];
                uint8_t b2 = code[counter + 2];
                method_t *pool = find_method_from_index((uint16_t)(b1 << 8) | b2, class);
                uint16_t size = get_number_of_parameters(pool);
                int32_t *new_locals =
                    (int32_t *) malloc(pool->code.max_locals * sizeof(int32_t));

                size_t i = size;
                while (i > 0) {
                    if (top < 1) {
                        exit(ERROR);
                    }
                    --i;
                    new_locals[i] = stack[--top];
                }

                optional_value_t rec = execute(pool, new_locals, class, heap);
                free(new_locals);
                if (rec.has_value) {
                    stack[top++] = (int32_t) rec.value;
                }
                counter += 3;
                break;
            }
            case i_nop:
                counter += 1;
                break;
            case i_dup: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t a = stack[top - 1];
                stack[top++] = a;
                counter += 1;
                break;
            }
            case i_newarray: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t count = stack[--top];
                if (count < 0) {
                    free(stack);
                    exit(ERROR);
                }
                int32_t *arr = (int32_t *) calloc((count + 1), sizeof(int32_t));
                arr[0] = count;
                int32_t ref = heap_add(heap, arr);
                stack[top++] = ref;
                counter += 2;
                break;
            }
            case i_arraylength: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t ref = stack[--top];
                int32_t *data = heap_get(heap, ref);
                stack[top++] = data[0];
                counter += 1;
                break;
            }
            case i_areturn: {
                if (top < 1) {
                    exit(ERROR);
                }
                int32_t ref = stack[--top];
                result.has_value = true;
                result.value = ref;
                free(stack);
                return result;
            }
            case i_iastore: {
                if (top < 3) {
                    exit(ERROR);
                }
                int32_t value = stack[--top];
                int32_t index = stack[--top];
                int32_t ref = stack[--top];
                int32_t *arr = heap_get(heap, ref);
                arr[index + 1] = value;
                counter += 1;
                break;
            }
            case i_iaload: {
                if (top < 2) {
                    exit(ERROR);
                }
                int32_t index = stack[--top];
                int32_t ref = stack[--top];
                int32_t *arr = heap_get(heap, ref);
                stack[top++] = arr[index + 1];
                counter += 1;
                break;
            }
            case i_aload: {
                uint8_t i = code[counter + 1];
                stack[top++] = locals[i];
                counter += 2;
                break;
            }
            case i_astore: {
                if (top < 1) {
                    exit(ERROR);
                }
                uint8_t i = code[counter + 1];
                int32_t ref = stack[--top];
                locals[i] = ref;
                counter += 2;
                break;
            }
            case i_aload_0:
            case i_aload_1:
            case i_aload_2:
            case i_aload_3: {
                uint8_t i = (uint8_t)(op - i_aload_0);
                stack[top++] = locals[i];
                counter += 1;
                break;
            }
            case i_astore_0:
            case i_astore_1:
            case i_astore_2:
            case i_astore_3: {
                if (top < 1) {
                    exit(ERROR);
                }
                uint8_t i = (uint8_t)(op - i_astore_0);
                int32_t ref = stack[--top];
                locals[i] = ref;
                counter += 1;
                break;
            }
            default:
                fprintf(stderr, "Default error\n");
                exit(ERROR);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <class file>\n", argv[0]);
        return 1;
    }

    // Open the class file for reading
    FILE *class_file = fopen(argv[1], "r");
    assert(class_file != NULL && "Failed to open file");

    // Parse the class file
    class_file_t *class = get_class(class_file);
    int error = fclose(class_file);
    assert(error == 0 && "Failed to close file");

    // The heap array is initially allocated to hold zero elements.
    heap_t *heap = heap_init();

    // Execute the main method
    method_t *main_method = find_method(MAIN_METHOD, MAIN_DESCRIPTOR, class);
    assert(main_method != NULL && "Missing main() method");
    /* In a real JVM, locals[0] would contain a reference to String[] args.
     * But since TeenyJVM doesn't support Objects, we leave it uninitialized. */
    int32_t locals[main_method->code.max_locals];
    // Initialize all local variables to 0
    memset(locals, 0, sizeof(locals));
    optional_value_t result = execute(main_method, locals, class, heap);
    assert(!result.has_value && "main() should return void");

    // Free the internal data structures
    free_class(class);

    // Free the heap
    heap_free(heap);
}
