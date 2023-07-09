# Diseño de de Virtual Memory en Nachos

A continuación se presenta el diseño de la memoria virtual en Nachos. Este diseño se basa en el estudio y análisis de los capítulos 9 y 10 sobre "Memoria Principal" y "Memoria Virtual" respectivamente de libro de OS. 

En este proyecto se decidió una implementación de una Unidad de Gestión de Memoria (Memory Management Unit - MMU) y un espacio de intercambio (Swap) para el manejo de memoria virtual, construido sobre el sistema operativo didáctico Nachos.

Esto para manejar todo el codigo de memoria virtual en un solo lugar, de manera que fuera mas facil de entender, modificar y depurar.

## Estructura del Código

El código fuente del proyecto se compone de varias partes claves. Estas estan en el archivo `VMDataStructures.h` define la estructura y funcionalidades de la Unidad de Gestión de Memoria (MMU) y el espacio de intercambio (Swap). Algunos otros ligeros cambios fueron necesarios en exception.cc y en system.h, pero estos son menores.

La mayor parte de las funciones se encuentran en el archivo `VMDataStructures.cc`. Una función para el manejo de page faults se encuentra en `exception.cc`. Lo unico que hace es identificar el tipo de excepcion y llamar a la funcion correspondiente en `MemoryManagementUnit`.

### MemoryManagementUnit

Para la memoria virtual se identificaron funciones claves que se necesitan para el manejo de la memoria virtual.Estas funciones requieren el manejo de Table Lookaside Buffer (TLB) y de los frames de memoria física.
Estas funciones se implementaron en la clase `MemoryManagementUnit`.

La clase `MemoryManagementUnit` implementa las siguientes funciones:

- Manejar una falla de página (page fault):
Esta función se encarga de, dado un codigo de excepción, una pagina virtual y un address Space Id (puntero a address Space)
decide lo que debe hacerse para manejar la falla de página. En palabras simples:

**Falla de Página Dura Sucia (HARD_FAULT_DIRTY)**: Se produce cuando la página requerida no está en memoria, pero sí está en el espacio de intercambio (swap space). En otras palabras, la página ya ha sido cargada previamente en la memoria y ha sido modificada (por eso es "sucia"), pero fue enviada al espacio de intercambio debido a la falta de espacio en la memoria. En este caso, la página se carga de nuevo en la memoria desde el espacio de intercambio.

Para solucionar esto se busca la pagina en el swap y se carga en memoria, luego se actualiza la tabla de paginas y la TLB.

**Falla de Página Dura Limpia (HARD_FAULT_CLEAN)**: Se produce cuando la página requerida no está ni en la memoria ni en el espacio de intercambio. En este caso, la página se carga en la memoria desde el archivo ejecutable. La página es "limpia" porque no ha sido modificada desde que se cargó del archivo ejecutable.

esto se soluciona cargando la pagina desde el archivo ejecutable a memoria, luego se actualiza la tabla de paginas y la TLB.

**Falla de Página Suave (SOFT_FAULT)**: Se produce cuando la página requerida es válida pero no está en la memoria física. En este caso, se resuelve utilizando la tabla de páginas invertida.

Para solucionar esto se busca la pagina en la tabla de paginas invertida, luego se actualiza la tabla de paginas y la TLB. Se coloca en valida.

**Falla de Página por Copia al Escribir (COPY_ON_WRITE_FAULT)**: Se produce cuando un proceso intenta escribir en una página que está marcada como de solo lectura. Esto es típicamente parte de una optimización llamada "copia al escribir" donde varias copias de una página se representan inicialmente como una única página de solo lectura en la memoria, y se crean copias separadas solo si uno de los procesos intenta escribir en la página. 

Esto se soluciona creando una copia de la pagina, luego se actualiza la tabla de paginas y la TLB. Se coloca en valida.

- Identificar un marco (frame) de memoria física libre

Para cualquier carga de página, se necesita un marco de memoria física libre para cargar la página. Esta función se encarga de encontrar un marco de memoria física libre. Si no hay marcos de memoria física libres, se desaloja un marco de memoria física. Si la
pagina esta sucia se guarda en el swap, si no se elimina..

- Identificar una entrada libre en la TLB


Para cada manejo de page faults se necesita actualizar la TLB de la maquina. Esta función se encarga de encontrar una entrada libre en la TLB. Si no hay entradas libres en la TLB, se desaloja una entrada de la TLB.

- Actualizar la información de acceso de una página


Esto es para aumentar el contador de accesos de una pagina en la tabla de paginas invertida.
se usa para el LRU replacement policy.

- Actualizar la información de modificación de una página


Esto es para aumentar el contador de modificaciones de una pagina en la tabla de paginas invertida.
se usa para el LRU replacement policy.

- Desalojar una página de 


Esto es para desalojar una pagina de memoria, se usa para el LRU replacement policy.

- Desalojar una entrada de la TLB


Esto es para desalojar una entrada de la TLB, se usa para el LRU replacement policy.

- Encontrar una página específica


Esto es para encontrar una pagina en la tabla de paginas invertida.

- Proteger las páginas de un proceso antes de un cambio de contexto

Se colocan las pagina del proceso actualmente cargadas en memoria como no validas, para que no se puedan acceder a ellas.

- Restaurar las páginas de un proceso después de un cambio de contexto


Se busca en la tabla de paginas invertida las paginas del proceso para saber cuales paginas siguen en memoria fisica y estas colocan como validas.

- Cargar una página desde el ejecutable a la memoria

se encarga de abrir el ejecutable del proceso generando page faults para cargar las paginas del proceso en memoria.

- Cargar una página desde el archivo de intercambio a la memoria

se encarga de abrir el archivo de intercambio del proceso generando page faults para cargar las paginas del proceso en memoria.

Además se implementan funciónes para imprimir el estado de los frames de memoria física, de la TLB y de la tabla de páginas invertida.
Actualmente estas funciones no se usan en el código, pero pueden ser útiles para debuggear. (por ejemplo, en función handlePageFault estan pero se encuentran comentadas)
### Swap

La clase `Swap` implementa las siguientes funciones:

- Escribir una página a un archivo de intercambio desde la memoria
- Leer una página desde un archivo de intercambio a la memoria
- Eliminar una página de un archivo de intercambio

## Uso del Código

Para compilar el código, use su compilador C++ de elección. En carpeta vm dentro de la carpeta code, ejecute los siguientes comandos:

```
make depend
make
```

Una vez compilado el código, puede ejecutar su simulador Nachos con soporte para memoria virtual. Asegúrese de revisar y entender cada una de las funciones en el código para aprovechar completamente las características de manejo de memoria virtual que se han implementado.

## No implementado

Aunque el codigo esta implementado. Actualmente solo funciona el test halt. Esto porque no encontré suficiente tiempo para 

## Referencias

Este proyecto se basa en el estudio y análisis de los capítulos 9 y 10 sobre "Memoria Principal" y "Memoria Virtual" respectivamente. Aquí están los enlaces a los resúmenes de dichos capítulos:

- Capítulo 9: [Resumen](https://docs.google.com/document/d/1bz3pGqLm4JYFTQE4Bbb_mA-pSNJdsl-C4GBYVsRicgE/edit?usp=sharing)
- Capítulo 10: [Resumen](https://docs.google.com/document/d/1blfbwG5GN8eIenkpId_QIAN9cdTTlzlnfL06auq8ghs/edit?usp=sharing)

