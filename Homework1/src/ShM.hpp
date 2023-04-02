/** class to encapsulate Unix shared memory intrinsic structures and system calls
  *  Author: Operating systems (Francisco Arroyo)
  *  Version: 2023/Mar/15
  *
  * Ref.: https://en.wikipedia.org/wiki/Shared_memory
  *
 **/
class ShM {
  public:
    ShM( int size = 0 );	// Parameter represent segment size
    ~ShM();
    void * attach();
    int detach();

  private:
    int id;		// shared memory indentifier
    void * area;	// pointer to shared memory area
};
