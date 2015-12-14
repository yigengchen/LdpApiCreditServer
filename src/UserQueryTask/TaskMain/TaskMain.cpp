/*
 * TaskMain.cpp
 */

#include "TaskMain.h"
#include "../../CommonTools/UrlEncode/UrlEncode.h"
#include "../../UserQueryWorkThreads/UserQueryWorkThreads.h"

#include "../../CommonTools/Base64Encode/Base64.h"
#include "../../CommonTools/Base64Encode/Base64_2.h"
#include "../../../include/json/json.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/err.h>



extern CLog *gp_log;
const char* CTaskMain::m_pszHttpHeaderEnd = "\r\n\r\n";
const char* CTaskMain::m_pszHttpLineEnd = "\r\n";
const std::string CTaskMain::keyEdcpMd5Sign="edc_543_key_155&";
extern std::map<std::string,BDXPERMISSSION_S> g_mapUserInfo;
extern std::map<std::string,int> g_mapUserQueryLimit;
extern std::map<std::string,QUERYAPIINFO_S> g_vecUrlAPIS;


extern pthread_rwlock_t p_rwlock;
extern pthread_rwlockattr_t p_rwlock_attr;
extern pthread_mutex_t mutex;
extern std::string g_strTokenString ;
extern std::string ssToken;
extern u_int  g_iNeedUpdateToken ;
extern int iAPIQpsLimit;
extern std::map<int,std::string>mapIntStringOperator;

int InitSSLFlag = 0;

static const string http=" HTTP/1.1";

static const char http200ok[] = "HTTP/1.1 200 OK\r\nServer: Bdx DMP/0.1.0\r\nCache-Control: must-revalidate\r\nExpires: Thu, 01 Jan 1970 00:00:00 GMT\r\nPragma: no-cache\r\nConnection: Keep-Alive\r\nContent-Type: application/json;charset=UTF-8\r\nDate: ";
//static const char http200ok[] = "";
static const char httpReq[]="GET %s HTTP/1.1\r\nHost: %s\r\nAccept-Encoding: identity\r\n\r\n";

#define __EXPIRE__


CTaskMain::CTaskMain(CTcpSocket* pclSock):CUserQueryTask(pclSock)
{
	// TODO Auto-generated constructor stub
	m_piKLVLen = (int*)m_pszKLVBuf;
	m_piKLVContent = m_pszKLVBuf + sizeof(int);
	*m_piKLVLen = 0;
}

CTaskMain::CTaskMain()
{

}

CTaskMain::~CTaskMain() {
	// TODO Auto-generated destructor stub

}

int CTaskMain::BdxRunTask(BDXREQUEST_S& stRequestInfo, BDXRESPONSE_S& stResponseInfo)
{
	string keyReq = "Req_"+BdxTaskMainGetTime();
	string keyEmptyRes = "EmptyRes_"+BdxTaskMainGetTime();
	string strErrorMsg,errValue;
	string retKey,retKeyType,retUser;
    HIVELOCALLOG_S stHiveEmptyLog;
	int iRes = 0;
	if(!m_pclSock) {
		LOG(ERROR, "[thread: %d]m_pclSock is NULL.", m_uiThreadId);
		return LINKERROR;
	}

	iRes = 	BdxGetHttpPacket(stRequestInfo,stResponseInfo,retKey,retKeyType,retUser,strErrorMsg);	
	LOG(DEBUG,"BdxGetHttpPacket iRes=%d",iRes);
	//printf("strErrorMsg=%s\n",strErrorMsg.c_str());
	if(iRes == SUCCESS )//&& !stRequestInfo.m_strUserID.empty() /*&& m_bSend*/) 
	{
		return BdxSendRespones( stRequestInfo, stResponseInfo,strErrorMsg);
	}
	else
	{
				
		if ( iRes == OTHERERROR)
		{

			errValue = "{\r\n\"code\":\""+strErrorMsg+"\",\r\n\"msg\":\"authentication failure\",\r\n\"data\":\"\"\r\n}";		
		
			if(CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo.find(stResponseInfo.ssUserName)	
				!=	CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo.end())
			{

				m_pDataRedis->UserIncr(stResponseInfo.ssUserCountKeyEmptyRes); 
				CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stResponseInfo.ssUserName].m_ullEmptyResNum++;

			}
			
			printf("ssUserCountKeyEmptyRes=%s\n",stResponseInfo.ssUserCountKeyEmptyRes.c_str());
			if( stResponseInfo.queryType==2 )
			{
				m_pGoodsRedis->UserIncr(stResponseInfo.ssUserCountKeyEmptyRes);
			}
			else
			{
				m_pDataRedis->UserIncr(stResponseInfo.ssUserCountKeyEmptyRes);
			}
			
			//return BdxSendEmpyRespones(stResponseInfo);
		}
		else if ( iRes == EXCEEDLIMIT)
		{
			CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stResponseInfo.ssUserName].m_ullEmptyResNum++;
			m_pDataRedis->UserIncr(stResponseInfo.ssUserCountKeyEmptyRes);
		}
		else
		{

			stHiveEmptyLog.strValue=strErrorMsg;
			if(retKey.empty())
			{
				stHiveEmptyLog.strTelNo="Empty";
			}
			else
			{	
				stHiveEmptyLog.strTelNo=retKey;
			}
			if(retKeyType.empty())
			{
				stHiveEmptyLog.strValue="Empty";
			}
			else
			{	
				stHiveEmptyLog.strValue=retKeyType;
			}

			stHiveEmptyLog.strAuthId="Empty";
			stHiveEmptyLog.strCustName="Empty";
			stHiveEmptyLog.strQuerytime=BdxTaskMainGetFullTime();
			stHiveEmptyLog.strAccessKeyId=retUser;
			stHiveEmptyLog.strAction="Empty";
			stHiveEmptyLog.strSinature="Empty";
			stHiveEmptyLog.strDayId=BdxTaskMainGetDate();
			stHiveEmptyLog.strHourId=stHiveEmptyLog.strQuerytime.substr(8,2);	
			CUserQueryWorkThreads::m_vecHiveLog[m_uiThreadId].push(stHiveEmptyLog);	
			
			errValue = "{\r\n\"code\":\""+strErrorMsg+"\",\r\n\"msg\":\"authentication failure\",\r\n\"data\":\"\"\r\n}";		
			CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[""].m_ullReqNum++;
			CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[""].m_ullEmptyResNum++;
			m_pDataRedis->UserIncr(keyReq);
			m_pDataRedis->UserIncr(keyEmptyRes);
		}
			
			return BdxSendEmpyRespones(errValue);

	}
	return iRes;
}


