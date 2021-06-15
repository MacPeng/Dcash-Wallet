#include <iostream>
#include <map>

#include <stdlib.h>
#include <stdio.h>

#include "HttpService.h"
#include "ClientError.h"

using namespace std;

HttpService::HttpService(string pathPrefix) {
  this->m_pathPrefix = pathPrefix;
}

User *HttpService::getAuthenticatedUser(HTTPRequest *request)  {
    User *user;
    WwwFormEncodedDict requests;
    string authToken;
    string username;
    
    //have authtoken
    if(request->hasAuthToken()){
        authToken = request->getAuthToken();//to get authtoken
        //loop through checking user
        for(std::map<string, User*>::iterator i = m_db->auth_tokens.begin(); i != m_db->auth_tokens.end(); ++i){
            if (i->first == authToken){
                user = i->second;
            }
        }
        return user;
    }
    else{
        requests = request->formEncodedBody();//get request from encoded body
        username = requests.get("username");//get username
        //loop through checking user
        for(std::map<string, User*>::iterator i = m_db->users.begin(); i != m_db->users.end(); ++i){
            if (i->first == username){
                user = i->second;
                return user;
            }
        }
    }
    return NULL;
}

string HttpService::pathPrefix() {
    return m_pathPrefix;
}

void HttpService::head(HTTPRequest *request, HTTPResponse *response) {
    cout << "HEAD " << request->getPath() << endl;
    throw ClientError::methodNotAllowed();
}

void HttpService::get(HTTPRequest *request, HTTPResponse *response) {
    cout << "GET " << request->getPath() << endl;
    throw ClientError::methodNotAllowed();
}

void HttpService::put(HTTPRequest *request, HTTPResponse *response) {
    cout << "PUT " << request->getPath() << endl;
    throw ClientError::methodNotAllowed();
}

void HttpService::post(HTTPRequest *request, HTTPResponse *response) {
    cout << "POST " << request->getPath() << endl;
    throw ClientError::methodNotAllowed();
}

void HttpService::del(HTTPRequest *request, HTTPResponse *response) {
    cout << "DELETE " << request->getPath() << endl;
    throw ClientError::methodNotAllowed();
}

