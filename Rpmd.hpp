#ifndef RPMD_H
#define RPMD_H

#include <ctime>
#include <string>


class Schedule{

private:
	int pType;
        std::string uValue;
        int stringToInt(std::string);

public:
	int pillNumber;
        int petNumber;
        std::string weekDay;
        std::string deliverTime;
        std::string lastDate;

	Schedule(); //default constructor 
	Schedule(int nTempPill, int nTempPet, std::string nTempDay, std::string nTempTime)
             {pillNumber=nTempPill;
              petNumber=nTempPet;
              weekDay=nTempDay;
              deliverTime=nTempTime;
              lastDate="NOT RUN";
              }

        std::string toString();

};


class UserParam{

private:
	 int pType;
        std::string uValue;
        int stringToInt(std::string);

public:
	int paramType; //0=schedule change, 1=email
	std::string userValue;

	UserParam(); //default constructor 
	UserParam(int nParamType, std::string nUserValue)
        {paramType=nParamType; userValue=nUserValue;}
        std::string toString();

};

class OnDemand{

private:
	int petNum;
       int pillNum;
       int stringToInt(std::string);

public:
	int petNumber; //0=schedule change, 1=email
	int pillNumber;

	OnDemand(); //default constructor 
	OnDemand(int nPetNumber, int nPillNumber)
        {petNumber=nPetNumber; pillNumber=nPillNumber;}
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
