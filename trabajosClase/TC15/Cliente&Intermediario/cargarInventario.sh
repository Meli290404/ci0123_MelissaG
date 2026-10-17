#!/bin/bash
# crea el archivo de la cafeteria con dos bodegas, 13 categorias y 104 productos usando adminBodegas.
# cada categoria tiene 8 productos para que la respuesta 22 del protocolo quede bajo los 256 caracteres.
# uso: ./cargarInventario.sh [archivo]   (por defecto cafeteria.dat)

ARCHIVO=${1:-cafeteria.dat}
ADMIN=./adminBodegas

if [ ! -x "$ADMIN" ]; then
    echo "no se encontro $ADMIN, compile primero con make"
    exit 1
fi
if [ -e "$ARCHIVO" ]; then
    echo "$ARCHIVO ya existe; borrelo primero si quiere crearlo de nuevo"
    exit 1
fi

# cada llamada se detiene en el primer error para no dejar el archivo a medias sin avisar
correr() {
    "$ADMIN" "$ARCHIVO" "$@" > /dev/null || { echo "fallo: $ADMIN $ARCHIVO $*"; exit 1; }
}

bodega()    { correr agregar "$1" "$2"; }
categoria() { correr categoria "$1" "$2" "$3"; }
producto()  { correr producto "$1" "$2" "$3" "$4" "$5"; }   # bodega categoria nombre cantidad precioCentavos

correr crear TicAmazon

# bodega 1: bebidas y comidas
bodega bod1 Bodega-1

categoria bod1 cafes "Cafes"
producto bod1 cafes Espresso         40 110000
producto bod1 cafes Americano        40 120000
producto bod1 cafes Capuchino        35 160000
producto bod1 cafes Latte            35 170000
producto bod1 cafes Mocaccino        25 190000
producto bod1 cafes Macchiato        25 165000
producto bod1 cafes CafeConLeche     30 140000
producto bod1 cafes Cortado          20 135000

categoria bod1 bebfr "Bebidas frias"
producto bod1 bebfr CafeFrio         20 150000
producto bod1 bebfr Frappe           25 210000
producto bod1 bebfr Limonada         15 100000
producto bod1 bebfr TeFrio           30 110000
producto bod1 bebfr Batido-Fresa     18 180000
producto bod1 bebfr Batido-Mango     18 180000
producto bod1 bebfr Horchata         22 120000
producto bod1 bebfr AguaPipa         12 130000

categoria bod1 bebca "Bebida caliente"
producto bod1 bebca CafeCaliente     30 140000
producto bod1 bebca Chocolate        25 160000
producto bod1 bebca TeVerde          30 110000
producto bod1 bebca TeNegro          30 110000
producto bod1 bebca Manzanilla       20 100000
producto bod1 bebca AguaDulce        20  90000
producto bod1 bebca ChaiLatte        15 190000
producto bod1 bebca Matcha           15 210000

categoria bod1 desay "Desayunos"
producto bod1 desay GalloPinto       20 350000
producto bod1 desay Huevos-Revueltos 20 280000
producto bod1 desay Panqueques       15 300000
producto bod1 desay Waffles          15 320000
producto bod1 desay Tostadas         25 180000
producto bod1 desay Omelette         15 330000
producto bod1 desay Avena            20 200000
producto bod1 desay Granola          20 220000

categoria bod1 empar "Emparedados"
producto bod1 empar Sandwich-Jamon   20 260000
producto bod1 empar Sandwich-Pollo   20 290000
producto bod1 empar Club-Sandwich    15 340000
producto bod1 empar Croissant-Jamon  18 270000
producto bod1 empar Bagel-Queso      18 240000
producto bod1 empar Panini           15 310000
producto bod1 empar Wrap-Pollo       15 300000
producto bod1 empar Wrap-Vegetal     15 280000

categoria bod1 ensal "Ensaladas"
producto bod1 ensal Ensalada-Cesar   12 380000
producto bod1 ensal Ensalada-Griega  12 360000
producto bod1 ensal Ensalada-Frutas  15 250000
producto bod1 ensal Ensalada-Atun    10 370000
producto bod1 ensal Ensalada-Caprese 10 390000
producto bod1 ensal Ensalada-Pollo   12 380000
producto bod1 ensal Ensalada-Verde   15 280000
producto bod1 ensal Ensalada-Pasta   12 330000

categoria bod1 postr "Postres"
producto bod1 postr Tiramisu         10 250000
producto bod1 postr Cheesecake       10 260000
producto bod1 postr Flan             15 150000
producto bod1 postr Brownie          20 120000
producto bod1 postr TresLeches       12 200000
producto bod1 postr Pie-Limon        10 220000
producto bod1 postr Mousse           10 210000
producto bod1 postr Arroz-con-Leche  15 140000

# bodega 2: dulces, panes y snacks
bodega bod2 Bodega-2

categoria bod2 caram "Caramelos"
producto bod2 caram Chupeta          50  30000
producto bod2 caram Menta            80  20000
producto bod2 caram Gomitas          60  50000
producto bod2 caram Caramelo-Fresa   70  25000
producto bod2 caram Caramelo-Leche   70  25000
producto bod2 caram Toffee           40  35000
producto bod2 caram Malvaviscos      40  60000
producto bod2 caram Regaliz          30  40000

categoria bod2 repos "Reposteria"
producto bod2 repos PanDulce         25  80000
producto bod2 repos Galleta-Avena    40  50000
producto bod2 repos Galleta-Chispas  40  55000
producto bod2 repos Muffin-Arandano  20 120000
producto bod2 repos Muffin-Choco     20 120000
producto bod2 repos Dona-Glaseada    30  90000
producto bod2 repos Cupcake          25 110000
producto bod2 repos Rollo-Canela     20 130000

categoria bod2 panes "Panes"
producto bod2 panes Baguette         15 150000
producto bod2 panes Pan-Integral     15 180000
producto bod2 panes Pan-Centeno      10 190000
producto bod2 panes Pan-Queso        30  70000
producto bod2 panes Pan-Coco         25  80000
producto bod2 panes Bolillo          40  30000
producto bod2 panes Croissant        25 100000
producto bod2 panes Ciabatta         12 160000

categoria bod2 choco "Chocolates"
producto bod2 choco Bombones         20 250000
producto bod2 choco Trufa            20 180000
producto bod2 choco Barra-Chocolate  30 120000
producto bod2 choco Chocolate-Blanco 25 130000
producto bod2 choco Chocolate-Amargo 25 140000
producto bod2 choco Almendras-Choco  20 200000
producto bod2 choco Mani-Choco       30 110000
producto bod2 choco Pasas-Choco      30 100000

categoria bod2 snack "Snacks"
producto bod2 snack Papas-Tostadas   40  90000
producto bod2 snack Platanitos       40  80000
producto bod2 snack Palomitas        30  70000
producto bod2 snack Mani-Salado      35  60000
producto bod2 snack Nachos           25 150000
producto bod2 snack Tortillitas      30  85000
producto bod2 snack Pretzels         25  95000
producto bod2 snack Barra-Granola    35  75000

categoria bod2 helad "Helados"
producto bod2 helad Helado-Vainilla  20 120000
producto bod2 helad Helado-Choco     20 120000
producto bod2 helad Helado-Fresa     20 120000
producto bod2 helad Paleta-Mango     30  60000
producto bod2 helad Paleta-Limon    30  60000
producto bod2 helad Cono-Doble       15 160000
producto bod2 helad Sundae           12 220000
producto bod2 helad Banana-Split     10 300000

echo "listo: $ARCHIVO con dos bodegas"
"$ADMIN" "$ARCHIVO" libres