int CTaskMain::BdxGetHttpPacket(BDXREQUEST_S& stRequestInfo,BDXRESPONSE_S &stResponseInfo,std::string &retKey,std::string &retKeyType,std::string &retUser,std::string &errorMsg)
{

	int iRes = 0;//,istrRemoteValueLength=0;
	//int iQueryCategory = 1,isError=0;
	m_httpType = 0;
	//bool bUpdateDatabase = false;
	//bool bQueryUser = false;
	//bool bQueryGoods = false;
	//bool bUserIdentity = false;
	int iIdentity=0; //10001 begin with 1 is verity,20002 beginwith 2 is query
	//int iQueryInfo=0;//
	//bool bTelVerify = false;
	int iNoDataFlag = 0,strSource;//,isQueryAction = 0,
	std::string strPrivateKey="8c42f63e-3611-11e5-8e67-dasdas448c8e";
	std::string	strMobile,strSign,strParams,strSelect,
	ssUser,ssValue,ssKey,ssmoidValue,strUser,filterDate,strToken,strKey,strKeyType,strKeyFilter,tempstrKeyFilter,strShopId,strGoodsId,strProvince,strOperator,strMoId;
	std::string strProvinceReq,strProvinceRes,strProvinceEmptyRes,strProvinceResTag,strOperatorName;
	std::string 
	strTimeStamp,strLiveTime,strAccessKeyId,strAccessPrivatekey,strSinature,strTelNo,strTelNoTemp,strMonth,strCertType,strCertCode,strUserName,strAuthId,strCustName,strUserIdentity,strUserTelVerity,strHost,strRemoteValue;
	std::map<std::string,std::string> map_UserValueKey;
	std::map<std::string,std::string>::iterator iter2;
	std::map<std::string,BDXPERMISSSION_S>::iterator iter;
	std::vector<std::string>::iterator itRights;
	int inUseTime;//iIncludeNoFields = 0;
	//int mapActionFilterSize;
	std::map<std::string,int> mapActionFilter;
	std::string mResUserAttributes,mResUserAttributes2,strTempValue;
	Json::Value jValue,jRoot,jResult,jTemp;
	
	int lenStrTemp;
	Json::FastWriter jFastWriter;
	//int isExpire = 0, iIsLocal=0,iRemoteApiFlag = 0;
	int iOperator = 0;
	string strCreateTime,strFirst3Bit,strCreateTime2,strAction,strLastDataTime,strLastDataTime2,mResValueLocal,mResValueRemote;
	//struct tm ts;
	//time_t timetNow,timetCreateTime,timetCreateTime2;
	//Json::Reader jReader;
	//Json::Reader *jReader= new Json::Reader(Json::Features::strictMode()); // turn on strict verify mode
	Json::Reader *jReader= new Json::Reader(Json::Features::all());
	char chHostName[30];
	char *temp[PACKET]; 
	int  index = 0;
	char bufTemp[PACKET];
	char *buf;
	char *outer_ptr = NULL;  
	char *inner_ptr = NULL;  
	char m_httpReq[_8KBLEN];
	char m_httpResValueE20003[_8KBLEN];
	memset(chHostName, 0,30);
	memset(m_pszAdxBuf, 0, _8KBLEN);
	memset(m_httpReq, 0, _8KBLEN);
	memset(m_httpResValueE20003, 0, _8KBLEN);
	//memset(buf, 0, _8KBLEN);
	memset(bufTemp, 0, PACKET);
	

	iRes = m_pclSock->TcpRead(m_pszAdxBuf, _8KBLEN);

  	LOG(DEBUG,"Requrest= %s\n",m_pszAdxBuf);  
	printf("Requrest= %s\n",m_pszAdxBuf);  
	if(iRes <= (int)http.length()) 
	{		
		LOG(DEBUG, "[thread: %d]Read Socket Error [%d].", m_uiThreadId, iRes);
		errorMsg="1100";
		return LINKERROR;
	}

	std::string ssContent = std::string(m_pszAdxBuf);
	std::string tempssContent,strActionUrl,strReqParams;
	unsigned int ipos = ssContent.find(CTRL_N,0);
	unsigned int jpos = ssContent.find(REQ_TYPE,0);
	
	if( std::string::npos !=jpos )
	{
		m_httpType = 1;
	}
	//printf("m_httpType=%d\n",m_httpType);
	if(m_httpType )
	{
		ssContent = ssContent.substr(jpos+4,ipos-http.length()-(jpos+4));
		//printf("ssContent2222=%s\n",ssContent.c_str());
		int ibegin = ssContent.find(SEC_Q,0);
		int iend =   ssContent.find(BLANK,0);
		//int iend = ssContent.rfind(BLANK,ssContent.length());
		strActionUrl =  ssContent.substr(0,ssContent.find(SEC_Q,0));
		
		if (ibegin!=-1 && iend !=-1)
		{

				ssContent = ssContent.substr(ibegin+1,iend - ibegin-1);	
				strReqParams = ssContent;
				memcpy(bufTemp,ssContent.c_str(),ssContent.length());
				buf=bufTemp;
				while((temp[index] = strtok_r(buf, STRING_AND, &outer_ptr))!=NULL)   
				{  	
				    buf=temp[index];  
				    while((temp[index]=strtok_r(buf, STRING_EQUAL, &inner_ptr))!=NULL)   
				    {   if(index%2==1)
				        {
				            map_UserValueKey[temp[index-1]]=temp[index];
				            
				            
				        }
				        index++;
				        buf=NULL;  
				    }  
				    buf=NULL;  
				}  

				if(map_UserValueKey.find(KEY_TEL_NO)!=map_UserValueKey.end())
				{
					strTelNo = map_UserValueKey.find(KEY_TEL_NO)->second;	
					strTelNoTemp=strTelNo;
					strFirst3Bit = strTelNo.substr(0,3);
					
					std::map<int,std::string>::iterator itOp;
					for(itOp=mapIntStringOperator.begin();itOp!=mapIntStringOperator.end();itOp++)
					{
						if(itOp->second.find(strFirst3Bit,0)!=std::string::npos)
						{
							iOperator = itOp->first;
							if( iOperator == 2 )
							{
								strSource = 1002;//zhongyi  zjyd
							}
							if( iOperator == 1 || iOperator == 3 )
							{
								strSource = 1001;//huayuan  dx ,lt
							}
							break;
						}
			
					}
				    if( iOperator==0 )
					{
						errorMsg ="unknow operator"; //param missing or error
						LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
						printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
						return ERRORPARAM;

					}

				}
				else
				{
					errorMsg ="1100"; //param missing or error
					LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
					printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
					return ERRORPARAM;
				}
				LOG(DEBUG,"strFirst3Bit=%s,iOperator=%d",strFirst3Bit.c_str(),iOperator);
				printf("Line:%d,strFirst3Bit=%s\n",__LINE__,strFirst3Bit.c_str());
				printf("Line:%d,iOperator=%d\n",__LINE__,iOperator);
				
				if(strcasecmp(strActionUrl.substr(0,strActionUrl.find(SEC_Q,0)).c_str(),KEY_USER_IDENTITY)== 0)
				{
					iIdentity = 10001;	//userIdentity  phone name idcard
					if( iOperator == 2 )// 2 zjyd ;1,dianxin 3 liantong
					{
						strParams = "/api/test?apiid=fbdsidentity";
						strSource = 1002;
					}
					if( iOperator == 1 || iOperator == 3 )
					{
						strParams = "/openapi/api/searchreport?";
					}
					strAction="userIdentity";
				}else if(strcasecmp(strActionUrl.substr(0,strActionUrl.find(SEC_Q,0)).c_str(),KEY_USER_IDENTITY_NAME)== 0)
				{
					iIdentity = 10002;	//userIdentityName  phone name
					
					if( iOperator == 2 )
					{
						strParams = "/api/test?apiid=fbdsidentityname";
					}
					if( iOperator == 1 || iOperator == 3 )
					{
						strParams = "/openapi/api/searchreport?";
					}
					
					strAction="userIdentityName";
				}else if(strcasecmp(strActionUrl.substr(0,strActionUrl.find(SEC_Q,0)).c_str(),KEY_USER_IDENTITY_ID)== 0)
				{
					iIdentity = 10003; //userIdentityID	phone idcard
					
					if( iOperator == 2 )
					{
						strParams = "/api/test?apiid=fbdsidentityphone";
					}
					if( iOperator == 1 || iOperator == 3 )
					{
						strParams = "/openapi/api/searchreport?";
					}
					
					strAction="userIdentityID";
				}else if(strcasecmp(strActionUrl.substr(0,strActionUrl.find(SEC_Q,0)).c_str(),KEY_TEL_VERIFY)== 0)
				{
					iIdentity = 20001;//telStatusVerify		status in use
					if( iOperator == 2 )
					{
						strParams = "/api/test?apiid=fbdsphonestatus";  //not in use
					}
					if( iOperator == 1 || iOperator == 3 )
					{
						strParams = "/openapi/api/searchreport?";
					}
					strAction="telStatusVerify";
				}else if(strcasecmp(strActionUrl.substr(0,strActionUrl.find(SEC_Q,0)).c_str(),KEY_TEL_LIFE_QUERY)== 0)
				{
					iIdentity = 20003;//telServiceLifeQuery	life in use
					if( iOperator == 2 )
					{
						strParams = "/api/test?apiid=fbdsservicelife";
					}
					if( iOperator == 1 || iOperator == 3 )
					{
						strParams = "/openapi/api/searchreport?";
					}
					
					strAction="telServiceLifeQuery";
				}
				else
				{
					errorMsg ="1100"; //request info is error  
					LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
					return ERRORPARAM;
				}

				
				if(map_UserValueKey.find(KEY_ACCESS_KEY_ID)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_TIME_STAMP)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_LIVE_TIME)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_TEL_NO)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_SIGNATURE)!=map_UserValueKey.end())
				{	
					strAccessKeyId = map_UserValueKey.find(KEY_ACCESS_KEY_ID)->second;
					strTimeStamp = map_UserValueKey.find(KEY_TIME_STAMP)->second;
					strLiveTime = map_UserValueKey.find(KEY_LIVE_TIME)->second;
					strSinature = map_UserValueKey.find(KEY_SIGNATURE)->second;
					strTelNo = map_UserValueKey.find(KEY_TEL_NO)->second;


					stResponseInfo.ssUserName=strAccessKeyId;//+"_"+strSinature;
					//stResponseInfo.ssUserCountKeyUserLimitReq = "Limit_"+BdxTaskMainGetDate()+"_"+strAccessKeyId+"_"+strAction;
					stResponseInfo.ssUserCountKeyUserLimitReq = "Limit_"+strAccessKeyId;
					stResponseInfo.ssOperatorNameKeyLimit = "Limit_"+BdxTaskMainGetTime()+"_"+strAccessKeyId;//+"_"+strAction;


					stResponseInfo.ssUserCountKeyReq = "Req_" + BdxTaskMainGetTime()+"_"+strAccessKeyId;//+"_"+strAction;
					stResponseInfo.ssUserCountKeyRes = "Res_" + BdxTaskMainGetTime()+"_"+strAccessKeyId;//"_"+strAction;
					stResponseInfo.ssUserCountKeyEmptyRes ="EmptyRes_"+BdxTaskMainGetTime()+"_"+ strAccessKeyId;//+"_"+strAction;
					//stResponseInfo.ssUserName = strUser+"_"+strKeyType;//map_UserValueKey.find(KEY_USER)->second ;

					
					m_pDataRedis->UserIncr(stResponseInfo.ssUserCountKeyReq); //req++
					CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stResponseInfo.ssUserName].m_ullReqNum++;
					//CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stResponseInfo.ssUserName].m_ullTotalReqNum++;
					if(m_pDataRedis->UserGet(stResponseInfo.ssUserCountKeyUserLimitReq,ssmoidValue)&&(g_mapUserInfo.find(map_UserValueKey.find(KEY_ACCESS_KEY_ID)->second)->second.mIntGoodsTimes>0))
				    {	
						if(atoi(ssmoidValue.c_str())>= g_mapUserInfo.find(KEY_ACCESS_KEY_ID)->second.mIntGoodsTimes)
						{
							errorMsg = "5000";//user query times limit
							LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
							return EXCEEDLIMIT;
						}
						else
						{
							m_pDataRedis->UserIncr(stResponseInfo.ssUserCountKeyUserLimitReq);
							CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stResponseInfo.ssUserName].m_ullResTagNum++;
							
						}
					}

					iter = g_mapUserInfo.find(strAccessKeyId);
					if(iter!=g_mapUserInfo.end())
					{	
					    if(iter->first == strAccessKeyId)
					    {					
					    	strAccessPrivatekey = iter->second.mResToken;
					    	LOG(DEBUG,"strAccessPrivatekey=%s",strAccessPrivatekey.c_str());
					    	printf("Line:%d,strAccessPrivatekey=%s\n",__LINE__,strAccessPrivatekey.c_str());
							if(	map_UserValueKey.find(KEY_TEL_NO)!= map_UserValueKey.end())
							{
								ssContent = strTelNo;
							}
							else
							{								
								errorMsg = "1100";  // request KEY_TEL_NO is missing  
								LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
								printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
								return OTHERERROR;
							}
						}
						else
						{
							errorMsg = "1200"; //accesskey id is not match
							LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
							printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());	
							return OTHERERROR;
						}
						
					}
					else
					{
						errorMsg ="1200"; //accekeykey is not exists
						LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
						printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
						return OTHERERROR;
					}
					//verify time ,verify signature
					time_t tmpTime = time(NULL);	//get the current time seconds
					long int diff = tmpTime - atoi(strTimeStamp.c_str());
					LOG(DEBUG,"tmpTime=%ld,diff=%ld,strLiveTime=%s",tmpTime,diff,strLiveTime.c_str());
					printf("tmpTime=%ld,diff=%ld,strLiveTime=%s\n",tmpTime,diff,strLiveTime.c_str());
					if( diff >= atoi(strLiveTime.c_str()))
					{
						errorMsg ="1300";
						LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
						printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
						return OTHERERROR;
					}
					std::string strMD5Signature;
					//transform(strSinature.begin(), strSinature.end(), strSinature.begin(), toupper)
					strMD5Signature = strTimeStamp + strAccessKeyId;
					strMD5Signature = BdxGetParamSign(strMD5Signature,strAccessPrivatekey);
					LOG(DEBUG,"strMD5Signature=%s",strMD5Signature.c_str());
					printf("Line:%d,strMD5Signature=%s\n",__LINE__,strMD5Signature.c_str());
					if(strMD5Signature.compare(strSinature)!=0)
					{
						errorMsg ="1200";
						LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
						printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
						return OTHERERROR;

					}	
					
				 }
				 else
				 {
						errorMsg ="1100"; //param missing or error
						LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
						printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
						return OTHERERROR;
				 }

				printf("Line:%d,iIdentity=%d\n",__LINE__,iIdentity);
				LOG(DEBUG,"iIdentity=%d",iIdentity);

				//strSelect = ;
				switch(iIdentity)
				{
					case 10001://userIdentity
						{
							string cardIDHash;
							string nameIDHash;
							//char idType[10];
							string idType;

								if(map_UserValueKey.find(KEY_CERT_TYPE)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_CERT_CODE)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_USER_NAME)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_AUTH_ID)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_CUST_NAME)!=map_UserValueKey.end())
								{	
									//strMonth = map_UserValueKey.find(KEY_MONTH)->second;
									strCertType = map_UserValueKey.find(KEY_CERT_TYPE)->second;
									strCertCode = map_UserValueKey.find(KEY_CERT_CODE)->second;
									strUserName = map_UserValueKey.find(KEY_USER_NAME)->second;
									strAuthId = map_UserValueKey.find(KEY_AUTH_ID)->second;
									strCustName = map_UserValueKey.find(KEY_CUST_NAME)->second;
								}
								else
								{
										errorMsg ="1100"; //param missing or error
										LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
										printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
										return OTHERERROR;
								}

							if(strCertType.compare("0101")==0)
							{
								//idType="…Ì∑›÷§";
								//sprintf(idType,"%c%c%c%c%c%c%c%c%c",0xE8,0xBA,0xBA,0xE4,0xBB,0xBD,0xE8,0xAF,0x81);
								idType="idCard";
								LOG(DEBUG,"idType=%s",idType.c_str());
								printf("Line:%d,idType=%s\n",__LINE__,idType.c_str());
							}
							if( iOperator == 1 )//|| iOperator == 3 ) //dx
							{
								cardIDHash = BdxGetParamSign(strCertCode,std::string(""));
								strUserName=url_decode(strUserName.c_str());
								printf("Line:%d,strUserName=%s\n",__LINE__,strUserName.c_str());
								nameIDHash = BdxGetParamSign(strUserName,std::string(""));
								printf("Line:%d,nameIDHash=%s\n",__LINE__,nameIDHash.c_str());
								printf("Line:%d,cardIDHash=%s\n",__LINE__,cardIDHash.c_str());
								strMobile =	strTelNo;
								std:: string tempStr="accountIDtest_customer";
								strSign= tempStr+"cardIDHash"+ cardIDHash+"idType"+idType+"mobile"+strMobile+"nameHash"+nameIDHash+"selectRZ007";
								LOG(DEBUG,"Before Md5 strSign=%s",strSign.c_str());
								strSign = BdxGetParamSign(strSign,strPrivateKey);
								LOG(DEBUG,"After  Md5 strSign=%s",strSign.c_str());
								printf("Line:%d,strSign=%s\n",__LINE__,strSign.c_str());
								strKey = strParams+"accountID=test_customer&select=RZ007&mobile="+strMobile+"&cardIDHash="+cardIDHash+"&nameHash="+nameIDHash+"&idType="+idType+"&sign="+strSign;

							}
							if( iOperator == 2 )//zjyd
							{
								strKey = strParams+"&phone="+strTelNo +"&name="+strUserName + "&idCard="+strCertCode+"&appkey=NXJjHy5nHwUfLmcf&userauthid="+strAuthId;

							}
							if( iOperator == 3 ) //lt
							{
								strMobile =	strTelNo;
								std:: string tempStr="accountIDtest_customer";
								strSign= tempStr+"cardID"+ strCertCode+"mobile"+strTelNo+"name"+strUserName+"selectRZ011";
								LOG(DEBUG,"Before Md5 strSign=%s",strSign.c_str());
								strSign = BdxGetParamSign(strSign,strPrivateKey);
								LOG(DEBUG,"After  Md5 strSign=%s",strSign.c_str());
								strKey = strParams+"accountID=test_customer&select=RZ011&mobile="+strTelNo +"&name="+strUserName + "&cardID="+strCertCode+"&sign="+strSign;
							}
							LOG(DEBUG,"strKey=%s",strKey.c_str());
							printf("Line:%d,strKey=%s\n",__LINE__,strKey.c_str());

							
						}
						break;
					case 10002:
						{
							if(/*map_UserValueKey.find(KEY_CERT_TYPE)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_CERT_CODE)!=map_UserValueKey.end()&&*/map_UserValueKey.find(KEY_USER_NAME)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_AUTH_ID)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_CUST_NAME)!=map_UserValueKey.end())
							{	
								//strMonth = map_UserValueKey.find(KEY_MONTH)->second;
								//strCertType = map_UserValueKey.find(KEY_CERT_TYPE)->second;
								//strCertCode = map_UserValueKey.find(KEY_CERT_CODE)->second;
								strUserName = map_UserValueKey.find(KEY_USER_NAME)->second;
								strAuthId = map_UserValueKey.find(KEY_AUTH_ID)->second;
								strCustName = map_UserValueKey.find(KEY_CUST_NAME)->second;
							}
							else
							{
									errorMsg ="1100"; //param missing or error
									LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
									printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
									return OTHERERROR;
							}

							if(iOperator == 1 || iOperator == 3)
							{
								
							}
							if( iOperator == 2 )
							{
								strKey = strParams+"&phone="+strTelNo +"&appkey=NXJjHy5nHwUfLmcf&userauthid="+strAuthId;
							}
							LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
							printf("Line:%d,strKey=%s\n",__LINE__,strKey.c_str());

							
						}
						break;
					case 10003:
						{

							string cardIDHash;
							//char idType[10];
							string idType;
							//memset(idType,0,10);
							
							if(map_UserValueKey.find(KEY_CERT_TYPE)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_CERT_CODE)!=map_UserValueKey.end()/*&&map_UserValueKey.find(KEY_USER_NAME)!=map_UserValueKey.end()*/&&map_UserValueKey.find(KEY_AUTH_ID)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_CUST_NAME)!=map_UserValueKey.end())
							{	
								//strMonth = map_UserValueKey.find(KEY_MONTH)->second;
								strCertType = map_UserValueKey.find(KEY_CERT_TYPE)->second;
								strCertCode = map_UserValueKey.find(KEY_CERT_CODE)->second;
								//strUserName = map_UserValueKey.find(KEY_USER_NAME)->second;
								strAuthId = map_UserValueKey.find(KEY_AUTH_ID)->second;
								strCustName = map_UserValueKey.find(KEY_CUST_NAME)->second;
							}
							else
							{
									errorMsg ="1100"; //param missing or error
									LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
									printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
									return OTHERERROR;
							}
							if(strCertType.compare("0101")==0)
							{
								//idType="…Ì∑›÷§";
								//sprintf(idType,"%c%c%c%c%c%c%c%c%c",0xE8,0xBA,0xBA,0xE4,0xBB,0xBD,0xE8,0xAF,0x81);
								idType="idCard";
								printf("Line:%d,idType=%s\n",__LINE__,idType.c_str());
							}
							if( iOperator == 1 || iOperator == 3 )
							{
								cardIDHash = BdxGetParamSign(strCertCode,std::string(""));
								printf("Line:%d,cardIDHash=%s\n",__LINE__,cardIDHash.c_str());
								strMobile =	strTelNo;
								std:: string tempStr="accountIDtest_customer";
								strSign= tempStr+"cardIDHash"+ cardIDHash+"idType"+idType+"mobile"+strMobile+"selectRZ006";
								LOG(DEBUG,"Before Md5 strSign=%s",strSign.c_str());
								strSign = BdxGetParamSign(strSign,strPrivateKey);
								LOG(DEBUG,"After  Md5 strSign=%s",strSign.c_str());
								printf("Line:%d,strSign=%s\n",__LINE__,strSign.c_str());
								strKey = strParams+"accountID=test_customer&select=RZ006&mobile="+strMobile+"&cardIDHash="+cardIDHash+"&idType="+idType+"&sign="+strSign;
								 

							}
							if( iOperator == 2 )
							{
								strKey = strParams+"&phone="+strTelNo +"&idCard="+strCertCode+"&appkey=NXJjHy5nHwUfLmcf&userauthid="+strAuthId;
							}
							LOG(DEBUG,"strKey=%s",strKey.c_str());
							printf("Line:%d,strKey=%s\n",__LINE__,strKey.c_str());

	
						}
						break;
					case 20001://telStatusVerify
						{
							if(/*map_UserValueKey.find(KEY_CERT_TYPE)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_CERT_CODE)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_USER_NAME)!=map_UserValueKey.end()&&*/map_UserValueKey.find(KEY_AUTH_ID)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_CUST_NAME)!=map_UserValueKey.end())
							{	
								//strMonth = map_UserValueKey.find(KEY_MONTH)->second;
								//strCertType = map_UserValueKey.find(KEY_CERT_TYPE)->second;
								//strCertCode = map_UserValueKey.find(KEY_CERT_CODE)->second;
								//strUserName = map_UserValueKey.find(KEY_USER_NAME)->second;
								strAuthId = map_UserValueKey.find(KEY_AUTH_ID)->second;
								strCustName = map_UserValueKey.find(KEY_CUST_NAME)->second;
							}
							else
							{
									errorMsg ="1100"; //param missing or error
									LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
									printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
									return OTHERERROR;
							}
							if(iOperator == 1 )//|| iOperator == 3)
							{
								strMobile =	strTelNo;
								std:: string tempStr="accountIDtest_customer";
								strSign= tempStr+"mobile"+strMobile+"selectZ0001";
								strSign = BdxGetParamSign(strSign,strPrivateKey);
								strKey = strParams+"accountID=test_customer&select=Z0001&mobile="+strMobile+"&sign="+strSign;
								
							}
							if( iOperator == 2 )
							{
								strKey = strParams+"&phone="+strTelNo +"&appkey=NXJjHy5nHwUfLmcf&userauthid="+strAuthId;
							}
							if(iOperator == 3 )
							{
								strMobile =	strTelNo;
								std:: string tempStr="accountIDtest_customer";
								strSign= tempStr+"mobile"+ strTelNo+"month201509selectLT002";
								LOG(DEBUG,"Before Md5 strSign=%s",strSign.c_str());
								strSign = BdxGetParamSign(strSign,strPrivateKey);
								LOG(DEBUG,"After  Md5 strSign=%s",strSign.c_str());
								strKey = 
								strParams+"accountID=test_customer&select=LT002&month=201509&mobile="+strTelNo+"&sign="+strSign;
							}
							LOG(DEBUG,"strKey=%s",strKey.c_str());
							printf("Line:%d,strKey=%s\n",__LINE__,strKey.c_str());

							
						}
						break;
					case 20003://telServiceLifeQuery
						{
							if(/*map_UserValueKey.find(KEY_CERT_TYPE)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_CERT_CODE)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_USER_NAME)!=map_UserValueKey.end()&&*/map_UserValueKey.find(KEY_AUTH_ID)!=map_UserValueKey.end()&&map_UserValueKey.find(KEY_CUST_NAME)!=map_UserValueKey.end())
							{	
								//strMonth = map_UserValueKey.find(KEY_MONTH)->second;
								//strCertType = map_UserValueKey.find(KEY_CERT_TYPE)->second;
								//strCertCode = map_UserValueKey.find(KEY_CERT_CODE)->second;
								//strUserName = map_UserValueKey.find(KEY_USER_NAME)->second;
								strAuthId = map_UserValueKey.find(KEY_AUTH_ID)->second;
								strCustName = map_UserValueKey.find(KEY_CUST_NAME)->second;
							}
							else
							{
									errorMsg ="1100"; //param missing or error
									LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
									printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
									return OTHERERROR;
							}

							if(iOperator == 1 )// || iOperator == 3)
							{
								strMobile =	strTelNo;
								string tempStr="accountIDtest_customermobile";
								strSign= tempStr+ strMobile+"month201509selectZ0003";
								LOG(DEBUG,"Before Md5 strSign=%s",strSign.c_str());
								printf("strSign=%s\n",strSign.c_str());
								strSign = BdxGetParamSign(strSign,strPrivateKey);
								LOG(DEBUG,"After Md5 strSign=%s",strSign.c_str());
								printf("Line:%d,strSign=%s\n",__LINE__,strSign.c_str());
								strKey = strParams+"accountID=test_customer&select=Z0003&mobile="+strMobile+"&month=201509&sign="+strSign;

							}
							if( iOperator == 2 )
							{
								strKey = strParams+"&phone="+strTelNo+"&appkey=NXJjHy5nHwUfLmcf&userauthid="+strAuthId;
							}
							if( iOperator == 3 )
							{
								strMobile =	strTelNo;
								std:: string tempStr="accountIDtest_customer";
								strSign= tempStr+"mobile"+ strTelNo+"month201509selectLT003";
								LOG(DEBUG,"Before Md5 strSign=%s",strSign.c_str());
								strSign = BdxGetParamSign(strSign,strPrivateKey);
								LOG(DEBUG,"After  Md5 strSign=%s",strSign.c_str());
								strKey = strParams+"accountID=test_customer&select=LT003&month=201509&mobile="+strTelNo+"&sign="+strSign;
							}
							LOG(DEBUG,"strKey=%s",strKey.c_str());
							printf("Line:%d,strKey=%s\n",__LINE__,strKey.c_str());

						}
						break;

					default:
						printf("default\n");
						break;


				}

				
				ssContent = ssContent.substr(0,ipos);
				//#define __NOLOCAL__
				#ifdef __NOLOCAL__
				{		

						LOG(DEBUG,"ssContent=%s",ssContent.c_str());
						ssContent=ssContent+"_"+strAction;
						//#define __MD5__
						#ifdef __MD5__
						ssContent = BdxGetParamSign(ssContent,std::string(""));
						#endif
						LOG(DEBUG,"ssContent Key=%s",ssContent.c_str());
						printf("Line:%d,ssContent=%s\n",__LINE__,ssContent.c_str());
						if(m_pDataRedis->UserGet(ssContent,ssmoidValue))
						{	
							LOG(DEBUG,"ssmoidValue=%s",ssmoidValue.c_str());
							printf("ssmoidValue=%s\n",ssmoidValue.c_str());
							stResponseInfo.mResValue = ssmoidValue;
							m_pDataRedis->UserIncr(stResponseInfo.ssUserCountKeyRes);
							CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stResponseInfo.ssUserName].m_ullResNum++;
							LOG(DEBUG,"stResponseInfo.mResValue=%s",stResponseInfo.mResValue.c_str());
							printf("stResponseInfo.mResValue=%s\n",stResponseInfo.mResValue.c_str());
							
						}
						else 
						{
							//when no data in local redis,then query remote api
							iNoDataFlag = 1;
							//return ERRORNODATA;
						}
						mResValueLocal = stResponseInfo.mResValue;
						
				 }
				#endif //__NOLOCAL__
			iNoDataFlag = 1;
			printf("Line:%d,iNoDataFlag=%d\n",__LINE__,iNoDataFlag);
			LOG(DEBUG,"iNoDataFlag=%d",iNoDataFlag); 

			char remoteBuffer[_8KBLEN];
			CTcpSocket* remoteSocket;
			std::string remoteIp;
			uint16_t remotePort;
			
			string key;
			//printf("Line:%d,strProvince=%s\n",__LINE__,strProvince.c_str());
			if( iNoDataFlag == 1 )
			{
				for( std::map<std::string,QUERYAPIINFO_S>::iterator itr=g_vecUrlAPIS.begin();itr!=g_vecUrlAPIS.end();itr++)
				{
					printf("Line:%d,itr->first=%s \n",__LINE__,itr->first.c_str());
					printf("Line:%d,strParams.c_str=%s\n",__LINE__,strParams.c_str());
					printf("Line:%d,itr->second.mParam=%s\n",__LINE__,itr->second.mParam);
					
					if(atoi(itr->second.mParam) == strSource)  //????
					{
						strHost=itr->first;
						remoteIp.assign(itr->first,0,itr->first.find(":",0));
						remotePort = atoi(itr->first.substr(itr->first.find(":",0)+1).c_str());
						printf("sslIp=%s,sslPort=%d\n",remoteIp.c_str(),remotePort);
						remoteSocket=new CTcpSocket(remotePort,remoteIp);
						if(remoteSocket->TcpConnect()!=0)
						{
							errorMsg = "5000";
							LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
							return OTHERERROR;
						}
						break;
					}

				}
				

					#if 0	
					if( bUserIdentity )
					{
						strUserIdentity="/credit/userIdentity/";
						strRemoteValue = "{\r\n\"sendTelNo\":\""
						+ strTelNo+"\",\r\n\"certType\":\""
						+strCertType+"\",\r\n\"certCode\":\""
						+strCertCode+"\",\r\n\"userName\":\""
						+strCustName+"\"\r\n}";
						istrRemoteValueLength= strRemoteValue.length();
						
					}
					if(bTelVerify )
					{
						strUserTelVerity="/credit/telVerify/";
						
					}		
					#endif
					
					//sprintf(m_httpReq,"GET %s HTTP/1.1\r\nAccept: */*\r\nAccept-Language: zh-cn\r\ntoken: %s\r\nhost: %s\r\n\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\nConnection:keep-alive\r\n",strUserIdentity.c_str(),strToken.c_str(),strHost.c_str(),istrRemoteValueLength,strRemoteValue.c_str());
					if(strSource==1001)
					{
						sprintf(m_httpReq,"GET %s HTTP/1.1\r\nHost: %s\r\nAccept-Encoding: identity\r\n\r\n",strKey.c_str(),string("api.shuzunbao.com").c_str());
					}
					if(strSource==1002)
					{
						sprintf(m_httpReq,"GET %s HTTP/1.1\r\nHost: %s\r\nAccept-Encoding: identity\r\n\r\n",strKey.c_str(),string("61.151.247.36").c_str());
					}
					
					printf("Line:%d,%s\n",__LINE__,m_httpReq);
					LOG(DEBUG,"m_httpReq=%s",m_httpReq);
					printf("Line:%d,remoteSocket->TcpGetSockfd()=%d\n",__LINE__,remoteSocket->TcpGetSockfd());
					if(remoteSocket->TcpWrite(m_httpReq,strlen(m_httpReq))!=0)
					{
							memset(remoteBuffer,0,_8KBLEN);
							remoteSocket->TcpReadAll(remoteBuffer,_8KBLEN);
							printf("Line:%d,remoteBuffer=%s\n",__LINE__,remoteBuffer);
							LOG(DEBUG,"remoteBuffer=%s",remoteBuffer);
							if( strlen(remoteBuffer) > 0 )
							{
								stResponseInfo.mResValue = std::string(remoteBuffer);
								mResValueRemote = stResponseInfo.mResValue;
								m_pDataRedis->UserIncr(stResponseInfo.ssUserCountKeyRes); 
								CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stResponseInfo.ssUserName].m_ullReqNum++;
								remoteSocket->TcpClose();
							}
							else
							{
								remoteSocket->TcpClose();
								return OTHERERROR;
							}
					}
					else
					{
						remoteSocket->TcpClose();
						printf("Line:%d,remoteSocket->TcpGetSockfd()=%d\n",__LINE__,remoteSocket->TcpGetSockfd());
						return OTHERERROR;
					}

			}

		}
		else
		{
			 errorMsg = "1100";	// request param is error
			 LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
			 return OTHERERROR;
		}
	}
	else
	{
		errorMsg = "1100";	// request type  is error ( GET )
		LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
		return OTHERERROR;
	}

	LOG(DEBUG,"mResValueRemote=%s",mResValueRemote.c_str());
	printf("Line:%d,stResponseInfo.mResValue=%s\n",__LINE__,stResponseInfo.mResValue.c_str());
	if( iNoDataFlag == 1 )
	{		
		if(mResValueRemote.empty())//&&isExpire == 1)
		{			
			errorMsg = "5000";  // data is not exists
			LOG(DEBUG,"errorMsg=%s,stResponseInfo.mResValue=%s",errorMsg.c_str(),stResponseInfo.mResValue.c_str());
			printf("line %d,s Error: %s,value %s\n",__LINE__,errorMsg.c_str(),stResponseInfo.mResValue.c_str());
			return OTHERERROR;
		}

		lenStrTemp = mResValueRemote.length();
		if( mResValueRemote.find("\r\n\r\n")!=std::string::npos )
		{
			mResValueRemote = mResValueRemote.substr(mResValueRemote.find("\r\n\r\n")+4,lenStrTemp -(mResValueRemote.find("\r\n\r\n")+4));
		}
		
		lenStrTemp =  mResValueRemote.length();
		stResponseInfo.mResValue = mResValueRemote;
		
		int ipos1=stResponseInfo.mResValue.find("{",0);
		int ipos2=stResponseInfo.mResValue.rfind("}",stResponseInfo.mResValue.length());
		printf("Line:%d,ipos1=%d,ipos2=%d\n",__LINE__,ipos1,ipos2);

		if( ipos1 < 0 || ipos2 <= 0 )
		{
			errorMsg = "5000";  // data is not exists
			LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
			printf("line %d,s Error: %s,value %s\n",__LINE__,errorMsg.c_str(),stResponseInfo.mResValue.c_str());
			return OTHERERROR;
		}
		
		stResponseInfo.mResValue = stResponseInfo.mResValue.substr(ipos1,ipos2-ipos1+1);
		

	#define __LOCAL_STORE__
	#ifdef __LOCAL_STORE__

		strTelNo=strTelNo+"_"+strAction;
		#ifdef __MD5__
		strTelNo = BdxGetParamSign(strTelNo,std::string(""));	
		#endif
		LOG(DEBUG,"Local Store strTelNo=%s",strTelNo.c_str());
		printf("line %d,s strTelNo: %s,value %s\n",__LINE__,strTelNo.c_str());
		if(m_pDataRedis->UserPut(strTelNo,stResponseInfo.mResValue))
		{	
			
			 	LOG(ERROR, "[thread: %d]Set HotKey Error.", m_uiThreadId);								
		}
	#endif
		
	}
	
	HIVELOCALLOG_S stHiveLog;

	//stHiveLog.logtime=logtime;
	stHiveLog.strAccessKeyId=strAccessKeyId;
	stHiveLog.strTelNo=strTelNoTemp;
	stHiveLog.strTimeStamp=strTimeStamp;
	stHiveLog.strLiveTime=strLiveTime;
	stHiveLog.strSinature=strSinature;
	stHiveLog.strAuthId=strAuthId;
	stHiveLog.strCustName=strCustName;
	stHiveLog.strAction=strAction;
	stHiveLog.strReqParams=strReqParams;
	stHiveLog.strValue=stResponseInfo.mResValue;
	stHiveLog.strQuerytime=BdxTaskMainGetFullTime();
	stHiveLog.strDayId=BdxTaskMainGetDate();
	stHiveLog.strHourId=stHiveLog.strQuerytime.substr(8,2);	

	//stResponseInfo.mResValue="{\"resCode\":\"0000\",\"resMsg\":\"«Î«Û≥…π¶\",\"sign\":\"DEA501DC38718AE61EF0033684AC1759\",\"data\":[{\"resCode\":\"0000\",\"resMsg\":\"«Î«Û≥…π¶\",\"quotaInfo\":{\"quotaValuePercent\":0,\"quotaID\":\"Z0003\",\"quotaName\":\" ÷ª˙∫≈¬Î‘⁄Õ¯ ±≥§\",\"quotaType\":1,\"quotaValue\":\"[36,+)\",\"quotaPrice\":100,\"quotaValueType\":2}}]}";
	LOG(DEBUG,"stResponseInfo.mResValue=%s",stResponseInfo.mResValue.c_str());
	printf("Line:%d,stResponseInfo.mResValue=%s\n",__LINE__,stResponseInfo.mResValue.c_str());
	if( /*iNoDataFlag == 1 &&*/ strSource == 1001)
	{
				if(!jReader->parse(stResponseInfo.mResValue, jValue,true))
				{ 

					errorMsg = "5000";
					LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
					printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
					return	OTHERERROR;
				}

				if(jValue[string("resCode").c_str()].asString()!="0000")
				{
					stResponseInfo.mResValue = "{\r\n\"code\":\"1200\",\r\n\"msg\":\"authentication failure\",\r\n\"data\":\"\"\r\n}";
					return SUCCESS;
				}

				if(!jReader->parse(jValue["data"].toStyledString(), jValue))
				{ 

					errorMsg = "5000";
					LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
					printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
					return	OTHERERROR;
				}

				for(unsigned int i = 0;i<jValue.size();++i)
				{

					if(!jReader->parse(jValue[i]["quotaInfo"].toStyledString(), jTemp))
					{
						errorMsg = "5000";
						LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
						printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
						return	OTHERERROR;		
					}
					else
					{

				switch(iIdentity)
				{
					case 10001://userIdentity
						{	
							std::string strRet = "01";
							printf("Line:%d,jTemp[\"quotaValue\"]=%d\n",__LINE__,atoi(jTemp["quotaValue"].asString().c_str()));
							if(jTemp["quotaValue"].asString().find("0,0,0")!=std::string::npos)
							{
								strRet = "00";
							}
							sprintf(m_httpResValueE20003,"{\r\n\"code\":\"1000\",\r\n\"msg\":\"success\",\r\n\"data\":{\r\n\"action\":\"%s\",\r\n\"checkResult\":\"%s\"\r\n}\r\n}",strAction.c_str(),strRet.c_str());
							stResponseInfo.mResValue=string(m_httpResValueE20003);
							printf("stResponseInfo.mResValue=%s\n",stResponseInfo.mResValue.c_str());
						}
						break;
					case 10002:
						break;
					case 10003:
						{
							std::string strRet = "01";
							printf("Line:%d,jTemp[\"quotaValue\"]=%d\n",__LINE__,atoi(jTemp["quotaValue"].asString().c_str()));
							if(jTemp["quotaValue"].asString().find("0,0")!=std::string::npos)
							{
								strRet = "00";
							}
							if((jTemp["quotaValue"].asString().find("1,-1")!=std::string::npos)||(jTemp["quotaValue"].asString().find("1,-1")!=std::string::npos))
							{
								strRet = "02";
							}
							sprintf(m_httpResValueE20003,"{\r\n\"code\":\"1000\",\r\n\"msg\":\"success\",\r\n\"data\":{\r\n\"action\":\"%s\",\r\n\"checkResult\":\"%s\"\r\n}\r\n}",strAction.c_str(),strRet.c_str());
							stResponseInfo.mResValue=string(m_httpResValueE20003);
							printf("stResponseInfo.mResValue=%s\n",stResponseInfo.mResValue.c_str());
						}
						break;
					case 20001://telStatusVerify
						{
							printf("Line:%d,jTemp[\"quotaValue\"]=%d\n",__LINE__,atoi(jTemp["quotaValue"].asString().c_str()));
							sprintf(m_httpResValueE20003,"{\r\n\"code\":\"1000\",\r\n\"msg\":\"success\",\r\n\"data\":{\r\n\"action\":\"%s\",\r\n\"telStatus\":%d\r\n}\r\n}",strAction.c_str(),atoi(jTemp["quotaValue"].asString().c_str()));
							stResponseInfo.mResValue=string(m_httpResValueE20003);
							printf("stResponseInfo.mResValue=%s\n",stResponseInfo.mResValue.c_str());
						}
						break;
					case 20003://telServiceLifeQuery
						{
							printf("jValue[\"quotaValue\"]=%s\n",jTemp["quotaValue"].asString().c_str());
							if(jTemp["quotaValue"].asString()=="-1") 
							{
								inUseTime = -1;
							}
							if(jTemp["quotaValue"].asString()=="null") 
							{
								inUseTime = -2;
							}
							if(jTemp["quotaValue"].asString()=="[36,+)") 
							{
								inUseTime = 5;
							}
							if(jTemp["quotaValue"].asString()=="[24-36)") 
							{
								inUseTime = 4;
							}
							if(jTemp["quotaValue"].asString()=="[12-24)") 
							{
								inUseTime = 3;
							}
							if(jTemp["quotaValue"].asString()=="[6-12)") 
							{
								inUseTime = 2;
							}
							if(jTemp["quotaValue"].asString()=="[0-6)") 
							{
								inUseTime = 1;
							}
							printf("inUseTime=%d\n",inUseTime);
							sprintf(m_httpResValueE20003,"{\r\n\"code\":\"1000\",\r\n\"msg\":\"success\",\r\n\"data\":{\r\n\"action\":\"%s\",\r\n\"inUseTime\":%d\r\n}\r\n}",strAction.c_str(),inUseTime);
							stResponseInfo.mResValue=string(m_httpResValueE20003);
							printf("stResponseInfo.mResValue=%s\n",stResponseInfo.mResValue.c_str());
						}
						break;

					default:
						printf("default\n");
						break;


				}


					}		

				}

	}
	if( /*iNoDataFlag == 1 &&*/  strSource == 1002)
	{
				if(!jReader->parse(stResponseInfo.mResValue, jValue,true))
				{ 

					errorMsg = "5000";
					LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
					printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
					return	OTHERERROR;
				}

				if(jValue.toStyledString().find("error_message")!=std::string::npos)
				{
					errorMsg = "5000";
					LOG(DEBUG,"errorMsg=%s",errorMsg.c_str());
					printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
					return	OTHERERROR;
				}
				else
				{
					switch(iIdentity)
					{
						case 20003://userIdentity
							{	
								
								printf("Line:%d,jTemp[\"quotaValue\"]=%d\n",__LINE__,atoi(jValue["result"].asString().c_str()));
								if(jValue["result"].asString().find("NULL")!=std::string::npos)
								{
									inUseTime = -1;
								}
								else
								{
									if(atoi(jValue["result"].asString().c_str())<=6)
									{
										inUseTime = 1;
									}
									if(atoi(jValue["result"].asString().c_str())<=12)
									{
										inUseTime = 2;
									}
									if(atoi(jValue["result"].asString().c_str())<=24)
									{
										inUseTime = 3;
									}
									if(atoi(jValue["result"].asString().c_str())<=36)
									{
										inUseTime = 4;
									}
									if(atoi(jValue["result"].asString().c_str())>=37)
									{
										inUseTime = 5;
									}
								}
								
								
								printf("inUseTime=%d\n",inUseTime);
								sprintf(m_httpResValueE20003,"{\r\n\"code\":\"1000\",\r\n\"msg\":\"success\",\r\n\"data\":{\r\n\"action\":\"%s\",\r\n\"inUseTime\":%d\r\n}\r\n}",strAction.c_str(),inUseTime);
								stResponseInfo.mResValue=string(m_httpResValueE20003);
								printf("stResponseInfo.mResValue=%s\n",stResponseInfo.mResValue.c_str());
							}
							break;
						case 10001:
						case 10002:
						case 10003://userIdentity
							{	
								std::string strRet = "01";
								printf("Line:%d,jTemp[\"quotaValue\"]=%d\n",__LINE__,atoi(jTemp["quotaValue"].asString().c_str()));
								if(jValue["result"].asString().find("1")!=std::string::npos)
								{
									strRet = "00";
								}
								sprintf(m_httpResValueE20003,"{\r\n\"code\":\"1000\",\r\n\"msg\":\"success\",\r\n\"data\":{\r\n\"action\":\"%s\",\r\n\"checkResult\":\"%s\"\r\n}\r\n}",strAction.c_str(),strRet.c_str());
								stResponseInfo.mResValue=string(m_httpResValueE20003);
								printf("stResponseInfo.mResValue=%s\n",stResponseInfo.mResValue.c_str());
							}
							break;
						default:
							printf("default\n");
							break;
					  }
				}




	}
    return SUCCESS;

	#if 0
	if( bUserIdentity )
	{
		stResponseInfo.mResValue = 
		"{\r\n\"code\":\"1000\",\r\n\"msg\":\"success\",\r\n\"data\":{\r\n\"action\":\"%s\",\r\n\"checkResult\":\""+jValue["checkResult"].asString()+"\",\r\n}\r\n}";
	}
	if( bTelVerify )
	{
		stResponseInfo.mResValue = "{\r\n\"code\":\"1000\",\r\n\"msg\":\"success\",\r\n\"data\":{\r\n\"action\":\"telVerify\",\r\n\"telStatus\":\""+jValue["telStatus"].asString()+"\",\r\n\"inUseTime\":\""+jValue["inUseTime"].asString()+"\"}\r\n}";
	}	
	#endif
	

