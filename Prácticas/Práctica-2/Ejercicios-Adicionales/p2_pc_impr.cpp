// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// Archivo: prodcons1_su.cpp
//
// Ejemplo de un monitor en C++11 con semántica SU, para el problema
// del productor/consumidor, con productor y consumidor únicos.
// Opcion LIFO
//
// Historial:
// Creado el 30 Sept de 2022. (adaptado de prodcons2_su.cpp)
// 20 oct 22 --> paso este archivo de FIFO a LIFO, para que se corresponda con lo que dicen las transparencias
// -----------------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std ;
using namespace scd ;

constexpr int
   num_items = 40 ,                    // número de items a producir/consumir
   np        = 8,                      // número de hebras productoras
   nc        = 5,                      // número de hebras consumidoras
   p         = num_items/np,           // número de elementos que produce un productor
   c         = num_items/nc;           // número de elementos que consume un consumidor   
   
constexpr int               
   min_ms    = 5,     // tiempo minimo de espera en sleep_for
   max_ms    = 20 ;   // tiempo máximo de espera en sleep_for


mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: producidos
   cont_cons[num_items] = {0}, // contadores de verificación: consumidos
   cont_items_prod[np]  = {0}; // contador del número de elementos producidos por cada productor

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato( unsigned i)
{
   assert(i <= np-1);

   this_thread::sleep_for( chrono::milliseconds( aleatorio<min_ms,max_ms>() ));
   const int valor_producido = i*p + cont_items_prod[i];
   mtx.lock();
   cout << "hebra productora " << i << ", produce " << valor_producido << endl << flush ;
   mtx.unlock();
   cont_prod[valor_producido]++ ;
   cont_items_prod[i]++; //El productor i produce un dato más.
   return valor_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned valor_consumir )
{
   if ( num_items <= valor_consumir )
   {
      cout << " valor a consumir === " << valor_consumir << ", num_items == " << num_items << endl ;
      assert( valor_consumir < num_items );
   }
   cont_cons[valor_consumir] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<min_ms,max_ms>() ));
   mtx.lock();
   cout << "                  hebra consumidora, consume: " << valor_consumir << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << endl ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, multiples prod/cons

class ProdConsSU1 : public HoareMonitor
{
 private:
 static const int           // constantes ('static' ya que no dependen de la instancia)
   num_celdas_total = 10;   //   núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//   buffer de tamaño fijo, con los datos
   primera_libre ,          //   indice de celda de la próxima inserción
   primera_ocupada,         //   indice de celda de la próxima lectua
   num_ocupadas,            //   número de celdas ocupadas
   cont_mult_cinco,         //   número de múltiplos de 5 producidos
   mult_cinco_prev;         //   número de múltiplos de 5 insertados en la llamada anterior

 CondVar                    // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ,                 //  cola donde espera el productor  (n<num_celdas_total)
   impresora;               //  cola donde espera la impresora

 public:                    // constructor y métodos públicos
   ProdConsSU1() ;             // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
   bool metodo_impresora();    // 
} ;
// -----------------------------------------------------------------------------

ProdConsSU1::ProdConsSU1(  )
{
   primera_libre   = 0 ;
   primera_ocupada = 0;
   num_ocupadas    = 0;
   cont_mult_cinco = 0;
   mult_cinco_prev = 0;
   ocupadas        = newCondVar();
   libres          = newCondVar();
   impresora       = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsSU1::leer(  )
{
   // esperar bloqueado hasta que 0 < num_ocupadas
   if ( num_ocupadas == 0 )
      ocupadas.wait();

   assert( num_ocupadas > 0);

   // hacer la operación de lectura, actualizando estado del monitor
   const int valor = buffer[primera_ocupada] ;
   primera_ocupada = (primera_ocupada +1) % num_celdas_total;
   num_ocupadas--;
   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsSU1::escribir( int valor )
{
   // esperar bloqueado hasta que num_ocupadas < num_celdas_total
   if ( num_ocupadas == num_celdas_total )
      libres.wait();

   assert( num_ocupadas < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre = (primera_libre + 1) % num_celdas_total;
   num_ocupadas++;

   if (valor % 5 == 0)
   {
      cont_mult_cinco++;
      impresora.signal();
   }

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
bool ProdConsSU1::metodo_impresora()
{

   if (cont_mult_cinco == num_items/5)
      return false;
   
   if ((cont_mult_cinco-mult_cinco_prev) == 0) impresora.wait();

   cout << "Se han insertado en el buffer un total de " << cont_mult_cinco-mult_cinco_prev << " nuevos múltiplos de 5." << endl;
   
   mult_cinco_prev = cont_mult_cinco;

   return true;

}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdConsSU1> monitor, unsigned num_hebra )
{
   for( unsigned i = 0 ; i < p ; i++ )
   {
      int valor = producir_dato( num_hebra) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdConsSU1>  monitor )
{
   for( unsigned i = 0 ; i < c ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------
void funcion_hebra_impresora(MRef<ProdConsSU1> monitor)
{
   do
   {
   
      this_thread::sleep_for( chrono::milliseconds( aleatorio<10,50>() ));
      
   }while (monitor->metodo_impresora());

}
// -----------------------------------------------------------------------------

int main()
{
   cout << "---------------------------------------------------------------------------------------" << endl
        << "Problema del productor-consumidor múltiples (Monitor SU, buffer FIFO) con impresora.   " << endl
        << "---------------------------------------------------------------------------------------" << endl
        << flush ;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<ProdConsSU1> monitor = Create<ProdConsSU1>() ;

   // crear y lanzar las hebras
   thread hebra_productora[np], hebra_consumidora[nc];

   thread hebra_impresora(funcion_hebra_impresora,monitor);
   
   for (int i = 0; i < np; i++)
   	hebra_productora[i] = thread ( funcion_hebra_productora, monitor, i );
   for (int i = 0; i < nc; i++)
   	hebra_consumidora[i] = thread ( funcion_hebra_consumidora, monitor);

   // esperar a que terminen las hebras
   for (int i = 0; i < np; i++)
   	hebra_productora[i].join() ;
   for (int i = 0; i < nc; i++)
   	hebra_consumidora[i].join() ;

   hebra_impresora.join();

   test_contadores() ;
}
