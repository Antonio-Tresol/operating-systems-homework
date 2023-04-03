// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
class Semaphore {
 public:
   Semaphore(int initialValue = 0);
   Semaphore(int initialValue, int semId);
   ~Semaphore();
   int Signal();	// Is the same as V method in Dijkstra definition
   int Wait();	// Is the same as P method in Dijkstra definition

 private:
   int id;		// Semaphore indentifier
};
