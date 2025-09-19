# Laboratorio 7 - Compresión Paralela con Pthreads

**Universidad del Valle de Guatemala**  
**Facultad de Ingeniería**  
**Departamento de Ciencia de la Computación**  

**Curso:** CC3086 - Programación de Microprocesadores  

---

## Descripción

Implementación de un compresor de archivos paralelo utilizando pthreads y la librería zlib. El programa divide archivos en bloques independientes procesados simultáneamente por múltiples hilos.

### Objetivos
- Implementar paralelización con pthreads
- Aplicar mecanismos de sincronización
- Comparar rendimiento secuencial vs paralelo
- Medir speedup y eficiencia

## Características

- Compresión paralela con 1-100 hilos configurables
- Medición de tiempos de ejecución
- Menú interactivo
- Sincronización segura de hilos
- Comparación automática de rendimiento
- División en bloques de tamaño óptimo

## Instalación de Dependencias

### Ubuntu/WSL
```bash
sudo apt update
sudo apt install build-essential zlib1g-dev
```

### Fedora/CentOS
```bash
sudo dnf install gcc-c++ zlib-devel
# o para versiones más antiguas:
sudo yum install gcc-c++ zlib-devel
```

### macOS
```bash
brew install zlib
# Si usas Xcode, las herramientas ya están disponibles
```

## Compilación

```bash
# Usando Makefile (recomendado)
make

# Compilación manual
g++ -std=c++11 -pthread -o compresor src/parallel_compression.cpp -lz

# Limpiar archivos compilados
make clean
```

## Uso

### 1. Generar archivos de prueba
```bash
./generar_archivos_prueba.sh
```

Opciones disponibles:
- **Archivo pequeño** (~50KB) - Para desarrollo y pruebas rápidas
- **Archivo mediano** (~5MB) - Para pruebas de rendimiento
- **Archivo grande** (~50MB) - Para benchmarks completos

### 2. Ejecutar el compresor
```bash
./compresor
```

#### Opciones del menú:
1. **Comprimir archivo (paralelo)** - Compresión usando múltiples hilos
2. **Descomprimir archivo (paralelo)** - Descompresión paralela (en desarrollo)
3. **Comprimir archivo (secuencial)** - Compresión tradicional de un solo hilo
4. **Comparar tiempos** - Benchmarks automáticos con diferentes números de hilos
5. **Salir** - Terminar programa

### 3. Ejemplo de uso paso a paso

```bash
# 1. Compilar
make

# 2. Generar archivo de prueba
./generar_archivos_prueba.sh
# Seleccionar opción 2 (archivo mediano)

# 3. Comprimir con 4 hilos
./compresor
# Seleccionar: 1
# Archivo: tests/archivo_mediano.txt
# Salida: resultado_4hilos.bin
# Hilos: 4

# 4. Comparar con versión secuencial
./compresor
# Seleccionar: 3
# Archivo: tests/archivo_mediano.txt
# Salida: resultado_secuencial.bin
```

### 4. Pruebas automatizadas
```bash
./pruebas_completas.sh
```

Este script ejecuta automáticamente:
- Compilación del proyecto
- Generación de archivos de prueba
- Compresión con 1, 2, 4, 8, 16 hilos
- Comparación de resultados
- Limpieza de archivos temporales

## Estructura del Proyecto

```
lab7-compresion-paralela/
├── src/
│   └── parallel_compression.cpp    # Código principal
├── docs/
│   ├── reporte.md                  # Documentación técnica
│   └── reporte_lab7.pdf           # Reporte en PDF
├── tests/
│   └── paralelismo_teoria.txt     # Archivo base de prueba
├── generar_archivos_prueba.sh     # Generador de archivos de prueba
├── pruebas_completas.sh           # Script de pruebas automatizadas
├── Makefile                       # Configuración de compilación
├── .gitignore                     # Archivos excluidos de git
└── README.md                      # Esta documentación
```

## Algoritmo de Paralelización

### División en Bloques
- Tamaño máximo de bloque: 1 MB
- División proporcional según número de hilos
- Balanceamiento automático de carga

### Sincronización
- **pthread_mutex_t**: Protege escritura al archivo final
- **pthread_join**: Espera completion de todos los hilos
- **Ordenamiento**: Mantiene secuencia correcta de bloques

