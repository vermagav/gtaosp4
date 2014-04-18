CS 6210: Advanced Operating Systems
===================================

Project 4: Recoverable Virtual Memory

How to build the library and run tests?

1. Run the attached makefile using the command "make all"
2. Build any test cases that you migth want to test our project with in the same folder
3. Run your test executables
4. Run gcc -o <testname> <testname>.c  librvm.a `pkg-config --cflags --libs glib-2.0` manually for each test file
	Note the --libs glib-2.0 as we use Glib in our project

How you use logfiles to accomplish persistency plus transaction semantics?

We ensure transaction semanticsby doing the following:
1. When a region of memory is about to be modified, we copy the region into the memory backup store.
2. When the transaction is aborted, we copy the contents from the backup store to the corresponding memory region which ensures that all the changes are reverted back.
3. When the transaction is committed, we copy the contents of the memory region into the log files in the format below:
	<segname>~<segsize>~<memdump><\n>

Persistence is achieved by recording all the modifications to the disk log-file, which is stored in the same directory as given in rvm_init.

What goes in them? How do the files get cleaned up, so that they do not expand indefinitely?

There are two files stored: the transaction.log, which stores the actual transactions and segments in the format shown, and .seg files in the directory of rvm_init, that contains the raw bytes form memory. The RVM Log gets cleaned up in two ways. First, when a segment is mapped, we consume all the modifications from the segment from the log file, and secondly from the explicit call to truncate the log using rvm_truncate_log.

Note: We have provided the rvm.c file instead of rvm.cpp because of linking issues. Hence, we have provided a makefile that works with our project. If you are going to use your own makefile to test and grade our project, please modify it to accept rvm.c (a C file) and not rvm.cpp.