#ifndef RPMD_H
#define RPMD_H

#include <ctime>
#include <string>

class UserParam{

private:
	int pType;
        std::string uValue;
        int stringToInt(std::string);

public:
	std::string paramType; //0=schedule change, 1=email
	std::string userValue;

	UserParam(); //default constructor 
	UserParam(std::string nParamType, std::string nUserValue)
        {paramType=nParamType; userValue=nUserValue;}
        std::string toString();

};

//void UserParam::setParam (char* nParamType, std::string nUserValue){
// paramType = nParamType;
// userValue = nUserValue;
//}

//class Schedule{
//    
//private:
//    int year, month, day;	//month = [1,12], day = [1, 31] for 31 day month
//    int hour, min, sec;	//hour = [0, 23], min = [0, 59], sec = [0, 59]
//    int pillNumber;
//    int petNumber;
//    int lastYear, lastMonth, lastDay;
//XXXX STOPPED HERE
  
#endif
