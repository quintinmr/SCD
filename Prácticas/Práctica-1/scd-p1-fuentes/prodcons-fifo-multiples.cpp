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
	tam_vec   = 10 ,   // tamaño del buffer
   np        = 4,     // número de hebras productoras (divisor de num_items)
   nc        = 4,     // número de hebras consumidoras (divisor de num_items)
   p         = num_items/np, // número de items que produce cada productor
   c         = num_items/nc; // número de items que consume cada consumidor
unsigned  
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
   siguiente_dato       = 0 ;  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)

unsigned buffer[tam_vec] = {0},  // buffer
         primera_libre   = 0,    // indice en el vector de la primera celda libre (se incrementa al escribir)
         primera_ocupada = 0,    // índice en el vector de la primera celda ocupada (se incrementa al leer)
         producidos[np]  = {0};  // guarda los items ya producidos por cada hebra

Semaphore libres(tam_vec),       // semáforo que controla las posiciones libres en el buffer
          ocupadas(0),           // semáforo que controla las posiciones ocupadas en el buffer.
          modificador_P(1),
          modificador_C(1);   // poner mejor "exclusion_mutua" en vez de modificador

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato(unsigned i, unsigned j)
{

   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   unsigned dato_producido = i*p + j;
   cont_prod[dato_producido]++;    // se contabiliza las veces que dato_producido ha sido producido

   cout << "Productor " << i << "produce: " << dato_producido << endl << flush;

   producidos[i]++; //El productor i produce un dato más.
   
   return dato_producido ;

}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato)
{
   assert(dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido:" << dato << endl ;

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

void  funcion_hebra_productora( unsigned ind_hebra )
{
   
   for( unsigned i = 0 ; i < p ; i++ ){

      int dato = producir_dato(ind_hebra,i) ;
      // Sección crítica
      sem_wait(libres);

         sem_wait(modificador_P);
            buffer[primera_libre] = dato;
            primera_libre = (primera_libre + 1) % tam_vec;
            cout << "Productor: " << ind_hebra << " ha introducido el valor:  "<< dato << endl; //
         sem_signal(modificador_P);
      
      sem_signal(ocupadas);
   }
         
      
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora( unsigned ind_hebra )
{
   for( unsigned i = 0 ; i < c ; i++ )
   {
      int dato ;
      
      // Sección crítica
      sem_wait(ocupadas);
         
         assert(primera_ocupada >= 0 && primera_ocupada < tam_vec);
         sem_wait(modificador_C);
            dato = buffer[primera_ocupada];
            primera_ocupada = (primera_ocupada + 1) % tam_vec;
            cout << "Se ha retirado del buffer el valor:  " << dato << endl;
         sem_signal(modificador_C);

      sem_signal(libres);
      consumir_dato( dato ) ;
      
    }
}
//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución LIFO)." << endl
        << "------------------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora[np], hebra_consumidora[nc];
   
   for (int i = 0; i < np; i++)
   	hebra_productora[i] = thread ( funcion_hebra_productora, i );
   for (int i = 0; i < nc; i++)
   	hebra_consumidora[i] = thread ( funcion_hebra_consumidora, i );
   
   for (int i = 0; i < np; i++)
   	hebra_productora[i].join() ;
   for (int i = 0; i < nc; i++)
   	hebra_consumidora[i].join() ;

   test_contadores();
}
