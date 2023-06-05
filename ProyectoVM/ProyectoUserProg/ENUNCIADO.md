# Proyecto Programado #2
Threads y Llamadas al Sistema

## Versión original: Tom Anderson
## Adaptación y traducción: Francisco Arroyo

## Descripción
La segunda fase de NachOS consiste en brindar soporte a la multiprogramación. Al igual que en la primera asignación, se les proporciona parte del código necesario, y su trabajo será completar y mejorar el sistema. Hasta ahora, el código que ha escrito para NachOS solo ha sido parte del kernel del sistema operativo. En un sistema operativo real, el kernel no solo utiliza sus procedimientos internamente, sino que también permite que los programas de los usuarios accedan a algunas de sus rutinas utilizando llamadas al sistema (system calls).

El primer paso para desarrollar esta asignación es leer y comprender la parte del sistema que hemos escrito para ustedes. Los archivos del kernel se encuentran en "userprog", se añaden algunas partes para la simulación de la máquina Mips en el directorio "machine", y un sistema de archivos inicial en "filesys". Los programas de los usuarios están en "test" y las utilidades para crear los programas ejecutables de NachOS se encuentran en "bin". Tenga en cuenta que los programas de los usuarios deben ser compilados y ejecutados en máquinas con arquitectura Mips (que la ECCI no proporciona, por lo que es necesario utilizar un cross-compiler). NachOS debe ser compilado utilizando las utilidades de las máquinas Linux.

Con el código que se les presenta, NachOS solo puede ejecutar un programa de usuario codificado en C a la vez. Como caso de prueba, se proporciona un programa de usuario trivial llamado "halt", que utiliza una llamada al sistema para "apagar" la máquina. Para ejecutar el programa "halt", debe compilarlo (make) y ejecutar NachOS (./nachos -x ../test/halt). Debe seguir la ejecución, observar cómo se comporta cuando el programa se carga en memoria, se ejecuta y luego invoca una llamada al sistema (halt).

Puede resultar útil el artículo de Narten ("Road Map"). También pueden realizar cambios razonables en algunas de las constantes que se encuentran en el archivo "machine.h", por ejemplo, cambiar el tamaño de la memoria física, si eso ayuda a crear mejores casos de prueba. Los archivos utilizados para esta asignación son:

- userkernel.h, userkernel.cc: rutinas para iniciar y probar el kernel de multiprogramación.
- addrspace.h, addrspace.cc: crea un espacio de direcciones en el cual se ejecuta un programa de usuario, y lo carga desde el disco.
- syscall.h: es la interfaz para las llamadas al sistema; son los procedimientos del kernel que los programas de usuario pueden invocar.
- exception.cc: el controlador de las llamadas al sistema y otras excepciones del usuario, como "page-faults". En el sistema que se presenta, solo se soporta la llamada al sistema "halt".
- bitmap.h, bitmap.cc: rutinas para manipular bitmaps (que pueden ser útiles para llevar el control de los marcos de memoria física).
- filesys.h, filesys.cc (en el directorio "filesys"): definen una versión preliminar del sistema de archivos de NachOS. Para esta asignación, hemos implementado el sistema de archivos de NachOS utilizando directamente las rutinas del sistema de archivos de Unix, de manera que solo se debe probar una cosa a la

 vez.
