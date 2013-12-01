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

#include <Rpmd.hpp>

using namespace std;

//Preprocessor Definitions
#define SERVER "localhost"
#define USER "root"
#define PASSWORD "raspberry"
#define DATABASE "rpmd"
#define LOADWAIT 5 //time to wait for user during load
#define PETWAIT 6 //Value x10 is seconds to wait for 
                  //pet to eat pill and leave before tray close
#define BAUD 9600 //baud rate for serial device


//Global Variables
bool verbose=false;
vector<UserParam> userParams;
vector<Schedule> pillSchedule;
int serialDevice; //serial device


//MYSQL pointer to MYSQL connection
MYSQL *connection;

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

int main(int argc,char *argv[]){

  //Process command line arguments
  for(int i=1;i<argc;i++)
  {
    //Use 'v' command line argument for verbose mode
    if(strcmp(argv[i],"v")==0){verbose=true;}
  }

  //List enabled command line options if verbose
  if(verbose==true){cout<<"Verbose Mode"<<endl;}

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

     for(int i=0;i<3;i++){

        //Check for On Demand requests
        if(verbose==true){cout<<"Wait 10 Seconds"<<endl;}
        sleep(10);
        if(verbose==true){cout<<"Checking On Demand"<<endl;}
        checkOnDemand();
     } //end loop 3x check on demand

  } //end repeated procedure
  


// sendEmail(0);

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
  if(verbose==true){cout<<"Beginning Get Pill Schedule"<<endl;

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
               int tempPet = std::sot (row[1]);
               string tempDay = row[3];
               string tempTime = row[4];

               Schedule tempSchedule (tempPill, tempPet, tempDay, tempTime); //create temp vector
               pillSchedule.push_back(tempSchedule); //add temp vector to full set
               if(verbose==true){cout << "Added: "<<tempPill<<" "<<tempPet;
                  cout<<" "<<tempDay<<" "<<tempTime<<endl;}
            }//end while

            mysql_free_result(result);
         }//end if result!=null
      }//end if connection!=null   

if(verbose==true){cout<<"End User Param"<<endl;}
       
return params;
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

int verifyPill(int pillNumber){
  if(verbose==true){cout<<"Starting Verify Pill"<<endl;}
 
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
  
  if(verbose==true){cout<<"Verification Result: "<<(char) dataReceived<<endl;}
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

int verifyPet(int petNumber){
  if(verbose==true){cout<<"Starting Verify Pet"<<endl;}

  int verified=1;
  
  if(verbose==true&&verified==1){cout<<"End Verify Pet - Pet Detected"<<endl;}
  if(verbose==true&&verified==0){cout<<"End Verify Pet - Pet Not Detected"<<endl;}

return verified;
}
//====================


/*

vector<Schedule> getSchedule(){
    vector<Schedule> sched;   //a vector of the scheduled times

    if(connection != NULL){
        //Retrieve all data from alarm_times
        if(mysql_query(connection, "SELECT * FROM alarm_times")){
            cout <<"getAlarmTimes 1 :: ";
            handleDBErr(connection);
            cout << endl;
        }
        
        MYSQL_RES *result = mysql_store_result(connection);
        
        if(result != NULL){
            //Get the number of columns
            int num_fields = mysql_num_fields(result);
            
            //Get all the alarms, and add them to the list
            MYSQL_ROW row;
            int i = 0;
            while((row = mysql_fetch_row(result))){
                if(num_fields == 4){
                    string date = row[0], time = row[1], repeat = row[2],
                           audioFile = row[3];
                    AlarmTime tempTime (date, time, audioFile);
                    times.push_back(tempTime);
                    //cout << "Added: " << tempTime.toString() << endl;
                    i++;
                }else{
                    cout << "MySQL: Wrong number of columns." << endl;
                }
            }
            
            mysql_free_result(result);
            
            //If the number of alarms has changed, print it out
            if(times.size() != numAlarms){
                cout << "**********************************" << endl;
                cout << "Alarms" << endl;
                cout << "**********************************" << endl;
                for(long unsigned i = 0; i < times.size(); i++){
                    cout << times[i].toString() << endl;
                }
                if(times.size() == 0){
                    cout << "No alarms!" << endl;
                }
                cout << "**********************************" << endl << endl;
            }
            numAlarms = times.size();
            return times;
        }else{
            cout <<"getAlarmTimes 2 :: ";
            handleDBErr(connection);
            cout << endl;
        }
    }
    
    return times;
}//===========================
*/
