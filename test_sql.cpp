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

//Function Declaractions

vector<UserParam> getUserParams();
void connectToDatabase();
void disconnectFromDatabase();
void handleDBErr(MYSQL *con);

int main(){

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


//"echo \"this is the message body\" | mail -s \"subject\" eric1313@gmail.com"


string msgEmail = userParams[1].userValue;
string msgPart1 = "echo \"this is the message body\" | mail -s \"subject\" "+msgEmail; 
string message1 = msgPart1;
char * cmessage1 = new char [message1.length()+1];
std::strcpy (cmessage1, message1.c_str());

//send email
  system (cmessage1);

  return 0;
}

//===============================

vector<UserParam> getUserParams(){
  vector<UserParam> params; //vector for user parameters

  connectToDatabase();

    if(connection != NULL){
        //Retrieve all data
        if(mysql_query(connection, "SELECT * FROM user_params")){
            cout <<"PARAMSS :: ";
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


/**
 * Connect to the MySQL Database identified by the constants SERVER, USER, 
 * PASSWORD, DATABASE.
 */
void connectToDatabase(){
    //initialize MYSQL object for connections
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

/**
 * Disconnect from the MySQL Database identified by the constants SERVER, USER,
 * PASSWORD, DATABASE
 */
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
