# Sección de preguntas 
Para entender el funcionamiento de la cache y memoria virtual en NachOS debe conocer las respuestar a estas preguntas y las funcionalidad indicadas

## ¿Qué es un "page fault"?
**Respuesta**: *"In computing, a page fault is an exception that the memory management unit (MMU) raises when a process accesses a memory page without proper preparations. Accessing the page requires a mapping to be added to the process's virtual address space. Besides, the actual page contents may need to be loaded from a backing store, such as a disk. The MMU detects the page fault, but the operating system's kernel handles the exception by making the required page accessible in the physical memory or denying an illegal memory access. Valid page faults are common and necessary to increase the amount of memory available to programs in any operating system that uses virtual memory"*

- Para la lectura/escritura de datos a la memoria, el simulador utilizar los métodos "machine::ReadMem" y "machine::WriteMem". Estos métodos utilizan direcciones virtuales o lógicas debe entender ¿Por qué?, además debe comprender por qué es posible que estos métodos retornen falso

**Respuesta**: *estas funciones que estan en el file translate.cc en carpeta machine reciben un virtual address y adentro lo traducen a virtual address. Esto es porque los procesos no saben en que memoria direcion de la memoria fisica estan las cosas, solo saben en que direccion virtual estan. Por eso se hace la traduccion. Si la traduccion falla, es porque no se encontro la direccion virtual en la tabla de paginas, por lo que se hace un page fault.*

- Entender el funcionamiento del método "machine::Translate, parámetros, valor de retorno

**Respuesta**: *retorna el tipo de exception ocurrido. PageFaultException se retorna cuando: el numero de pagina virtual es mayor al tamano de la tabla de paginas o si la tabla no es valida (no se que quiere decir eso)*

## ¿Qué es un "TLB"?
**Respuesta**: *A translation lookaside buffer (TLB) is a memory cache that stores the recent translations of virtual memory to physical memory. It is used to reduce the time taken to access a user memory location. It can be called an address-translation cache.*

- Comprender la composición de las entradas de la "page table"
**Respuesta**: *las entradas estos atributos (numero de pagina virtual, numero de pagina fisica, valid bit, use bit, dirty bit). El bit valid indica si la pagina esta en memoria fisica o no. El bit use indica si la pagina fue usada recientemente. El bit dirty indica si la pagina fue modificada recientemente.*

- Comprender la composición de las entradas de la "TLB"
**Respuesta**: *Son las mismas que page table. Es un objeto que se llama TranslationEntry.La TLB es un arreglo de Translation Entry tienen estos atributos (numero de pagina virtual, numero de pagina fisica, valid bit, use bit, dirty bit). El bit valid indica si la pagina esta en memoria fisica o no. El bit use indica si la pagina fue usada recientemente. El bit dirty indica si la pagina fue modificada recientemente.*

- En los cambios de contexto, ¿Cuál debe ser el valor inicial de esta estructura, cuando un proceso recién comienza su ciclo de CPU?

**Respuesta**: *La Translation Lookaside Buffer (TLB) es una estructura de memoria caché que guarda las traducciones recientes de las direcciones de memoria virtual a física. Cuando se produce un cambio de contexto, es decir, cuando un proceso deja de usar la CPU y otro proceso comienza a usarla, generalmente se limpia o se invalida la TLB.*

*La razón de esto es que las entradas de la TLB son específicas para cada proceso debido a su propia tabla de páginas y espacio de dirección único. Permitir que un nuevo proceso vea o use las entradas de la TLB del proceso anterior sería un error y podría provocar problemas de seguridad y corrección.*

*Por lo tanto, cuando un proceso recién comienza su ciclo de CPU después de un cambio de contexto, el valor inicial de la TLB para ese proceso generalmente está vacío o invalidado. A medida que el nuevo proceso comienza a ejecutarse y a acceder a las direcciones de memoria, la TLB se llena gradualmente con las traducciones de direcciones más recientes y relevantes para ese proceso. Esto permite un acceso más rápido a las traducciones de direcciones de memoria que se utilizan con frecuencia, mejorando así el rendimiento general del proceso.*

## ¿Qué representa la variable "tlb" en la clase "Machine"? ¿Cuántas hay?
**Respuesta**: *la variable tlb es un arreglo de TranslationEntry. Hay una sola global que es de READ ONLY para procesos, solo la puede modificar el kernel.*

