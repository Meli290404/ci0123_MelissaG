CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread
SSLFLAGS = -lssl -lcrypto

COMUNES = FileSystem.cc Bitmap.cc Bodega.cc AdministradorBodegas.cc Validaciones.cc
RED = VSocket.cc Socket.cc

.PHONY: all certificado clean limpiar

all: servidorProductos intermediario cliente adminBodegas

servidorProductos: ServidorProductos.cc AtencionBodega.cc Protocolo.cc Descubrimiento.cc Bitacora.cc $(RED) $(COMUNES)
	$(CXX) $(CXXFLAGS) $^ -o servidorProductos

intermediario: Intermediario.cc AtencionCliente.cc ConsultaServidor.cc ConexionBodega.cc TablaRutas.cc TablaIslas.cc Mancomunado.cc AtencionIsla.cc Reservas.cc ConexionIsla.cc Descubrimiento.cc Carritos.cc PaginaWeb.cc HttpUtil.cc Protocolo.cc Validaciones.cc SSLSocket.cc Bitacora.cc $(RED)
	$(CXX) $(CXXFLAGS) $^ -o intermediario $(SSLFLAGS)

cliente: Cliente.cc HttpUtil.cc $(RED)
	$(CXX) $(CXXFLAGS) $^ -o cliente

adminBodegas: AdminBodegas.cc $(COMUNES)
	$(CXX) $(CXXFLAGS) $^ -o adminBodegas

# certificado autofirmado para que el intermediario tambien atienda https en el puerto 8080
certificado: certificados/ci0123.pem

certificados/ci0123.pem:
	mkdir -p certificados
	openssl req -x509 -newkey rsa:2048 -keyout certificados/key.pem -out certificados/cert.pem \
		-days 365 -nodes -subj "/C=CR/O=UCR/CN=ci0123" 2> /dev/null
	cat certificados/key.pem certificados/cert.pem > certificados/ci0123.pem

clean:
	rm -f servidorProductos intermediario cliente adminBodegas

# borra las bitacoras, los filesystems de los servidores y el certificado (no toca ejecutables)
limpiar:
	rm -f bitacora-*.log fs*.dat.ci0123 *.csv
	rm -rf certificados