# benzl programming language

benzl (Ben’s Lisp) is an interpreted Lisp-alike programming language, written over 8 weeks as a learning exercise.

Much of the core design is based on the ‘lispy’ language from [Daniel Holden‘s](https://github.com/orangeduck) excellent ‘[Build your own Lisp](http://buildyourownlisp.com)’ book. All the good parts of this language are his, all the bad parts are mine. 🙂

I picked up the book because I was interested in learning Lisp, and this seemed like a fun place to start. I have made substantial changes to the sample language in the book, based on what seemed useful or interesting to work on.

## Features

A few simple examples:

### Numbers

    (def {x} 13) ; Integer
    (def {y} -133.42) ; Float
    (def {z} 0xFF) ; Byte
    (* (+ x y) z) ; Result will be Float

### Strings

    ; Define a string
    (def {a} "Ben")

    ; + can be used to concatenate strings
    (set {a} (+ 5 " " "Red" " " "Balloons"))

    ; String formatting
    (format "Hello, %!" a)

### Lists

    ; Define a list
    (def {l} (list 1 2 3 4 5))

    ; Reverse sort list
    (rsort l)

    ; Split list in two after 2nd item
    (split-at 2 (list 1 2 3 4))

    ; Remove 2nd and 3rd items, replace with "hello"
    (splice 1 2 "hello" (list 1 2 3 4 5))

### Buffers

    ; Create 3-byte buffer
    (def {b} (buffer-with-bytes 0x00 0x01 0x02))

    ; Create a zeroed 10 byte buffer
    (def {b2} (create-buffer 10))

    ; Returns the second byte of the buffer
    (get-byte b 1)

    ; Returns a new buffer with the first two bytes modified
    (put-unsigned-short b 0 0xFFFF)

    ; Create a buffer with the contents of a file
    (def {b3} (read-file "/Users/ben/Desktop/myfile.txt"))

    ; Cast the buffer to a string
    (def {s} (to-string b3))

### Dictionaries (untyped collection of keys and values)

    ; Create a dictionary with two keys and values
    (def {d} (dict x:10 y:12))

    ; Get value
    (d x)

    ; Returns new dictionary with value modified
    (set-prop {d x} 13)

    ; Returns new dictionary with value added
    (set-prop {d z} 14)

### Custom types (struct)

    ; Define a type with two members
    (def-type {Point x y})

    ; Create an instance of the type, providing values for both members
    (def {pnt} (Point x:10 y:12))

    ; Same thing with type specifiers:
    (def-type {PointF x:Float y:Float})

    ; Integers will be cast to floats
    (set {pnt} (PointF x:10 y:12))

    ; Error will be thrown because "Hello" can't be cast to float
    (set {pnt} (PointF x:10 y:"Hello"))

    ; Returns a new Point with x changed to 20
    (set-prop {pnt x} 20)

    ; Throws an error because Point has no member called 'z'
    (set-prop {pnt z} 20)

### Conditionals and flow control

    ; Conditionals
    if (> x 1)
        {printf "% is greater than 1"}
        {printf "% is not greater than 1"}

    ; logical operators (and/or/not)
    if (and (>= x 2) (<= x 5))
        {printf "% is between 2 and 5"}
        {printf "% is not between 2 and 5"}

    ; Multi-case if
    (cond
        {(== x 3) "X is three"}
        {(== y 3) "Y is three"}
        {else "Something else"}
    )

    ; Switch-like control structures
    (case (cmd)
        {"add" "cmd was add"}
        {"remove" "cmd was remove"}
        {"find" "cmd was find"}
    )

    ; Do several things in order
    ; Each expression is evaluated,
    ; the value of the last expression is returned
    (do
        (def {x} 1)
        (def {y} (+ x 1))
        (* x y) ; <- Return value
    )

### Error handling

    ; Throwing errors
    ; Uncaught errors stop evaluation
    (fun {my-func x y} {
        if ((type-of x) (type-of y))
            {+ x y}
            {error "x and y must be the same type!"}
    })

    ; Catching errors
    (try
        {def {x:Integer} "Hello"}
        {catch e {
            printf "Got error: %" e
        }}
    )

### Functions

    ; Defining functions
    (def {add} (lambda {x} {+ x 1}))

    ; Short form for defining a named functions
    (fun {add-one x} {+ x 1})

    ; Same thing with type specifier
    (fun {add-one-alt x:Float} {+ x 1})

    ; Higher-order functions
    (reduce (lambda {acc x} {+ acc x}) 5 (list 1 2 3 4 5))

    ; Most list functions also work on strings
    (filter (lambda {x} {!= x "a"}) "baba")

    ; ...and buffers too
    (map (lambda {x:Byte} {* x 0x02}) (buffer-with-bytes 0x01 0x02 0x03)

    ; buffer-map is a specialised map function for buffers
    ; This code reads 4 bytes at once
    (def {b3} (buffer-with-bytes 0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07))
    (buffer-map b3 4 (lambda {buf idx} {
        printf "%:(%,%,%,%)" idx (nth 0 buf) (nth 1 buf) (nth 2 buf) (nth 3 buf)
    }))

### Misc

    ; Attempts to include and evaluate the contents of 'my-benzl-module.benzl'
    ; Requiring the same file again will not evaluate it again
    require("sample/my-benzl-module")

    ; Evaluates the passed expression and prints out how long it took
    (profile (expensive-function x y z))

    ; Gets a list of arguments provided to the benzl script at launch
    (launch-args)


## Real programs made with benzl

[sample/](https://github.com/pokeb/benzl/tree/master/sample) contains a couple of interesting programs:

* [todo.benzl](https://github.com/pokeb/benzl/blob/master/sample/todo.benzl)
A simple console-based to-do list app

* [image.benzl](https://github.com/pokeb/benzl/blob/master/sample/image.benzl)
Demonstrates rendering bitmapped images in benzl. benzl has no built-in functionality for working with images – this file includes simple rendering functions and a BMP encoder in pure benzl, on top of Buffer objects (see - [bitmap.benzl](https://github.com/pokeb/benzl/blob/master/sample/bitmap.benzl), [rgba-color.benzl](https://github.com/pokeb/benzl/blob/master/sample/rgba-color.benzl), [geometry.benzl](https://github.com/pokeb/benzl/blob/master/sample/geometry.benzl)).

There's also a [simple testing framework](https://github.com/pokeb/benzl/blob/master/test/test-runner.benzl) used by the [tests](https://github.com/pokeb/benzl/blob/master/test/stdlib-tests.benzl).

Finally, there's the [standard library](https://github.com/pokeb/benzl/blob/master/src/stdlib.benzl) - much of the language is written in benzl itself. This is built-in to the benzl binary as part of the make process, so if you want to make changes, you'll have to re-build benzl.

## How to use

benzl has been tested to work on macOS 10.14.x and Ubuntu Linux 18.04. Other versions of macOS, Linux and other unices will probably work too.

### Build benzl and run the tests:

    # make test

If you're on Linux, you may need to install editline (this is used by the REPL):

    # sudo apt-get install libedit-dev
    (or similar depending on your distribution)

### Run the benzl REPL:

    # ./benzl
    --
    benzl v0.1
    Type 'help' for examples of things to try, or 'quit' to exit
    --
    benzl> printf "1 + 1 is %" (+ 1 1)
    One and one is 2
    benzl> exit

### Run a benzl program from the command line

    # ./benzl sample/image.benzl

Running scripts that start with a shebang is also supported (use 'make install' to put benzl in /usr/local/bin)

    # sample/image.benzl

## Changes from ‘lispy’

If you already have the ‘Build your own Lisp’ book and are interested in the changes I made, here‘s a partial list of the bigger changes:

* Lots of new types, including Float, Byte, Buffer, Type, Dictionary (hash table), CustomType (struct) and CaughtError.
* Optional type specifiers for variables, function params and custom type properties. Type specifiers for function return types are not supported. Type checking is at evaluation time, so it can't find problems ahead of time, but it does make writing code that doesn't break substantially easier.
* Errors are significantly improved: try/catch blocks for handling errors, errors now print a simple stack trace, many functions in the standard library now return errors on invalid input
* Memory management: benzl uses a pool allocator for lvals, and lvals use a simple reference counting system to avoid using lval_copy unless absolutely necessary. These two changes make benzl considerably faster. benzl handles evaluation differently from lispy - input expressions to built-in functions are constants, built-in functions must return new objects rather than mutating their input.
* Environments use a simple hash table for storing their bound values
* Many new functions in the standard library (eg sort / slice / pad / index-of etc)
* Lots of new built-in functions in C (eg printf / profiling / read+write files etc)
* Almost all list functions now also work on strings and buffers (eg head / join / map etc)
* I removed 'partial evaluation of functions'. Maybe I'm not mathsy enough to think about things this way, but if I call a function that requires 3 arguments with only 2 arguments, I prefer it to tell me I've made a mistake, rather than returning a new function that is waiting for the argument I missed.
* benzl uses its own parser rather than the mpc library. This parser is based on the one in the [appendix of the Build your own Lisp book](https://github.com/orangeduck/BuildYourOwnLisp/blob/master/appendix_a_hand_rolled_parser.html) (this didn't appear in my printed copy of the book).
* lvals use a union for the different types of values they can store – this helps to reduce overhead as we allocate a lot of these, and benzl has a lot more type-specific information to store than lispy.
* Added enums for operators to reduce the number of strcmp() calls for trivial functions like '+'
* Some very commonly used stdlib built-ins (eg first/last) now implemented in C, because this provides such a huge speedup to almost all benzl code
* Split everything into different files to make working with the code a bit easier
* Extensive tests covering most built-in and stdlib functions in test/stdlib-test.benzl
* Built-in help for REPL
* Simple make-based build process
* Lots of other little changes

## Known issues

* benzl is pretty slow, as the image sample program demonstrates. Most functionality is heavily dependent on recursion but there's no system for turning tail recursion into iteration. Many operations involve a lot of temporary allocations, which is the price for composing functionality on top of head, tail and join, elegant though this is...

* benzl only correctly supports strings using ASCII encoding at present.

* Untested on Windows. I imagine porting should be fairly simple.

* benzl was made as a learning exercise. Please don't use it for anything important!