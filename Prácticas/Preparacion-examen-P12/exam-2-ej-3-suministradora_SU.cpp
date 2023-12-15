#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

// numero de fumadores 

const int num_fumadores = 3 ;

Semaphore exclusion_mutua(1);

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   sem_wait(exclusion_mutua);
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
   sem_signal(exclusion_mutua);

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   sem_wait(exclusion_mutua);
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;
   sem_signal(exclusion_mutua);

   return num_ingrediente ;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Monitor Buffer
//----------------------------------------------------------------------
//----------------------------------------------------------------------
class Buffer : public HoareMonitor
{
    private:

        // Declaramos variables permanentes
        static const unsigned num_celdas_total = 5;  //número de celdas del buffer
        
        unsigned buffer[num_celdas_total],           //buffer donde la suministradora irá
                                                     //almacenando ingredientes
                 primera_libre,                      //índice de celdas libres
                 primera_ocupada,                    //índice de celdas ocupadas
                 num_ocupadas;                       //número de celdas ocupadas

        CondVar
            buff_libres,                             // cola donde espera la suministradora
            buff_ocupadas;                           // cola donde espera el estanquero

    public:

        Buffer();
        unsigned leer ();                           //el estanquero lee el valor del buffer
        void escribir(unsigned valor);              //la suministradora escribe el ingr. en el buffer

};

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Implementación procedimientos del monitor
//----------------------------------------------------------------------
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------
Buffer::Buffer()
{
    primera_libre   = 0;
    primera_ocupada = 0;
    num_ocupadas    = 0;
    buff_libres     = newCondVar();
    buff_ocupadas   = newCondVar();
}

//----------------------------------------------------------------------
// Escribir en el buffer el ingrediente correspondiente
//----------------------------------------------------------------------
void Buffer::escribir(unsigned valor)
{
    if (num_ocupadas == num_celdas_total)
        buff_libres.wait();

    assert(0<=valor<=2);

    buffer[primera_libre] = valor;
    primera_libre = (primera_libre+1) % num_celdas_total;
    num_ocupadas++;

    buff_ocupadas.signal();
}

//----------------------------------------------------------------------
// Leer del buffer el ingrediente correspondiente
//----------------------------------------------------------------------
unsigned Buffer::leer()
{
    if (num_ocupadas == 0)
        buff_ocupadas.wait();

    unsigned valor = buffer[primera_ocupada];
    primera_ocupada = (primera_ocupada+1) % num_celdas_total;
    num_ocupadas--;
    
    buff_libres.signal();

    return valor;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Monitor Estanco
//----------------------------------------------------------------------
//----------------------------------------------------------------------

class Estanco : public HoareMonitor
{

   private:

      // Declaramos variables permanentes
      int num_ingrediente;		     // (-1 cuando no hay ingrediente en el mostrador, 0,1,2 en caso contrario)

      CondVar                                // colas condición
         mostrador,  			     // cola en la que espera el estanquero a que el mostrador se vacíe.                         
         ingr_disp[num_fumadores];	     // colas en las que cada fumador espera a que su ingrediente esté disponible en el mostrador.

      // Procedimientos
      public:

         Estanco();
         void ponerIngrediente(unsigned i);
         void esperarRecogidaIncrediente();
         void obtenerIngrediente(unsigned i);

};

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Implementación procedimientos del monitor
//----------------------------------------------------------------------
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------

Estanco::Estanco()
{
   num_ingrediente = -1;
   mostrador       = newCondVar();
   for (unsigned i = 0; i < num_fumadores; i++)
      ingr_disp[i] = newCondVar();

}

//----------------------------------------------------------------------
// Obtener ingrediente i del mostrador
//----------------------------------------------------------------------

void Estanco::obtenerIngrediente(unsigned i)
{
   assert( i < num_fumadores);
   
   // Si el ingrediente no es del fumador, espera
   if (num_ingrediente != i) ingr_disp[i].wait();

   cout << "Se ha retirado del mostrador el ingrediente nº: "<< i << endl;
   // Lo retira del mostrador
   num_ingrediente = -1;
   mostrador.signal();

}

//----------------------------------------------------------------------
// Poner ingrediente en el mostrador
//----------------------------------------------------------------------

void Estanco::ponerIngrediente(unsigned i)
{
   
   // El número de ingrediete pasa a ser i
   num_ingrediente = i;
   cout << "Ingediente nº "<< i << " puesto en el mostrador." << endl;
   ingr_disp[i].signal();
   
}

//----------------------------------------------------------------------
// Esperar recogida del ingrediente
//----------------------------------------------------------------------

void Estanco::esperarRecogidaIncrediente()
{
   
   // Si hay un ingrediente encima del mostrador, el estanquero espera a que se recoja
   if(num_ingrediente != -1){
      cout << "Estanquero espera a que se recoja el ingrediente " << num_ingrediente << endl;
      mostrador.wait();
   }
   
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del suministradora

void funcion_hebra_suministradora( MRef<Buffer> monitor )
{
   while (true)
   {
      unsigned i = producir_ingrediente();
      monitor->escribir(i);
   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( MRef<Estanco> monitor1, MRef<Buffer> monitor2 )
{
   while (true)
   {
      unsigned ingr = monitor2->leer();
      monitor1->ponerIngrediente(ingr);
      monitor1->esperarRecogidaIncrediente();
   }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
    sem_wait(exclusion_mutua);
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    sem_signal(exclusion_mutua);

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
    sem_wait(exclusion_mutua);
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
    sem_signal(exclusion_mutua);
    
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( MRef<Estanco> monitor, unsigned i )
{
   while( true )
   {
      monitor->obtenerIngrediente(i);
      fumar(i);

   }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   cout << "____________________________________________________________________" << endl;
   cout << endl;
   cout << "	  PROBLEMA DE LOS FUMADORES CON MONITORES                       " << endl;
   cout << "____________________________________________________________________" << endl << flush;
   
   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<Estanco> monitor1 = Create<Estanco>() ;
   MRef<Buffer> monitor2 = Create<Buffer>() ;
   // declarar hebras y ponerlas en marcha
   
   thread hebras_fumadoras[num_fumadores];
   thread hebra_estanquero(funcion_hebra_estanquero, monitor1,monitor2);
   thread hebra_suministradora(funcion_hebra_suministradora, monitor2);
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k] = thread (funcion_hebra_fumador,monitor1, k);
   	
   hebra_estanquero.join();
   hebra_suministradora.join();
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k].join();
   
}
