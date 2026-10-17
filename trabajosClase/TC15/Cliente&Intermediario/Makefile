CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

COMUNES = FileSystem.cc Bitmap.cc Bodega.cc AdministradorBodegas.cc Validaciones.cc
RED = VSocket.cc Socket.cc

all: servidorProductos intermediario cliente adminBodegas

servidorProductos: ServidorProductos.cc AtencionBodega.cc Protocolo.cc $(RED) $(COMUNES)
	$(CXX) $(CXXFLAGS) $^ -o servidorProductos

intermediario: Intermediario.cc AtencionCliente.cc ConexionBodega.cc Carritos.cc PaginaWeb.cc HttpUtil.cc Protocolo.cc Validaciones.cc $(RED)
	$(CXX) $(CXXFLAGS) $^ -o intermediario

cliente: Cliente.cc HttpUtil.cc $(RED)
	$(CXX) $(CXXFLAGS) $^ -o cliente

adminBodegas: AdminBodegas.cc $(COMUNES)
	$(CXX) $(CXXFLAGS) $^ -o adminBodegas

clean:
	rm -f servidorProductos intermediario cliente adminBodegas