- translate.h, translate.cc: rutinas para la traducción de direcciones. En el código que se presenta, para ejecutar "halt", asumimos que cada dirección virtual es igual a cada dirección real, lo cual limita el sistema a ejecutar solo un programa de usuario a la vez. Es necesario generalizar este esquema para poder ejecutar varios programas de usuario de manera concurrente. No se requiere implementar el manejo de memoria virtual en esta asignación, se delega hasta la asignación #3, por ahora cada página de memoria debe estar en la memoria principal.
- machine.h, machine.cc: emula la parte de la máquina que ejecuta los programas de usuario: memoria principal, registros del procesador, etc.
- mipssim.h, mipssim.cc: emula el conjunto de instrucciones de enteros del procesador MIPS R2/3000.
- console.h, console.cc: emula el dispositivo de terminal utilizando archivos de Unix. Una terminal es (i) orientada a caracteres (bytes), (ii) los caracteres se pueden leer y escribir simultáneamente, y (iii) los caracteres pueden llegar de manera asíncrona (como resultado de las teclas que presiona el usuario), sin ser solicitados explícitamente.
- synchconsole.h, synchconsole.cc: proporciona acceso sincronizado al dispositivo de consola.

En esta asignación se les presenta una CPU simulada que modela una CPU real. De hecho, la CPU simulada es la misma que la CPU real (un chip Mips), pero al simular la ejecución, se tiene control total sobre cuántas instrucciones se ejecutan, cómo funciona el espacio de direcciones y cómo se manejan las excepciones (incluyendo las llamadas al sistema) e interrupciones.

Nuestro simulador puede ejecutar programas escritos en C y compilados para la arquitectura Mips. Consulte el directorio "test" para encontrar algunos ejemplos. Los programas compilados (utilizando un cross-compiler) deben ser enlazados con una serie de flags especiales (ver el "Makefile"), y luego convertidos al formato de NachOS utilizando la utilidad "coff2noff" (que se proporciona en "bin"). La única restricción es que no se admiten las instrucciones de punto flotante de Mips.

Debe proporcionar un kernel de NachOS que sea "a prueba de balas" contra los programas de usuario, es decir, no debe haber nada dentro de un programa de usuario que haga que el sistema operativo "se caiga" (con la excepción de llamar explícitamente a la función "halt").

## Tareas
1. Implementar el manejo de excepciones y llamadas al sistema. Se deben soportar todas las llamadas al sistema definidas en "syscall.h". Se presenta una rutina en ensamblador llamada "syscall" que proporciona la forma de invocar una llamada al sistema desde una rutina en C (Unix tiene un método similar, consulte "man syscall"). Se necesita completar la parte 2 de esta asignación para probar las llamadas al sistema "exec" y "wait".
2. Implementar la multiprogramación. El código proporcionado restringe la ejecución a un solo programa de usuario a la vez. Se necesita:
   - Encontrar la forma de asignar marcos de memoria física de manera que varios programas de usuario puedan residir en la memoria principal al mismo tiempo (ver "bitmap.h").
   - Prop

orcionar una forma de copiar datos entre el kernel y el espacio de direcciones virtual del usuario (ahora las direcciones que el programa de usuario "ve" no son las mismas que el kernel "ve").
   - Agregar sincronización a las rutinas que crean e inicializan el espacio de direcciones, de manera que puedan ser accedidas concurrentemente por múltiples programas. Tenga en cuenta que "scheduler.cc" ahora guarda y restaura el estado de la máquina en los cambios de contexto. Es deseable tener algunas rutinas similares a "cp" y "cat" de Unix, y utilizar el shell proporcionado en el directorio "test" para verificar el manejo de las llamadas al sistema y la multiprogramación.
3. Implementar programas de usuario con múltiples hilos. Implementar las llamadas al sistema "fork" y "yield", que permitan al usuario llamar a una rutina en el mismo espacio de direccionamiento y hacer "ping pong" entre los hilos (Sugerencia: es necesario cambiar la forma actual del kernel de asignar memoria en el espacio de direcciones del usuario para cada una de las pilas de los hilos).
4. (Extra, 5% adicional) La versión actual de la llamada al sistema "exec" no ofrece una forma para que el usuario pueda pasar parámetros o argumentos al nuevo espacio de direcciones creado. En Unix esto es posible, por ejemplo, se pueden pasar argumentos en la línea de comandos al nuevo espacio de direcciones. Implementar esta funcionalidad en "exec".

¡Feliz programación!