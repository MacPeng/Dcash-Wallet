#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

int errorCheck(HTTPResponse *response, string username, string password, User *user = NULL);
AuthService::AuthService() : HttpService("/auth-tokens") {
    
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response) {
    User *userExists = getAuthenticatedUser(request);
    int error = 0;
    string username;
    string password;
    string currUserID;
    string currUserToken;
    WwwFormEncodedDict Requests;
    
    Requests = request->formEncodedBody();
    username = Requests.get("username");
    password = Requests.get("password");
    if (userExists != NULL){ //has userexists
        currUserID = userExists->user_id;
        currUserToken = StringUtils::createAuthToken();
        error = errorCheck(response, username, password, userExists);
        if (error == 0) {
            //insert info to the data
            m_db->auth_tokens.insert(std::pair <string, User*>(currUserToken, userExists));
            //rapid json reply
            Document document;
            Document::AllocatorType& a = document.GetAllocator();
            Value o;
            o.SetObject();
            o.AddMember("auth_token", currUserToken, a);
            o.AddMember("user_id", currUserID, a);
            document.Swap(o);
            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            document.Accept(writer);
            response->setContentType("application/json");
            response->setBody(buffer.GetString() + string("\n"));
        }
    }
    else{
        int error = 0;
        User *newUser = new User;
        newUser->user_id = StringUtils::createUserId();
        newUser->username = username;
        newUser->password = password;
        error = errorCheck(response, username, password);
        if (error == 0){
            //rapid json reply
            currUserID = newUser->user_id;
            currUserToken = StringUtils::createAuthToken();
            m_db->auth_tokens.insert(std::pair <string, User*>(currUserToken, newUser));
            m_db->users.insert(std::pair <string, User*>(username, newUser));
            Document document;
            Document::AllocatorType& a = document.GetAllocator();
            Value o;
            o.SetObject();
            o.AddMember("auth_token", currUserToken, a);
            o.AddMember("user_id", currUserID, a);
            document.Swap(o);
            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            document.Accept(writer);
            response->setContentType("application/json");
            response->setBody(buffer.GetString() + string("\n"));
            response->setStatus(201);
        }
    }
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response) {
    string deleteAuthToken;
    User *deleteUser;
    User *getUser;
    if (request->hasAuthToken()){
        vector<string> path = request->getPathComponents();
        deleteAuthToken = path.back();
        getUser = getAuthenticatedUser(request);
        //check delete user
        for(std::map<string,User*>::iterator it = m_db->auth_tokens.begin(); it != m_db->auth_tokens.end(); ++it){
            if (it->first == deleteAuthToken){
                deleteUser = it -> second;
            }
        }
        if (getUser != deleteUser){
            throw ClientError::badRequest();
        }
        else{
            m_db->auth_tokens.erase(deleteAuthToken);
            response->setStatus(200);
        }
    }
    else{
        //no authtoken
        throw ClientError::badRequest();
    }
}

int errorCheck(HTTPResponse *response, string username, string password, User *user){
    //able to find user
    if (user){
        if (user->password != password){
            throw ClientError::forbidden();
            return 1;
        }
    }
    //nothing shown for username or password
    if ((username == "") || (password == "")){
        throw ClientError::badRequest();
        return 1;
    }
    //check username
    int size = username.size();
    char tempUsername[size];
    strcpy(tempUsername, username.c_str());
    //loop through username
    for (int i = 0; i < size; i++){
        if(!islower(tempUsername[i])){
            throw ClientError::badRequest();
            return 1;
        }
    }
    return 0;
}
