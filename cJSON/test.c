/*
  Copyright (c) 2009 Dave Gamble
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
 
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cJSON.h"
#include "Dlist.h"
#include <stdbool.h>

#define NAME_MAX_LEN 64
#define TIME_FORMAT_LEN 20//"2019-07-20_15:02:33\0"

typedef struct{
	uint32_t MessageID;
	uint8_t MessageSize;
	uint16_t SignalLength;
	uint16_t SignalBeginBit;
	uint8_t MessageName[NAME_MAX_LEN];
	uint8_t SignalName[NAME_MAX_LEN];
	float SignalPrecision;
	uint16_t MsgCycleTime;
	uint8_t ByteOrder;
	uint8_t Res;
	Dlist_t Node;
}MsgConfInfo_t;

typedef struct{
	uint32_t MessageID;
	uint8_t MessageName[NAME_MAX_LEN];
	uint8_t SignalName[NAME_MAX_LEN];
	double Value;
	char Time[TIME_FORMAT_LEN];
}MsgUpInfo_t;

static Dlist_t s_MsgConfList;

void ParseTspMsgConf(cJSON *json)
{
	int MsgNum = 0;
	int Index = 0;
	cJSON *ArrayObject = NULL;
	cJSON *Object = NULL;
	Dlist_t *pList = NULL;
	DlistInit(&s_MsgConfList);
	MsgNum = cJSON_GetArraySize(json);
	printf("Array Size = %d\n", MsgNum);
	for(; Index<MsgNum; Index++)
	{
		if(NULL != (ArrayObject = cJSON_GetArrayItem(json, Index)))
		{
			MsgConfInfo_t *MsgNode = malloc(sizeof(MsgConfInfo_t));
			if(NULL != (Object = cJSON_GetObjectItem(ArrayObject, "MessageID")))
				MsgNode->MessageID = Object->valueint;
			if(NULL != (Object = cJSON_GetObjectItem(ArrayObject, "MessageSize")))
				MsgNode->MessageSize = Object->valueint;
			if(NULL != (Object =  cJSON_GetObjectItem(ArrayObject, "SignalLength")))
				MsgNode->SignalLength = Object->valueint;
			if(NULL != (Object = cJSON_GetObjectItem(ArrayObject, "SignalBeginBit")))
				MsgNode->SignalBeginBit = Object->valueint;
			if(NULL != (Object = cJSON_GetObjectItem(ArrayObject, "MessageName")))
			{
				strncpy(MsgNode->MessageName, Object->valuestring, NAME_MAX_LEN);
				MsgNode->MessageName[NAME_MAX_LEN-1] = '\0';
			}
			if(NULL != (Object = cJSON_GetObjectItem(ArrayObject, "SignalName")))
			{
				strncpy(MsgNode->SignalName, Object->valuestring, NAME_MAX_LEN);
				MsgNode->SignalName[NAME_MAX_LEN-1] = '\0';
			}
			if(NULL != (Object = cJSON_GetObjectItem(ArrayObject, "SignalPrecision")))
				MsgNode->SignalPrecision = Object->valuedouble;
			if(NULL != (Object = cJSON_GetObjectItem(ArrayObject, "MsgCycleTime")))
				MsgNode->MsgCycleTime = Object->valueint;
			if(NULL != (Object = cJSON_GetObjectItem(ArrayObject, "ByteOrder")))
				MsgNode->ByteOrder = Object->valueint;
			if(NULL != (Object = cJSON_GetObjectItem(ArrayObject, "Res")))
				MsgNode->Res = Object->valueint;
			DlistAddTail(&s_MsgConfList, &MsgNode->Node);
		}
	}
	while(pList = DlistQueryNext(&s_MsgConfList, pList))
	{
		MsgConfInfo_t *pMsgNode = GET_CONTAINER_OF(pList, MsgConfInfo_t, Node);
		printf("File Manager is not empty, %d:%s:%d:%d:%d:%f\n", pMsgNode->MessageID, pMsgNode->MessageName, \
			pMsgNode->ByteOrder, pMsgNode->MessageSize, pMsgNode->MsgCycleTime, pMsgNode->SignalPrecision);
	}
}

char *CompactMsgToTsp(MsgUpInfo_t *pMsgUpInfo)
{
	char *Out;
	cJSON *Fmt = NULL;
	cJSON *root=cJSON_CreateArray();
	cJSON_AddItemToArray(root, Fmt = cJSON_CreateObject());
	cJSON_AddBoolToObject(Fmt, "MessageID", pMsgUpInfo->MessageID);
	cJSON_AddStringToObject(Fmt, "MessageName", pMsgUpInfo->MessageName);
	cJSON_AddStringToObject(Fmt, "SignalName", pMsgUpInfo->SignalName);
	cJSON_AddNumberToObject(Fmt, "Value", pMsgUpInfo->Value);
	cJSON_AddStringToObject(Fmt, "Time", pMsgUpInfo->Time);
	Out = cJSON_Print(root);
	printf("CompactMsgToTsp:\n%s\n", Out);
	cJSON_Delete(root);
	return Out;
}

void FreeCompactMsg(char *Out)
{
	if(NULL != Out)
		free(Out);
}

/* Parse text to JSON, then render back to text, and print! */
void doit(char *text, bool Type)
{
	char *out;cJSON *json;
	
	json=cJSON_Parse(text);
	if (!json) {printf("Error before: [%s]\n",cJSON_GetErrorPtr());}
	else
	{
		out=cJSON_Print(json);
		if(true == Type)
			ParseTspMsgConf(json);
		cJSON_Delete(json);
		printf("JSON FILE Content:\n%s\n",out);
		free(out);
	}
	
}

