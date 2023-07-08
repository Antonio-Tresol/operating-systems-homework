# Diseño de de Virtual Memory en Nachos
A continuación preseto mi diseño de la memoria virtual en Nachos. Este diseño se basa en el estudio y análisis de los capítulos 9 y 10 sobre "Memoria Principal" y "Memoria Virtual" respectivamente. 

Este proyecto decidí una implementación de una Unidad de Gestión de Memoria (Memory Management Unit - MMU) y un espacio de intercambio (Swap) para el manejo de memoria virtual, construido sobre el sistema operativo didáctico Nachos.

Esto para manejar todo el codigo de memoria virtual en un solo lugar, de manera que fuera mas facil de entender, modificar y depurar.

## Estructura del Código

El código fuente del proyecto se compone de varias partes claves. Estas estan en el archivo `VMDataStructures.h` define la estructura y funcionalidades de la Unidad de Gestión de Memoria (MMU) y el espacio de intercambio (Swap). 

### MemoryManagementUnit
Para la memoria virtual se identificaorn funciones claves que se necesitan para el manejo de la memoria virtual.Estas funciones requieren el manejo de Table Lookaside Buffer (TLB) y de los frames de memoria física.
Estas funciones se implementaron en la clase `MemoryManagementUnit`.

La clase `MemoryManagementUnit` implementa las siguientes funciones:

- Identificar un marco (frame) de memoria física libre
- Identificar una entrada libre en la TLB
- Actualizar la información de acceso de una página
- Actualizar la información de modificación de una página
- Manejar una falla de página (page fault)
- Desalojar una página de memoria
- Desalojar una entrada de la TLB
- Encontrar una página específica
- Proteger las páginas de un proceso antes de un cambio de contexto
- Restaurar las páginas de un proceso después de un cambio de contexto
- Cargar una página desde el ejecutable a la memoria
- Cargar una página desde el archivo de intercambio a la memoria

### Swap

La clase `Swap` implementa las siguientes funciones:

- Escribir una página a un archivo de intercambio desde la memoria
- Leer una página desde un archivo de intercambio a la memoria
- Eliminar una página de un archivo de intercambio

## Uso del Código

Para compilar el código, use su compilador C++ de elección. Tenga en cuenta que el código es dependiente de las bibliotecas Nachos y, por lo tanto, debe estar instalado y correctamente configurado en su entorno.

Una vez compilado el código, puede ejecutar su simulador Nachos con soporte para memoria virtual. Asegúrese de revisar y entender cada una de las funciones en el código para aprovechar completamente las características de manejo de memoria virtual que se han implementado.

## No implementado

Por que no pude encontrar el tiempo y apesar de que lo intente no pude implementar el todo manejo de la memoria virtual. Actualmente solo funciona el test halt.

## Referencias

Este proyecto se basa en el estudio y análisis de los capítulos 9 y 10 sobre "Memoria Principal" y "Memoria Virtual" respectivamente. Aquí están los enlaces a los resúmenes de dichos capítulos:

- Capítulo 9: [Resumen](https://docs.google.com/document/d/1bz3pGqLm4JYFTQE4Bbb_mA-pSNJdsl-C4GBYVsRicgE/edit?usp=sharing)
- Capítulo 10: [Resumen](https://docs.google.com/document/d/1blfbwG5GN8eIenkpId_QIAN9cdTTlzlnfL06auq8ghs/edit?usp=sharing)

