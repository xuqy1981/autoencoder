#
# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
CC = g++
#CC = g++-4.9
#CC = /lusr/bin/g++-4.8

#OpenBLAS dirs
#OPEN_BLAS_INC=/usr/local/opt/openblas/include
#OPEN_BLAS_LIB=/usr/local/opt/openblas/lib
OPEN_BLAS_INC=/home1/03155/jliang/Desktop/OpenBLAS/jliang_install/include
OPEN_BLAS_LIB=/home1/03155/jliang/Desktop/OpenBLAS/jliang_install/lib
HAS_OPENBLAS=1



# define any compile-time flags
CFLAGS = -Wall -O3 -fopenmp -std=c++11
#-O3 

# define any directories containing header files other than /usr/include
#
INCLUDES = -I. -I./mnist/include -I$(OPEN_BLAS_INC)

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS = -L$(OPEN_BLAS_LIB)

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = -lopenblas
#LIBS =

# define the C source files
SRCS = main.cpp neural_network.cpp hidden_layer.cpp autoencoder.cpp output_layer.cpp neural_network_cross.cpp auto_hidden_layer.cpp ga.cpp

# define the C object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)

# define the executable file 
MAIN = autoencoder

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean

all:    $(MAIN)

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -D HAS_OPENBLAS=$(HAS_OPENBLAS) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

# depend: $(SRCS)
# 	makedepend $(INCLUDES) $^

# # DO NOT DELETE THIS LINE -- make depend needs it
