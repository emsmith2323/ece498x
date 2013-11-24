#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mysql.h>
#include <pthread.h>
#include <string>
#include <unistd.h>
#include <sstream>
#include <vector>

using namespace std;

//********Variable Definitions****************************************


#define SERVER "localhost"
#define USER "root"
#define PASSWORD "raspberry"
#define DATABASE "rpmd"

/*
 a MYSQL pointer to a MYSQL connection
 */
MYSQL *connection;

void connectToDatabase();
void disconnectFromDatabase();
void handleDBErr(MYSQL *con);

int main(){


    connectToDatabase();

    if(connection != NULL){
        //Retrieve all data
        if(mysql_query(connection, "SELECT * FROM pet_names")){
            cout <<"NAMES :: ";
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
                    tinyint pet_number = row[0]
                    string pet_name = row[1];
                    //cout << "Added: " << pet_name << endl;
                    i++;
                }else{
                    cout << "MySQL: Wrong number of columns." << endl;
                }
            }
            
            mysql_free_result(result);
            

            }

   }


    disconnectFromDatabase();
    return 0;
}


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

