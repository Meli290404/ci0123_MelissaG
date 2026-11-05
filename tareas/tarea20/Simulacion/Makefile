FS = FileSystem.cc Bitmap.cc BodegaFS.cc AdministradorBodegas.cc
FUENTES = Buzon.cc Protocolo.cc Mancomunado.cc Validaciones.cc Bitacora.cc Bodega.cc Intermediario.cc Cliente.cc simulacion.cc $(FS)

simulacion: $(FUENTES)
	g++ -std=c++17 -pthread -Wall $(FUENTES) -o simulacion

clean:
	rm -f simulacion fs.dat.ci0123 fs5.dat.ci0123
	-ipcrm -Q 0xB54321 2>/dev/null
