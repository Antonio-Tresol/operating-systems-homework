// Copyright 2023 Antonio Badilla Olivas <anthonny.badilla@ucr.ac.cr>.
class ShM {
 public:
  ShM(int size = 0);	// Parameter represent segment size
  ~ShM();
  void * attach();
  int detach();

 private:
  int id;		// shared memory indentifier
  void * area;	// pointer to shared memory area
};
