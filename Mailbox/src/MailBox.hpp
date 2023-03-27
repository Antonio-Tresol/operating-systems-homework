/**
  *   C++ class to encapsulate Unix message passing intrinsic structures and system calls
  *
 **/

#define MAXDATA 1024


class MailBox {
   public:
      MailBox();
      ~MailBox();
      int send( long, void * buffer, int capacity );
      int recv( long, void * buffer, int capacity );

   private:
      int id;		// mailbox id

};
