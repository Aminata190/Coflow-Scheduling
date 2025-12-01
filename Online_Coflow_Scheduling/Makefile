# Copyright 2014, Gurobi Optimization, Inc.

#PLATFORM = linux64

PLATFORM = linux64
GUROBI_HOME = $(HOME)/Downloads/Programs/gurobi12.0.1_linux64/gurobi1201/linux64

CC       = gcc
CPP      = g++
CARGS    = -m64 -w -DGUROBI
INC      = $(GUROBI_HOME)/include
CLIB     = -L$(GUROBI_HOME)/lib -lgurobi120
CPPLIB   = -L$(GUROBI_HOME)/lib -lgurobi_c++ -lgurobi120 -lpthread -lm


#=== CLP ===
CLP_INC  = /usr/include/coin
CLP_LIB  = -lClp -lCoinUtils


all: exec_grb exec_clp

exec_grb: ./exec.cc ./Simulator.cc ./Network_grb.cc ./Flow.cc ./Link.cc ./EventList.cc ./Stats.cc

	$(CPP) $(CARGS) -g -o ./BIN/exec_grb ./exec.cc ./Simulator.cc ./Network_grb.cc ./Flow.cc ./Link.cc ./EventList.cc ./Stats.cc -I$(INC) -I$(CLP_INC) $(CLP_LIB) $(CPPLIB) -lpthread -lm


exec_clp: ./exec.cc ./Simulator.cc ./Network_clp.cc ./Flow.cc ./Link.cc ./EventList.cc ./Stats.cc

	$(CPP) $(CARGS) -g -o ./BIN/exec_clp ./exec.cc ./Simulator.cc ./Network_clp.cc ./Flow.cc ./Link.cc ./EventList.cc ./Stats.cc -I$(INC) -I$(CLP_INC) $(CLP_LIB) $(CPPLIB) -lpthread -lm

clean:
	rm -rf *.o *_c *_c++ *.class *.log *.rlp *.lp *.bas *.ilp *.dSYM
