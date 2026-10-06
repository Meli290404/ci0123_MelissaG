CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

COMUNES = FileSystem.cc Bitmap.cc Bodega.cc AdministradorBodegas.cc Validaciones.cc
RED = VSocket.cc Socket.cc

all: servidorProductos adminBodegas

servidorProductos: ServidorProductos.cc AtencionBodega.cc Protocolo.cc $(RED) $(COMUNES)
	$(CXX) $(CXXFLAGS) $^ -o servidorProductos

adminBodegas: AdminBodegas.cc $(COMUNES)
	$(CXX) $(CXXFLAGS) $^ -o adminBodegas

clean:
	rm -f servidorProductos adminBodegas