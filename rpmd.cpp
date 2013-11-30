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

//Preprocesor Definitions
#define SERVER "localhost"
#define USER "root"
#define PASSWORD "raspberry"
#define DATABASE "rpmd"

//Global Variables
bool verbose=false;
vector<UserParam> userParams;


//MYSQL pointer to MYSQL connection
MYSQL *connection;

//Function Declarations
vector<UserParam> getUserParams();
void checkOnDemand();
void connectToDatabase();
void disconnectFromDatabase();
void handleDBErr(MYSQL *con);
void sendEmail(int emailType);

int main(int argc,char *argv[]){


 printf ("begin\n");
	int data;
	int dev;
  //Open serial device for communication
  if ((dev=serialOpen ("/dev/ttyAMA0", 9600)) < 0)
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

int a='a';
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


  //Process command line arguments
  for(int i=1;i<argc;i++)
  {
    //Use 'v' command line argument for verbose mode
    if(strcmp(argv[i],"v")==0){verbose=true;}
  }

  //List enabled command line options
  if(verbose==true){cout<<"Verbose Mode"<<endl;}

  //Connect to SQL database
  connectToDatabase();

  //Populate User Parameters vector with database values
  if(verbose==true){cout<<"Calling getUserParams"<<endl;}
  userParams = getUserParams();


//Determine Day of Week
// const string DAY[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        time_t rawtime;
        tm * timeinfo;
        time(&rawtime);
        timeinfo=localtime(&rawtime);
    int wday=timeinfo->tm_wday; //Day of week Sun=0,Sat=7
    if(verbose==true){cout<<"Day of week "<<wday<<endl;}

//Print user parameters
if(verbose==true){cout<<"Listing Stored User Params"<<endl;
 for (unsigned i=0; i<userParams.size(); i++)
 {
   cout << "ROW" << i << "::";
   cout << "paramType " << userParams[i].paramType << " ";
   cout << "userValue " <<  userParams[i].userValue << endl ;
 }
}

//Check for On Demand requests
if(verbose==true){cout<<"Calling checkOnDemand"<<endl;}
checkOnDemand();

// sendEmail(0);

//Disconnect from SQL database
disconnectFromDatabase();

return 0;
}

//===============================

void checkOnDemand(){

  vector<OnDemand> demand; //vector for on demand requests
  if(verbose==true){cout<<"Fetching On Demand"<<endl;}

    //Connect to database if not connected
    if(connection==NULL){connectToDatabase();}

    if(connection != NULL)
    {
        //Retrieve all data
        if(mysql_query(connection, "SELECT * FROM on_demand"))
        {
            cout <<"ONDEMAND :: ";
            handleDBErr(connection);
            cout << endl;
        }

        MYSQL_RES *result = mysql_store_result(connection);

	//If there is data, handle it
        if(result != NULL)
        {

            //Process all the rows in table
            MYSQL_ROW row;
            while((row = mysql_fetch_row(result)))
            { 
                    int tempPillNumber = std::stoi (row[1]);
                    int tempPetNumber = std::stoi (row[2]);
                    OnDemand tempDemand (tempPillNumber, tempPetNumber); //create temp vector
                    demand.push_back(tempDemand); //add temp vector to full set
            } //end while
            
            mysql_free_result(result);
        

    	    //Delete all data
            if(mysql_query(connection, "DELETE * FROM on_demand"))
            {
              cout <<"ONDEMAND DEL :: ";
              handleDBErr(connection);
              cout << endl;
            }

//Print and delete on demand vector
for (unsigned j=demand.size(); j!=0; j--)
 {
   if(verbose==true){
     cout << "Delete ondemand ROW" << j << "::";
     cout << "petNumber " << demand[j].petNumber << " ";
     cout << "pillNumber " <<  demand[j].pillNumber << endl;
   }
   demand.pop_back();//delete last element
 }//end for
			   
        }//end result !=null
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

  vector<UserParam> params; //vector for user parameters
  if(verbose==true){cout<<"Fetching User Params"<<endl;}

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
            //Get the number of columns
         //   int num_fields = mysql_num_fields(result);

            //Get all the rows in table
            MYSQL_ROW row;
          //  int i = 0;
            while((row = mysql_fetch_row(result))){
                 int tempType = std::stoi (row[0]);
                 string tempValue = row[1];
                 UserParam tempParam (tempType, tempValue); //create temp vector
                 params.push_back(tempParam); //add temp vector to full set
                 if(verbose==true){cout << "Added: "<<tempType<<tempValue<<endl;}
           //      i++;
                }//end while

            mysql_free_result(result);
         }//end if result!=null
      }//end if connection!=null   
       
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


//send appropriate email
  if (system(NULL)) //a command processor is available
  { 
	if (emailType==0){system (message0);}
	if (emailType==1){system (message1);}
	if (emailType==2){system (message2);}
	if (emailType==3){system (message3);}
  }
  //future work:consider lighting error led if no command proc avail
}
//===================================

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
