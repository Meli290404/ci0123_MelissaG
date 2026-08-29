COMUNES = Buzon.cc Bitacora.cc

all: bodega intermediario

bodega: Bodega.cc $(COMUNES)
	g++ -std=c++17 -Wall $(COMUNES) Bodega.cc -o bodega

intermediario: Intermediario.cc $(COMUNES)
	g++ -std=c++17 -Wall $(COMUNES) Intermediario.cc -o intermediario

clean:
	rm -f bodega intermediario
	-ipcrm -M 0xB54322 2>/dev/null