#if 0
// get the fields according to rights	

	#ifdef __P_LOCK__
		pthread_rwlock_wrlock(&p_rwlock);
	#endif //__P_LOCK__
	std::string tagsNumber,oPeratorTagsNumber;
	for(itRights = g_mapUserInfo.find(strUser)->second.mVecFields.begin();itRights!= g_mapUserInfo.find(strUser)->second.mVecFields.end();itRights++)
	{
		
		for(unsigned int i=0;i<jValue.size();i++)
		{
		//printf("m_uiThreadId=%d\n jValue[%d][\"typeid\"]=%s\n",m_uiThreadId,i,jValue[i].asString().c_str());
		//if( jValue[i]["typeid"].asString().length()>0 )
		if(!jValue[i]["typeid"].isNull())
		{
			if((!jValue[i]["name"].isNull())&&(!jValue[i]["value"].isNull()))
			{
				jTemp.clear();
				jTemp[jValue[i]["name"].asString()] = jValue[i]["value"];
				

				CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stResponseInfo.ssOperatorName].m_ullResTagNum++;
				

				// deal with json string to pinyou format
				strTempValue = jFastWriter.write(jTemp); 
				strTempValue = mConf.replace_all(strTempValue,LEFTBIGBRACE,NULLSTRING);
				strTempValue = mConf.replace_all(strTempValue,RIGHTBIGBRACE,NULLSTRING);
				
				//cout<<"strTempValue="<<strTempValue<<::endl;
				//cout<<"strTempValue.length()"<<strTempValue.length()<<::endl;
				//cout<<"strTempValue[strTempValue.length()-2]"<<strTempValue[strTempValue.length()-2]<<::endl;
				//cout<<"strTempValue[0]"<<strTempValue[0]<<::endl;
				
				//if( (stResponseInfo.mResValue.find("\"")!=std::string::npos) && (stResponseInfo.mResValue.rfind("\"")!=std::string::npos)))
				
				if((strTempValue[0]=='"')&&(strTempValue[strTempValue.length()-2]=='"') )
				{	
					lenStrTemp = strTempValue.length();
					strTempValue = strTempValue.substr(1,lenStrTemp - 3);
					//cout<<"strTempValue="<<strTempValue<<::endl;
				}
				else
				{

						errorMsg = "E0006";
						printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
						stResponseInfo.ssUserCountKeyReqError=stResponseInfo.ssUserCountKeyReq+"_"+errorMsg;		
						m_pDataRedis->UserIncr(stResponseInfo.ssUserCountKeyReqError);
						
						#ifdef __P_LOCK__
						pthread_rwlock_unlock(&p_rwlock);
						#endif //__P_LOCK__
						return	ERRORNODATA;

				}
				strTempValue = mConf.replace_all(strTempValue,"\n",NULLSTRING);
				
			}
			
			if( *itRights == jValue[i]["typeid"].asString() )
			{
				jRoot[*itRights].append(strTempValue);

				tagsNumber="Restag_" + BdxTaskMainGetTime()+"_"+strUser+"_"+strKeyType+"_"+jValue[i]["typeid"].asString();
				//m_pDataRedis->UserIncrBy(tagsNumber,jValue[i].size());

				m_pDataRedis->UserIncr(tagsNumber);
				if( iIsLocal!=1 && iRemoteApiFlag ==1 )
				{
					oPeratorTagsNumber= strProvinceResTag + BdxTaskMainGetTime()+"_" + strUser + "_" + strKeyType + "_" + jValue[i]["typeid"].asString();
					m_pDataRedis->UserIncr(oPeratorTagsNumber);

				}
				//cout<<"jRoot"<<jFastWriter.write(jRoot);
				//cout<<"+++++++++++++++++++++++++++++++"<<std::endl;
				
			}

			#if 0
			switch jValue[*itRights]
			{
		
				case "1":
						if((!jValue["name"].isNull())&&(!jValue["value"].isNull()))
						{
							jTemp[jValue["name"]] = jValue["value"]];
						}
						jRoot[*itRights].append(jTemp);


			}
			#endif
				
		}	


		}

		
	}
	
	//mResUserAttributes = jFastWriter.write(jRoot);
