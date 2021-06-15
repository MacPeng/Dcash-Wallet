#define RAPIDJSON_HAS_STDSTRING 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "WwwFormEncodedDict.h"
#include "HttpClient.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

int API_SERVER_PORT = 8080;
string API_SERVER_HOST = "localhost";
string PUBLISHABLE_KEY = "";

string auth_token;
string user_id;
bool out;//check logout

vector<string> strr(string str){
    istringstream s(str);
    vector<string> input;
    string command;
    while(s >> command){
        input.push_back(command);
    }
    return input;
}

bool server(HTTPClientResponse *response){//check client response
    if (response->success()){
        return false;
    }
    else{
        return true;
    }
}

void logout(){//for logging out
    int hostSize = API_SERVER_HOST.length();
    char host[hostSize + 1];
    strcpy(host, API_SERVER_HOST.c_str());
    HttpClient deleteClient(host, API_SERVER_PORT, false);
    deleteClient.set_header("x-auth-token", auth_token);
    deleteClient.del("/auth-tokens/"+ auth_token);
    out = true;
}

void printBalance(HTTPClientResponse *response) {//for printing balance with precisions
    Document *doc = response->jsonBody();
    int balance = (*doc)["balance"].GetInt();
    delete doc;
    float show =  balance / 100;
    cout << "Balance: $" << setprecision(2) << fixed << show << endl;
}

void auth(vector<string> input){
    string username = input[1];
    string password = input[2];
    string email = "";
    if (input.size() == 4){
        email = input[3];
    }
    else{
        email = " ";
    }
    HTTPClientResponse *response;
    WwwFormEncodedDict body;
    body.set("username", username);
    body.set("password", password);
    string encodedbody = body.encode();
    
    int hostSize = API_SERVER_HOST.length();
    char host[hostSize + 1];
    strcpy(host, API_SERVER_HOST.c_str());
    
    HttpClient authCall(host, API_SERVER_PORT, false);
    response = authCall.post("/auth-tokens", encodedbody);
    if (server(response)){
        cout<<"Error"<<endl;
    }
    else{
        Document *d = response->jsonBody();
        string authTokenFromResponse = (*d)["auth_token"].GetString();
        string userIDFromResponse = (*d)["user_id"].GetString();
        delete d;
        
        if (auth_token.length() != 0){//has stuff in auth_token then delete user
            HttpClient deleteCall(host, API_SERVER_PORT, false);
            deleteCall.set_header("x-auth-token", auth_token);
            response = deleteCall.del("/auth-tokens/" + auth_token);
            if (server(response)){
                cout<<"Error"<<endl;
                return;
            }
        }
        auth_token = authTokenFromResponse;
        user_id = userIDFromResponse;
        WwwFormEncodedDict emailInfo;
        emailInfo.set("email", email);
        encodedbody = emailInfo.encode();
        HttpClient emailClient(host, API_SERVER_PORT, false);
        emailClient.set_header("x-auth-token", auth_token);
        response = emailClient.put("/users/" + user_id, encodedbody);
        if (server(response)){
            cout<<"Error"<<endl;
            return;
        }
        printBalance(response);
    }
}

void balance(vector<string> input){// for make balance
    HTTPClientResponse *response;
    int hostSize = API_SERVER_HOST.length();
    char host[hostSize + 1];
    strcpy(host, API_SERVER_HOST.c_str());
    HttpClient a(host, API_SERVER_PORT, false);
    a.set_header("x-auth-token", auth_token);
    response = a.get("/users/" + user_id);
    if (server(response)){
        cout<<"Error"<<endl;
        return;
    }
    printBalance(response);
}

