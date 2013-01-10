#  Copyright (C) 2005-2007 Fons Adriaensen <fons@kokkinizita.net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.



# Compilation flags
#
DEBUG_FLAGS = -g
OPTIM_FLAGS = -O3
SHOBJ_FLAGS = -fpic -D_REENTRANT
CPPFLAGS = $(OPTIM_FLAGS) -Wall
LDFLAGS += 
LDLIBS += -lm -lrt


# Targets
#

NFTEST_O = nftest.o hoafilt.o
nftest:	$(NFTEST_O)
	g++ -o $@ $(LDFLAGS) $(NFTEST_O) -lfftw3 $(LDLIBS) 

TEMP_O = temp.o hoafilt.o
temp:	$(TEMP_O)
	g++ -o $@ $(LDFLAGS) $(TEMP_O) -lfftw3 $(LDLIBS) 



JTEST_O = jtest.o hoafilt.o

jtest:	$(JTEST_O)
	g++ $(LDFLAGS) -o jtest $(JTEST_O) -ljack $(LDLIBS)



clean:
	/bin/rm -f *.o *.u *.so *~ f? t?