### Flujo de Ejecución
1. Lectura completa del archivo en memoria
2. División en bloques de tamaño calculado
3. Creación de hilos con pthread_create
4. Compresión paralela independiente por bloque
5. Sincronización y espera con pthread_join
6. Escritura secuencial ordenada del resultado

## Resultados Esperados

### Rendimiento Típico
| Hilos | Tiempo (ms) | Speedup | Eficiencia |
|-------|-------------|---------|------------|
| 1     | 1000        | 1.00x   | 100%       |
| 2     | 520         | 1.92x   | 96%        |
| 4     | 280         | 3.57x   | 89%        |
| 8     | 160         | 6.25x   | 78%        |
| 16    | 140         | 7.14x   | 45%        |

### Observaciones
- **Punto óptimo**: Usualmente entre 4-8 hilos
- **Speedup**: Mejora hasta saturación del hardware
- **Overhead**: Degradación con exceso de hilos debido a sincronización

## Troubleshooting

### Error de compilación
```bash
# Verificar herramientas instaladas
gcc --version
pkg-config --libs zlib

# Verificar que zlib está disponible
find /usr -name "libz.so*" 2>/dev/null

# Compilar con información de debug
g++ -std=c++11 -pthread -o compresor src/parallel_compression.cpp -lz -v
```

### Problema: No se ven mejoras de rendimiento
**Causa**: Archivo demasiado pequeño para beneficiarse del paralelismo
**Solución**:
```bash
./generar_archivos_prueba.sh
# Seleccionar opción 3 (archivo grande)
```

### Problema: Programa se cuelga
**Causa**: Deadlock o condición de carrera
**Solución**:
```bash
# Matar procesos colgados
pkill compresor

# Ejecutar con menos hilos
./compresor
# Usar 2-4 hilos inicialmente
```

### Problema: Archivos corruptos
**Causa**: Error en sincronización de bloques
**Verificación**:
```bash
# Comparar tamaños
ls -lh archivo_original.txt resultado_descomprimido.txt

# Comparar contenido
diff archivo_original.txt resultado_descomprimido.txt

# Verificar checksums
md5sum archivo_original.txt resultado_descomprimido.txt
```

## Verificación de Integridad

### Comparación de archivos
```bash
# Diferencias binarias
diff archivo_original.txt archivo_descomprimido.txt

# Checksums MD5
md5sum archivo_original.txt archivo_descomprimido.txt

# Checksums SHA256 (más seguro)
sha256sum archivo_original.txt archivo_descomprimido.txt
```

### Validación automática
El programa incluye verificaciones internas:
- Verificación de tamaños antes/después
- Validación de códigos de retorno de zlib
- Verificación de orden de bloques
- Cálculo de ratios de compresión

## Análisis de Rendimiento

### Métricas importantes
- **Tiempo de ejecución**: Medido con chrono::high_resolution_clock
- **Speedup**: T_secuencial / T_paralelo
- **Eficiencia**: Speedup / Número_de_hilos
- **Throughput**: Bytes_procesados / Tiempo_total

### Factores que afectan el rendimiento
- Tamaño del archivo de entrada
- Número de núcleos del procesador
- Velocidad de memoria RAM
- Tipo de contenido (compresibilidad)
- Overhead de sincronización

## Entregables

### Código fuente
- `src/parallel_compression.cpp` - Implementación completa
- `Makefile` - Configuración de compilación
- Scripts de prueba y generación de datos

### Documentación
- `docs/reporte.md` - Análisis técnico detallado
- `docs/reporte_lab7.pdf` - Reporte final en PDF
- Este README.md con instrucciones completas

### Video demostración
Debe incluir:
- Compilación y ejecución del programa
- Demostración de compresión con diferentes números de hilos
- Comparación de tiempos de ejecución
- Explicación de mecanismos de sincronización
- Verificación de integridad de archivos

## Autor

**Nombre**: Fatima Navarro Cortez
**Carnet**: 24044
**Universidad**: Universidad del Valle de Guatemala  
**Curso**: CC3086 - Programación de Microprocesadores  

---

## Licencia

Este proyecto es desarrollado con fines educativos para el curso CC3086 de la Universidad del Valle de Guatemala.
