#include "profile_mgmt_client.h"


std::wstring request_body;
credentials cred;

void print_out(const wchar_t* output)
{
	wcout << output;
	wcout.flush();
}

void uriCreation(REST_parameters* rest, ghttp_type req_type,string rest_element,string method_name,string rest_id)
{
  rest->req_type=req_type;
  rest->rest_element=rest_element;
  rest->method_name=method_name;
  rest->id=rest_id;
  rest->uri=string("http://192.16.124.211/REST/"+rest_element+rest_id+method_name);
  cout<<rest->uri<<endl;
} 

wchar_t *convertToWchar(string str)
{

char *str1=(char*)str.c_str();
wchar_t *w_data = (wchar_t*)malloc((strlen(str1)+1) * sizeof(wchar_t));
mbstowcs(w_data,str1,(strlen(str1)+1));
return w_data;


}

char *convertToChar(wstring str2)
{
wchar_t *str=(wchar_t*)str2.c_str();
char *str1=(char*)malloc((wcslen(str)+1) * sizeof(char));
wcstombs(str1,str,(wcslen(str)+1));
return str1;

}

int requestProcessing(REST_parameters*rest,wstring request_body,Resp *res)
{
  
  //cout<<"inside request processing"<<endl;
  ghttp_request *request = NULL;
  request = ghttp_request_new();
  ghttp_uri_validate((char*)rest->uri.c_str());
  ghttp_set_uri(request,(char*)rest->uri.c_str());
  ghttp_set_type(request,rest->req_type);
  ghttp_set_authinfo(request,(char*)cred.cred_username.c_str(),(char*)cred.cred_password.c_str());
  ghttp_set_body(request,convertToChar(request_body),wcslen(request_body.c_str()));
  ghttp_set_header(request,http_hdr_Connection, "Keep_Alive");
  ghttp_set_header(request,http_hdr_Content_Type,"application/json");
  ghttp_set_header(request,http_hdr_Server,"Apache/2.2.14 (Ubuntu)");
  ghttp_set_header(request,http_hdr_User_Agent,"Apache-HttpClient/4.0.3");
  ghttp_set_header(request,http_hdr_Host,"192.16.124.211");
  ghttp_set_header(request,http_hdr_Expect,"100-Continue");
  ghttp_prepare(request);
  ghttp_process(request);
  res->result=ghttp_get_body(request);
  cout<<"the response body(function)"<<endl<<res->result<<endl;
  res->status_code=ghttp_status_code(request);
  cout<<endl<<"response"<<res->status_code<<endl;
  ghttp_request_destroy(request);
  return res->status_code;
}


Resp nebula_register(profile *user_profile)
{
  cout<<"Registering User"<<endl;
  JSONValue *json_nebula_register;
  JSONValue username(convertToWchar(user_profile->username));
  JSONValue password(convertToWchar(user_profile->password));
  JSONValue fullName(convertToWchar(user_profile->fullName));
  JSONValue address(convertToWchar(user_profile->address));
  JSONValue email_address(convertToWchar(user_profile->email_address));
  JSONValue phoneNumber(convertToWchar(user_profile->phoneNumber));
  JSONObject json_register;
  json_register.insert(std::pair<wstring,JSONValue*>(L"username",&username));
  json_register.insert(std::pair<wstring,JSONValue*>(L"password",&password));
  json_register.insert(std::pair<wstring,JSONValue*>(L"fullName",&fullName));
  json_register.insert(std::pair<wstring,JSONValue*>(L"address",&address));
  json_register.insert(std::pair<wstring,JSONValue*>(L"email_address",&email_address));
  json_register.insert(std::pair<wstring,JSONValue*>(L"phoneNumber",&phoneNumber));
  json_nebula_register=new JSONValue(json_register);
  json_nebula_register->AsObject()=json_register;
  request_body=json_nebula_register->Stringify();
  REST_parameters rest;
  uriCreation(&rest,ghttp_type_post,"RESTProfiles/","","");
  cred.cred_username="";
  cred.cred_password="";
  Resp res; 
  requestProcessing(&rest,request_body,&res);
  json_nebula_register=JSON::Parse(convertToWchar(res.result));
  if(json_nebula_register==NULL)
  {
    cout<<"the parsing is not successfull\n";
  }
  else if(json_nebula_register->IsObject())
  {
   json_register=json_nebula_register->AsObject();
   if(json_register.find(L"id") != json_register.end() && json_register[L"id"]->IsNumber())
   {
	double user_id=(json_register[L"id"]->AsNumber());
	user_profile->userId=(int)user_id;
        cout<<"the result of the register is"<<user_profile->userId<<endl;
			
   }
   if(json_register.find(L"result") != json_register.end() && json_register[L"result"]->IsString())
   {
	wstring result=(json_register[L"result"]->AsString());
	res.result=convertToChar(result);
        cout<<"the result of the register is"<<res.result<<endl;
			
   }
  }
  return res;		

}



