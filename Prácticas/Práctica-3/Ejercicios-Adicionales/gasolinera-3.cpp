/*
   -Nombre: Quintín
   -Apellidos: Mesa Romero
   -DNI: 78006011Q
*/
#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>
#include <string>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const unsigned 
               
               num_surt              = 3,
               surtidores[num_surt]  = {4,5,3},

               num_hebras            = 15,     //número de hebras coche gasolina
               num_procesos_esperado = num_hebras+2,  // número de procesos esperado
               id_gasolinera         = num_hebras,    // id del proceso gasolinera
               id_impresor           = num_hebras+1,  // id del proceso impresor
               // etiqueta que indica comienzo de repostaje va desde 0 a (num-surt-1)
               etiq_finaliza         = num_surt;     // etiqueta que indica fin de repostaje

unsigned 
   num_surtidores_libres[ num_surt ] = { surtidores[0], surtidores[1], surtidores[2] };

//----------------------------------------------------------------------
/// @brief plantilla de función para generar un entero aleatorio uniformemente
/// distribuido entre dos valores enteros, ambos incluidos
/// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
/// @tparam min - valor mínimo (int)
/// @tparam max - valor máximo (int)
/// @return número 'int' aleatorio uniformemente distribuido entew 'min' y 'max', ambos incluidos
///
template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

void imprimir (const string & cadena)
{
    MPI_Ssend(cadena.c_str(), cadena.size(), MPI_CHAR, id_impresor, 0, MPI_COMM_WORLD);
}

//----------------------------------------------------------------------------------------
// Función repostar(), común a todas las hebras
//----------------------------------------------------------------------------------------

void repostar()
{

   // calcular milisegundos aleatorios de duración de la acción de repostar)
   chrono::milliseconds duracion_repo( aleatorio<10,100>() );
   // espera bloqueada un tiempo igual a ''duracion_repo' milisegundos
   this_thread::sleep_for( duracion_repo );
  

}
// ---------------------------------------------------------------------
// Función que ejecuta el proceso coche
// ---------------------------------------------------------------------
void funcion_coche(const unsigned id)
{

    const int tipo_combustible = aleatorio<0,num_surt-1>();

    while(true)
    {

        // solicitar a gasolinera comenzar a repostar
        MPI_Ssend(&tipo_combustible, 1, MPI_INT, id_gasolinera,tipo_combustible, MPI_COMM_WORLD);
        imprimir( "Coche " + to_string(id) + " inicia repostaje en surtidor tipo " + to_string(tipo_combustible) );
        // repostar
        repostar();

        // indicar a gasolinera que se ha terminado de repostar
        imprimir( "Coche " + to_string(id) + " finaliza repostaje en surtidor tipo " + to_string(tipo_combustible) );
        MPI_Ssend(&tipo_combustible, 1, MPI_INT, id_gasolinera,etiq_finaliza, MPI_COMM_WORLD);
    }
    
}

// ---------------------------------------------------------------------
// Función que ejecuta el proceso gasolinera
// ---------------------------------------------------------------------
void funcion_gasolinera()
{
    int peticion = 0,                  // petición recibida
        etiq_aux = 0;                  // etiqueta aceptable

    MPI_Status estado ;                     // metadatos de las dos recepciones

    while(true)
    {
        // determinar si hay mensajes de terminar
        int mensajes = 0;
        MPI_Iprobe(MPI_ANY_SOURCE, etiq_finaliza, MPI_COMM_WORLD, &mensajes, &estado);

        if (mensajes > 0)
        {
            // recibir petición de cualquier coche con esa etiqueta
            MPI_Recv(&peticion, 1, MPI_INT, estado.MPI_SOURCE, etiq_finaliza, MPI_COMM_WORLD, &estado);
            num_surtidores_libres[peticion]++;
            imprimir("Gasolinera procesando petición para inicio de repostaje de coche " +to_string(estado.MPI_SOURCE) + " en surtidor tipo " + to_string(peticion) );
        }

        // comprobar y recibir mensajes de comienzo de repostaje, por orden de tipo de combustible
        for (int i = 0; i < num_surt; i++)
            if (num_surtidores_libres[i] > 0)
            {
                int mensaje_tipo = 0;
                MPI_Iprobe(MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &mensaje_tipo, &estado);

                if (mensaje_tipo > 0)
                {
                    MPI_Recv(&peticion, 1, MPI_INT, estado.MPI_SOURCE, i, MPI_COMM_WORLD, &estado);
                    num_surtidores_libres[i]--;
                    mensajes = 1;
                    imprimir("Gasolinera procesando petición para fin de repostaje de coche " +to_string(estado.MPI_SOURCE) + " en surtidor tipo " + to_string(peticion) );

                }
            }

        if (mensajes == 0)
           this_thread::sleep_for( chrono::milliseconds( 20 )); 
        
    }
    

}

// ---------------------------------------------------------------------
// Función que ejecuta el proceso gasolinera
// ---------------------------------------------------------------------

void funcion_impresor()
{
    int long_cadena;
    MPI_Status estado;

    while(true)
    {
        // el impresor espera a que haya algún mensaje y cuenta los caracteres
        // del mismo, pero sin llegarlo a recibir aún
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
        MPI_Get_count(&estado, MPI_CHAR, &long_cadena);

        char * cadena = new char[long_cadena+1];

        // recibe la cadena un emisor
        MPI_Recv(cadena, long_cadena, MPI_CHAR, estado.MPI_SOURCE, estado.MPI_TAG, MPI_COMM_WORLD, &estado);
        cadena[long_cadena] = 0;

        cout << cadena << endl;

        delete[] cadena;

    }
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( id_propio == id_gasolinera )
         funcion_gasolinera();
        
      else if (id_propio == id_impresor)
         funcion_impresor();
      else
         funcion_coche(id_propio);
      
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
