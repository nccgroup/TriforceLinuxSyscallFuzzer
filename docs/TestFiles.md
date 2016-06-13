# Test Files

Test files use a proprietary format.  It supports multiple
system calls made of of multiple arguments.  There is limited
support for cross references between arguments.

The best source of information
for these formats is in `parse.c`, `sysc.c` and `argfd.c`.  The
`gen.py` generator is also a good source of information since it
is more concise.  

The rest of this document provides an overview of the file format.


# File format

Each file is made up of several sections separated by delimiters.
The delimiters are designed to be mutable (so AFL can break delimiters)
but not too likely to generate.  They are also intended to not
often conflict with meaningful values, but this is a risk with the
file format.

The file has a number of call records which are separated by
`B7 E3` bytes.  Each record describes a single system call and
the call record is parsed in isolation.

A call record itself has a number of buffers separated by
`A5 C9` bytes.  The first buffer is special and contains the
call header.  The other buffers are referenced by the call header.
A call record starts with a 16-bit number (all values are big-endian)
which specifies the system call number.  This is followed by
exactly six arguments (whether or not the system call needs it).

Each argument starts with an 8-bit number specifying the type:
* Type 0 contains a 64-bit number that is used verbatim.  
* Type 1 contains a 32-bit number that is used as an allocation
size.  The allocated buffer becomes the argument and the allocation
size is pushed on a size stack.
* Type 2 has no further information. It consumes the next
available buffer in the call record,
and a pointer to the buffer becomes the argument. Its size is pushed
on the size stack.
* Type 3 has no further information. It pops a size from the size
stack and uses it as the argument.
* Type 4 consumes the next buffer and writes it to a temporary file.
The file is opened and the file descriptor becomes the argument.
* Type 5 contains a 16-bit number that encodes a "file" type -- it
can represent sockets, special files, events, epolls, inotifys and
other unusual file types.  A file of that type is opened and
the file descriptor becomes the argument.
* Type 6 starts with an 8-bit number specifying a count.
An argument vector of this size is created by recursively parsing
that many more arguments and storing them in the vector.
The vector pointer becomes the argument.
* Type 7 has no further information. It conumes the next call buffer
and writes it to a temporary file. The filename becomes the argument
and the file size is pushed onto the size stack.

