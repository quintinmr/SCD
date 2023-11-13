#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

// Variables compartidas

const unsigned int num_hebras          = 10,     //número de hebras total
                   num_hebras_diesel   = 6,      //número de hebras diesel
                   num_hebras_gasolina = 4,      //número de hebras gasolina
                   num_surt_diesel     = 5,      //número de surtidores de diesel
                   num_surt_gasolina   = 3;      //número de surtidores de gasolina

unsigned int       num_surt_en_uso     = 0;      //número de surtidores en uso

Semaphore surtidor_diesel_libre(num_surt_diesel),       //semáforo que controla el núm. de surtidores de diesel libres
          surtidor_gasolina_libre(num_surt_gasolina),   //semáforo que controla el núm. de surtidores de gasolina libres
          exclusion_mutua(1);                           //semáforo que controla el acceso a la variable compartida num_surt_en_uso

//----------------------------------------------------------------------
// Función repostar(), común a todas las hebras
void repostar(unsigned int coche, string tipo)
{

   // calcular milisegundos aleatorios de duración de la acción de repostar)
   chrono::milliseconds duracion_repo( aleatorio<50,100>() );

   // informa de que comienza a repostar
   cout << "Comienza a repostar coche número " << coche << " tipo " << tipo << endl;

   // espera bloqueada un tiempo igual a ''duracion_repo' milisegundos
   this_thread::sleep_for( duracion_repo );

   // informa de que ha terminado de repostar
   cout << "Termina de repostar coche número " << coche << " tipo " << tipo << endl;
   
}
//----------------------------------------------------------------------
// Función que ejecuta la hebra de tipo gasoil
void funcion_hebra_gasoil(unsigned int num_hebra)
{
    while (true)
    {
        //{ esperar hasta que haya un surtidor adecuado libre }
        sem_wait(surtidor_diesel_libre);
            sem_wait(exclusion_mutua);

            //{ incrementar el número de surtidores en uso }
            num_surt_en_uso++;
            //{ imprimir el número de surtidores en uso }
            cout << "Número de surtidores en uso: " << num_surt_en_uso << endl;
            sem_signal(exclusion_mutua);
            
            repostar(num_hebra, "gasoil");
            
            sem_wait(exclusion_mutua);
            //{ decrementar el número de surtidores en uso }
            num_surt_en_uso--;
            //{ imprimir el número de surtidores en uso }
            cout << "Número de surtidores en uso: " << num_surt_en_uso << endl;
            sem_signal(exclusion_mutua);
        //{ señalar que se ha dejado el surtidor libre }
        sem_signal(surtidor_diesel_libre);

        //{ retraso de duración aleatoria (fuera de gasolinera) }
        chrono::milliseconds duracion_fuera( aleatorio<50,100>() );
        this_thread::sleep_for( duracion_fuera );


    }
}

//----------------------------------------------------------------------
// Función que ejecuta la hebra de tipo gasoil
void funcion_hebra_gasolina(unsigned int num_hebra)
{

    while (true)
    {
        //{ esperar hasta que haya un surtidor adecuado libre }
        sem_wait(surtidor_gasolina_libre);
            sem_wait(exclusion_mutua);
            //{ incrementar el número de surtidores en uso }
            num_surt_en_uso++;
            //{ imprimir el número de surtidores en uso }
            cout << "Número de surtidores en uso: " << num_surt_en_uso << endl;
            sem_signal(exclusion_mutua);
            
            repostar(num_hebra, "gasolina");
            
            sem_wait(exclusion_mutua);
            //{ decrementar el número de surtidores en uso }
            num_surt_en_uso--;
            //{ imprimir el número de surtidores en uso }
            cout << "Número de surtidores en uso: " << num_surt_en_uso << endl;
            sem_signal(exclusion_mutua);
        //{ señalar que se ha dejado el surtidor libre }
        sem_signal(surtidor_gasolina_libre);

        //{ retraso de duración aleatoria (fuera de gasolinera) }
        chrono::milliseconds duracion_fuera( aleatorio<50,100>() );
        this_thread::sleep_for( duracion_fuera );

    }

}


//----------------------------------------------------------------------


int main()
{
   // declarar hebras y ponerlas en marcha
   cout << "____________________________________________________________________" << endl;
   cout << endl;
   cout << "			PROBLEMA DE LA GASOLINERA                        " << endl;
   cout << "____________________________________________________________________" << endl << flush;
   
   // declarar hebras y ponerlas en marcha
   
   thread hebras_diesel[num_hebras_diesel];
   thread hebras_gasolina[num_hebras_gasolina];
  
   
   for (int k = 0; k < num_hebras_diesel; k++)
   	hebras_diesel[k] = thread (funcion_hebra_gasoil,k);

   for (int k = 0; k < num_hebras_gasolina; k++)
   	hebras_gasolina[k] = thread (funcion_hebra_gasolina,k);

   
   for (int k = 0; k < num_hebras_diesel; k++)
   	hebras_diesel[k].join();

    for (int k = 0; k < num_hebras_gasolina; k++)
   	hebras_gasolina[k].join();
   
}
