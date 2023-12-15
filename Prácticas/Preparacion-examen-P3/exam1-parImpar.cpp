/*
   -Nombre: Quintín
   -Apellidos: Mesa Romero
   -DNI: 78006011Q
*/
// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
// 
// Variación del problema de múltiples productores y consumidores en la que
// tenemos 1 productor y 5 consumidores.
// Los consumidores pares solo pueden consumir los valores pares.
// Los consumidores impares solo pueden consumir los valores impares.
//
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_items             = 10,
   np                    = 1,              // número de procesos productores
   nc                    = 5,              // número de procesos consumidores
   k                     = num_items/np,   // número de items que produce cada productor
   c                     = num_items/nc,   // número de items que consume cada consumidor              
   etiq_prod             = 1 ,
   id_buffer             = np ,
   etiq_cons             = 0 ,
   num_procesos_esperado = np+nc+1 ,
   tam_vector            = 10;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}
// ---------------------------------------------------------------------
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir()
{
   static int contador = 0 ;
   sleep_for( milliseconds( aleatorio<10,200>()) );
   contador++ ;
   cout << "Productor ha producido valor " << contador << endl << flush;
   return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor(unsigned orden)
{
   for ( unsigned int i= 0 ; i < k ; i++ )
   {
      // producir valor
      int valor_prod = producir();
      // enviar valor
      cout << "Productor " << orden << " va a enviar valor " << valor_prod << endl << flush;
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiq_prod, MPI_COMM_WORLD );
   }
}
// ---------------------------------------------------------------------

void consumir( int valor_cons, unsigned orden )
{
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "Consumidor " << orden<< " ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor(unsigned orden)
{
   int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < c; i++ )
   {
      MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD,&estado );
      cout << "Consumidor "<< orden <<" ha recibido valor " << valor_rec << endl << flush ;
      consumir( valor_rec, orden );
   }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
   int        buffer[tam_vector],      // buffer con celdas ocupadas y vacías
              valor,                   // valor recibido o enviado
              primera_libre       = 0, // índice de primera celda libre
              primera_ocupada     = 0, // índice de primera celda ocupada
              num_celdas_ocupadas = 0, // número de celdas ocupadas
              id_emisor_aceptable = MPI_ANY_SOURCE,    // identificador de emisor aceptable
              etiqueta_aceptable ;     // etiqueta de emisor aceptable
   MPI_Status estado ;                 // metadatos del mensaje recibido

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
            
      // 1. determinar si puede enviar solo prod., solo cons, o todos

      if ( num_celdas_ocupadas == 0 )               // si buffer vacío
      {
         int flag;
         MPI_Iprobe(id_emisor_aceptable, etiq_prod, MPI_COMM_WORLD, &flag, &estado);

         if (flag)
         {
            MPI_Recv( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_prod, MPI_COMM_WORLD, &estado );
            buffer[primera_libre] = valor ;
            primera_libre = (primera_libre+1) % tam_vector ;
            num_celdas_ocupadas++ ;
            cout << "Buffer ha recibido valor " << valor << endl ;
         }

      }
      else if ( num_celdas_ocupadas == tam_vector) // si buffer lleno 
      {
         int flag;
         MPI_Iprobe(id_emisor_aceptable, etiq_cons, MPI_COMM_WORLD, &flag, &estado);

         if (flag)
         {
            MPI_Recv( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_cons, MPI_COMM_WORLD, &estado );
            if (buffer[primera_ocupada] % 2 == estado.MPI_SOURCE % 2)
            {
               valor = buffer[primera_ocupada] ;
               primera_ocupada = (primera_ocupada+1) % tam_vector ;
               num_celdas_ocupadas-- ;
               cout << "Buffer va a enviar valor " << valor << endl ;
               MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_cons, MPI_COMM_WORLD);
            }
         }
      }
      else                                          // si no vacío ni lleno
      {
         
         MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado );
         switch( estado.MPI_TAG ) // leer emisor del mensaje en metadatos
         {
            case etiq_prod: // si ha sido el productor: insertar en buffer
               buffer[primera_libre] = valor ;
               primera_libre = (primera_libre+1) % tam_vector ;
               num_celdas_ocupadas++ ;
               cout << "Buffer ha recibido valor " << valor << endl ;
               break;

            case etiq_cons: // si ha sido el consumidor: extraer y enviarle
               if (buffer[primera_ocupada] % 2 == estado.MPI_SOURCE % 2)
               {
                  valor = buffer[primera_ocupada] ;
                  primera_ocupada = (primera_ocupada+1) % tam_vector ;
                  num_celdas_ocupadas-- ;
                  cout << "Buffer va a enviar valor " << valor << endl ;
                  MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_cons, MPI_COMM_WORLD);
               }
               break;
         }
         
      }
      
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   int id_propio, num_procesos_actual, orden;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( id_propio < id_buffer )
      {
         orden = id_propio;
         funcion_productor(orden);
      }
         
      else if ( id_propio == id_buffer )
         funcion_buffer();
      else
      {
         orden = id_propio;
         funcion_consumidor(orden);
      }
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