- Entender el significado de "TLB es una cache de la tabla de páginas:

**Respuesta**: *A translation lookaside buffer (TLB) is a memory cache that stores the recent translations of virtual memory to physical memory. It is used to reduce the time taken to access a user memory location. It can be called an address-translation cache. It is a part of the chip's memory-management unit (MMU). A TLB may reside between the CPU and the CPU cache, between CPU cache and the main memory or between the different levels of the multi-level cache. The majority of desktop, laptop, and server processors include one or more TLBs in the memory-management hardware, and it is nearly always present in any processor that utilizes paged or segmented virtual memory.*

*The TLB is sometimes implemented as content-addressable memory (CAM). The CAM search key is the virtual address, and the search result is a physical address. If the requested address is present in the TLB, the CAM search yields a match quickly and the retrieved physical address can be used to access memory. This is called a TLB hit. If the requested address is not in the TLB, it is a miss, and the translation proceeds by looking up the page table in a process called a page walk. The page walk is time-consuming when compared to the processor speed, as it involves reading the contents of multiple memory locations and using them to compute the physical address. After the physical address is determined by the page walk, the virtual address to physical address mapping is entered into the TLB.*

- Comprender la función de los métodos "AddrSpace::RestoreState" y "AddrSpace::SaveState" la "page table"
- 
**Respuesta**:
*Los métodos `AddrSpace::SaveState` y `AddrSpace::RestoreState` para gestionar el estado del espacio de direcciones de un proceso durante los cambios de contexto. Cuando ocurre un cambio de contexto, el sistema operativo debe cambiar del espacio de direcciones del proceso que se está ejecutando actualmente al espacio de direcciones del próximo proceso que será programado.*

*`AddrSpace::SaveState`:*
*Este método se utiliza para guardar el estado actual del espacio de direcciones cuando un proceso está siendo intercambiado fuera de la CPU. Por ejemplo, podría guardar el estado actual de la tabla de páginas del proceso, el estado de cualquier archivo mapeado en memoria, y cualquier otro estado relacionado con la memoria que sea necesario. Los detalles exactos de lo que se guarda pueden depender de la implementación específica y del diseño del sistema operativo.*

*`AddrSpace::RestoreState`:*
*Este método se utiliza para restaurar el estado guardado del espacio de direcciones cuando un proceso está siendo reintegrado a la CPU. Por ejemplo, podría restaurar el estado previamente guardado de la tabla de páginas, archivos mapeados en memoria, etc., preparando así el estado de la memoria para que el proceso continúe su ejecución.*

- Comprender cuándo son llamados estos métodos
**Respuesta**: *Estos métodos se llaman cuando se produce un cambio de contexto, es decir, cuando un proceso deja de usar la CPU y otro proceso comienza a usarla. Esto pasa en el file scheduler metodo run*

- Comprender para qué sirven las variables "pageTable" y "tlb" declaradas en la clase "Machine"
**Respuesta**: *sirven para manejar las direcciones fisicas y las direcciones virtuales. El pageTable se refiere a las cosas del proceso y el TLB es el cache de estas mismas*

- Explicar cómo se realiza la búsqueda de la dirección lógica en el método "machine::Translate"
**Respuesta**: *Primero se busca en el TLB, si no esta se busca en la pageTable, si no esta se produce una excepcion (en caso de que virtual page number sea mayor a la cantidad de paginas del proceso o si la pagina no esta en memoria)*
- Casos en el que el simulador utiliza la variable "pageTable"
- Casos en el que el simulador utiliza la variable "tlb"
**Respuesta**: *El simulador utiliza cuando lee de memoria y cuando escribe de memoria*

## ¿Cómo se determina que la página es válida?
- Funcionalidad de la variable "entry"
- Mapeo de la dirección lógica a la dirección física
**Respuesta**: *En general, si una página está en la memoria física, su entrada correspondiente en la tabla de páginas o en el TLB se marca como válida, es decir, el bit de validez se establece en 1. Si una página no está en la memoria física (por ejemplo, porque ha sido intercambiada al disco), su entrada correspondiente se marca como no válida, es decir, el bit de validez se establece en 0.*
*En la mayoría de los sistemas operativos, cuando un programa intenta acceder a una página cuya entrada está marcada como no válida, el sistema operativo lanza una excepción de fallo de página. A continuación, el sistema operativo intentará cargar la página requerida en la memoria física y actualizar la tabla de páginas o el TLB para marcar la entrada como válida.*
*Por lo tanto, si en el codigo pageTable.valid o tlb.valid, está comprobando el bit de validez para determinar si la página correspondiente está actualmente en la memoria física o no.*