#ifdef __P_LOCK__
	pthread_rwlock_unlock(&p_rwlock);
#endif //__P_LOCK__
	mResUserAttributes = jRoot.toStyledString();
	mResUserAttributes = mConf.replace_all(mResUserAttributes,LEFTMIDBRACE,LEFTBIGBRACE);
	mResUserAttributes = mConf.replace_all(mResUserAttributes,RIGHTMIDBRACE,RIGHTBIGBRACE);
	mResUserAttributes = mConf.replace_all(mResUserAttributes,"\\",NULLSTRING);
	if( mResUserAttributes == "null\n" )
	{

		errorMsg = "E0006";
		printf("line %d,s Error: %s\n",__LINE__,errorMsg.c_str());
		stResponseInfo.ssUserCountKeyReqError=stResponseInfo.ssUserCountKeyReq+"_"+errorMsg;		
		m_pDataRedis->UserIncr(stResponseInfo.ssUserCountKeyReqError);
		
		return	ERRORNODATA;
	}
	//std::cout<< "mResUserAttributes="<<mResUserAttributes<<::endl;
	stResponseInfo.mResValue = mResUserAttributes;

#endif	
	
}

int CTaskMain::BdxParseHttpPacket(char*& pszBody, u_int& uiBodyLen, const u_int uiParseLen)
{
	u_int uiHeadLen = 0;

	char* pszTmp = NULL;
	char* pszPacket = m_pszAdxBuf;
	if(strncmp(m_pszAdxBuf, "GET", strlen("GET"))) {
//		LOG(ERROR, "[thread: %d]It is not POST request.", m_uiThreadId);
		return PROTOERROR;
	}

	//find body
	pszTmp = strstr(pszPacket, m_pszHttpHeaderEnd);
	if(pszTmp == NULL) {
		LOG(ERROR, "[thread: %d]can not find Header End.", m_uiThreadId);
		return PROTOERROR;
	}
	pszBody = pszTmp + strlen(m_pszHttpHeaderEnd);
	uiHeadLen = pszBody - m_pszAdxBuf;

	return SUCCESS;
	//return OTHERERROR;
}