group_details * extractProfileInfo()
{
 int i;
 char* group_id; 
 REST_parameters rest;
 group_details *groups;
 groups=new group_details[200];
 uriCreation(&rest,ghttp_type_get,"RESTGroups/","","retrieveAllGroupsMembers/");
 Resp res;
 request_body =L"";
 requestProcessing(&rest,request_body,&res);
 JSONValue *login_output;
 JSONObject json_obj;
 login_output=JSON::Parse(res.result.c_str());
 if(login_output==NULL)
 {
    cout<<"the parsing is not successfull\n";
 }
 else if(login_output->IsObject())
 {
    json_obj=login_output->AsObject();
    JSONObject::iterator pos;
    for(pos = json_obj.begin(),i=0; pos != json_obj.end(); ++pos,++i)
    {
	groups[i].users = 0;
       groups[i].groupName=convertToChar((*pos).first);
       cout<<endl<<groups[i].groupName<<endl;
       if(pos->second->IsObject()) 
       { 
          JSONObject json_obj2 = pos->second->AsObject();
          groups[i].status=convertToChar(json_obj2[L"status"]->AsString().c_str());
          cout<<"status is "<<groups[i].status<<endl;
       	  group_id=convertToChar(json_obj2[L"id"]->AsString().c_str());
	  groups[i].groupId=atoi(group_id);
          cout<<"group id is "<<groups[i].groupId<<endl;		
	  JSONObject::iterator iter = json_obj2.begin();
	  for(; iter != json_obj2.end(); ++iter) 
          {
		int j = 0;
	     if(iter->second->IsObject()) 
            { 
             JSONObject json_obj3 = iter->second->AsObject();
	     JSONObject::iterator iter1 = json_obj3.begin();
	     {
		cout <<"------------------" << (convertToChar(iter1->first)) << "--------------------" << endl;
	        groups[i].user_profile[j].username=convertToChar(json_obj3[L"username"]->AsString().c_str());
                groups[i].user_profile[j].address=convertToChar(json_obj3[L"address"]->AsString().c_str());
                groups[i].user_profile[j].domain=convertToChar(json_obj3[L"domain"]->AsString().c_str());
                groups[i].user_profile[j].fullName=convertToChar(json_obj3[L"fullName"]->AsString().c_str());
                groups[i].user_profile[j].phoneNumber=convertToChar(json_obj3[L"phoneNumber"]->AsString().c_str());
                groups[i].user_profile[j].groupName=convertToChar(json_obj3[L"groupName"]->AsString().c_str());
                groups[i].user_profile[j].status=convertToChar(json_obj3[L"status"]->AsString().c_str());
                groups[i].user_profile[j].email_address=convertToChar(json_obj3[L"email_address"]->AsString().c_str());
                groups[i].user_profile[j].userId=atoi(convertToChar(json_obj3[L"id"]->AsString().c_str()));
		groups[i].users++;
          
            }
				
            } else { }
		++j;
          }
	} else { }
     }
   }
   return groups;

}

int nebula_login(credentials*cred1,group_details *grp)
{

  cout<<"inside nebula login"<<endl;
  cred.cred_username=cred1->cred_username;
  cred.cred_password=cred1->cred_password;
  REST_parameters rest;
  uriCreation(&rest,ghttp_type_get,"RESTGroups/","","retrieveAllGroupsMembers/");
  Resp res;
  /*requestProcessing(&rest,request_body,&res);
  if((res.status_code)==401)
  {
    std::cout << "Invalid username / password " << std::endl;
    return 600;
  }*/
  grp=extractProfileInfo(); 
  return 0;
}