/* Read a file, parse, render back, etc. */
void dofile(char *filename)
{
	FILE *f;long len;char *data;
	
	f=fopen(filename,"rb");fseek(f,0,SEEK_END);len=ftell(f);fseek(f,0,SEEK_SET);
	data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
	doit(data, true);
	free(data);
}

/* Used by some code below as an example datatype. */
struct record {const char *precision;double lat,lon;const char *address,*city,*state,*zip,*country; };

/* Create a bunch of objects as demonstration. */
void create_objects()
{
	cJSON *root,*fmt,*img,*thm,*fld;char *out;int i;	/* declare a few. */
	/* Our "days of the week" array: */
	const char *strings[7]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
	/* Our matrix: */
	int numbers[3][3]={{0,-1,0},{1,0,0},{0,0,1}};
	/* Our "gallery" item: */
	int ids[4]={116,943,234,38793};
	/* Our array of "records": */
	struct record fields[2]={
		{"zip",37.7668,-1.223959e+2,"","SAN FRANCISCO","CA","94107","US"},
		{"zip",37.371991,-1.22026e+2,"","SUNNYVALE","CA","94085","US"}};

	/* Here we construct some JSON standards, from the JSON site. */
	
	/* Our "Video" datatype: */
	root=cJSON_CreateObject();	
	cJSON_AddItemToObject(root, "name", cJSON_CreateString("Jack (\"Bee\") Nimble"));
	cJSON_AddItemToObject(root, "format", fmt=cJSON_CreateObject());
	cJSON_AddStringToObject(fmt,"type",		"rect");
	cJSON_AddNumberToObject(fmt,"width",		1920);
	cJSON_AddNumberToObject(fmt,"height",		1080);
	cJSON_AddFalseToObject (fmt,"interlace");
	cJSON_AddNumberToObject(fmt,"frame rate",	24);
	
	out=cJSON_Print(root);	cJSON_Delete(root);	printf("%s\n",out);	free(out);	/* Print to text, Delete the cJSON, print it, release the string. */

	/* Our "days of the week" array: */
	root=cJSON_CreateStringArray(strings,7);

	out=cJSON_Print(root);	cJSON_Delete(root);	printf("%s\n",out);	free(out);

	/* Our matrix: */
	root=cJSON_CreateArray();
	for (i=0;i<3;i++) cJSON_AddItemToArray(root,cJSON_CreateIntArray(numbers[i],3));

/*	cJSON_ReplaceItemInArray(root,1,cJSON_CreateString("Replacement")); */
	
	out=cJSON_Print(root);	cJSON_Delete(root);	printf("%s\n",out);	free(out);


	/* Our "gallery" item: */
	root=cJSON_CreateObject();
	cJSON_AddItemToObject(root, "Image", img=cJSON_CreateObject());
	cJSON_AddNumberToObject(img,"Width",800);
	cJSON_AddNumberToObject(img,"Height",600);
	cJSON_AddStringToObject(img,"Title","View from 15th Floor");
	cJSON_AddItemToObject(img, "Thumbnail", thm=cJSON_CreateObject());
	cJSON_AddStringToObject(thm, "Url", "http:/*www.example.com/image/481989943");
	cJSON_AddNumberToObject(thm,"Height",125);
	cJSON_AddStringToObject(thm,"Width","100");
	cJSON_AddItemToObject(img,"IDs", cJSON_CreateIntArray(ids,4));

	out=cJSON_Print(root);	cJSON_Delete(root);	printf("%s\n",out);	free(out);

	/* Our array of "records": */

	root=cJSON_CreateArray();
	for (i=0;i<2;i++)
	{
		cJSON_AddItemToArray(root,fld=cJSON_CreateObject());
		cJSON_AddStringToObject(fld, "precision", fields[i].precision);
		cJSON_AddNumberToObject(fld, "Latitude", fields[i].lat);
		cJSON_AddNumberToObject(fld, "Longitude", fields[i].lon);
		cJSON_AddStringToObject(fld, "Address", fields[i].address);
		cJSON_AddStringToObject(fld, "City", fields[i].city);
		cJSON_AddStringToObject(fld, "State", fields[i].state);
		cJSON_AddStringToObject(fld, "Zip", fields[i].zip);
		cJSON_AddStringToObject(fld, "Country", fields[i].country);
	}
	
/*	cJSON_ReplaceItemInObject(cJSON_GetArrayItem(root,1),"City",cJSON_CreateIntArray(ids,4)); */
	
	out=cJSON_Print(root);	cJSON_Delete(root);	printf("%s\n",out);	free(out);

}