int CTaskMain::BdxParseBody(char *pszBody, u_int uiBodyLen, BDXREQUEST_S& stRequestInfo)
{

    LOG(DEBUG,"SUCCESS");
	return SUCCESS;
}



int CTaskMain::BdxSendEmpyRespones(std::string errorMsg)
{
	m_clEmTime.TimeOff();
	std::string strOutput=errorMsg;	
	char pszDataBuf[_8KBLEN];
	memset(pszDataBuf, 0, _8KBLEN);
	sprintf((char *)pszDataBuf, "%s%sContent-Length: %d\r\n\r\n", http200ok,BdxGetHttpDate().c_str(),(int)strOutput.length());
	int iHeadLen = strlen(pszDataBuf);
	
	memcpy(pszDataBuf + iHeadLen, strOutput.c_str(), strOutput.length());
	printf("Line:%d,AdAdxSendEmpyRespones=%s\n",__LINE__,pszDataBuf);
	LOG(DEBUG,"Thread : %d ,AdAdxSendEmpyRespones=%s\n",m_uiThreadId,pszDataBuf);
	if(!m_pclSock->TcpWrite(pszDataBuf, iHeadLen + strOutput.length())) {
		LOG(ERROR, "[tread: %d]write empty response data error.", m_uiThreadId);
		return LINKERROR;
	}

	return SUCCESS;
}

