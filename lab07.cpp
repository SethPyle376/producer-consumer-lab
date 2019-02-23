/***********************************************************************
 * Program:
 *    Lab 06, Producer and Consumer
 * Author:
 *    Jason Pyle
 * Summary:
 *    This program will allow us to simulate the production run
 *    at McLaren Cars. It would be cool if it simulated the cars
 *    themselves, but that might be an assignment for another day
 *
 *    Note that you must compile this with the -lpthread switch:
 *        g++ -lpthread lab06.cpp
 ************************************************************************/

#include <iostream>      // for COUT
#include <semaphore.h>
#include <cassert>       // for ASSERT
#include <string>        // for STRING
#include <sstream>       // for STRINGSTREAM
#include <queue>         // for QUEUE
#include <map>
#include <ctime>         // for time(), part of the random process
#include <unistd.h>      // for usleep()
#include <stdlib.h>      // for rand() and srand()
#include "cars.h"        // for everything dealing with McLaren Cars

using namespace std;

// This variable represents the shared memory between the parent thread
// and all the children. Recall from the reading how the main way for
// threads to communicate is through shared memory. This shared memory
// needs to be global.
bool productionComplete = false;

void *   producer(void *p);   // you may need to change this
void *   consumer(void *p);   // you may need to change this also

int getNumber(const char * prompt, int max);

sem_t semaphore;

map<string, queue<string*>*> resultMap;

struct CarData
{
   Inventory *inventory;
   const char *model;
};

/***********************************************
 * MAIN
 * This will serve to prompt the user for the number
 * of models and the number of retailers. It will then
 * begin the simulation
 ***********************************************/
int main(int argc, char **argv)
{
   // set up the random number generator
   srand(argc == 1 ? time(NULL) : (int)argv[1][1]);

   sem_init(&semaphore, 0, 1);
   // determine how many producer and consumer threads
   int numProducer = getNumber("How many models?    ", numModels());
   int numConsumer = getNumber("How many retailers? ", numRetailers());

   pthread_t producerThreads[numProducer];
   pthread_t consumerThreads[numConsumer];

   // produce the cars. Note that this code needs to change. We will
   // need to launch one thread per producer here
   Inventory inventory;
   for (int i = 0; i < numProducer; i++)
   {
      struct CarData* carData = new CarData;
      carData->inventory = &inventory;
      carData->model = models[i];

      pthread_create(&(producerThreads[i]),
                     NULL,
                     producer,
                     (void*)carData);
   }
   
   for (int i = 0; i < numConsumer; i++)
   {
      struct CarData* carData = new CarData;
      carData->inventory = &inventory;
      carData->model = "test";
      
      pthread_create(&(consumerThreads[i]),
                     NULL,
                     consumer,
                     (void*)carData);

      resultMap.insert(make_pair(retailers[i] , new queue<string*>()));
    }

   for (int i = 0; i < numProducer; i++)
   {
      pthread_join(producerThreads[i], NULL);
   }

   while (!inventory.empty())
      continue;
   
   map<string, queue<string*>*>::iterator it = resultMap.begin();

   cout << "\nThe cars sold from the various retailers:\n\n";
   while(it != resultMap.end())
   {
      cout << it->first << ":\n";
      while(!(it->second->empty()))
      {
         cout << (*(string *)it->second->front()) << endl;
         it->second->pop();
      }
   }
   

   // final report
   cout << "Maximum size of the inventory: "
        << inventory.getMax()
        << endl;

   return 0;
}

/***********************************************************
 * PRODUCER
 * Create those cars.
 * This function is not currently thread safe. You will need
 * to introduce a critical section in such a way that we do
 * not compromise the queue nor produce two cars with the
 * same serial number.
 **********************************************************/
void* producer(void *p)
{
   struct CarData *carData = (struct CarData*)p;
   static int serialNumberNext = 0;

   // a car to be added to the inventory
   Car car;
   car.model = carData->model;

   // continue as long as we still need to produce cars in this run
   while (serialNumberNext < NUM_CARS)
   {
      // now that we decided to build a car, it takes some time
      usleep(rand() % 150000);
      sem_wait(&semaphore);
      // add the car to our inventory if we still need to
      if (serialNumberNext < NUM_CARS)
      {
         car.serialNumber = ++serialNumberNext;
         carData->inventory->makeCar(car);
      }
      sem_post(&semaphore);
   }

   // all done!
   productionComplete = true;
}

/***********************************************************
 * CONSUMER
 * Sell those cars.
 * This function is not currently thread safe. You will need
 * to introduce a critical section in such a way that we
 * do not compromise the queue nor sell the same car twice.
 **********************************************************/
void* consumer(void *p)
{
   stringstream sout;
   struct CarData *carData = (struct CarData*)p;
   // collect our sales history into one string   
   // continue while there are still customers floating around
   while (!(productionComplete && carData->inventory->empty()))
   {
      sem_wait(&semaphore);

      if (carData->inventory->empty())
      {
          
         // the car to sell
         Car car;
         sem_post(&semaphore);
         // it takes time to sell our car
         usleep(rand() % 150000);
         
         // do we have one to sell
         if (!carData->inventory->empty())
         {
            sem_wait(&semaphore);
            car = carData->inventory->sellCar();
            sout << "\t" << car << endl;
            sem_post(&semaphore);
         }
         sem_wait(&semaphore);
         string * report = new string(sout.str());
         resultMap[carData->model]->push(report);
         sem_post(&semaphore);
      }
      sem_post(&semaphore);
   }
}

/*********************************************
 * GET NUMBER
 * Generic prompt function with error checking
 ********************************************/
int getNumber(const char * prompt, int max)
{
   int value = -1;
   assert(cin.good());       // better not already be in error mode
   assert(prompt != NULL);   // the prompt better be a valid c-string
   assert(max > 0);          // it better be possible for valid data to exist

   // continue prompting until we have valid data
   while (value <= 0 || value > max)
   {
      cout << prompt;
      cin  >> value;

      // if the user typed a non-integer value, reprompt.
      if (cin.fail())
      {
         cin.clear();
         cin.ignore();
         cout << "Error: non-integer value specified\n";
      }

      // if the user typed a valid outside the range, reprompt
      else if (value <= 0 || value > max)
         cout << "Error: value must be between 1 and "
              << max
              << endl;
   }

   return value;
}
