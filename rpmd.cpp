#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mysql.h>
#include <pthread.h>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <wiringPi.h>
#include <wiringSerial.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/opencv.hpp"

#include <Rpmd.hpp>

using namespace std;
using namespace cv;

//Preprocessor Definitions

//// MySQL Configuration ////
#define SERVER "localhost"
#define USER "root"
#define PASSWORD "raspberry"
#define DATABASE "rpmd"

//// RPMD Options ////
#define LOADWAIT 5 //time to wait for user during load
#define PETWAIT 6 //Value x10 is seconds to wait for 
                  //pet to eat pill and leave before tray close

//// Pet Color Assignments ////
#define PET1HUE 52 //52 is green collar
#define PET2HUE 222 //222 is pink collar

//// Serial Connection Configuration ////
#define BAUD 9600 //baud rate for serial device

///// OPENCV DEFINITIONS /////
#define DELAY_MS    200
#define MAX_THRESH  255
#define nodog 0
#define yesdog 1

///// OPENCV COLORS /////
#define HUE_GREEN   60
#define HUE_PINK    220
#define HUE_BLUE    160
#define HUE_RED     7
#define HUE_PURPLE  180


//Global Variables

int serialDevice; //serial device
MYSQL *connection; //MySQL connection
bool verbose=false;
bool forceVerify1=false;
bool forceVerify2=false;
bool forcePillPresent=false;
vector<UserParam> userParams;
vector<Schedule> pillSchedule;


//Function Declarations

vector<UserParam> getUserParams();
vector<Schedule> getPillSchedule();
void checkOnDemand();
void connectToDatabase();
void disconnectFromDatabase();
void handleDBErr(MYSQL *con);
void sendEmail(int emailType);
void loadProcedure(int pillNumber);
void deliveryProcedure(int petNumber, int pillNumber);
void deliverPill(int petNumber, int pillNumber);
int summonPet(int petNumber);
void openTray(int pillNumber);
void closeTray(int pillNumber);
int verifyPet(int petNumber);
int verifyPill(int pillNumber);
//// OPENCV FUNCTIONS ////
int FindDog(int hue, VideoCapture video);
void FindColor(Mat& notprocessed, Mat& processed, int color);
int CountActivePixels(Mat& processed);




