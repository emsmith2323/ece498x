#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wiringPi.h>
#include <wiringSerial.h>			//Used for UART

int main()
{
printf ("begin\n");
	int data;
	int dev;

  if ((dev = serialOpen ("/dev/ttyAMA0", 9600)) < 0)
  {
    fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
    return 1 ;
  }

  if (wiringPiSetup () == -1)
  {
    fprintf (stdout, "Unable to start wiringPi: %s\n", strerror (errno)) ;
    return 1 ;
  }


printf ("initialized\n");



int a=104;
serialPutchar (dev, a);

printf ("char sent\n");

for (;;)
{   
	if (serialDataAvail (dev) >0)
	{
		data = serialGetchar (dev);
		printf("%c\n", data );
		//printf ( "%c", data);
	}

}

return 0;
}
