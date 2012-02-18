/*
Copyright (c) 2012 Vladimir Jimenez
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


*** Flag Reset Timer Details ***
Author:
Vladimir Jimenez (allejo)

Description:
Plugin will reset all unused flags except for team flags on a CTF map after a
certain amount of seconds. Plugin will also reset all unused flags except on a
FFA map after a certain amount of seconds. Plugin has one parameter when being
loaded.
 
-loadplugin /path/to/flagResetTimer.so,<time in minutes>

Slash Commands:
/flagresetunused
/frsettime <time in minutes>
/frcurrenttime

License:
BSD

Version:
2.0
*/

#include "bzfsAPI.h"
#include "plugin_utils.h"

#define TOTAL_CTF_TEAMS 4

int timeLimitMinutes = 5;
std::string gameStyle = "ffa";
double nextReset = 0;

class flagResetTimerHandler : public bz_Plugin , public bz_CustomSlashCommandHandler
{
public:
	virtual const char* Name (){return "Flag Reset Timer";}
	virtual void Init (const char* config);
	virtual void Event(bz_EventData *eventData);
	virtual void Cleanup ();
	virtual bool SlashCommand (int playerID, bz_ApiString, bz_ApiString, bz_APIStringList*);
};

BZ_PLUGIN(flagResetTimerHandler);

unsigned int getNumTeams()
{
  return TOTAL_CTF_TEAMS - (!bz_getTeamPlayerLimit(eRedTeam) +
                            !bz_getTeamPlayerLimit(eGreenTeam) +
                            !bz_getTeamPlayerLimit(eBlueTeam) +
                            !bz_getTeamPlayerLimit(ePurpleTeam));
}

double ConvertToInteger(std::string msg){
	int msglength = (int)msg.length();
	if (msglength > 0 && msglength < 4){
		double msgvalue = 0;
		double tens = 1;
		for ( int i = (msglength - 1); i >= 0; i-- ){
			if (msg[i] < '0' || msg[i] > '9')
				return 0;
			tens *= 10;
			msgvalue +=  (((double)msg[i] - '0') / 10) * tens;
		}
		if (msgvalue >= 1 && msgvalue <= 120)
			return msgvalue;
	}
	return 0;
}

void flagResetTimerHandler::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
	{
		case bz_eTickEvent:
		{
			double timeLimitSeconds = timeLimitMinutes*60;
			std::string flagname = bz_getName(0).c_str();
			
			if(flagname=="R*" || flagname=="G*" || flagname=="B*" || flagname=="P*"){
				gameStyle = "ctf";
			}
			
			if(nextReset<bz_getCurrentTime()){
				if(gameStyle=="ctf"){
					for(unsigned int i = getNumTeams(); i < bz_getNumFlags(); i++){
  						if(bz_flagPlayer(i)==-1)
      						bz_resetFlag(i);
  					}
				}
				else{
					for(unsigned int i = 0; i < bz_getNumFlags(); i++){
  						if(bz_flagPlayer(i)==-1)
      						bz_resetFlag(i);
  					}
				}
				nextReset = bz_getCurrentTime()+timeLimitSeconds;
			}
			
		}
		break;
		default:break;
	}
}

bool flagResetTimerHandler::SlashCommand (int playerID, bz_ApiString _command, bz_ApiString _message, bz_APIStringList *params)
{
	std::string command = _command.c_str();
	std::string message = _message.c_str();
		
    bz_BasePlayerRecord *playerdata;
    playerdata = bz_getPlayerByIndex(playerID);
    
	if(command == "flagresetunused" && (bz_hasPerm(playerID, "flagMaster")||bz_hasPerm(playerID, "FLAGMASTER"))){
		if(gameStyle=="ctf"){
			for(unsigned int i = getNumTeams(); i < bz_getNumFlags(); i++){
  				if(bz_flagPlayer(i)==-1)
      				bz_resetFlag(i);
  			}
		}
		else{
			for(unsigned int i = 0; i < bz_getNumFlags(); i++){
  				if(bz_flagPlayer(i)==-1)
      				bz_resetFlag(i);
  			}
		}
		nextReset = bz_getCurrentTime()+(timeLimitMinutes*60);
		return 1;
	}
	else if(command == "frsettime" && (bz_hasPerm(playerID, "flagMaster")||bz_hasPerm(playerID, "FLAGMASTER"))){
		double invalue = ConvertToInteger(message);
		if (invalue > 0){
			timeLimitMinutes=invalue;
			bz_sendTextMessagef (BZ_SERVER, BZ_ALLUSERS, "Flag reset time has been set to %i minutes by %s.", timeLimitMinutes,playerdata->callsign.c_str());
			nextReset = bz_getCurrentTime()+(timeLimitMinutes*60);
		}
		else{
			bz_sendTextMessagef (BZ_SERVER, playerID, "Flag reset time invalid: must be between 1 and 120 minutes.");
		}
		return 1;
	}
	else if(command == "frcurrenttime"){
		bz_sendTextMessagef (BZ_SERVER, playerID, "Current flag reset time is set to: %i minute(s).",timeLimitMinutes);
	}
	else{
		bz_sendTextMessage(BZ_SERVER,playerID,"You do not have the permission to run the flag reset commands.");
	}
	bz_freePlayerRecord(playerdata);
	return 1;
}

void flagResetTimerHandler::Init(const char* commandLine)
{
	bz_debugMessage(4,"flagResetTimer plugin loaded");
	Register(bz_eTickEvent);
	
	bz_registerCustomSlashCommand("flagresetunused", this);
	bz_registerCustomSlashCommand("frsettime", this);
	bz_registerCustomSlashCommand("frcurrenttime", this);
	
	if(&commandLine[0] != "" ){
		timeLimitMinutes=atoi(&commandLine[0]);
	}
	else{
		timeLimitMinutes=5;
	}
}

void flagResetTimerHandler::Cleanup(void)
{
	Flush();
	
	bz_removeCustomSlashCommand("flagresetunused");
	bz_removeCustomSlashCommand("frsetttime");
	bz_removeCustomSlashCommand("frcurrenttime");
	bz_debugMessage(4,"flagResetTimer plugin unloaded");
}
