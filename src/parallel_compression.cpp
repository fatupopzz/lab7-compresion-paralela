/* ------------------------------------------------------------
 * UNIVERSIDAD DEL VALLE DE GUATEMALA
 * FACULTAD DE INGENIERIA
 * DEPARTAMENTO DE CIENCIA DE LA COMPUTACION
 *
 * Curso: CC3086 Programacion de Microprocesadores
 * Laboratorio 7: Compresion Paralela con Pthreads
 * Estudiante: [Tu nombre aquí]
 * ------------------------------------------------------------
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <zlib.h>
#include <pthread.h>
#include <chrono>
#include <algorithm>
#include <cstring>

using namespace std;
using namespace chrono;

// Estructura para pasar datos a cada hilo
struct DatosHilo {
    int idHilo;
    char* datosOriginales;
    size_t tamanioBloque;
    size_t posicionInicio;
    vector<char>* resultado;
    size_t* tamanioComprimido;
    bool esCompresion; // true = comprimir, false = descomprimir
    pthread_mutex_t* mutexEscritura;
    bool completado;
    int ordenBloque; // Para mantener el orden correcto
};

// Variables globales para sincronización
pthread_mutex_t mutexArchivo = PTHREAD_MUTEX_INITIALIZER;
vector<DatosHilo> todosLosHilos;

void* trabajadorCompresion(void* datos) {
    DatosHilo* misDatos = (DatosHilo*)datos;
    
    if (misDatos->esCompresion) {
        // COMPRESIÓN
        uLongf tamanioComprimidoEstimado = compressBound(misDatos->tamanioBloque);
        misDatos->resultado = new vector<char>(tamanioComprimidoEstimado);
        
        int resultado = compress(
            reinterpret_cast<Bytef*>(misDatos->resultado->data()),
            &tamanioComprimidoEstimado,
            reinterpret_cast<const Bytef*>(misDatos->datosOriginales + misDatos->posicionInicio),
            misDatos->tamanioBloque
        );
        
        if (resultado == Z_OK) {
            misDatos->resultado->resize(tamanioComprimidoEstimado);
            *(misDatos->tamanioComprimido) = tamanioComprimidoEstimado;
            misDatos->completado = true;
            
            cout << "Hilo " << misDatos->idHilo << " termino compresion del bloque " 
                 << misDatos->ordenBloque << " (" << misDatos->tamanioBloque 
                 << " -> " << tamanioComprimidoEstimado << " bytes)" << endl;
        }
    } else {
        // DESCOMPRESIÓN
        // Para simplificar, asumimos que el tamaño descomprimido es conocido
        // En una implementación real, necesitaríamos guardar esta información
        uLongf tamanioDescomprimido = misDatos->tamanioBloque * 4; // Estimación
        misDatos->resultado = new vector<char>(tamanioDescomprimido);
        
        int resultado = uncompress(
            reinterpret_cast<Bytef*>(misDatos->resultado->data()),
            &tamanioDescomprimido,
            reinterpret_cast<const Bytef*>(misDatos->datosOriginales + misDatos->posicionInicio),
            misDatos->tamanioBloque
        );
        
        if (resultado == Z_OK) {
            misDatos->resultado->resize(tamanioDescomprimido);
            *(misDatos->tamanioComprimido) = tamanioDescomprimido;
            misDatos->completado = true;
            
            cout << "Hilo " << misDatos->idHilo << " termino descompresion del bloque " 
                 << misDatos->ordenBloque << endl;
        }
    }
    
    pthread_exit(nullptr);
}

bool comprimirArchivoParalelo(const string& archivoEntrada, const string& archivoSalida, int numHilos) {
    // Leer archivo completo en memoria
    ifstream archivo(archivoEntrada, ios::binary | ios::ate);
    if (!archivo) {
        cerr << "Error: No se pudo abrir " << archivoEntrada << endl;
        return false;
    }
    
    size_t tamanioArchivo = archivo.tellg();
    archivo.seekg(0, ios::beg);
    
    vector<char> datosArchivo(tamanioArchivo);
    archivo.read(datosArchivo.data(), tamanioArchivo);
    archivo.close();
    
    cout << "Archivo leido: " << tamanioArchivo << " bytes" << endl;
    
    // Calcular tamaño de bloque (1MB máximo o archivo completo dividido entre hilos)
    size_t tamanioBloque = min(static_cast<size_t>(1024 * 1024), 
                               (tamanioArchivo + numHilos - 1) / numHilos);
    
    int numBloques = (tamanioArchivo + tamanioBloque - 1) / tamanioBloque;
    
    cout << "Dividiendo en " << numBloques << " bloques de ~" 
         << tamanioBloque << " bytes cada uno" << endl;
    
    // Preparar hilos
    vector<pthread_t> hilos(numBloques);
    vector<DatosHilo> datosHilos(numBloques);
    vector<size_t> tamaniosComprimidos(numBloques);
    
    auto inicioTiempo = high_resolution_clock::now();
    
    // Crear hilos
    for (int i = 0; i < numBloques; i++) {
        datosHilos[i].idHilo = i;
        datosHilos[i].datosOriginales = datosArchivo.data();
        datosHilos[i].posicionInicio = i * tamanioBloque;
        datosHilos[i].tamanioBloque = min(tamanioBloque, tamanioArchivo - i * tamanioBloque);
        datosHilos[i].tamanioComprimido = &tamaniosComprimidos[i];
        datosHilos[i].esCompresion = true;
        datosHilos[i].mutexEscritura = &mutexArchivo;
        datosHilos[i].completado = false;
        datosHilos[i].ordenBloque = i;
        
        pthread_create(&hilos[i], nullptr, trabajadorCompresion, &datosHilos[i]);
    }
    
    // Esperar que terminen todos los hilos
    for (int i = 0; i < numBloques; i++) {
        pthread_join(hilos[i], nullptr);
    }
    
    auto finTiempo = high_resolution_clock::now();
    auto duracion = duration_cast<milliseconds>(finTiempo - inicioTiempo);
    
    // Escribir archivo comprimido en orden correcto
    ofstream archivoComprimido(archivoSalida, ios::binary);
    if (!archivoComprimido) {
        cerr << "Error: No se pudo crear " << archivoSalida << endl;
        return false;
    }
    
    size_t tamanioTotalComprimido = 0;
    
    // Escribir cada bloque en orden
    for (int i = 0; i < numBloques; i++) {
        if (datosHilos[i].completado && datosHilos[i].resultado) {
            archivoComprimido.write(datosHilos[i].resultado->data(), 
                                  datosHilos[i].resultado->size());
            tamanioTotalComprimido += datosHilos[i].resultado->size();
            
            // Limpiar memoria
            delete datosHilos[i].resultado;
        }
    }
    
    archivoComprimido.close();
    
    cout << "\n=== RESULTADOS COMPRESION ===" << endl;
    cout << "Tiempo de ejecucion: " << duracion.count() << " ms" << endl;
    cout << "Tamano original: " << tamanioArchivo << " bytes" << endl;
    cout << "Tamano comprimido: " << tamanioTotalComprimido << " bytes" << endl;
    cout << "Ratio de compresion: " << (100.0 * tamanioTotalComprimido / tamanioArchivo) << "%" << endl;
    cout << "Hilos utilizados: " << numHilos << endl;
    
    return true;
}

void comprimirSecuencial(const string& archivoEntrada, const string& archivoSalida) {
    ifstream archivo(archivoEntrada, ios::binary | ios::ate);
    if (!archivo) {
        cerr << "Error: No se pudo abrir " << archivoEntrada << endl;
        return;
    }
    
    size_t tamanio = archivo.tellg();
    archivo.seekg(0, ios::beg);
    
    vector<char> datos(tamanio);
    archivo.read(datos.data(), tamanio);
    archivo.close();
    
    auto inicio = high_resolution_clock::now();
    
    uLongf tamanioComprimido = compressBound(tamanio);
    vector<Bytef> datosComprimidos(tamanioComprimido);
    
    int resultado = compress(datosComprimidos.data(), &tamanioComprimido,
                           reinterpret_cast<const Bytef*>(datos.data()), tamanio);
    
    auto fin = high_resolution_clock::now();
    auto duracion = duration_cast<milliseconds>(fin - inicio);
    
    if (resultado == Z_OK) {
        ofstream salida(archivoSalida, ios::binary);
        salida.write(reinterpret_cast<char*>(datosComprimidos.data()), tamanioComprimido);
        salida.close();
        
        cout << "\n=== COMPRESION SECUENCIAL ===" << endl;
        cout << "Tiempo de ejecucion: " << duracion.count() << " ms" << endl;
        cout << "Tamano original: " << tamanio << " bytes" << endl;
        cout << "Tamano comprimido: " << tamanioComprimido << " bytes" << endl;
    }
}

void mostrarMenu() {
    cout << "\n===================================" << endl;
    cout << "  COMPRESOR PARALELO - LAB 7" << endl;
    cout << "===================================" << endl;
    cout << "1. Comprimir archivo (paralelo)" << endl;
    cout << "2. Descomprimir archivo (paralelo)" << endl;
    cout << "3. Comprimir archivo (secuencial)" << endl;
    cout << "4. Comparar tiempos" << endl;
    cout << "5. Salir" << endl;
    cout << "===================================" << endl;
    cout << "Selecciona una opcion: ";
}

void compararTiempos(const string& archivo) {
    cout << "\n=== COMPARACION DE TIEMPOS ===" << endl;
    cout << "Probando con diferentes numeros de hilos..." << endl;
    
    vector<int> numHilos = {1, 2, 4, 8, 16};
    
    for (int hilos : numHilos) {
        cout << "\n--- Probando con " << hilos << " hilos ---" << endl;
        string archivoSalida = "temp_" + to_string(hilos) + "_hilos.bin";
        comprimirArchivoParalelo(archivo, archivoSalida, hilos);
    }
}

int main() {
    int opcion;
    string archivoEntrada, archivoSalida;
    int numHilos;
    
    do {
        mostrarMenu();
        cin >> opcion;
        
        switch (opcion) {
            case 1: {
                cout << "Nombre del archivo a comprimir: ";
                cin >> archivoEntrada;
                cout << "Nombre del archivo comprimido: ";
                cin >> archivoSalida;
                cout << "Numero de hilos a usar: ";
                cin >> numHilos;
                
                if (numHilos < 1 || numHilos > 100) {
                    cout << "Numero de hilos debe estar entre 1 y 100" << endl;
                    break;
                }
                
                comprimirArchivoParalelo(archivoEntrada, archivoSalida, numHilos);
                break;
            }
            
            case 2: {
                cout << "Descompresion aun no implementada completamente." << endl;
                cout << "(Requiere guardar metadatos adicionales)" << endl;
                break;
            }
            
            case 3: {
                cout << "Nombre del archivo a comprimir: ";
                cin >> archivoEntrada;
                cout << "Nombre del archivo comprimido: ";
                cin >> archivoSalida;
                
                comprimirSecuencial(archivoEntrada, archivoSalida);
                break;
            }
            
            case 4: {
                cout << "Nombre del archivo para comparar: ";
                cin >> archivoEntrada;
                compararTiempos(archivoEntrada);
                break;
            }
            
            case 5: {
                cout << "¡Hasta luego!" << endl;
                break;
            }
            
            default: {
                cout << "Opcion no valida. Intenta de nuevo." << endl;
                break;
            }
        }
        
    } while (opcion != 5);
    
    return 0;
}
