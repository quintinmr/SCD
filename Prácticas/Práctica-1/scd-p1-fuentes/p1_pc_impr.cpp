#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;

//**********************************************************************
// Variables globales

const unsigned 
   num_items = 40 ,   // número de items
	tam_vec   = 10 ;   // tamaño del buffer
unsigned  
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
   siguiente_dato       = 0 ,  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)
   num_ocupadas         = 0;   // número de celdas ocupadas en el buffer no consumidas (debe actualizarse por productor y consumidr)

unsigned buffer[tam_vec] = {0},  // buffer
         primera_libre   = 0,    // indice en el vector de la primera celda libre (se incrementa al escribir)
         primera_ocupada = 0;    // índice en el vector de la primera celda ocupada (se incrementa al leer)
Semaphore libres(tam_vec),       // semáforo que controla las posiciones libres en el buffer
          ocupadas(0),           // semáforo que controla las posiciones ocupadas en el buffer.
          impresora(0),          // semáforo que controla el acceso a la impresora (inicialmente impresora apagada)
          productora(0),         // semáforo para controlar la actividad de la hebra productora
          seccion_critica(1);    // semáforo para el control de la sección crítica (cuando se maneja la variable de cuentas)

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato()
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   const unsigned dato_producido = siguiente_dato ;
   siguiente_dato++ ;
   cont_prod[dato_producido] ++ ;
   cout << "producido: " << dato_producido << endl << flush ;
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: " << dato << endl ;

}


//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void funcion_hebra_impresora()
{
   while (true)
   {
      sem_wait(impresora);
         cout << "Número de celdas ocupadas en el búffer: " << num_ocupadas << endl;
      sem_signal(productora);
   }
}

//----------------------------------------------------------------------

void  funcion_hebra_productora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int dato = producir_dato() ;
      sem_wait(libres);
      
            assert(0 <= primera_libre && primera_libre < tam_vec);
            buffer[primera_libre] = dato;
            cout << "Se ha introducido el valor:  "<< buffer[primera_libre] << endl;
            num_ocupadas++;
            primera_libre = (primera_libre+1)%tam_vec;
            if (dato%5 == 0){
               sem_signal(impresora);
               sem_wait(productora);
            }
            
        
      sem_signal(ocupadas);
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int dato ;
      sem_wait(ocupadas);
       
            assert(0 <= primera_ocupada && primera_ocupada < tam_vec);
            dato = buffer[primera_ocupada];
            cout << "Leido del buffer el numero "<< dato << endl;
            num_ocupadas--;
            primera_ocupada = (primera_ocupada+1)%tam_vec;
            if(primera_ocupada >= tam_vec)
               primera_ocupada = 0; 
     
      sem_signal(libres);

      consumir_dato( dato ) ;
    }
}

//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores FIFO con impresora." << endl
        << "------------------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora ( funcion_hebra_productora ),
          hebra_consumidora( funcion_hebra_consumidora ),
          hebra_impresora( funcion_hebra_impresora );

   hebra_productora.join() ;
   hebra_consumidora.join() ;
   hebra_impresora.join();

   test_contadores();
}
