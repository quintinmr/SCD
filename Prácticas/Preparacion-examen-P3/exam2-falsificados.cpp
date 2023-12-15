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
//  Producto falsificado
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
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
   num_items             = 80,
   np                    = 4,              // número de procesos productores
   nc                    = 8,              // número de procesos consumidores
   k                     = num_items/np,   // número de items que produce cada productor. Los prod. falsificadores producen tambíen k falsos
   c                     = num_items/nc,   // número de items que consume cada consumidor              
   npf                   = 2,              // número de procesos productores de productos falsificados
   num_buff              = 2,
   id_buffer             = np+npf ,
   id_buffer_false       = np+npf+1,
   etiq_prod             = 0 ,
   etiq_cons             = 1 ,
   etiq_prodfalse        = 2 , 
   num_procesos_esperado = np+nc+npf+num_buff ,
   tam_vector            = 10,
   prod_falsificado      = 44;             // producto falsificado (44)


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
int producir(unsigned orden)
{
   static int contador = 0 + orden*(np); 
   sleep_for( milliseconds( aleatorio<10,100>()) );
   contador++ ;
   cout << "Productor " << orden << " ha producido valor " << contador /*<< " y tag "<< tag_productor */<< endl << flush;
   return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor(unsigned orden)
{
   for ( unsigned int i= 0 ; i < k ; i++ )
   {
      // producir valor
      int valor_prod = producir(orden);
      // enviar valor
      cout << "Productor " << orden << " va a enviar valor " << valor_prod << endl << flush;
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiq_prod, MPI_COMM_WORLD );
   }
}

// ---------------------------------------------------------------------

void funcion_productor_False(unsigned orden)
{
   for ( unsigned int i= 0 ; i < num_items/npf ; i++ )
   {
      // producir valor
      int valor_prod = prod_falsificado;
      // enviar valor
      cout << "Productor " << orden << " va a enviar valor " << valor_prod << endl << flush;
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer_false, etiq_prodfalse, MPI_COMM_WORLD );
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
      if (orden == 0)
      {
        
        MPI_Ssend( &peticion,  1, MPI_INT, id_buffer_false, etiq_cons, MPI_COMM_WORLD);
        MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer_false, etiq_cons, MPI_COMM_WORLD,&estado );
        cout << "Consumidor "<< orden <<" ha recibido valor falsificado" << valor_rec << endl << flush ;
      }

        MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD);
        MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD,&estado );
        cout << "Consumidor "<< orden <<" ha recibido valor " << valor_rec << endl << flush ;
        consumir( valor_rec, orden );
    
   }

   if (orden == 0){ 
   	for (unsigned int i = 0; i < (num_items - c); i++)
   	{
   		MPI_Ssend( &peticion,  1 , MPI_INT, id_buffer_false , etiq_cons , MPI_COMM_WORLD);
      	MPI_Recv ( &valor_rec, 1 , MPI_INT, id_buffer_false , etiq_cons , MPI_COMM_WORLD,&estado );
      	cout << "El consumidor " << orden << " ha consumido un valor falso " << valor_rec << endl;	
   	}
   }
}
// ---------------------------------------------------------------------

void funcion_buffer(int id)
{
   int        buffer[tam_vector],      // buffer con celdas ocupadas y vacías
              valor,                   // valor recibido o enviado
              valor_r,
              primera_libre       	= 0, // índice de primera celda libre
              primera_ocupada     	= 0, // índice de primera celda ocupada
              num_celdas_ocupadas 	= 0, // número de celdas ocupadas
              id_emisor_aceptable = MPI_ANY_SOURCE,    // identificador de emisor aceptable
              etiqueta_aceptable ;     // etiqueta de emisor aceptable
   MPI_Status estado ;                 // metadatos del mensaje recibido

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos
      if (num_celdas_ocupadas == 0)
      {
        if (id == 0)
        {
            cout << "El buffer " << id << " entra en almacén bueno." << endl;
            etiqueta_aceptable = etiq_prod;
        }
        else
        {
            cout << "El buffer " << id << " entra en almacén falso." << endl;
            etiqueta_aceptable = etiq_prodfalse;
        } 

      }
      else if ( num_celdas_ocupadas == tam_vector ) // si buffer lleno
         etiqueta_aceptable = etiq_cons ;      // $~~~$ solo cons.
      else                                          // si no vacío ni lleno
         etiqueta_aceptable = MPI_ANY_TAG ;     // $~~~$ cualquiera

      // 2. recibir un mensaje del emisor o emisores aceptables

      MPI_Recv( &valor_r, 1, MPI_INT, id_emisor_aceptable, etiqueta_aceptable, MPI_COMM_WORLD, &estado );

      // 3. procesar el mensaje recibido

      switch( estado.MPI_TAG ) // leer emisor del mensaje en metadatos
      {
         case etiq_prod:case etiq_prodfalse: // si ha sido el productor: insertar en buffer
            buffer[primera_libre] = valor_r ;
            primera_libre = (primera_libre+1) % tam_vector ;
            num_celdas_ocupadas++ ;
            cout << "Buffer ha recibido valor " << valor_r << endl ;
            break;

         case etiq_cons: // si ha sido el consumidor: extraer y enviarle
            valor = buffer[primera_ocupada] ;
            primera_ocupada = (primera_ocupada+1) % tam_vector ;
            num_celdas_ocupadas-- ;
            cout << "Buffer va a enviar valor " << valor << endl ;
            MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_cons, MPI_COMM_WORLD);
            break;

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
      if ( id_propio < np )
      {
         funcion_productor(id_propio);
      }
         
      else if ( id_propio < (np+npf) )
         funcion_productor_False((id_propio % np));

      else if ( id_propio <= id_buffer_false)
         funcion_buffer(id_propio % (np+npf));
      else
      {
         funcion_consumidor(id_propio % (np+npf+num_buff));
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
