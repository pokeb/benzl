// This header defines built-in functions
// That is: those you can call from benzl code that are built-in to the language
// and implemented in C
//
// Part of benzl - https://github.com/pokeb/benzl

#pragma once

#include "benzl-lval.h"
#include "benzl-lenv.h"

#pragma mark - Loading

// Loads all the built-in benl functions
void lenv_add_builtins(lenv *e);

#pragma mark - Evaluation
// Implemented in benzl-builtin-eval.c

// (eval {+ 1 2}) => 3
lval* builtin_eval(lenv *e, const lval *a);

// (eval-string "(+ 1 2)") => 3
lval* builtin_eval_string(lenv *e, const lval *a);
lval* builtin_load_str(lenv *e, char *input, lval *source_file);

// Load contents of file and evaluate them
// (load "~/myscript.benzl")
lval* builtin_load(lenv *e, const lval *a);


#pragma mark - Conditionals
// Implemented in benzl-builtin-conditional.c

// (if (> 2 1) {print "2 > 1!"} {print "2 < 1!"})
lval* builtin_if(lenv *e, const lval *a);

// (or true false) => true
lval* builtin_logical_or(lenv *e, const lval *a);

// (and true false) => false
lval* builtin_logical_and(lenv *e, const lval *a);

// (not false false false true) => false
lval* builtin_logical_not(lenv *e, const lval *a);


#pragma mark - Functions
// Implemented in benzl-builtin-function.c

// (fun {multiply-and-add x y z} {+ (* x y) z})
// (multiply-and-add 5 2 3) => 13
lval *builtin_fun(lenv *e, const lval *a);

// (def {add-1} (lambda {x} {+ x 1}))
// (add-1 3) => 4
lval *builtin_lambda(lenv *e, const lval *a);


#pragma mark - Enviroment / Variables
// Implemented in benzl-builtin-env.c

// (def {x} 10) => 'x' now refers to 10
lval* builtin_def(lenv *e, const lval *a);

// (set {x} 20) => 'x' now refers to 20
// (set {undefined-variable-name} 20) => error!
lval* builtin_set(lenv *e, const lval *a);

// (set-prop {p x} 10)
lval* builtin_set_prop(lenv *e, const lval *a);

// Loads all built-in functions into the passed environment
// (Generally, this is called once at startup time)
void lenv_add_builtins(lenv *e);

// Returns the name of the passed function for debug printing
char* builtin_func_string(lbuiltin func);


#pragma mark - List operations
// Implemented in benzl-builtin-list.c

// (head 1 2 3) => 1
// (head "hello") => "h"
lval* builtin_head(lenv *e, const lval* a);

// (tail 1 2 3) => {2 3}
// (tail "hello") => "ello"
lval *builtin_tail(lenv *e, const lval *a);

// (list 1 2 3) => {1 2 3}
lval* builtin_list(lenv *e, const lval *a);

// (join {1 2 3} {4 5 6}) => {1 2 3 4 5 6}
// (join "hello" " " "there") => "hello there"
lval* builtin_join(lenv *e, const lval *a);

// (len {1 2 3}) => 3
// (len "hello") => 5
lval* builtin_len(lenv *e, const lval *a);

// drop, take and last were originally in the stdlib
// They are now builtins because it they are so commonly used
// that we get a big speedup from not composing these operations
// out of head, tail and join

// (drop 2 {1 2 3 5}) => {3 5}
// (drop 2 "hello") => "llo"
lval *builtin_drop(lenv *e, const lval *a);

// (take 2 {1 2 3 5}) => {1 2}
// (take 2 "hello") => "he"
lval *builtin_take(lenv *e, const lval *a);

// (first {1 2 3 5}) => 1
// (first "hello") => "o"
lval* builtin_first(lenv *e, const lval* a);

// (second {1 2 3 5}) => 2
// (second "hello") => "e"
lval* builtin_second(lenv *e, const lval* a);

// (last {1 2 3 5}) => 5
// (last "hello") => "o"
lval* builtin_last(lenv *e, const lval* a);

// (nth 2 {1 2 3 5}) => 3
// (nth 2 "hello") => "l"
lval* builtin_nth(lenv *e, const lval* a);


#pragma mark - Mathematical operations
// Implemented in benzl-builtin-math.c

// (+ 2 3) => 5
// (+ "hello" "there") => "hellothere"
// (+ {1 2} {3 4}) => {1 2 3 4}
lval *builtin_add(lenv *e, const lval *a);

// (- 5 3) => 2
lval *builtin_subtract(lenv *e, const lval *a);

// (* 5 2) => 10
lval *builtin_multiply(lenv *e, const lval *a);

// (/ 5 3) => 1
lval *builtin_divide(lenv *e, const lval *a);

// (% 5 3) => 2
lval *builtin_modulo(lenv *e, const lval *a);

// (>> 0x0000FFFF 8) => 0x000000FF
lval *builtin_right_shift(lenv *e, const lval *a);

