CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

COMUNES = Protocolo.cc Validaciones.cc HttpUtil.cc
FS = FileSystem.cc Bitmap.cc Bodega.cc
SOCKETS = Socket.cc VSocket.cc

INTERMEDIARIO_FUENTES = Intermediario.cc Despachador.cc $(SOCKETS) $(COMUNES) $(FS)
CLIENTE_FUENTES = Cliente.cc $(SOCKETS) $(COMUNES)

all: intermediario clienteTicAmazon

intermediario: $(INTERMEDIARIO_FUENTES)
	$(CXX) $(CXXFLAGS) $(INTERMEDIARIO_FUENTES) -o intermediario

clienteTicAmazon: $(CLIENTE_FUENTES)
	$(CXX) $(CXXFLAGS) $(CLIENTE_FUENTES) -o clienteTicAmazon

clean:
	rm -f intermediario clienteTicAmazon cafeteria.dat