#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

bool errorCheck(HTTPRequest *request, HTTPResponse *response);

DepositService::DepositService() : HttpService("/deposits") { }

void DepositService::post(HTTPRequest *request, HTTPResponse *response) {
    if (errorCheck(request, response)){
        WwwFormEncodedDict Requests = request->formEncodedBody();
        int amount = stoi(Requests.get("amount"));
        string stripeToken = Requests.get("stripe_token");
        User *getUser = getAuthenticatedUser(request);
        
        WwwFormEncodedDict body;
        body.set("amount", amount);
        body.set("source", stripeToken);
        body.set("currency", "usd");
        string encodedbody = body.encode();
        
        HttpClient client("api.stripe.com", 443, true);
        client.set_basic_auth(m_db->stripe_secret_key, "");
        HTTPClientResponse *client_response = client.post("/v1/charges", encodedbody);
        if (client_response->success()){
            Document *d = client_response->jsonBody();
            Deposit *newDeposit = new Deposit(); //make new deposit
            newDeposit->amount = (*d)["amount"].GetInt();
            getUser->balance += newDeposit->amount;
            newDeposit->to = getUser;
            newDeposit->stripe_charge_id = (*d)["id"].GetString();
            delete d;
            m_db->deposits.push_back(newDeposit);
            //rapid json reply
            Document document;
            Document::AllocatorType& a = document.GetAllocator();
            Value o;
            o.SetObject();
            o.AddMember("balance", getUser->balance, a);
            Value array;
            array.SetArray();
            for(unsigned i = 0; i < m_db->deposits.size(); i++){
                if (m_db->deposits[i]->to == getUser){//user deposit
                    Value to;
                    to.SetObject();
                    to.AddMember("to", m_db->deposits[i]->to->username, a);
                    to.AddMember("amount", m_db->deposits[i]->amount , a);
                    to.AddMember("stripe_charge_id", m_db->deposits[i]->stripe_charge_id , a);
                    array.PushBack(to, a);
                }
            }
            
            o.AddMember("deposits", array, a);
            document.Swap(o);
            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            document.Accept(writer);
            response->setContentType("application/json");
            response->setBody(buffer.GetString() + string("\n"));
            response->setStatus(200);
        }else{
            throw ClientError::badRequest();
        }
    }
}

bool errorCheck(HTTPRequest *request, HTTPResponse *response){//checking if has error
    WwwFormEncodedDict Requests = request->formEncodedBody();
    string getAmount = Requests.get("amount");
    string stripeToken = Requests.get("stripe_token");
    //nothing in stripetoken or getamount
    if (stripeToken == "" || getAmount == ""){
        throw ClientError::badRequest();
        return false;
    }
    std::string::size_type sz;
    int amount = std::stoi(getAmount, &sz);
    //error checking for amount<50
    if (amount < 50){
        throw ClientError::badRequest();
        return false;
    }
    //don't have authtoken
    if (!(request->hasAuthToken())){
        throw ClientError::badRequest();
    }
    
    return true;
}
