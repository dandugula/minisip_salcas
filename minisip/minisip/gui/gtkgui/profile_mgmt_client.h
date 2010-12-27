#include<iostream>
#include<string>
#include<sstream>
#include <stdio.h>
#include<string.h>
#include <ghttp.h>
#include<string.h>
#include<stdlib.h>
#include"JSON.h"
#include"JSONValue.h"

using namespace std;

struct profile
{
 int userId;
 string username;
 string phoneNumber;
 string fullName;
 string email_address;
 string address;
 string domain;
 string status;
 string groupName;
 string password;

};

struct credentials
{
 string cred_username;
 string cred_password;
}; 

struct REST_parameters
{
 ghttp_type req_type;
 string rest_element;
 string method_name;
 string id;
 string uri;
};

struct Resp
{
 int status_code;
 string result;
};

struct group_details
{
 string groupName;
 string status;
 int groupId;
 struct profile user_profile[50];
 int users;
};



Resp nebula_register(profile *user_profile);
int nebula_login(credentials*cred1,group_details *grp);
Resp nebula_addContact(string contactName,string nickName);
Resp nebula_addGroup(group_details *grp);
Resp nebula_insertUserIntoGroup(string username,int groupId);
Resp nebula_deleteContact(profile *user);
Resp nebula_deleteGroup(group_details *grp);
group_details * extractProfileInfo();