void deposit(vector<string> input){// for deposite
    int count = stoi(input[1]) * 100;
    string cardNumber = input[2];
    string yearEXP = input[3];
    string monthEXP = input[4];
    string CVC = input[5];
    string encodedbody;
    HTTPClientResponse *response;
    int hostSize = API_SERVER_HOST.length();
    char host[hostSize + 1];
    strcpy(host, API_SERVER_HOST.c_str());
    
    if (cardNumber.length() != 16){
        cout<<"Error"<<endl;
        return;
    }
    if (count < 0){
        cout<<"Error"<<endl;
        return;
    }
    
    HttpClient stripeCall("api.stripe.com", 443, true);
    stripeCall.set_header("Authorization", string("Bearer ") + PUBLISHABLE_KEY);
    WwwFormEncodedDict body;
    body.set("card[number]", cardNumber);
    body.set("card[exp_year]", yearEXP);
    body.set("card[exp_month]", monthEXP);
    body.set("card[cvc]", CVC);
    encodedbody = body.encode();
    response = stripeCall.post("/v1/tokens", encodedbody);
    
    if (server(response)){
        cout<<"Error"<<endl;
        return;
    }
    Document *d = response->jsonBody();
    string stripeTokenFromResponse = (*d)["id"].GetString();
    delete d;
    
    WwwFormEncodedDict depositInfo;
    depositInfo.set("amount", count);
    depositInfo.set("stripe_token", stripeTokenFromResponse);
    encodedbody = depositInfo.encode();
    HttpClient depositCall(host, API_SERVER_PORT, false);
    depositCall.set_header("x-auth-token", auth_token);
    response = depositCall.post("/deposits", encodedbody);
    
    if (server(response)){
        cout<<"Error"<<endl;
        return;
    }
    
    printBalance(response);
}

void sendMoney(vector<string> input){// for sending money
    HTTPClientResponse *response;
    int hostSize = API_SERVER_HOST.length();
    char host[hostSize + 1];
    strcpy(host, API_SERVER_HOST.c_str());
    string sendToUser = input[1];
    int count = stoi(input[2]) * 100;
    string encodedbody;
    WwwFormEncodedDict depositInfo;
    depositInfo.set("to", sendToUser);
    depositInfo.set("amount", count);
    encodedbody = depositInfo.encode();
    HttpClient transferPost(host, API_SERVER_PORT, false);
    transferPost.set_header("x-auth-token", auth_token);
    response = transferPost.post("/transfers", encodedbody);
    
    if (server(response)){
        cout<<"Error"<<endl;
        return;
    }
    
    printBalance(response);
}


void execute(vector<string> input){// manipulate diff executable cmds
    if (input[0] == "logout"){
        if (input.size() != 1){
            cout<<"Error"<<endl;
            return;
        }
        logout();
    }
    else if (input[0] == "auth"){
        if (input.size() < 3){
            cout<<"Error"<<endl;
            return;
        }
        auth(input);
    }
    else if (input[0] == "balance"){
        if (input.size() != 1){
            cout<<"Error"<<endl;
            return;
        }
        balance(input);
    }
    else if (input[0]== "deposit"){
        if (input.size() != 6){
            cout<<"Error"<<endl;
            return;
        }
        deposit(input);
    }
    else if (input[0] == "send"){
        if (input.size() != 3){
            cout<<"Error"<<endl;
            return;
        }
        sendMoney(input);
    }
}

int main(int argc, char *argv[]) {
    stringstream config;
    int fd = open("config.json", O_RDONLY);
    if (fd < 0) {
        cout << "could not open config.json" << endl;
        exit(1);
    }
    int ret;
    char buffer[4096];
    while ((ret = read(fd, buffer, sizeof(buffer))) > 0) {
        config << string(buffer, ret);
    }
    
    Document d;
    d.Parse(config.str());
    API_SERVER_PORT = d["api_server_port"].GetInt();
    API_SERVER_HOST = d["api_server_host"].GetString();
    PUBLISHABLE_KEY = d["stripe_publishable_key"].GetString();
    vector<string> input;
    string str;
    
    if (argc == 1){
        cout <<"D$>";
        while(getline(cin, str)){
            if (!out){
                input = strr(str);
                if (input.size() == 0){
                    cout<<"Error"<<endl;
                }
                else{
                    execute(input);
                }
                cout<<"D$>";
            }
        }
    }
    else if (argc == 2){
        ifstream inputFile;
        inputFile.open(argv[1]);
        if (!inputFile){
            cout<<"Error"<<endl;
        }
        else{
            while(getline(inputFile, str)){
                input = strr(str);
                if (input.size() != 0){
                    execute(input);
                }
                else{
                    cout<<"Error"<<endl;
                }
            }
        }
        inputFile.close();
    }
    return 0;
}


