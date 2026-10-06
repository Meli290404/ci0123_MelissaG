CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

COMUNES = FileSystem.cc Bitmap.cc Bodega.cc AdministradorBodegas.cc Validaciones.cc
RED = VSocket.cc Socket.cc HttpUtil.cc Protocolo.cc

all: servidor adminBodegas cliente

servidor: Intermediario.cc Despachador.cc PaginaWeb.cc $(RED) $(COMUNES)
	$(CXX) $(CXXFLAGS) $^ -o servidor

adminBodegas: AdminBodegas.cc $(COMUNES)
	$(CXX) $(CXXFLAGS) $^ -o adminBodegas

cliente: Cliente.cc $(RED)
	$(CXX) $(CXXFLAGS) $^ -o cliente

clean:
	rm -f servidor adminBodegas cliente