// (<< 0x0000FFFF 8) => 0x00FFFF00
lval *builtin_left_shift(lenv *e, const lval *a);

// (| 0xFF00FF00 0x00FFFF00) => 0x0000FF00
lval *builtin_bitwise_and(lenv *e, const lval *a);

// (| 0xFF00FF00 0x00FFFF00) => 0xFFFFFF00
lval *builtin_bitwise_or(lenv *e, const lval *a);

// (^ 0xFF00FF00 0x00FFFF00) => 0xFFFF0000
lval *builtin_bitwise_xor(lenv *e, const lval *a);

// (min 5 3) => 3
lval *builtin_min(lenv *e, const lval *a);

// (max 5 3) => 5
lval *builtin_max(lenv *e, const lval *a);

// (floor 10.75) => 10
lval *builtin_floor(lenv *e, const lval *a);

// (ceil 10.75) => 11
lval *builtin_ceil(lenv *e, const lval *a);


#pragma mark - Comparisions
// Implemented in benzl-builtin-compare.c

// (> 2 1) => true
lval* builtin_greater_than(lenv *e, const lval *a);

// (< 2 1) => false
lval* builtin_less_than(lenv *e, const lval *a);

// (>= 2 1) => true
lval* builtin_greater_than_or_equal(lenv *e, const lval *a);

// (<= 2 1) => false
lval* builtin_less_than_or_equal(lenv *e, const lval *a);

// (== 2 1) => false
lval* builtin_equal(lenv *e, const lval *a);

// (!= 2 1) => true
lval* builtin_not_equal(lenv *e, const lval *a);


#pragma mark - String formatting & printing
// Implemented in benzl-builtin-format.c

// (format "Hello, %." "Ben") => "Hello, Ben."
lval* builtin_format(lenv *e, const lval *a);

// (print "hello" 2 1.3) ; Prints "hello 2 1.3" to console
lval* builtin_print(lenv *c, const lval *a);

// (printf "Hello, %." "Ben") ; Prints "Hello, Ben." to console
lval* builtin_printf(lenv *e, const lval *a);


#pragma mark - Buffers
// Implemented in benzl-builtin-buffer.c

// Creates a buffer 32 bytes long:
// (create-buffer 32)
lval* builtin_create_buffer(lenv *e, const lval *a);

// Creates a buffer passing bytes for the contents:
// (buffer-with-bytes 0xFF 0xFE 0x00)
lval* builtin_buffer_with_bytes(lenv *e, const lval *a);

// Iterates over chunks of n bytes, creating a new
// buffer with values supplied by the passed function
// (buffer-map buffer 4 (lambda {currentBytes, offset} {...}))
lval* builtin_buffer_map(lenv *e, const lval *a);

// Sets the first byte of the buffer to 0xFF:
// (put-byte buffer 0 0xFF)
lval* builtin_put_byte(lenv *e, const lval *a);

// Get the first byte of the buffer:
// (get-byte buffer 0) ; Returns a Byte
lval* builtin_get_byte(lenv *e, const lval *a);

// Put '255' into the first byte of the buffer:
// (put-unsigned-char buffer 0 255) ; Returns a Byte
lval* builtin_put_unsigned_char(lenv *e, const lval *a);

// Get the first byte of the buffer as an integer,
// treating the value as an unsigned char:
// (get-unsigned-char buffer 0) ; Returns an Integer
lval* builtin_get_unsigned_char(lenv *e, const lval *a);

// Put '-127' into the first byte of the buffer:
// (put-signed-char buffer 0 -127)
lval* builtin_put_signed_char(lenv *e, const lval *a);

// Get the first byte of the buffer as an integer,
// treating the value as a signed char:
// (get-signed-char buffer 0) ; Returns an Integer
lval* builtin_get_signed_char(lenv *e, const lval *a);

// Put '65535' into the first two bytes of the buffer:
// (put-unsigned-short buffer 0 65535)
lval* builtin_put_unsigned_short(lenv *e, const lval *a);

// Get the first two bytes of the buffer as an integer,
// treating the value as an unsigned short:
// (get-unsigned-short buffer 0) ; Returns an Integer
lval* builtin_get_unsigned_short(lenv *e, const lval *a);

// Put '-32767' into the first two bytes of the buffer:
// (put-signed-short buffer 0 -32767)
lval* builtin_put_signed_short(lenv *e, const lval *a);

// Get the first two bytes of the buffer as an integer,
// treating the value as a signed short:
// (get-signed-short buffer 0) ; Returns an Integer
lval* builtin_get_signed_short(lenv *e, const lval *a);

// Put '4294967295' into the first four bytes of the buffer:
// (put-unsigned-integer buffer 0 4294967295)
lval* builtin_put_unsigned_integer(lenv *e, const lval *a);

