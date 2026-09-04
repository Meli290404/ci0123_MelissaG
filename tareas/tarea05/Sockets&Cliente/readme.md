## Client & sockt class

- Las pruebas de Cliente son "pruebas.sh".

### Cómo compilar y correr (`Sockets&Cliente/`)
Requiere OpenSSL (`libssl-dev`) instalado.
```bash
cd Sockets&Cliente
make
./Client
```
Comandos disponibles una vez corriendo: `<categoria>` (listado), `PRODUCTO <nombre>` (búsqueda), `AGREGAR <nombre> <cantidad>` (carrito), `FACTURA`, `FALLA` (simula error de comunicación), `salir`. Los casos de prueba automatizados corren con `./pruebas.sh` o `make test`.