# Archive of an OS kernel I wrote as an undergraduate

Metakernel was the most significant project I undertook during my undergraduate
education.  It produced a core OS kernel that was capable of booting on an x86
machine and starting threads.  The kernel supported SMP multiprocessing,
virtual memory, paging, swap, and had a very basic filesystem and device driver
layer.

The only driver I ever wrote for it was a parallel-port driver, which I used
to get debug logging out of the bochs emulator that I used for testing.

## Design Overview

Metakernel was an attempt to design an interoperable OS kernel.  Its design
philosophy was to decompose the OS interfaces in POSIX, Windows, and others
down to a set of "atoms"- minimal bits of functionality that could be used
to implement the functionality found in other OSes.  In this regard, it was
somewhat similar to the MIT Exokernel work, but it didn't aim to completely
eliminate OS abstractions altogether.  It also had similarities with
microkernels, though its overall architecture was that of a monolithic kernel.

I had anticipated the performance problems in having lots of little atomic
system calls, so I had planned a batching mechanism to group them together
called "syscall lists".  The idea for this came from OpenGL display lists.  This
mechanism would have also performed authorization checks statically, so they
were similar to modern capabilities work.  I also planned to support very
fine-grained capabilities which could be surrendered by any entity (user,
process, thread).  This was approaching the capabilities work from another
direction.  However, I never fully made the connections that lead to modern
capabilities work.

### Processes and Threads

Processes in metakernel were solely resource-allocation and authorization
entities in essence, they were thread-groups.  Processes had some number of
threads, and coud be set to be destroyed as soon as they ran out of threads.

Threading in metakernel was 1:1, with threads being the fundamental unit of
scheduling and time-slicing.  The threading system had mutexes and condition
variables, and these were designed to avoid priority inversion and detect
deadlocks.

Threads could exist independent of processes as well.

### Virutal Memory

Virtual memory was decoupled from the notion of processes, in keeping with the
kernel's design philosophy.  Address spaces could be created and managed
directly via system calls, and each thread had an address-space in which it
was executing.  Multiple threads could share a given address space, and
address sub-spaces could be created which would guarantee address parity
between the parent address-space and its children.

I had also planned for a protected address subspace system, which would allow
for multiple "layers" of memory protection with the intent of allowing many
system call implementations to be moved out into user-level libraries.  This
was inspired by Exokernel and microkernel architectures.

VM objects represented individual mappings in an address space.  VM objects
could be created for anonymous objects as well as from inodes.  VM objects
could also be mapped into multiple address spaces, possibly at different
addresses.  VM objects could also overlap eachother's mappings without issue.

This decoupling and explicit management of address spaces and VM objects was
heavily inspired by microkernel architecture, particularly the Spring nucleus
papers.

### I/O System

The I/O system used inodes to refer to specific data sources.  Unlike
traditional UNIX inodes, metakernel's inodes were not specifically filesystem
objects, and were built around an interface explicitly designed for asynchronous
I/O.

Name resolution was to be done in user-space, not in the kernel.  While this may
seem inefficient, as it precludes a path cache, the intent was for a system
library to makke use of the VM object and address subspace functionality to
share this information across processes.

There was also no chroot-like system call.  This was accomplished by throwing
away the root inode (in hindsight, this was a bad design choice, as it didn't
actually guarantee security at all).

## Notes

The source here is from a backup copy I took around April 14, 2004.  Some work I
did is notably missing.  I will update with a later copy, should I succeed in
locating one.