// Get the first four bytes of the buffer as an integer,
// treating the value as a 32-bit unsigned int:
// (get-unsigned-integer buffer 0) ; Returns an Integer
lval* builtin_get_unsigned_integer(lenv *e, const lval *a);

// Put '-2147483647' into the first four bytes of the buffer:
// (put-signed-integer buffer 0 -2147483647)
lval* builtin_put_signed_integer(lenv *e, const lval *a);

// Get the first four bytes of the buffer as an integer,
// treating the value as a 32-bit signed int:
// (get-signed-integer buffer 0) ; Returns an integer
lval* builtin_get_signed_integer(lenv *e, const lval *a);

// Get the first eight bytes of the buffer as an integer,
// treating the value as a 64-bit unsigned int:
// (get-unsigned-long buffer 0) ; Returns an integer
lval* builtin_get_unsigned_long(lenv *e, const lval *a);

// Put '9223372036854775807' into the first eight bytes of the buffer:
// (put-unsigned-long buffer 0 9223372036854775807)
lval* builtin_put_unsigned_long(lenv *e, const lval *a);

// Get the first eight bytes of the buffer as an integer,
// treating the value as a 64-bit signed int:
// (get-signed-long buffer 0) ; Returns an integer
lval* builtin_get_signed_long(lenv *e, const lval *a);

// Put '−9223372036854775808' into the first eight bytes of the buffer:
// (put-signed-long buffer 0  −9223372036854775808)
lval* builtin_put_signed_long(lenv *e, const lval *a);

// Put a null-terminated string into the buffer
// (Will write 7 bytes total):
// (put-string buffer 0 "hello!")
lval *builtin_put_string(lenv *e, const lval *a);

// Get a null-terminated string from the buffer:
// (get-string buffer 0)
lval *builtin_get_string(lenv *e, const lval *a);

// Copy the passed bytes into the start of the buffer:
// (put-bytes buffer 0 buffer2)
lval *builtin_put_bytes(lenv *e, const lval *a);

// Get the first 128 bytes of the buffer as a new buffer
// (get-bytes buffer 0 128)
lval *builtin_get_bytes(lenv *e, const lval *a);


#pragma mark - Reading and writing files
// Implemented in benzl-builtin-file.c

// Reads binary data from file as a list of bytes
// (read "~/my-data.bin") => {0xFF 0x00 0xFF}
lval *builtin_read_file(lenv *e, const lval *a);

// Writes binary data to file
// (write "~/my-data.bin" {0x01 0x02 0xFF})
lval *builtin_write_file(lenv *e, const lval *a);


#pragma mark - Errors
// Implemented in benzl-builtin-error.c

// Errors will stop evalation but can be caught using try/catch:
// (error "This shouldn't happen")
lval* builtin_error(lenv *e, const lval *a);

// Errors normally interrupt evalulation in benzl
// Using a try/catch block, you can catch and handle the error:
// (try (risky-func x y)
//   {printf \"It's actually fine.\"}
//   {catch e {printf \"Error: %\" e}}
lval* builtin_try(lenv *e, const lval *a);


#pragma mark - Types
// Implemented in benzl-builtin-type.c

// (type-of "Hello") => "String"
// (type-of 123.3) => "Float"
lval* builtin_type_of(lenv *e, const lval *a);

// (def-type {Point} x:Float y:Float)
lval* builtin_def_type(lenv *e, const lval *a);

// (to-string {1 2 3}) => "{1 2 3}"'"
// (to-string (buffer-with-bytes 0x48 0x65 0x6C 0x6C 0x6F 0x00)) => "Hello"
lval* builtin_to_string(lenv *e, const lval *a);

// (to-number "123") => 123
// (to-number "123.33") => 123.33
// (to-number "0x01") => 1
lval* builtin_to_number(lenv *e, const lval *a);


#pragma mark - Dictionaries
// Implemented in benzl-builtin-dictionary.c

// (dictionary) => (Empty dictionary)
// (dictionary k1:v1 k2:v2) => (Dictionary with two initial key-value pairs)
lval* builtin_dictionary(lenv *e, const lval *a);


#pragma mark - Misc
// Implemented in benzl-built-misc.c

// (console-size) => {80 25}
lval* builtin_console_size(lenv *e, const lval *a);

// (def {start} (cpu-time-since 0))
// ...
// (cpu-time-since start) => ms of CPU time used by benzl since first call
lval* builtin_cpu_time_since(lenv *e, const lval *a);

// (exit 0) ; Exits program with no error
// (exit 1) ; Exits program with error code
 __attribute__((noreturn)) lval* builtin_exit(lenv *e, const lval *a);

// (version) =>
//--
// benzl v0.1
// Made by Ben Copsey, based on a design by Daniel Holden
// https://github.com/pokeb/benzl
// --
lval* builtin_version(lenv *e, const lval *a);

// (print-env) ; Prints bound variables to console
// (For debugging benzl only)
lval* builtin_print_env(lenv *e, const lval *a);