int CTaskMain::BdxSendRespones(BDXREQUEST_S& stRequestInfo, BDXRESPONSE_S& stAdxRes,std::string errorMsg)
{
	memset(m_pszAdxResponse, 0, _64KBLEN);
	if( stAdxRes.mResValue.empty())
	{		
		std::string strOutput=errorMsg;
	}
	if(m_httpType)
	{
		sprintf((char *)m_pszAdxResponse, "%s%sContent-Length: %d\r\n\r\n", http200ok,BdxGetHttpDate().c_str(),(int)stAdxRes.mResValue.length());
		int iHeadLen = strlen(m_pszAdxResponse);
		memcpy(m_pszAdxResponse + iHeadLen, stAdxRes.mResValue.c_str(),stAdxRes.mResValue.length());
	}
	else
	{
		sprintf((char *)m_pszAdxResponse,"%s",stAdxRes.mResValue.c_str());
	}
	
	int iBodyLength = strlen(m_pszAdxResponse);
	iBodyLength=strlen(m_pszAdxResponse);



	if(!m_pclSock->TcpWrite(m_pszAdxResponse, iBodyLength)) 
	{
		CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stAdxRes.ssUserName].m_ullEmptyResNum++;
		CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stAdxRes.ssUserName].m_ullTotalEmptyResNum++;
		if(stAdxRes.queryType==1)// 1 query user index ,2 query goods 
		{
			m_pDataRedis->UserIncr(stAdxRes.ssUserCountKeyRes);

		}
		LOG(ERROR, "[thread: %d]write  response error.", m_uiThreadId);
		return LINKERROR;
	}

	CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stAdxRes.ssUserName].m_ullResNum++;
	CUserQueryWorkThreads::m_vecReport[m_uiThreadId].m_strUserInfo[stAdxRes.ssUserName].m_ullTotalResNum++;
	
	if(stAdxRes.queryType==1)// 1 query user index ,2 query goods 
	{
		m_pDataRedis->UserIncr(stAdxRes.ssUserCountKeyRes);

	}
	
	LOG(DEBUG, "[thread: %d]write response iBodyLength=%d.",m_uiThreadId,iBodyLength);
	
    return SUCCESS;
}