Resp nebula_addContact(string contactName,string nickName)
{
  cout<<"Adding a contact"<<endl; 
  JSONValue *json_nebula_addContact;
  JSONValue contactname(convertToWchar(contactName));
  JSONValue nickname(convertToWchar(nickName));
  JSONObject json_add_contact;
  json_add_contact.insert(std::pair<wstring,JSONValue*>(L"username",&contactname));
  json_add_contact.insert(std::pair<wstring,JSONValue*>(L"nickname",&nickname));
  json_nebula_addContact=new JSONValue(json_add_contact);
  json_nebula_addContact->AsObject()=json_add_contact;
  request_body=json_nebula_addContact->Stringify();
  cout<<"----------------------"<<convertToChar(request_body)<<"---------------"<<endl;
  REST_parameters rest;
  uriCreation(&rest,ghttp_type_post,"RESTGroups/","","addContact/");
  Resp res;
  requestProcessing(&rest,request_body,&res);
  cout<<"the reponse code is  "<<res.status_code<<endl;
  json_nebula_addContact=JSON::Parse(convertToWchar(res.result));
  if(json_nebula_addContact==NULL)
  {
    cout<<"the parsing is not successfull\n";
  }
  else if(json_nebula_addContact->IsObject())
  {
   json_add_contact=json_nebula_addContact->AsObject();
   if(json_add_contact.find(L"result") != json_add_contact.end() && json_add_contact[L"result"]->IsString())
   {
	wstring result=(json_add_contact[L"result"]->AsString());
	res.result=convertToChar(result);
        cout<<"the result of adding contact is  "<<res.result<<endl;
			
   }
  }
  return res;
}



Resp nebula_addGroup(group_details *grp)
{
  cout<<"Adding group"<<endl; 
  JSONValue *json_nebula_addGroup;
  JSONValue groupname(convertToWchar(grp->groupName));
  JSONValue status(convertToWchar(grp->status));
  JSONObject json_add_group;
  json_add_group.insert(std::pair<wstring,JSONValue*>(L"groupName",&groupname));
  json_add_group.insert(std::pair<wstring,JSONValue*>(L"status",&status));
  json_nebula_addGroup=new JSONValue(json_add_group);
  json_nebula_addGroup->AsObject()=json_add_group;
  request_body=json_nebula_addGroup->Stringify();
  cout<<"----------------------"<<convertToChar(request_body)<<"---------------"<<endl;
  REST_parameters rest;
  uriCreation(&rest,ghttp_type_post,"RESTGroups/","","insertGroup/");
  Resp res;
  requestProcessing(&rest,request_body,&res);
  cout<<"the reponse code is"<<res.status_code<<endl;
  json_nebula_addGroup=JSON::Parse(convertToWchar(res.result));
  if(json_nebula_addGroup==NULL)
  {
    cout<<"the parsing is not successfull\n";
  }
  else if(json_nebula_addGroup->IsObject())
  {
   json_add_group=json_nebula_addGroup->AsObject();
   if(json_add_group.find(L"id") != json_add_group.end() && json_add_group[L"id"]->IsNumber())
   {
	double group_id=(json_add_group[L"id"]->AsNumber());
	grp->groupId=(int)group_id;
        cout<<"the result of the adding group is  "<<grp->groupId<<endl;
			
   }
   if(json_add_group.find(L"result") != json_add_group.end() && json_add_group[L"result"]->IsString())
   {
	wstring result=(json_add_group[L"result"]->AsString());
	res.result=convertToChar(result);
        cout<<"the result of the adding group is  "<<res.result<<endl;
   }
  }
 
  return res;
}