## ¿Qué representa la variable "pageFrame"?

**Respuesta**: *En la gestión de memoria virtual, el término "pageFrame" o marco de pagina se refiere a un bloque de memoria física de longitud fija en el cual se puede mapear una página (un bloque de memoria virtual de igual tamaño).*

*Cuando un proceso está en ejecución, accede a la memoria utilizando direcciones virtuales. Cada una de estas direcciones consta de un número de página y un desplazamiento dentro de esa página(el offset). Sin embargo, los datos reales se almacenan en memoria física. Para facilitar la traducción entre direcciones virtuales y físicas, el sistema mantiene una tabla de páginas (o en algunos sistemas, una memoria caché de traducción de direcciones o TLB, por sus siglas en inglés), donde cada entrada contiene la información de mapeo para una página.*
*En este contexto, un "pageFrame" es la unidad en memoria física donde se carga la página de la memoria virtual. La variable "pageFrame" en el código almacena el número del marco de página al que se mapea la página (identificada por el número de página virtual, 'vpn') en la memoria física. Esta información se almacena normalmente en la tabla de páginas o en el TLB y se utiliza para calcular la dirección física correspondiente a una dirección virtual dada.*

*Específicamente, la dirección física se calcula como:*

```
direccionFisica(physAddrs) = pageFrame * TamañoPagina(pageSize) + desplazamiento(offset)
```

*Donde `pageFrame` es el número de marco de página física, `TamañoPagina` es el tamaño de una página (o marco de página - tienen el mismo tamaño), y `desplazamiento` es el desplazamiento dentro de la página. Este cálculo proporciona la dirección real en la memoria física donde se encuentra el dato solicitado.*

## ¿Cómo se deriva la dirección física y cómo es devuelta por el método "machine::Translate"?
- Entender cómo se modifican los valores de "use" y "dirty" y en cuál estructura ocurren los cambios
**Respuesta**: *El valor de "use" se modifica en la tabla de páginas y si estamos escribienod el valor de "dirty" se modifica entry para indicar que ha sido modificada desde que se cargó por última vez en la memoria física. Estos cambios se realizan en la tabla de páginas o en el TLB, según el caso de donde se encontro*
- Identificar y comprender los casos en que son generadas excepciones en el método "machine::AddrSpace"


- Comprender cómo resolver cada uno de los casos en que ocurre una excepción "PageFaultException" en el método "machine::Translate"



## ¿Es posible que en los casos anteriores la página si esté en memoria? ¿Cómo determinar si la página realmente no está en memoria?
- Explicar cómo proceder con los otros tipos de excepciones
- El simulador de MIPS no avanza los contadores de programa cuando ocurren las excepciones, en el caso de "SysCallException", lo tuvimos que - realizar 'manualmente', analizar que debería ocurrir con los tipos de excepciones encontrados
- Comprender cómo conseguir los marcos de memoria para asignarlos a las páginas faltantes
## ¿Cómo aplicar los algoritmos de reemplazo?
## ¿Qué pasa si toda la memoria física está llena?
## ¿Cómo reemplazar marcos de memoria?
## ¿Cómo determinar si un marco debe ir al SWAP?
- Comprender cómo conseguir la página faltante
- Determinar el origen de la página faltante, recordar los segmentos de los programas MIPS
## ¿Cómo recuperar la página faltante?
## ¿Puede estar en el archivo ejecutable original?
## ¿Puede estar en el SWAP?
## ¿Cuáles estructuras debe actualizar para no volver a producir la excepción?

# SWAP
- Conocer la estructura interna del archivo de intercambio
- Conocer el momento en que el archivo debe construirse y destruirse
- ¿Cómo determinar la cantidad de elementos que debe contener?
- Estrategia a seguir para determinar cuáles elementos están ocupados/libres