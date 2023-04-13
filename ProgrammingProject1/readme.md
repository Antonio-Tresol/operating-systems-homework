# Juego de la “papa” caliente

Vamos a construir un anillo de procesos (n) simulando la ronda en el juego de la papa
caliente (paso de token). El proceso i le pasa la papa (un mensaje) al proceso i+1 o al i-1,
el proceso n-1 se la pasará al 0, o el 0 lo hará al n-1 . La papa, que tiene un valor inicial
(v), es movida entre los participantes, siguiendo el sentido de rotación, cada participante
del juego activo modifica su valor, comprueba si la papa estalló, en cuyo caso “sale” del
juego y debe escoger al azar un nuevo valor inicial para la papa (vi), para luego pasar la
papa al siguiente participante. Cada participante pasivo pasa el valor de la papa sin
modificarlo hasta que el juego termine.

Cada participante estará representado por un proceso que tendrá un identificador
único que utilizará para comunicarse con su vecino, por ejemplo: 10, 11, 12, 13, ..., 10 + n-
1. El programa recibirá tres parámetros el primero que indica la cantidad de miembros de
la ronda (n), el segundo un número inicial para la papa (v) y el tercero el sentido de
rotación (r) de la papa (estos valores pueden estar inicializados por omisión: n=100,
v=2023, r=[i pasa la papa a i - 1]). Cada proceso recibe el valor de la papa y le aplica las
reglas de Collatz, si el resultado es uno, indicativo de que la papa explota, ese proceso
“sale” del juego y se convierte en un comunicador pasivo. Gana el último proceso en
“salir” del juego, al que le tocará avisar a los demás procesos que el juego terminó,
pasando para ello la papa con un valor negativo (mensaje), para que los demás puedan
finalizar su ejecución. El programa al menos debe desplegar el valor del proceso ganador y
la manera en que los participantes van saliendo.

Para simular este juego, construya un programa que genere un proceso (fork) para
cada participante de la ronda (n) y establezca los elementos de sincronización. Este
programa elige al azar el participante que va a arrancar con el valor indicado como
parámetro para la papa (v). Utilice semáforos y memoria compartida para intercambiar
la papa entre los participantes. Despliegue información para determinar el estado del
juego. El procedimiento principal (“main”) no participará en las decisiones de sincronización
(excepto para iniciar el juego), ni en la ronda, una vez creados los recursos, solo debe
esperar a que todos los participantes terminen, para eliminar los elementos de
sincronización y finalizar la ejecución del programa.

## como compilar

usar 'make' para compilar el programa
para ejecutar el programa usar './ProgammingProject1 <n> <v> <r>'
usar 'make clean' para limpiar los archivos compilados
usar 'make run' para compilar y ejecutar el programa con los valores por defecto