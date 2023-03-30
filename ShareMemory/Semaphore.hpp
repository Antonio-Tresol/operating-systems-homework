/** class to encapsulate Unix semaphore intrinsic structures and system calls
  *  Author: Operating systems (Francisco Arroyo)
  *  Version: 2023/Mar/15
  *
  * Ref.: https://en.wikipedia.org/wiki/Semaphore_(programming)
  *
 **/

class Semaphore {
  public:
    Semaphore( int = 0 );	// Parameter is initial value
    ~Semaphore();
    int Signal();	// Is the same as V method in Dijkstra definition
    int Wait();	// Is the same as P method in Dijkstra definition

  private:
    int id;		// Semaphore indentifier
};
