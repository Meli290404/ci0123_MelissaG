# compilador y banderas: c++17, todos los avisos, y pthread porque Filesystem usa std::mutex
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

# archivos fuente que necesita gestionBodega: el programa mismo mas las tres
# clases de almacenamiento 
FUENTES = GestionBodega.cc FileSystem.cc Bitmap.cc Bodega.cc

# compilar: parado en esta carpeta, correr "make" genera el binario gestionBodega
gestionBodega: $(FUENTES)
	$(CXX) $(CXXFLAGS) $(FUENTES) -o gestionBodega

# "make clean" borra el binario y el archivo de almacenamiento, para empezar de cero
clean:
	rm -f gestionBodega cafeteria.dat