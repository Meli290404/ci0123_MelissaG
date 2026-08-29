// Simula el servidor intermediario.
// Le manda peticiones a la bodega y muestra lo que responde.

#include <cstdio>
#include <string>
#include <vector>

#include "Buzon.h"
#include "Protocolo.h"
#include "Bitacora.h"

// Manda una peticion a la bodega y espera la respuesta antes de seguir.
void pedir( Buzon & buzon, const std::string & peticion ) {
    buzon.Enviar( peticion.c_str(), CANAL_A_BODEGA );
    rotular( "Intermediario", "Bodega", peticion, "solicitud enviada" );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    buzon.Recibir( &resp, sizeof( resp.texto ), CANAL_A_INTERMEDIARIO );
    rotular( "Bodega", "Intermediario", std::string( resp.texto ), "respuesta recibida" );
}

int main() {
    Buzon buzon;

    // guion de prueba: pide el listado, un producto que existe y otro que no
    std::vector<std::string> peticiones = {
        "LIST_PROD",
        "FIND_PROD pan",
        "FIND_PROD queso"
    };

    for ( const auto & peticion : peticiones ) {
        pedir( buzon, peticion );
    }

    // avisa a la bodega que ya puede terminar
    buzon.Enviar( "EXIT", CANAL_A_BODEGA );
    rotular( "Intermediario", "Bodega", "EXIT", "fin del guion, avisa cierre" );

    return 0;
}