int main (int argc, const char * argv[]) {
	/* a bunch of json: */
	char text1[]="{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n}\n}";	
	char text2[]="[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]";
	char text3[]="[\n    [0, -1, 0],\n    [1, 0, 0],\n    [0, 0, 1]\n	]\n";
	char text4[]="{\n		\"Image\": {\n			\"Width\":  800,\n			\"Height\": 600,\n			\"Title\":  \"View from 15th Floor\",\n			\"Thumbnail\": {\n				\"Url\":    \"http:/*www.example.com/image/481989943\",\n				\"Height\": 125,\n				\"Width\":  \"100\"\n			},\n			\"IDs\": [116, 943, 234, 38793]\n		}\n	}";
	char text5[]="[\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.7668,\n	 \"Longitude\": -122.3959,\n	 \"Address\":   \"\",\n	 \"City\":      \"SAN FRANCISCO\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94107\",\n	 \"Country\":   \"US\"\n	 },\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.371991,\n	 \"Longitude\": -122.026020,\n	 \"Address\":   \"\",\n	 \"City\":      \"SUNNYVALE\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94085\",\n	 \"Country\":   \"US\"\n	 }\n	 ]";

	/* Process each json textblock by parsing, then rebuilding: */
#if 1
	doit(text1, false);
	doit(text2, false);	
	doit(text3, false);
	doit(text4, false);
	doit(text5, false);
#endif
	/* Parse standard testfiles: */
/*	dofile("../../tests/test1"); */
/*	dofile("../../tests/test2"); */
/*	dofile("../../tests/test3"); */
/*	dofile("../../tests/test4"); */
/*	dofile("../../tests/test5"); */

	/* Now some samplecode for building objects concisely: */
	create_objects();
	printf("Garden test:\n");
	dofile("./test.json");

	MsgUpInfo_t MsgUpInfoTem = {
		255,"ZYC_TEST_MSG","ZYC_TEST_SIGNAL", 66.88, "2019-07-20_16:38:12"
	};
	char *Out = CompactMsgToTsp(&MsgUpInfoTem);
	printf("%s\n", Out);
	FreeCompactMsg(Out);
	return 0;
}
