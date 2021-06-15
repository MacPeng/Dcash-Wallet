#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AccountService::AccountService() : HttpService("/users") {
    
}

void AccountService::get(HTTPRequest *request, HTTPResponse *response) {
    User *UserIDs;
    User *requestUser = getAuthenticatedUser(request);
    string email;
    int balance;
    if (requestUser == NULL){
        throw ClientError::badRequest();
    }
    else{
        vector<string> way = request->getPathComponents();
        string takeUserID = way.back();
        //check user
        for(std::map<string,User*>::iterator iter = m_db->users.begin(); iter != m_db->users.end(); ++iter){
            if (iter->second->user_id == takeUserID){
                UserIDs = iter -> second;
            }
        }
        if (UserIDs != requestUser){
            throw ClientError::forbidden();
        }
        else{
            //rapid json reply
            email = UserIDs->email;
            balance = UserIDs->balance;
            Document document;
            Document::AllocatorType& a = document.GetAllocator();
            Value o;
            o.SetObject();
            o.AddMember("balance", balance, a);
            o.AddMember("email", email, a);
            document.Swap(o);
            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            document.Accept(writer);
            response->setContentType("application/json");
            response->setBody(buffer.GetString() + string("\n"));
        }
    }
}

void AccountService::put(HTTPRequest *request, HTTPResponse *response) {
    User *UserAuthTokens = getAuthenticatedUser(request);
    User *checkUserID;
    int balance;
    string userIDURL;
    vector<string> way = request->getPathComponents();
    
    //if has wuthtoken then get user info
    if (request->hasAuthToken()){
        WwwFormEncodedDict fullRequest = request->formEncodedBody();
        string email = fullRequest.get("email");
        userIDURL = way.back();
        if (email == ""){
            throw ClientError::badRequest();
        }
        else{
            // find user
            for(std::map<string, User*>::iterator iter1 = m_db->users.begin(); iter1 != m_db->users.end(); ++iter1){
                if (iter1->second->user_id == userIDURL){
                    checkUserID = iter1->second;
                }
            }
            if (checkUserID != UserAuthTokens){
                throw ClientError::unauthorized();
            }
            else{
                UserAuthTokens->email = email;
                balance = UserAuthTokens->balance;
                Document document;
                Document::AllocatorType& a = document.GetAllocator();
                Value o;
                o.SetObject();
                o.AddMember("balance", balance, a);
                o.AddMember("email", email, a);
                document.Swap(o);
                StringBuffer buffer;
                PrettyWriter<StringBuffer> writer(buffer);
                document.Accept(writer);
                response->setContentType("application/json");
                response->setBody(buffer.GetString() + string("\n"));
            }
        }
    }
    else{
        throw ClientError::badRequest(); //should have authtoken
    }
}