int main(int argc,char *argv[]){

  //Process command line arguments
  for(int i=1;i<argc;i++)
  {
    //Use 'v' command line argument for verbose mode
    if(strcmp(argv[i],"v")==0){verbose=true;}

    //Use '1' command line argument to force Pet 1 verified to YES
    if(strcmp(argv[i],"1")==0){forceVerify1=true;}

    //Use '2' command line argument to force Pet 2 verified to YES
    if(strcmp(argv[i],"2")==0){forceVerify2=true;}

   //Use 'p' command line argument to force 'verify pill' to true
   if(strcmp(argv[i],"p")==0){forcePillPresent=true;}
  }

  //List enabled command line options if verbose
  if(verbose==true){cout<<"Verbose Mode"<<endl;}
  if(forceVerify1==true&&verbose==true){cout<<"Force Verify Pet 1 Enabled"<<endl;}
  if(forceVerify2==true&&verbose==true){cout<<"Force Verify Pet 2 Enabled"<<endl;}
  if(forcePillPresent==true&&verbose==true){cout<<"Force Pill Detected Enabled"<<endl;}

  //Open serial device for communication
  if(verbose==true){cout<<"Initializing serial device"<<endl;}
  if ((serialDevice=serialOpen ("/dev/ttyAMA0", BAUD)) < 0)
  {
    fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
    return 1 ;
  }

  //Setup wiring pi
  if(verbose==true){cout<<"Setting up Wiring Pi"<<endl;}
  if (wiringPiSetup () == -1)
  {
    fprintf (stdout, "Unable to start wiringPi: %s\n", strerror (errno)) ;
    return 1 ;
  }

  //Connect to SQL database
  connectToDatabase();

  //Begin repeated procedure
  for(;;){

    //Establish Current Date and Time
    time_t rawtime;
    tm * timeinfo;
    time(&rawtime);
    timeinfo=localtime(&rawtime);
    int wDay=timeinfo->tm_wday; //Day of week Sun=0,Sat=7
    int currentMinute=timeinfo->tm_min;
    int currentHour=timeinfo->tm_hour;
    int currentDay=timeinfo->tm_mday;
    int currentMonth=1+timeinfo->tm_mon;
    int currentYear=1900+timeinfo->tm_year;
    int currentTime=(currentHour*100)+currentMinute;
    long currentDate=(currentYear*10000)+(currentMonth*100)+currentDay;
    const string dayAbbr[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    string dayToday=dayAbbr[wDay];
    if(verbose==true){cout<<"Today is "<<dayToday<< " ";
       cout<<currentDate<<endl;
       cout<<"Time is hhmm "<<currentTime<<endl;
    }
    

     //Populate User Parameters vector with database values
     if(verbose==true){cout<<"Calling getUserParams"<<endl;}
     userParams = getUserParams();

     //Print user parameters if verbose
     if(verbose==true){cout<<"Listing Stored User Params"<<endl;
       for (unsigned i=0; i<userParams.size(); i++)
       {
          cout << "ROW" << i << "::";
          cout << "paramType " << userParams[i].paramType << " ";
          cout << "userValue " <<  userParams[i].userValue << endl ;
       }
     }

     //Populate Pill Schedule vector with database values
     //If there has been a change (user param 0 has value y)
     if(verbose==true){cout<<"Calling getPillSchedule"<<endl;}
     pillSchedule = getPillSchedule();

     //Review Pill Schedule
     for (unsigned i=0; i<pillSchedule.size(); i++)
     {
        if(verbose==true){cout<<"Reviewing Scheduled Items"<<endl;
          cout << "ROW" << i << "::";
          cout << "pillNumber " << pillSchedule[i].pillNumber << " ";
          cout << "petNumber " <<  pillSchedule[i].petNumber << " ";
          cout << "weekDay " << pillSchedule[i].weekDay << " ";
          cout << "deliverTime " << pillSchedule[i].deliverTime << " ";
          cout << "lastDate " << pillSchedule[i].lastDate << endl ;
        }

        //Determine if a scheduled item should run
        if(wDay==pillSchedule[i].weekDay //Check day of week
           &&pillSchedule[i].lastDate<currentDate//Confirm not delivered today
           &&pillSchedule[i].deliverTime==currentTime)//Check time
           {
            //Run Schedule
            if(verbose==true){cout<<"Running Schedule  Now!!!!!"<<endl;}
            pillSchedule[i].lastDate=currentDate;
            deliveryProcedure(pillSchedule[i].petNumber,pillSchedule[i].pillNumber);
            }
     }

     for(int i=0;i<3;i++){

        //Check for On Demand requests
        if(verbose==true){cout<<"Wait 10 Seconds"<<endl;}
        sleep(10);
        if(verbose==true){cout<<"Checking On Demand"<<endl;}
        checkOnDemand();
     } //end loop 3x check on demand

  } //end repeated procedure
  


//Disconnect from SQL database
disconnectFromDatabase();

return 0;
}

//===============================

void checkOnDemand(){
  if(verbose==true){cout<<"Beginning Check On Demand"<<endl;}

  vector<OnDemand> demand; //vector for on demand requests

    //Connect to database if not connected
    if(connection==NULL){connectToDatabase();}

    if(connection != NULL)
    {
        //Retrieve all data from table in descending order to process oldest first
        if(mysql_query(connection, "SELECT * FROM on_demand ORDER BY sort_order DESC"))
        {
            cout <<"ONDEMAND :: ";
            handleDBErr(connection);
            cout << endl;
        }

	  //Store result
        MYSQL_RES *result = mysql_store_result(connection);

	  //If there was data, reset SQL table and process on demand requests 
        if(result != NULL)
        {

    	     //Delete all data from SQL table
           if(mysql_query(connection, "DELETE FROM on_demand"))
           {
              cout <<"ONDEMAND DEL :: ";
              handleDBErr(connection);
              cout << endl;
            }

    	     //Reset 'sort_order' in SQL table
           if(mysql_query(connection, "ALTER TABLE on_demand AUTO_INCREMENT=1"))
           {
              cout <<"ONDEMAND DEL :: ";
              handleDBErr(connection);
              cout << endl;
           }

            //Add all the rows in result to demand vector
            MYSQL_ROW row;
            while((row = mysql_fetch_row(result)))
            { 
               int tempPillNumber = std::stoi (row[1]);
               int tempPetNumber = std::stoi (row[2]);
               OnDemand tempDemand (tempPillNumber, tempPetNumber); //create temp vector
               demand.push_back(tempDemand); //add temp vector to full set
               if(verbose==true){
                  cout<<"Added to ondemand vector: "<<tempPillNumber;
                  cout<<" "<<tempPetNumber<<endl;
                  }
            } //end while there are rows
            
            mysql_free_result(result);

            //Process on demand vector
            for (int i=demand.size()-1; i!=-1; i--)
              {
                 if(verbose==true){cout<<"Processing on demand command"<<endl;}
                 if(demand[i].petNumber==0){loadProcedure(demand[i].pillNumber);}
                 else{deliveryProcedure(demand[i].petNumber,demand[i].pillNumber);}

                 if(verbose==true){
                   cout << "Delete ondemand ROW" << i << "::";
                   cout << "petNumber " << demand[i].petNumber << " ";
                   cout << "pillNumber " <<  demand[i].pillNumber << endl;
                  }
                  demand.pop_back();//delete last element
             }//end for demand vector elements
			   
        }//end result of sql query !=null
   }//end connection !=null

if(verbose==true){
 cout<<"At end of checkOnDemand vector contains:"<<endl;
   for (unsigned i=0; i<demand.size(); i++)
   {
      cout << "ROW" << i << "::";
      cout << "petNumber "<<demand[i].petNumber << " ";
      cout << "pillNumber "<<demand[i].pillNumber << endl ;
   }
 }

}
//======================== 


//Get User Parameters

vector<UserParam> getUserParams(){
  if(verbose==true){cout<<"Beginning User Param"<<endl;}
  vector<UserParam> params; //vector for user parameters


 // connectToDatabase();

    if(connection != NULL){
        //Retrieve all data
        if(mysql_query(connection, "SELECT * FROM user_params")){
            cout <<"PARAMS :: ";
            handleDBErr(connection);
            cout << endl;
        }

        MYSQL_RES *result = mysql_store_result(connection);

        if(result != NULL){
  
            //Get all the rows in table
            MYSQL_ROW row;

            //Add rows to params vector
            while((row = mysql_fetch_row(result))){
               int tempType = std::stoi (row[0]);
               string tempValue = row[1];
               UserParam tempParam (tempType, tempValue); //create temp vector
               params.push_back(tempParam); //add temp vector to full set
               if(verbose==true){cout << "Added: "<<tempType<<" "<<tempValue<<endl;}
            }//end while

            mysql_free_result(result);
         }//end if result!=null
      }//end if connection!=null   

if(verbose==true){cout<<"End User Param"<<endl;}
       
return params;
}
//============================

//Get Schedule

vector<Schedule> getPillSchedule(){
  if(verbose==true){cout<<"Beginning Get Pill Schedule"<<endl;}

  vector<Schedule> pSchedule;

 // connectToDatabase();

    if(connection != NULL){
        //Retrieve all data
        if(mysql_query(connection, "SELECT * FROM pill_schedule")){
            cout <<"SCHEDULE :: ";
            handleDBErr(connection);
            cout << endl;
        }

        MYSQL_RES *result = mysql_store_result(connection);

        if(result != NULL){
  
            //Get all the rows in table
            MYSQL_ROW row;

            //Add rows to pill schedule vector
            while((row = mysql_fetch_row(result))){
               int tempPill = std::stoi (row[0]);
               int tempPet = std::stoi (row[1]);
               int tempDay = std::stoi (row[2]);
               string strTime = row[3];
               int tempHour = std::stoi (strTime.substr(0,2));
               int tempMinute = std::stoi (strTime.substr(3,2));
               int tempTime = (tempHour*100)+tempMinute;
               Schedule tempSchedule (tempPill, tempPet, tempDay, tempTime); //create temp vector
               pSchedule.push_back(tempSchedule); //add temp vector to full set
               if(verbose==true){cout << "Added: "<<tempPill<<" "<<tempPet;
                  cout<<" "<<tempDay<<" "<<tempTime<<endl;}
            }//end while

            mysql_free_result(result);
         }//end if result!=null
      }//end if connection!=null   

if(verbose==true){cout<<"End User Param"<<endl;}
       
return pSchedule;
}
//============================


//Connect to MySQL using SERVER, USER, PASSWORD, DATABASE

void connectToDatabase(){
    //initialize MYSQL object for connection
    connection = mysql_init(NULL);
    
    if(connection == NULL){
        fprintf(stderr, "%s\n", mysql_error(connection));
        return;
    }
    
    //Connect to the database
    if(mysql_real_connect(connection, SERVER, USER, PASSWORD,
            DATABASE, 0, NULL, 0) == NULL){
        handleDBErr(connection);
    }else{
        if(verbose==true){cout<<"Database connection successful"<<endl;}
    }
}
//================================


//Disconnect from Database

void disconnectFromDatabase(){
    mysql_close(connection);
    if(verbose==true){cout<<"Disconnected from database"<<endl;}
}
//=========================================


//Handle database connection errors

void handleDBErr(MYSQL *con){ 
    fprintf(stderr, "%s\n", mysql_error(con));
    //mysql_close(con);
    //exit(1);
}
//=========================================

//Send Email

void sendEmail(int emailType){
 if(verbose==true){cout<<"Sending Email # "<<emailType<<endl;}

 string msgEmail;
 
 //Review all user parameters to find email address
 for (unsigned i=0; i<userParams.size(); i++)
 { 
    if(userParams[i].paramType==1) //paramType 1 is email addr
    {
	//Set email address based on sql value
	msgEmail = userParams[i].userValue;
    }
 }

//Create variables for each email message type

string msgPart0 = "echo \"Pill delivered\" | mail -s \"RPMD Status\" "+msgEmail; 
char * message0 = new char [msgPart0.length()+1]; //allocate message var
std::strcpy (message0, msgPart0.c_str()); //populate message var

string msgPart1 = "echo \"Pet did not come when called\" | mail -s \"RPMD Status\" "+msgEmail; 
char * message1 = new char [msgPart1.length()+1];
std::strcpy (message1, msgPart1.c_str());

string msgPart2 = "echo \"No pill to deliver\" | mail -s \"RPMD Status\" "+msgEmail; 
char * message2 = new char [msgPart2.length()+1];
std::strcpy (message2, msgPart2.c_str());

string msgPart3 = "echo \"Pet did not eat pill\" | mail -s \"RPMD Status\" "+msgEmail; 
char * message3 = new char [msgPart3.length()+1];
std::strcpy (message3, msgPart3.c_str());

string msgPart4 = "echo \"Pill not detected after load\" | mail -s \"RPMD Status\" "+msgEmail; 
char * message4 = new char [msgPart4.length()+1];
std::strcpy (message4, msgPart4.c_str());

//send appropriate email
  if (system(NULL)) //a command processor is available
  { 
     if (emailType==0){system (message0);}
     if (emailType==1){system (message1);}
     if (emailType==2){system (message2);}
     if (emailType==3){system (message3);}
     if (emailType==4){system (message4);}
  }
  //future work:consider lighting error led if no command proc avail

if(verbose==true){cout<<"Send Email Complete"<<endl;}
}
//===================================


//Load procedure
void loadProcedure(int pillNumber){
   if(verbose==true){cout<<"Starting Load Procedure"<<endl;}

   openTray(pillNumber);
   
   if(verbose==true){cout<<"Waiting for pill to be loaded"<<endl;}
   sleep(LOADWAIT); //wait ## seconds for user to load pill

   closeTray(pillNumber);

   //If pill not detected after load cycle, inform user via email
   if(verifyPill(pillNumber)==0){sendEmail(4);}

   if(verbose==true){cout<<"Load Procedure Complete"<<endl;}
}
//====================


//Delivery procedure
void deliveryProcedure(int petNumber, int pillNumber){
   if(verbose==true){cout<<"Starting Delivery Procedure"<<endl;}
   
   if(verifyPill(pillNumber)==0){ //Pill not present
       sendEmail(2); //inform user no pill present
   }else{ //Pill is present, begin delivery
       if(summonPet(petNumber)==1){ //Pet is present
          deliverPill(petNumber, pillNumber); //deliver pill informs user of status
       }else{//Pet is not present
          sendEmail(1); //Pet did not come when called
       }//end pet not present else
    }//end pill is present else 
 
   if(verbose==true){cout<<"Delivery Procedure Complete"<<endl;}

}
//=======================

//Deliver pill
void deliverPill(int petNumber, int pillNumber){
   if(verbose==true){cout<<"Delivering Pill"<<endl;}

   openTray(pillNumber);

   //wait for PETWAIT x10 seconds pet to leave
   if(verbose==true){cout<<"Begin Waiting for Pet to Leave"<<endl;}
   for(int i=0; i<PETWAIT; i++){
      sleep(10); //wait 10 seconds
      if(verifyPet(petNumber)==0){break;}//pet left area
   }

   closeTray(pillNumber); //Pet left area or timeout so close tray

   if(verifyPill(pillNumber)==0){ //Pill not present
      sendEmail(0); //inform user Delivery success
   }else{ //Pill present
      sendEmail(3); //inform user Delivery timeout
    }

   if(verbose==true){cout<<"Deliver Pill Complete"<<endl;}

}
//=======================

//Verify Pill
int verifyPill(int pillNumber){
  if(verbose==true){cout<<"Starting Verify Pill"<<endl;}

  if(forcePillPresent==true){
     if(verbose==true){cout<<"Verification Complete (forced pill present)"<<endl;}
     return 1;
  }

  int sendData;
  int dataReceived;  

  if(pillNumber==1){sendData='1';}
  if(pillNumber==2){sendData='2';}
  if(pillNumber==3){sendData='3';}
  if(pillNumber==4){sendData='4';}
  if(pillNumber==5){sendData='5';}
  if(pillNumber==6){sendData='6';}
 
  if(verbose==true){cout<<"Sending Code: "<<(char) sendData<<endl;}
  serialPutchar (serialDevice, sendData);
  
  if(verbose==true){cout<<"Waiting for Reply up to 30 seconds"<<endl;}
  for(int i; i<30; i++) //poll for data for 30 seconds max
  {   
     if (serialDataAvail (serialDevice) >0)  //if data is waiting
	{
		dataReceived = serialGetchar (serialDevice);
            break;
	}
      sleep(1); //check for new data once per second
  }
  
  if(verbose==true){cout<<"Verification Complete Received from Serial: "<<(char) dataReceived<<endl;}
  if(dataReceived==121){return 1;} //121 is ascii 'y'

return 0;  //no pill or timeout

}
//===================


void openTray(int pillNumber){
  if(verbose==true){cout<<"Starting Open Tray"<<endl;}  

  int sendData=0; //initialized to 'do not send command' value

  if(pillNumber==1){sendData='a';}
  if(pillNumber==2){sendData='b';}
  if(pillNumber==3){sendData='c';}
  if(pillNumber==4){sendData='d';}
  if(pillNumber==5){sendData='e';}
  if(pillNumber==6){sendData='f';}

  if(verbose==true){cout<<"Sending "<<(char) sendData<<endl;}
  if(sendData!=0){
     serialPutchar (serialDevice, sendData);
  }
}
//=====================

void closeTray(int pillNumber){
  if(verbose==true){cout<<"Starting Close Tray"<<endl;}  

  int sendData=0; //initialized to 'do not send command' value

  if(pillNumber==1){sendData='g';}
  if(pillNumber==2){sendData='h';}
  if(pillNumber==3){sendData='i';}
  if(pillNumber==4){sendData='j';}
  if(pillNumber==5){sendData='k';}
  if(pillNumber==6){sendData='m';}

  if(verbose==true){cout<<"Sending "<<(char) sendData<<endl;}
  if(sendData!=0){
     serialPutchar (serialDevice, sendData);
  }
}
//=====================



//Summon Pet
int summonPet(int petNumber){
   if(verbose==true){cout<<"Summoning Pet"<<endl;}

   int summoned=1;   
 
   if(verbose==true){cout<<"Summon Pet Complete"<<endl;}

return summoned; 
}
//=======================



//Verify Pet is Present
int verifyPet(int petNumber){
  if(verbose==true){cout<<"Starting Verify Pet"<<endl;}

  int color;
  int answer=nodog;
  int i=0;

  if(petNumber==1){color=PET1HUE;}
  if(petNumber==2){color=PET2HUE;}

  if((forceVerify1==true&&petNumber==1)||(forceVerify2==true&&petNumber==2)){
     answer=yesdog;}

  VideoCapture video(0);
/*
  //give the camera 40 frames attempt to get the camera object, 
  //if it fails after X (40) attemts the app will terminatet, 
  //till then it will display 'Accessing camera' note;

  int j = 40;
  while (true) {

      Mat videoFrame;
      video >> videoFrame;
      if ( videoFrame.total() > 0 ) {

         if(verbose==true){cout<<"Calling Find Dog"<<endl;}
         while(answer==nodog&&i<100)
            {
               i++;
               answer=FindDog(color, video);
            }//end while no dog

         if(verbose==true&&answer==yesdog){cout<<"End Verify Pet - Pet Detected"<<endl;}
         if(verbose==true&&answer==nodog){cout<<"End Verify Pet - Pet Not Detected"<<endl;}

         return answer;
        //  Mat displayFrame( cameraFrame.size(), CV_8UC3 );
        //  doSomething( cameraFrame, displayFrame );
       //   imshow("Image", displayFrame );
      }//endif 
      else {
          if(verbose==true){cout<<"::: Initializing Camera ::: "<<j<< endl;}
          if ( j > 0 ){ j--;}
          else{ break;}
      }//end else

      int key = waitKey(200);
      if(verbose==true){cout<<"Key 27 for fail"<<key<<endl;}
      if (key == 27) break;

  }//end while(true)
*/

  if((forceVerify1==true&&petNumber==1)||(forceVerify2==true&&petNumber==2)){
     answer=yesdog;}

  while(answer==nodog&&i<100)
  {
    i++;
    answer=FindDog(color, video);
  }

  if(verbose==true&&answer==yesdog){cout<<"End Verify Pet - Pet Detected"<<endl;}
  if(verbose==true&&answer==nodog){cout<<"End Verify Pet - Pet Not Detected"<<endl;}

return answer;
}
//====================


int FindDog(int color, VideoCapture video)
{
    int pixels;
    int answer=nodog;
    Size_<int> windowsize; // variable of type size that is a data structure



    // Get the dimensions of the camera stream in pixels
    windowsize.width = video.get(CV_CAP_PROP_FRAME_WIDTH);
    windowsize.height = video.get(CV_CAP_PROP_FRAME_HEIGHT);


    // Create Matrix data type with a size of
    //  'windowsize', 8-bit colors, and 3 colors per pixel
    Mat notprocessed(windowsize, CV_8UC3);

    // Create Matrix to perform processing with
    Mat processed (windowsize, CV_8UC3);

    video.read(notprocessed);
    //imshow("Not Processed",notprocessed); used to show window

    FindColor(notprocessed, processed, color);

    pixels=CountActivePixels(processed);

    if (pixels >= 0.05*processed.cols*processed.rows)
    {
        answer=yesdog;
    }
    else
    {
      answer=nodog;
    }

    //imshow("Processed", processed);  used to show window
    //waitKey(DELAY_MS);

    return answer;
}


void FindColor(Mat& notprocessed, Mat& processed, int color)
{
    Mat convertbw;

    int colormax = color+10;
    int colormin = color-10;

    cvtColor(notprocessed,processed,CV_BGR2HLS_FULL);

    // Choose threshold boundry pixels
    Scalar pixelmin(colormin, 20, 45);    // Saturation Range: 0->255
    Scalar pixelmax(colormax, 230, 255);  // Lightness Range: 0->255

    inRange(processed, pixelmin, pixelmax, convertbw);

    vector<vector<Point> > listofshapes;

    findContours(convertbw, listofshapes, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    processed = Mat::zeros(notprocessed.size(), notprocessed.type());

    drawContours(processed, listofshapes, -1, Scalar::all(255), CV_FILLED);

    processed &= notprocessed;
}

int CountActivePixels(Mat& processed)
{
    Mat grayimage;
    int pixelCount = 0;
    int pixelValue;

    // Fixed threshold for the color black in a grayscale image, 0 black, 255 white
    int threshold = 10;

    // Convert Image to grayscale
    cvtColor(processed,grayimage,CV_BGR2GRAY);

    // Loop through every pixel in the image
    for( int y = 0; y < grayimage.rows; y++ )
    {
        for( int x = 0; x < grayimage.cols; x++ )
        {
            // Get the value of the individual gray pixel, 0->255
            pixelValue = grayimage.data[grayimage.channels()*(grayimage.cols*y + x) + 0];

            // If pixel value is above threshold, then pixel is NOT black
            if(pixelValue > threshold)
            {
                pixelCount++;
            }
        }
    }
    return pixelCount;
}
