#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

//bool errorCheck(User* transferFromUser, HTTPRequest *request, HTTPResponse *response, map<string, User*> users);
bool errorCheck(User *transferFromUser, HTTPRequest *request, HTTPResponse *response,  map<string, User*> users){//checking for diff errors
    WwwFormEncodedDict Requests = request->formEncodedBody();
    string transferToUsername = Requests.get("to");
    string amountToTransferStr = Requests.get("amount");
    //nothing shown
    if (transferToUsername == "" || amountToTransferStr == ""){
        throw ClientError::badRequest();
        return false;
    }
    std::string::size_type sz;
    int amount = stoi(amountToTransferStr, &sz);
    User *transferToUser = NULL;
    
    if (amount < 0 || (transferFromUser->balance - amount < 0)){//invalid balance
        throw ClientError::badRequest();
        return false;
    }
    for(std::map<string, User*>::iterator iter = users.begin(); iter != users.end(); ++iter){
        if (iter->first == transferToUsername){
            transferToUser = iter->second;
        }
    }
    if (transferFromUser == transferToUser){//transfer to self
        throw ClientError::badRequest();
        return false;
    }
    if (transferToUser == NULL){//no such user
        throw ClientError::notFound();
        return false;
    }
    return true;
}

TransferService::TransferService() : HttpService("/transfers") { }


void TransferService::post(HTTPRequest *request, HTTPResponse *response) {
    User *transferFromUser = getAuthenticatedUser(request);
    //dont have error
    if (errorCheck(transferFromUser, request, response, m_db->users)){
        WwwFormEncodedDict Requests = request->formEncodedBody();
        int amount = stoi(Requests.get("amount"));
        string userToTransfer = Requests.get("to");
        
        User *transferToUser;
        for(std::map<string,User*>::iterator it = m_db->users.begin(); it != m_db->users.end(); ++it){
            if (it->first == userToTransfer){
                transferToUser = it -> second;
            }
        }
        //make new teansfer
        Transfer *forTransfer = new Transfer();
        forTransfer->amount = amount;
        forTransfer->from = transferFromUser;
        forTransfer->to = transferToUser;
        transferFromUser->balance -= amount;//modify balance
        transferToUser->balance += amount;//modify balance
        m_db->transfers.push_back(forTransfer);
        Document document;
        Document::AllocatorType& a = document.GetAllocator();
        Value o;
        o.SetObject();
        Value array;
        array.SetArray();
        Value to;
        
        o.AddMember("balance", transferFromUser->balance, a);
        unsigned size = m_db->transfers.size();
        for(unsigned i = 0; i < size; i++){
            //get user deposit
            if (m_db->transfers[i]->from->username == transferFromUser->username && m_db->transfers[i]->to->username == transferToUser->username){
                to.SetObject();
                to.AddMember("from", m_db->transfers[i]->from->username, a);
                to.AddMember("to", m_db->transfers[i]->to->username , a);
                to.AddMember("amount", m_db->transfers[i]->amount , a);
                array.PushBack(to, a);
            }
        }
        
        o.AddMember("transfers", array, a);
        document.Swap(o);
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        document.Accept(writer);
        response->setContentType("application/json");
        response->setBody(buffer.GetString() + string("\n"));
    }
}