std::string CTaskMain::BdxTaskMainGetTime(const time_t ttime)
{

	time_t tmpTime;
	if(ttime == 0)
		tmpTime = time(0);
	else
		tmpTime = ttime;
	struct tm* timeinfo = localtime(&tmpTime);
	char dt[20];
	memset(dt, 0, 20);
	sprintf(dt, "%4d%02d%02d%02d", timeinfo->tm_year + 1900,timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour);
	//sprintf(dt, "%4d%02d%02d%02d%02d%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	//return (timeinfo->tm_year + 1900) * 10000 + (timeinfo->tm_mon + 1) * 100 + timeinfo->tm_mday;
	return std::string(dt);
}

std::string CTaskMain::BdxTaskMainGetMinute(const time_t ttime)
{

	time_t tmpTime;
	if(ttime == 0)
		tmpTime = time(0);
	else
		tmpTime = ttime;
	struct tm* timeinfo = localtime(&tmpTime);
	char dt[20];
	memset(dt, 0, 20);
	sprintf(dt, "%4d%02d%02d%02d%02d", timeinfo->tm_year + 1900,timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min);
	//sprintf(dt, "%4d%02d%02d%02d%02d%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	//return (timeinfo->tm_year + 1900) * 10000 + (timeinfo->tm_mon + 1) * 100 + timeinfo->tm_mday;
	return std::string(dt);
}

