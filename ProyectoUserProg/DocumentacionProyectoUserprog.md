# Proyecto userprog

## Estudiante: A. Badilla Olivas
## Carné: B80874

### 1. Descripción de la solución

#### Para address space:
- Se editó el addrspace.h y addrspace.cc para que pudieran mantener varios procesos en memoria principal. Para ello se agregó un bitmap en clase system de la carpeta threads para manejar las páginas disponibles.
- Se agregó un constructor a partir de otro addrspace para poder copiar el espacio de direcciones de un proceso a otro. Esto se utilizó en el fork.
### Para las system calls:
- Se creó una tabla de archivos para manejar los archivos abiertos por un thread. Dado que la tabla puede ser compartida por varios threads se utilizó un shared pointer para que la tabla no se destruyera únicamente cuando el último thread que la utilizaba terminara.
- Se editó exception.cc para agregar las excepciones de los system calls. Lo que hacemos es hacer llamados a los system calls de linux y devolver los resultados a través de los registros, esto en el caso de OPEN, CREATE, READ, WRITE, CLOSE y SOCKETS. Para el resto de system calls se devuelve un error. Estas llamadas agregan debido control de concurrencia en el caso de los archivos a través de semáforos.
- Varias estructuras fueron añadidas al system.h y system.cc para agregar la funcionalidad de los llamados al sistema. Para compilarlas fue necesario editar el archivo Makefile.common para agregar los archivos a compilar.
- Para semáforos se creyó una estructura auxiliar (sysDataStructures.h y sysDataStructures.cc en carpeta threads) que contiene un mapa thread safe de llave entera y de valor puntero a "Semaphore" y de un bitmap, juntos asignan un identificador al semáforo y guardan información relacionada con este. Semaphore contiene el puntero al semáforo desarrollado en la tarea 1 threads/synch.cc.
- Para los sockets se tiene una estructura de la misma manera. Además, se utiliza la clase sysSocket, que es la clase desarrollada en el contexto de trabajos del curso de Proyecto Integrador de Sistemas Operativos y Redes. Estas clases de manejo de sockets se encuentra en threads/sysSocket.h y threads/sysSocket.cc, threads/sockExcept.h y threads/sockExcept.h, threads/SocketLib.h y threads/SocketLib.cc Esta clase se encarga de manejar los sockets y de hacer.
- Para los system calls de fork y exec se crearon dos funciones auxiliares que se le pasan como parámetro a la función Fork de la clase. En el caso de del system call Fork, se crea un nuevo thread y se le pasa como parámetro la función auxiliar que se encarga de copiar el addrspace del thread padre al hijo, ademas se le pasa la dirección de la función que debe ejecutar. En el caso de Exec, se crea un nuevo thread y se le pasa como parámetro la función auxiliar que se encarga de cargar el programa en memoria a través de un nombre de ejecutable.
- Para el system call Join el padre accede a la tabla de threads y busca el thread hijo, si este no ha terminado, el padre se bloquea en el semáforo del hijo, si el hijo ya termino, el padre devuelve el valor de retorno del hijo y remueve el thread de la tabla de threads. En caso de que el padre termine antes que el hijo, el hijo se remueve de la tabla de threads de manera autónoma en el system call exit. El system call exit revisa los tipos de thread para saber como deber terminar.
### Estructuras para manejo de threads:
- Se creó una estructura para manejar threads (sysDataStructures.h y sysDataStructures.cc en carpeta threads). Esta consiste de un mapa thread safe de llave entera y de valor puntero a "ThreadData" y de un bitmap, juntos  asignan un identificador al thread y guardan información relacionada con este. Thread Data contiene el puntero al thread, el nombre del ejecutable, el estatus de salida y un semaforo para hacer join.
### Threads:
- La clase thread recibió nuevos atributos como un número identificador, un identificador de padre, un puntero compartido a la tabla de archivos, un tipo (definido como un enum Main, user fork, user exec).
#### (Nota: para un descripción detallada, revisar documentación interna en el código)

## 2. Pruebas
Se adjuntan capturas de pantalla de los test funcionando. Se han probado todos los test de la carpeta test, a excepción de aquellos que necesitan del proyecto de memoria virtual. Además, se agregó un semTest para probar semáforos, createBasic para probar create, open y write. También se le cambió el nombre del test "todos" por "All". Shell fue editado para que funcionara tal que al recibir "q" terminara el programa.

## 3. Manual de usuario
-para compilar se hace make en la carpeta code/userprog/ requisitos: tener instalado el g++, make y las librerias de linux para ssl.
para ejecutar tests se hace:
```
./nachos -x ../test/testDeseado
```
para compilar tests se hace desde la carpeta code/test:
```
make NombreDelTestSinLaCAlFinal
```
ejemplo
```
make All
```

Nota: no se implementan las otras llamadas al sistema de concurrencia pues el profesor que con semáforos era suficiente.