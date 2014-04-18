CS 6210: Advanced Operating Systems
===================================

Project 4: Recoverable Virtual Memory


How to build the library and run tests?
1. Run python build.py
2. Put all the test(.c) files in the testing directory
3. Run python test.py

Alternatively,

1. Run make all
2. Put all the test(.c) files in the testing directory
3. Put the librvm.a file in the testing directory
4. Run gcc -o <testname> <testname>.c  librvm.a `pkg-config --cflags --libs glib-2.0` manually for each test file


How you use logfiles to accomplish persistency plus transaction semantics?

We ensure transaction semanticsby doing the following:
1. When a region of memeory is about to be modified, we copy the region into the memory backup area.
2. When the transaction is aborted, we copy the contents from the backup area to the corresponding memory region which ensures that all the changes are reverted back.
3. When the transaction is commited, we copy the contents of the memory region into th elog files in the format below:

Persistence is achieved by recording all the modifications to the disk log-file, which is stored in the same directory as given in rvm_init.

What goes in them? How do the files get cleaned up, so that they do not expand indefinitely?
The RVM Log gets cleaned up in two ways. First, when a segment is mapped, we consume all the modifications from the segment from the log file, and secondly from the explicit call to truncate the log using rvm_truncate_log.
