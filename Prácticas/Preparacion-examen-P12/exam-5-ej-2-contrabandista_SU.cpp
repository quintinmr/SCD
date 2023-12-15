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
mutex mutx;

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   mutx.lock();
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
   mutx.unlock();

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   mutx.lock();
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;
   mutx.unlock();

   return num_ingrediente ;
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
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( MRef<Estanco> monitor )
{
   while (true)
   {
      unsigned i = producir_ingrediente();
      monitor->ponerIngrediente(i);
      monitor->esperarRecogidaIncrediente();
   }
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Monitor Buffer
//----------------------------------------------------------------------
//----------------------------------------------------------------------

class Buffer : public HoareMonitor
{

    private:

        // Declaración de variables permanentes
        unsigned buffer[num_fumadores],             //array de 3 elementos donde se insertarán los cigarrillos
                 cont_inserciones,                  //contador del número de inserciones que se hacen en el buzón
                 primera_libre,                     //variable para gestión de ocupación del buffer (casillas libres)
                 primera_ocupada;                   //variable para gestión de ocupación del buffer (casillas ocupadas)

        CondVar 
                 cola_buzon_lleno,                  //cola en la que esperan los fumadores que se encuentran el buzón lleno
                 cola_buzon_vacio;                  //cola en la que espera el contrabandista al encontrarse el buzón vacío

    public:

        // Declaración de procedimientos del monitor
        Buffer();                                   // constructor
        unsigned extraer();                         // extraer sobre del buzón
        void insertar(unsigned num_fumador);                            // insertar sobre en el buzón

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
    cont_inserciones = 0;
    primera_libre    = 0;
    primera_ocupada  = 0;
    cola_buzon_lleno = newCondVar();
    cola_buzon_vacio = newCondVar();
}

//----------------------------------------------------------------------
// Función para la inserción de un sobre en el buffer
//----------------------------------------------------------------------
void Buffer::insertar(unsigned num_fumador)
{
    if (cont_inserciones == 3)   // si el buzón está lleno
        cola_buzon_lleno.wait(); // fumador espera a que se vacíe un poco el buzón

    // inserción del sobre y contabilización de inserciones
    buffer[primera_libre] = num_fumador;
    primera_libre = (primera_libre+1) % num_fumadores;
    cont_inserciones++;
    
    // se despierta al contrabandista por si estaba esperando a que hubiera sobres
    cola_buzon_vacio.signal();

}

//----------------------------------------------------------------------
// Función para la extracción de un sobre del buffer
//----------------------------------------------------------------------
unsigned Buffer::extraer()
{
    if (cont_inserciones == 0)   // si el buzón está lleno
        cola_buzon_vacio.wait(); // esperar a que se llene un poco el buzón

    // extracción del sobre y reducción de inserciones
    unsigned sobre = buffer[primera_ocupada];
    primera_ocupada = (primera_ocupada+1) % num_fumadores;
    cont_inserciones--;
    
    // se despierta al fumador por si estaba esperando a que se vaciara un poco el buzón
    cola_buzon_lleno.signal();

    return sobre;

}
//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
    mutx.lock();
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    mutx.unlock();

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
    mutx.lock();
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
    mutx.unlock();

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( MRef<Estanco> monitor1,MRef<Buffer> monitor2, unsigned i )
{
   while( true )
   {
      monitor1->obtenerIngrediente(i);
      this_thread::sleep_for( chrono::milliseconds( aleatorio<20,150>() ));

      monitor2->insertar(i);

      mutx.lock();
      cout << "Fumador " << i << " inserta sobre en el buzón." << endl;
      mutx.unlock();

   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_contrabandista ( MRef<Buffer> monitor)
{
    unsigned cont_extracciones = 0,
             cont_cigarrillos[num_fumadores] = {0};
    
   
   while( true )
   {
      this_thread::sleep_for( chrono::milliseconds( aleatorio<20,150>() ));

      mutx.lock();
      cout << "Contrabandista intenta sacar sobre del buzón." << endl;
      mutx.unlock();
      
      unsigned sobre = monitor->extraer();

      mutx.lock();
      cout << "Contrabandista saca un sobre del buzón. Concretamente, el del fumador " << sobre << endl;
      cont_extracciones++;
      cont_cigarrillos[sobre]++;
      mutx.unlock();

      if (cont_extracciones % 4 == 0)
      {
           for (unsigned i = 0; i < num_fumadores; i++)
           {
            mutx.lock();
            cout << "*********FUMADOR " << i << " ha enviado " << cont_cigarrillos[i] << " cigarrillos*********" << endl;
            mutx.unlock();
           }
      }
      

   }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   cout << "____________________________________________________________________" << endl;
   cout << endl;
   cout << "			PROBLEMA DE LOS FUMADORES-CONTRABANDISTA                      " << endl;
   cout << "____________________________________________________________________" << endl << flush;
   

    MRef<Estanco> monitor1 = Create<Estanco>();
    MRef<Buffer> monitor2 = Create<Buffer>();
   // declarar hebras y ponerlas en marcha
   
   thread hebras_fumadoras[num_fumadores];
   thread hebra_estanquero(funcion_hebra_estanquero,monitor1);
   thread hebra_contrabandista(funcion_hebra_contrabandista,monitor2);
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k] = thread (funcion_hebra_fumador,monitor1,monitor2,k);
   	
   hebra_estanquero.join();

   hebra_contrabandista.join();
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k].join();
   
}