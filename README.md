# sistemas-digitales
Programas desarrollados para TPs de Sistemas Digitales - FIUBA
Por Gonzalo Ávila Alterach (2015)

# Parser VCD
Convierte los resultados de una simulación de un sistema con salida VGA en la imagen que generaría si fuera conectada a un monitor. Parsea las señales provenientes de un simulador, como puede ser GHDL, ISim. No es necesario cambiar nada del código existente, el único requerimiento es que existan las señales r, g, b, vs y hs. 

# Enviador de puntos
Programa que envía coordenadas por un puerto serie, realizando un procesamiento previo para reducir la cantidad de bits por punto y eliminar los repetidos. Usado para el TP4. Funciona tanto en Windows como en Linux.

# Generador de ROM de Caracteres desde PNG
Pequeño programa que convierte una imagen PNG en un código vhd que genera una memoria donde cada bit corresponde al valor de un pixel.
Ideal para usarlo con tipografías grandes, que sean tediosas de copiar a mano. Se incluyen tipografías propias.

# Simulación Rotador CORDIC
Programa interactivo desarrollado en C++ con la biblioteca SDL, que permite simular el TP4 y observar el comportamiento del algoritmo CORDIC a medida que se disminuyen la cantidad de iteraciones. También permite calcular los coeficientes para cada iteración.