Resp nebula_insertUserIntoGroup(profile *user,group_details *grp)
{
  cout<<"inside insertintogroup"<<endl; 
  JSONValue *json_nebula_insertUser;
  JSONValue group_id((double)grp->groupId);
  JSONValue username(convertToWchar(user->username));
  JSONObject json_insertIntoGroup;
  json_insertIntoGroup.insert(std::pair<wstring,JSONValue*>(L"username",&username));
  json_insertIntoGroup.insert(std::pair<wstring,JSONValue*>(L"groupID",&group_id));
  json_nebula_insertUser=new JSONValue(json_insertIntoGroup);
  json_nebula_insertUser->AsObject()=json_insertIntoGroup;
  request_body=json_nebula_insertUser->Stringify();
  cout<<"----------------------"<<convertToChar(request_body)<<"---------------"<<endl;
  REST_parameters rest;
  uriCreation(&rest,ghttp_type_post,"RESTGroups/","","insertUserIntoGroup/");
  Resp res;
  requestProcessing(&rest,request_body,&res);
  cout<<"the reponse code is"<<res.status_code<<endl;
  json_nebula_insertUser=JSON::Parse(convertToWchar(res.result));
  if(json_nebula_insertUser==NULL)
  {
    cout<<"the parsing is not successfull\n";
  }
  else if(json_nebula_insertUser->IsObject())
  {
   if(json_insertIntoGroup.find(L"result") != json_insertIntoGroup.end() && json_insertIntoGroup[L"result"]->IsString())
   {
	wstring result=(json_insertIntoGroup[L"result"]->AsString());
	res.result=convertToChar(result);
        cout<<"the result of the adding group is  "<<res.result<<endl;
   }
  }
}

string intToString(int x)
{
       string r;
       stringstream s;

       s << x;
       r = s.str();
       return r;

}

Resp nebula_deleteContact(profile *user)
{
 REST_parameters rest;
 uriCreation(&rest,ghttp_type_delete,"RESTGroups/","/deleteContact/",intToString(user->userId));
 request_body=L"";
 Resp res;
 requestProcessing(&rest,request_body,&res);  
}

Resp nebula_deleteGroup(group_details *grp)
{  
 REST_parameters rest;
 uriCreation(&rest,ghttp_type_delete,"RESTGroups/","/deleteGroup/",intToString(grp->groupId));
 request_body=L"";
 Resp res;
 requestProcessing(&rest,request_body,&res);
}
 

void nebula_modifyContact()
{
 /* cout<<"inside nebula register"<<endl;
  JSONValue *json_nebula_modifyContact;
  JSONValue username(convertToWchar(user_profile->username));
  JSONValue password(convertToWchar(user_profile->password));
  JSONValue fullName(convertToWchar(user_profile->fullName));
  JSONValue address(convertToWchar(user_profile->address));
  JSONValue email_address(convertToWchar(user_profile->email_address));
  JSONValue phoneNumber(convertToWchar(user_profile->phoneNumber));
  JSONObject json_modify;
  json_modify.insert(std::pair<wstring,JSONValue*>(L"username",&username));
  json_modify.insert(std::pair<wstring,JSONValue*>(L"password",&password));
  json_modify.insert(std::pair<wstring,JSONValue*>(L"fullName",&fullName));
  json_modify.insert(std::pair<wstring,JSONValue*>(L"address",&address));
  json_modify.insert(std::pair<wstring,JSONValue*>(L"email_address",&email_address));
  json_modify.insert(std::pair<wstring,JSONValue*>(L"phoneNumber",&phoneNumber));
  printf("inserted\n");
  json_nebula_modifyContact=new JSONValue(json_modify);
  json_nebula_modifyContact->AsObject()=json_modify;
  request_body=json_nebula_modifyContact->Stringify();
  REST_parameters rest;
  uriCreation(&rest,ghttp_type_put,"RESTGroups/","modifyContact/","");
  Resp res; 
  requestProcessing(&rest,request_body,&res);*/

}




/*int main()
{
  profile user;
  
  user.username="boris";
  user.password="boris";
  user.fullName="Boris Ristov";
  user.address="Kista";
  user.phoneNumber="0046";
  user.email_address="ristov@kth.se";
  user.userId=73;
  //cout<<"goign to call register";
  //nebula_register(&user);
  
  cred.cred_username="saad";
  cred.cred_password="saad";
  nebula_login(&cred);
  //nebula_addContact("sukru","Sukru Senli");
  //nebula_retrieveGroups();
  group_details grp;
  grp.groupName="SALCAS";
  grp.status="available";
  grp.groupId=256;
   //nebula_addGroup(&grp);
  //nebula_insertUserIntoGroup(&user,&grp); 
  //nebula_deleteContact(&user);
  //nebula_deleteGroup(&grp);  
}
*/
