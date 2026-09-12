CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

FUENTES = PruebaFS.cc FileSystem.cc Bitmap.cc Bodega.cc
BINARIO = pruebaFS

$(BINARIO): $(FUENTES) Formatos.h FileSystem.h Bitmap.h Bodega.h
	$(CXX) $(CXXFLAGS) $(FUENTES) -o $(BINARIO)

clean:
	rm -f $(BINARIO) cafeteria.dat