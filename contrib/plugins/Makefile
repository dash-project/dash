CXX = dash-mpicxx
CXXFLAGS = -shared -fPIC -I../../dart-if/include/dash/dart/if
TEMANEJO_HOME=$(HOME)/opt/temanejo
EXTRAE_HOME=$(HOME)/opt/extrae-3.7.1/

all: libpdummy libpayudame libpextrae

libpdummy:
	$(CXX) ./dummy/libpdummy.cc $(CXXFLAGS) -o ./dummy/libdummy.so

libpayudame:
	$(CXX) ./ayudame/libpayudame.cc $(CXXFLAGS) -o ./ayudame/libpayudame.so -I $(TEMANEJO_HOME)/include/

libpextrae:
	$(CXX) ./extrae/libpextrae.cc $(CXXFLAGS) -o ./extrae/libpextrae.so  -I $(EXTRAE_HOME)/include -std=c++17
