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

#include <Rpmd.hpp>

using namespace std;

//Compiler Definitions

#define SERVER "localhost"
#define USER "root"
#define PASSWORD "raspberry"
#define DATABASE "rpmd"

vector<UserParam> userParams;

//MYSQL pointer to MYSQL connection
 
MYSQL *connection;

//Function Declarations

vector<UserParam> getUserParams();
void connectToDatabase();
void disconnectFromDatabase();
void handleDBErr(MYSQL *con);
void sendEmail(int emailType);

int main(){

  //Populate User Parameters vector with database values
  userParams = getUserParams();


//Get Day of Week
// const string DAY[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        time_t rawtime;
        tm * timeinfo;
        time(&rawtime);
        timeinfo=localtime(&rawtime);
    int wday=timeinfo->tm_wday; //Day of week Sun=0,Sat=7

//Print user parameters
for (unsigned i=0; i<userParams.size(); i++)
{
  cout << endl << wday ;
  cout << endl << "ROW" << i << "::";
  cout << "COL1: " << userParams[i].paramType << " ";
  cout << "COL2: " <<  userParams[i].userValue << endl ;
}

 sendEmail(0);

  return 0;
}

//===============================

void sendEmail(int emailType){

 //Review user parameters to find email address
 for (unsigned i=0; i<userParams.size(); i++)
 { 
    if(userParams[i].paramType==1)
    {
	//Set email address based on sql value
	string msgEmail = userParams[i].userValue;
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

}
//===================================

vector<UserParam> getUserParams(){
  vector<UserParam> params; //vector for user parameters

  connectToDatabase();

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
            int num_fields = mysql_num_fields(result);

            //Get all the rows
            MYSQL_ROW row;
            int i = 0;
            while((row = mysql_fetch_row(result))){
                if(num_fields == 2){
                    char* tempType = row[0];
                    string tempValue = row[1];
                    UserParam tempParam (tempType, tempValue);
                    params.push_back(tempParam);
                    cout << "Added: " << tempType << tempValue << endl;
                    i++;
                }else{
                    cout << "MySQL: Wrong number of columns." << endl;
                }
            }
            
            mysql_free_result(result);
            
            }
   }


    disconnectFromDatabase();

return params;
}
//============================


//Connect to MySQL Database identified by constants SERVER, USER, PASSWORD, DATABASE

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
        printf("Database connection successful.\n");
    }
}//================================


//Disconnect from Database

void disconnectFromDatabase(){
    mysql_close(connection);
    cout << "Disconnected from database." << endl;
}//=========================================

/**
 * Handle database errors. This function will print the error to the screen, 
 * close the connection to the database, and exit the program with an error.
 * @param con a MYSQL pointer to the connection object
 */
void handleDBErr(MYSQL *con){ 
    fprintf(stderr, "%s\n", mysql_error(con));
    //mysql_close(con);
    //exit(1);
}//=========================================


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
