import os
import glob

# copy the library file to the directory
os.system("cp ../librvm.a .")
os.system("cp ../rvm.h .")
os.system("cp ../rvm_internal.h .")

# compilation
for files in  glob.glob("*.c"):
	outputF = str(files) + ".o"
	os.system("gcc -o " + outputF +" " + str(files) +" librvm.a `pkg-config --cflags --libs glib-2.0`")

# run
for files in  glob.glob("*.o"):
	 os.system("./" + str(files))