std::string CTaskMain::BdxTaskMainGetFullTime(const time_t ttime)
{

	time_t tmpTime;
	if(ttime == 0)
		tmpTime = time(0);
	else
		tmpTime = ttime;
	struct tm* timeinfo = localtime(&tmpTime);
	char dt[20];
	memset(dt, 0, 20);
	sprintf(dt, "%4d%02d%02d%02d%02d%02d", timeinfo->tm_year + 1900,timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	//sprintf(dt, "%4d%02d%02d%02d%02d%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	//return (timeinfo->tm_year + 1900) * 10000 + (timeinfo->tm_mon + 1) * 100 + timeinfo->tm_mday;
	return std::string(dt);
}
std::string CTaskMain::BdxTaskMainGetUCTime(const time_t ttime)
{

	time_t tmpTime;
	if(ttime == 0)
	{
		tmpTime = time(0);
	}
	else
	{
		tmpTime = ttime;
	}
	tmpTime -= 8*3600;
	struct tm* timeinfo = localtime(&tmpTime);
	char dt[20];
	memset(dt, 0, 20);

	sprintf(dt, "%4d-%02d-%02dT%02d:%02d:%02dZ", timeinfo->tm_year + 1900,timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	//sprintf(dt, "%4d%02d%02d%02d%02d%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	//return (timeinfo->tm_year + 1900) * 10000 + (timeinfo->tm_mon + 1) * 100 + timeinfo->tm_mday;
	return std::string(dt);
}

std::string CTaskMain::BdxTaskMainGetDate(const time_t ttime)
{

	time_t tmpTime;
	if(ttime == 0)
		tmpTime = time(0);
	else
		tmpTime = ttime;
	struct tm* timeinfo = localtime(&tmpTime);
	char dt[20];
	memset(dt, 0, 20);
	sprintf(dt, "%4d%02d%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon+1,timeinfo->tm_mday);
	//return (timeinfo->tm_year + 1900) * 10000 + (timeinfo->tm_mon + 1) * 100 + timeinfo->tm_mday;
	return std::string(dt);
}

std::string CTaskMain::BdxTaskMainGetMonth(const time_t ttime)
{

	time_t tmpTime;
	if(ttime == 0)
		tmpTime = time(0);
	else
		tmpTime = ttime;
	struct tm* timeinfo = localtime(&tmpTime);
	char dt[20];
	memset(dt, 0, 20);
	sprintf(dt, "%4d%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon+1);
	//return (timeinfo->tm_year + 1900) * 10000 + (timeinfo->tm_mon + 1) * 100 + timeinfo->tm_mday;
	return std::string(dt);
}

std::string CTaskMain::BdxGenNonce(int length) 
{
        char CHAR_ARRAY[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b','c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x','y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H','I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T','U', 'V', 'W', 'X', 'Y', 'Z'};
        srand((int)time(0));
         
        std::string strBuffer ;
        //int nextPos = strlen(CHAR_ARRAY);
        int nextPos = sizeof(CHAR_ARRAY);
        //printf("nextPos=%d\n",nextPos);
        int tmp = 0;
        for (int i = 0; i < length; ++i) 
        { 
            tmp = rand()%nextPos;
            
            strBuffer.append(std::string(1,CHAR_ARRAY[tmp]));
        }
        return strBuffer;
}

std::string CTaskMain::GenPasswordDigest(std::string utcTime, std::string nonce, std::string appSecret)
{
		std::string strDigest;

		std::string strValue = nonce + utcTime + appSecret;

        unsigned char *dmg = mdSHA1.SHA1_Encode(strValue.c_str());
        const  char *pchTemp = (const  char *)(char*)dmg;
        //std::string strDmg = base64_encode((const unsigned char*)pchTemp,strlen(pchTemp));
        std::string strDmg = base64_encode((const unsigned char*)pchTemp,SHA_DIGEST_LENGTH);
		//std::string strDmg = base64_encode(reinterpret_cast<const char *>(static_cast<void*>(dmg)),strlen(dmg));
        return strDmg;
}

string   CTaskMain::BdxTaskMainReplace_All(string    str,   string   old_value,   string   new_value)   
{   
    while(true)   {   
        string::size_type   pos(0);   
        if(   (pos=str.find(old_value))!=string::npos   )   
            	str.replace(pos,old_value.length(),new_value);   
        else   break;   
    }   
    return   str;   
}   

std::string CTaskMain::BdxGetParamSign(const std::string& strParam, const std::string& strSign)
{
	char pszMd5Hex[33];
	std::string strParamKey = strParam + strSign;
	printf("Line:%d,strParamKey=%s\n",__LINE__,strParamKey.c_str());

    //ËÆ°ÁÆóÂèÇÊï∞‰∏≤ÁöÑ128‰ΩçMD5
    m_clMd5.Md5Init();
    m_clMd5.Md5Update((u_char*)strParamKey.c_str(), strParamKey.length());

    u_char pszParamSign[16];
    m_clMd5.Md5Final(pszParamSign);

    //‰ª•16ËøõÂà∂Êï∞Ë°®Á§∫
    for (unsigned char i = 0; i < sizeof(pszParamSign); i++) {
    	sprintf(&pszMd5Hex[i * 2], "%c", to_hex(pszParamSign[i] >> 4));
    	sprintf(&pszMd5Hex[i * 2 + 1], "%c", to_hex((pszParamSign[i] << 4) >> 4));
    }
    pszMd5Hex[32] = '\0';
    return std::string(pszMd5Hex);
}
