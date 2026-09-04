## Client & sockt class

- Usé el código que yo desarrollé para el proyecto, para un historial de versiones más actualzado, verificar los dommits del repositorio del Grupo4.

- Las pruebas de Cliente son "pruebas.sh".

### Cómo compilar y correr (`Sockets&Cliente/`)
Requiere OpenSSL (`libssl-dev`) instalado.
```bash
cd Sockets&Cliente
make
./Client
```
Comandos disponibles una vez corriendo: `<categoria>` (listado), `PRODUCTO <nombre>` (búsqueda), `AGREGAR <nombre> <cantidad>` (carrito), `FACTURA`, `FALLA` (simula error de comunicación), `salir`. Los casos de prueba automatizados corren con `./pruebas.sh` o `make test`.