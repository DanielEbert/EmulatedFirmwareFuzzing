
CXXFLAGS = -g -O3 -funroll-loops -fPIC -fpermissive -std=c++11
#CXXFLAGS = -g -O0 -fPIC -fpermissive -std=c++11
CXX ?= clang++

all: libfuzzer-mutator.so

FuzzerCrossOver.o:	FuzzerCrossOver.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerDataFlowTrace.o:	FuzzerDataFlowTrace.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerDriver.o:	FuzzerDriver.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerExtFunctionsDlsym.o:	FuzzerExtFunctionsDlsym.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerExtFunctionsWeak.o:	FuzzerExtFunctionsWeak.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerExtFunctionsWindows.o:	FuzzerExtFunctionsWindows.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerExtraCounters.o:	FuzzerExtraCounters.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerFork.o:	FuzzerFork.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerIO.o:	FuzzerIO.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerIOPosix.o:	FuzzerIOPosix.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerIOWindows.o:	FuzzerIOWindows.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerLoop.o:	FuzzerLoop.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerMerge.o:	FuzzerMerge.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerMutate.o:	FuzzerMutate.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerSHA1.o:	FuzzerSHA1.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerTracePC.o:	FuzzerTracePC.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerUtil.o:	FuzzerUtil.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerUtilDarwin.o:	FuzzerUtilDarwin.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerUtilFuchsia.o:	FuzzerUtilFuchsia.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerUtilLinux.o:	FuzzerUtilLinux.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerUtilPosix.o:	FuzzerUtilPosix.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

FuzzerUtilWindows.o:	FuzzerUtilWindows.cpp
	$(CXX) $(CXXFLAGS) -I. -c $^

libfuzzer.o:	libfuzzer.cpp
	$(CXX) $(CXXFLAGS) -I../../sim -I. -c $^

libfuzzer-mutator.so:	FuzzerCrossOver.o FuzzerDataFlowTrace.o FuzzerDriver.o FuzzerExtFunctionsDlsym.o FuzzerExtFunctionsWeak.o FuzzerExtFunctionsWindows.o FuzzerExtraCounters.o FuzzerFork.o FuzzerIO.o FuzzerIOPosix.o FuzzerIOWindows.o FuzzerLoop.o FuzzerMerge.o FuzzerMutate.o FuzzerSHA1.o FuzzerTracePC.o FuzzerUtil.o FuzzerUtilDarwin.o FuzzerUtilFuchsia.o FuzzerUtilLinux.o FuzzerUtilPosix.o FuzzerUtilWindows.o libfuzzer.o
	$(CXX) $(CXXFLAGS) -I../../sim -I. -shared -o libfuzzer-mutator.so *.o

clean:
	rm -f *.o *~ *.so core
