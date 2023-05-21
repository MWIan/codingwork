/*
 Matthew Ianiro
 0995619
 CIS*2750 Assignment 2
 mianiro@uoguelph.ca
 */

#include "CalendarParser.h"
#include <ctype.h>


typedef enum alarms {NONE, AUDIO, DISPLAY, EMAIL, ERROR} anAlarm;
typedef enum otherErrors {GOOD, BAD} otherError;
typedef enum jsonValues {STRING, OTHER} JSONValue;

typedef struct jsonObjectItem {
    char *string;
    char *value;
    JSONValue jvalue;
} JSON;


anAlarm findAlarmType(char *desc, char delimiter);
ICalErrorCode extractDateTime(DateTime *toCreate, char *desc);
void toUpperCase(char **line);
void removeComments(char **buffer);
bool checkVersion(char *version);
otherError writeProperties(FILE *fp, List *properties);
otherError writeEvents(FILE *fp, List *events);
otherError writeDate(FILE *fp, DateTime date, int creationOrStart);
otherError writeAlarms(FILE *fp, List *alarms);
ICalErrorCode validateCalendarProperties(List *properties);
ICalErrorCode validateEvents(List *events);
ICalErrorCode validateEventProperties(List *properties);
ICalErrorCode validateDateTime(DateTime v);
ICalErrorCode validateAlarms(List *alarms);
ICalErrorCode validateAlarmProperties(List *properties, anAlarm alarmType);
char *getSummary(List *properties);
ListIterator createIteratorCONST(const List* list);
JSON **parseJSONObject(const char *object, int *numItems);
char *createString(int start, int end, const char *string, int nameORdesc, JSONValue *jvalue); /* 0 - name, 1 - desc */
void deleteJSON(JSON **item, int count);
void switchError(ICalErrorCode errorFound, ICalErrorCode *toChange);
int truncate(double toTruncate);
char *calendarFileToJSON(char *filename);
char *calendarToEventJSON(char *filename);
char *alarmToJSON(const Alarm* alarm);
char *propToJSON(const Property *prop);
char *createCalendarFromAjax(char *filename, char *version, char *prodid, char *creationdate, char *creationtime, char *startdate, char *starttime, char *utc, char *summary, char *uid);
char *addEventtoCalendarFromAjax(char *filename, char *creationdate, char *creationtime, char *startdate, char *starttime, char *utc, char *summary, char *uid);


char *addEventtoCalendarFromAjax(char *filename, char *creationdate, char *creationtime, char *startdate, char *starttime, char *utc, char *summary, char *uid){
    Calendar *obj;
    Event *e;
    Property *sum;

    if (createCalendar(filename, &obj) != OK){
        return "BAD";
    }

    e = malloc(sizeof(Event));
    if (e == NULL){
        return "BAD";
    }

    strcpy(e->UID, uid);
    e->properties = initializeList(printProperty, deleteProperty, compareProperties);
    e->alarms = initializeList(printAlarm, deleteAlarm, compareAlarms);
    strcpy(e->creationDateTime.date, creationdate);
    strcpy(e->creationDateTime.time, creationtime);
    if (strcmp(utc, "true") == 0){
        e->creationDateTime.UTC = true;
    } else {
        e->creationDateTime.UTC = false;
    }

    strcpy(e->startDateTime.date, startdate);
    strcpy(e->startDateTime.time, starttime);
    if (strcmp(utc, "true") == 0){
        e->startDateTime.UTC = true;
    } else {
        e->startDateTime.UTC = false;
    }

    sum = malloc(sizeof(Property) + strlen(summary)+1);
    if (sum == NULL){
        return "BAD";
    }
    strcpy(sum->propName, "SUMMARY");
    strcpy(sum->propDescr, summary);

    /* add summary to event */
    insertFront(e->properties, sum);
        
    /* add event to calendar */
    insertFront(obj->events, e);

    /* create the icalendar file */
    if (writeCalendar(filename, obj) != OK){
        return "BAD";
    } else {
        return calendarFileToJSON(filename);
    }

    deleteCalendar(obj);
}

char *createCalendarFromAjax(char *filename, char *version, char *prodid, char *creationdate, char *creationtime, char *startdate, char *starttime, char *utc, char *summary, char *uid){
    Calendar *obj;
    Event *e;
    Property *sum;
    int i;

    obj = malloc(sizeof(Calendar));
    if (obj == NULL){
        return "BAD";
    }
    obj->version = atof(version);
    strcpy(obj->prodID, prodid);

    obj->properties = initializeList(printProperty, deleteProperty, compareProperties);
    obj->events = initializeList(printEvent, deleteEvent, compareEvents);

    /* create a calendar with the first event */
    e = malloc(sizeof(Event));
    if (e == NULL){
        return "BAD";
    }
    strcpy(e->UID, uid);
    e->properties = initializeList(printProperty, deleteProperty, compareProperties);
    e->alarms = initializeList(printAlarm, deleteAlarm, compareAlarms);
    strcpy(e->creationDateTime.date, creationdate);
    strcpy(e->creationDateTime.time, creationtime);
    if (strcmp(utc, "true") == 0){
        e->creationDateTime.UTC = true;
    } else {
        e->creationDateTime.UTC = false;
    }

    strcpy(e->startDateTime.date, startdate);
    strcpy(e->startDateTime.time, starttime);
    if (strcmp(utc, "true") == 0){
        e->startDateTime.UTC = true;
    } else {
        e->startDateTime.UTC = false;
    }

    sum = malloc(sizeof(Property) + strlen(summary)+1);
    if (sum == NULL){
        return "BAD";
    }
    strcpy(sum->propName, "SUMMARY");
    strcpy(sum->propDescr, summary);

    /* add summary to event */
    insertFront(e->properties, sum);
        
    /* add event to calendar */
    insertFront(obj->events, e);

    /* create the icalendar file */
    if (writeCalendar(filename, obj) != OK){
        return "BAD";
    } else {
        return calendarFileToJSON(filename);
    }
    deleteCalendar(obj);
}


char *propListToJSON(const List *propList){
    char *string;
    int i;
    char *jsonProps[propList->length];
    int bytestoMalloc = 3 + propList->length-1;

    ListIterator iterate;
    Property *p;

    if (propList == NULL){
        return "[]";
    }

    iterate = createIteratorCONST(propList);
    p = nextElement(&iterate);

    for (i = 0; i < propList->length; i++){
        jsonProps[i] = malloc(sizeof(char*));
        if (jsonProps[i] == NULL){
            return "[]";
        }
        jsonProps[i] = propToJSON(p);
        bytestoMalloc += strlen(jsonProps[i]);
        p = nextElement(&iterate);
    }
    string = malloc(bytestoMalloc);
    if (string == NULL){
        return "[]";
    }
    string[bytestoMalloc-1] = '\0';
    strcpy(string, "[");
    for (i = 0; i < propList->length; i++){
        strcat(string, jsonProps[i]);
        if (i < propList->length-1){
            strcat(string, ",");
        }
    }
    strcat(string, "]");
    return string;
}

char *propToJSON(const Property *prop){
    char *string;
    int size;
    if (prop == NULL){
        return "{}";
    }

    size = 200 + strlen(prop->propDescr) + 25;

    string = malloc(size);
    if (string == NULL){
        return "{}";
    }

    snprintf(string, size, "{\"name\":\"%s\",\"desc\":\"%s\"}", prop->propName, prop->propDescr);
    return string;
}

char *alarmToJSON(const Alarm* alarm){
    char *string;
    int size;

    if (alarm == NULL){
        return "{}";
    }

    size = 200 + strlen(alarm->trigger)+30;

    string = malloc(size);
    if (string == NULL){
        return "{}";
    }

    snprintf(string, size, "{\"action\":\"%s\",\"trigger\":\"%s\",\"numProps\":%d}", alarm->action, alarm->trigger, alarm->properties->length);

    return string;
}

char *alarmListtoJSON(const List *alarmList){
    char *string;
    int i;
    char *jsonAlarms[alarmList->length];
    int bytestoMalloc = 3 + alarmList->length-1; /* includes both square brackets with null terminating character and all commas*/

    ListIterator iterate;
    Alarm *a;

    if (alarmList == NULL){
        return "[]";
    }

    iterate = createIteratorCONST(alarmList);
    a = nextElement(&iterate);

    for (i = 0; i < alarmList->length; i++){
        jsonAlarms[i] = malloc(sizeof(char*));
        if (jsonAlarms[i] == NULL){
            return "[]";
        }

        jsonAlarms[i] = alarmToJSON(a);
        bytestoMalloc += strlen(jsonAlarms[i]);
        a = nextElement(&iterate);
    }

    string = malloc(bytestoMalloc);
    if (string == NULL){
        return "[]";
    }
    string[bytestoMalloc-1] = '\0';
    strcpy(string, "[");
    for (i = 0; i < alarmList->length; i++){
        strcat(string, jsonAlarms[i]);
        if (i < alarmList->length-1){
            strcat(string, ",");
        }
    }
    strcat(string, "]");
    return string;
}


char *calendarToEventJSON(char *filename){

    Calendar *obj = NULL;
    char *json;
    if (createCalendar(filename, &obj) != OK){
        return "[]";
    }
    if (validateCalendar(obj) != OK){
        return "[]";
    }

    json = eventListToJSON(obj->events);
    deleteCalendar(obj);

    return json;
}

char *calendarFileToJSON(char *filename){
    char *json;
    Calendar *obj = NULL;

    if (createCalendar(filename, &obj) != OK){
        return "{}";
    }

    if (validateCalendar(obj) != OK){
        return "{}";
    }

    json = calendarToJSON(obj);
    deleteCalendar(obj);
    return json;
}


/** Function to converting a DateTime into a JSON string
 *@pre N/A
 *@post DateTime has not been modified in any way
 *@return A string in JSON format
 *@param prop - a DateTime struct
 **/
char* dtToJSON(DateTime prop){
    char *string;

    if (prop.UTC == true){
        string = malloc(49);
        if (string == NULL){
            return NULL;
        }
        snprintf(string, 49, "{\"date\":\"%s\",\"time\":\"%s\",\"isUTC\":true}", prop.date, prop.time);
    } else
    if (prop.UTC == false){
        string = malloc(50);
        if (string == NULL){
            return NULL;
        }
        snprintf(string, 50, "{\"date\":\"%s\",\"time\":\"%s\",\"isUTC\":false}", prop.date, prop.time);
    } else {
        return NULL;
    }
    return string;
}

/** Function to converting an Event into a JSON string
 *@pre Event is not NULL
 *@post Event has not been modified in any way
 *@return A string in JSON format
 *@param event - a pointer to an Event struct
 **/
char* eventToJSON(const Event* event){
    char *string;
    char *startDate;
    char numProps[5], numAlarms[5];
    char *summary;
    int numBytestoMalloc = 64; /* main characters including null terminating */
    char *alarms, *props;

    if (event == NULL){
        string = malloc(3);
        if (string == NULL){
            return NULL;
        }
        string[2] = '\0';
        strcpy(string, "{}");
        return string;
    }

    alarms = alarmListtoJSON(event->alarms);
    props = propListToJSON(event->properties);

    startDate = dtToJSON(event->startDateTime);

    sprintf(numProps, "%d", event->properties->length+3);
    sprintf(numAlarms, "%d", event->alarms->length);

    numBytestoMalloc += strlen(startDate) + strlen(numProps) + strlen(numAlarms) + strlen(alarms) + strlen(props) + strlen(event->UID) + 30;

    summary = getSummary(event->properties);
    
    if (summary == NULL){
        string = malloc(numBytestoMalloc);
        if (string == NULL){
            return NULL;
        }
        snprintf(string, numBytestoMalloc, "{\"uid\":\"%s\",\"startDT\":%s,\"numProps\":%s,\"numAlarms\":%s,\"summary\":\"\",\"alarms\":%s,\"properties\":%s}", event->UID, startDate, numProps, numAlarms, alarms, props);
    } else {
        numBytestoMalloc += strlen(summary);
        string = malloc(numBytestoMalloc);
        if (string == NULL){
            return NULL;
        }
    snprintf(string, numBytestoMalloc, "{\"uid\":\"%s\",\"startDT\":%s,\"numProps\":%s,\"numAlarms\":%s,\"summary\":\"%s\",\"alarms\":%s,\"properties\":%s}", event->UID, startDate, numProps, numAlarms, summary, alarms, props);
    }

    return string;
}

/** Function to find the summary of a list of properties
 */
char *getSummary(List *properties){
    ListIterator iterate;
    Property *prop;
    int i;

    iterate = createIterator(properties);
    prop = nextElement(&iterate);

    for (i = 0; i < properties->length; i++){
        if (strcmp(prop->propName, "SUMMARY") == 0){
            return prop->propDescr;
        }
        prop = nextElement(&iterate);
    }
    return NULL;
}

/** Function to converting an Event list into a JSON string
 *@pre Event list is not NULL
 *@post Event list has not been modified in any way
 *@return A string in JSON format
 *@param eventList - a pointer to an Event list
 **/
char* eventListToJSON(const List* eventList){
    char *string;
    int i;
    char *jsonEvents[eventList->length];
    int bytestoMalloc = 3 + eventList->length-1; /* includes both square brackets with null terminating character and all commas*/

    ListIterator iterate;
    Event *e;

    if (eventList == NULL){
        string = malloc(3);
        if (string == NULL){
            return NULL;
        }
        string[2] = '\0';
        strcpy(string, "[]");
        return string;
    }

    iterate = createIteratorCONST(eventList);
    e = nextElement(&iterate);

    for (i = 0; i < eventList->length; i++){
        jsonEvents[i] = malloc(sizeof(char*));
        if (jsonEvents[i] == NULL){
            return NULL;
        }
        jsonEvents[i] = eventToJSON(e);
        bytestoMalloc += strlen(jsonEvents[i]);
        e = nextElement(&iterate);
    }

    string = malloc(bytestoMalloc);
    if (string == NULL){
        return NULL;
    }
    string[bytestoMalloc-1] = '\0';

    strcpy(string, "[");
    for (i = 0; i < eventList->length; i++){
        strcat(string, jsonEvents[i]);
        if (i < eventList->length-1){
            strcat(string, ",");
        }
    }
    strcat(string, "]");

    return string;
}

/** Function to create iterator with a CONST list
 */
ListIterator createIteratorCONST(const List* list){
    ListIterator iter;

    iter.current = list->head;
    
    return iter;
}

/** Function to converting a Calendar into a JSON string
 *@pre Calendar is not NULL
 *@post Calendar has not been modified in any way
 *@return A string in JSON format
 *@param cal - a pointer to a Calendar struct
 **/
char* calendarToJSON(const Calendar* cal){
    char *string;
    int numProps;
    int numEvents;
    int numBytes;

    if (cal == NULL){
        string = malloc(3);
        if (string == NULL){
            return NULL;
        }
        string[2] = '\0';
        strcpy(string, "{}");
        return string;
    }

    numProps = 2 + cal->properties->length;
    numEvents = cal->events->length;
    numBytes = 90 + strlen(cal->prodID);/* num of bytes to malloc, this is mandatory characters plus null terminating */

    string = malloc(numBytes);
    if (string == NULL){
        return NULL;
    }

    snprintf(string, numBytes, "{\"version\":%.1f,\"prodID\":\"%s\",\"numProps\":%d,\"numEvents\":%d}", cal->version, cal->prodID, numProps, numEvents);

    return string;
}

/** function to truncate a double
 */
int truncate(double toTruncate){
    int toReturn;

    toReturn = (int)(toTruncate*1000000);
    toReturn /= 1000000;
    return toReturn;
}

/** Function to converting a JSON string into a Calendar struct
 *@pre JSON string is not NULL
 *@post String has not been modified in any way
 *@return A newly allocated and partially initialized Calendar struct
 *@param str - a pointer to a string
 **/
Calendar* JSONtoCalendar(const char* str){
    int numItems, i;
    JSON **items;
    Calendar *toReturn = NULL;
    bool prodid = false, version = false;

    if (str == NULL){
        return NULL;
    }

    items = parseJSONObject(str, &numItems);
    if (items == NULL){
        return NULL;
    }

    toReturn = malloc(sizeof(Calendar));
    if (toReturn == NULL){
        deleteJSON(items, numItems);
        return NULL;
    }

    /* initialize lists */
    toReturn->properties = initializeList(printProperty, deleteProperty, compareProperties);
    toReturn->events = initializeList(printEvent, deleteEvent, compareEvents);

    for (i = 0; i < numItems; i++){
        /* GO THROUGH JSON ITEMS HERE */
        if (strcmp(items[i]->string,"version") == 0){
            if (items[i]->jvalue != OTHER){
                deleteJSON(items, numItems);
                deleteCalendar(toReturn);
                return NULL;
            }
            if (checkVersion(items[i]->value)){
                toReturn->version = atof(items[i]->value);
            } else {
                deleteJSON(items, numItems);
                deleteCalendar(toReturn);
                return NULL;
            }
            if (version == false){
                version = true;
            } else { /* duplicate version */
                deleteJSON(items, numItems);
                deleteCalendar(toReturn);
                return NULL;
            }
        } else
        if (strcmp(items[i]->string, "prodID") == 0){
            if (items[i]->jvalue != STRING){
                deleteJSON(items, numItems);
                deleteCalendar(toReturn);
                return NULL;
            }
            if (strcmp(items[i]->value, "") == 0){
                deleteJSON(items, numItems);
                deleteCalendar(toReturn);
                return NULL;
            }
            if (prodid == false){
                prodid = true;
            } else { /* duplicate prodid */
                deleteJSON(items, numItems);
                deleteCalendar(toReturn);
                return NULL;
            }
            strcpy(toReturn->prodID, items[i]->value);
        } else {
            deleteJSON(items, numItems);
            deleteCalendar(toReturn);
            return NULL;
        }
    }

    if (prodid == false || version == false){
        deleteJSON(items, numItems);
        deleteCalendar(toReturn);
        return NULL;
    }


    deleteJSON(items, numItems);
    return toReturn;
}

/** Function to parse json object into names with values
 */
JSON **parseJSONObject(const char *object, int *numItems){
    int length, startChar, endChar;
    int count = 0;
    bool done = false;
    JSON **items = NULL;
    char *name, *desc;
    bool mallocd = false;
    JSONValue jvalue;

    if (object == NULL){
        return NULL;
    }

    length = strlen(object);
    /* verify start and end characters are valid */
    if (object[0] != '{' || object[length-1] != '}' || length < 5){
        return NULL;
    }

    startChar = endChar = 1;

    while (!done){
        if (object[startChar] != '"'){ /* must be the start of the name string */
            if (mallocd){
                deleteJSON(items, count);
            }
            return NULL;
        }
        endChar++;
        /* go to end of name string */
        while (object[endChar] != '"'){
            if (object[endChar] == '\\'){
                if (object[endChar+1] == '"'){
                    endChar++; /* skip over */
                }
            }
            endChar++;
            if (endChar >= length){
                if (mallocd){
                    deleteJSON(items, count);
                }
                return NULL;
            }
        }
        /* when we get to the final double quotation 
        create the NAME */
        name = createString(startChar, endChar, object, 0, &jvalue);
        if (name == NULL){
            if (mallocd){
                deleteJSON(items, count);
            }
            return NULL;
        }

        if (mallocd == false){
            items = malloc(sizeof(JSON*));
            if (items == NULL){
                free(name);
                return NULL;
            }
            items[count] = malloc(sizeof(JSON));
            if (items[count] == NULL){
                free(items);
                free(name);
                return NULL;
            }
            mallocd = true;
        } else {
            items = realloc(items, sizeof(JSON*)*(count+1));
            if (items == NULL){
                free(name);
                return NULL;
            }
            items[count] = malloc(sizeof(JSON));
            if (items[count] == NULL){
                free(name);
                deleteJSON(items, count);
                return NULL;
            }
        }

        items[count]->string = name;

        /* check for colon to delimit name from description */
        endChar++;
        startChar = endChar;
        if (object[endChar] != ':'){
            return NULL;
        }
        endChar++;
        /* go to end of description (we will see comma followed by double quotation OR just closing curly bracket) */
        while (object[endChar] != ',' && object[endChar] != '}'){
            endChar++;
            if (endChar >= length){
                return NULL;
            }
            if (object[endChar] == ','){
                if (object[endChar+1] != '"'){
                    endChar++;
                    continue;
                }
            }
        }
        desc = createString(startChar, endChar, object, 1, &jvalue);

        /* add description */
        items[count]->value = desc;
        items[count]->jvalue = jvalue;

        if (object[endChar] == '}'){
            if (endChar >= length-1){
                done = true; /* must be the last character */
            } else {
                deleteJSON(items, count+1);
                return NULL;
            }
        }

        endChar++;
        startChar = endChar;
        count++;
        name = NULL;
        desc = NULL;
    }
    *numItems = count;
    return items;
}

/** Function to delete JSON item
 */
void deleteJSON(JSON **item, int count){
    int i;

    for (i = 0; i < count; i++){
        free(item[i]->string);
        free(item[i]->value);
        free(item[i]);
    }
    free(item);
}

/** Function to create a subset of a string NOT including start and end characters
 */
char *createString(int start, int end, const char *string, int nameORdesc, JSONValue *jvalue){
    char *toReturn;
    int i;
    int length;
    bool isString = false;

    /* check to see if starts/ends with quotations, if so we will remove them */
    if (string[start+1] == '"' && string[end-1] == '"'){
        if (nameORdesc == 1){
            *jvalue = STRING;
        }
        isString = true;
        start++;
        end--;
    } else {
        if (nameORdesc == 1){
            *jvalue = OTHER;
        }
    }
    
    length = end-start-1;

    if (length < 0){
        return NULL;
    }

    toReturn = malloc(end-start);
    if (toReturn == NULL){
        return NULL;
    }
    toReturn[length] = '\0';

    for (i = 0; i < length; i++){
        toReturn[i] = string[start+i+1];
    }

    /* go through return and replace any \" with "*/
    if (isString || nameORdesc == 0){
        for (i = 0; i < length-1; i++){
            if (toReturn[i] == '\\'){
                if (toReturn[i+1] == '"'){
                    memcpy(&toReturn[i], &toReturn[i+1], length-i);
                } else
                if (toReturn[i+1] == 'r'){
                    toReturn[i+1] = '\r';
                    memcpy(&toReturn[i], &toReturn[i+1], length-i);
                } else
                if (toReturn[i+1] == 'n'){
                    toReturn[i+1] = '\n';
                    memcpy(&toReturn[i], &toReturn[i+1], length-i);
                } else
                if (toReturn[i+1] == 't'){
                    toReturn[i+1] = '\t';
                    memcpy(&toReturn[i], &toReturn[i+1], length-i);
                }
            }
        }
    }


    return toReturn;
}

/** Function to converting a JSON string into an Event struct
 *@pre JSON string is not NULL
 *@post String has not been modified in any way
 *@return A newly allocated and partially initialized Event struct
 *@param str - a pointer to a string
 **/
Event* JSONtoEvent(const char* str){
    int numItems;
    JSON **items;
    Event *toReturn = NULL;

    if (str == NULL){
        return NULL;
    }

    items = parseJSONObject(str, &numItems);
    if (items == NULL){
        return NULL;
    }

    if (numItems != 1){
        deleteJSON(items, numItems);
        return NULL;
    }

    toReturn = malloc(sizeof(Event));
    if (toReturn == NULL){
        deleteJSON(items, numItems);
        return NULL;
    }

    /* initialize lists */
    toReturn->properties = initializeList(printProperty, deleteProperty, compareProperties);
    toReturn->alarms = initializeList(printAlarm, deleteAlarm, compareAlarms);

    /* check UID */
    if (strcmp(items[0]->string, "UID") != 0){
        deleteJSON(items, numItems);
        deleteEvent(toReturn);
        return NULL;
    }

    if (strcmp(items[0]->value, "") == 0){
        deleteJSON(items, numItems);
        deleteEvent(toReturn);
        return NULL;
    }

    if (items[0]->jvalue != STRING){
        deleteJSON(items, numItems);
        deleteEvent(toReturn);
        return NULL;
    }

    strcpy(toReturn->UID, items[0]->value);

    deleteJSON(items, numItems);
    return toReturn;
}

/** Function to adding an Event struct to an ixisting Calendar struct
 *@pre arguments are not NULL
 *@post The new event has been added to the calendar's events list
 *@return N/A
 *@param cal - a Calendar struct
 *@param toBeAdded - an Event struct
 **/
void addEvent(Calendar* cal, Event* toBeAdded){
    if (cal == NULL || toBeAdded == NULL){
        return;
    }

    insertBack(cal->events, toBeAdded);
}



/** Function to validating an existing a Calendar object
 *@pre Calendar object exists and is not null
 *@post Calendar has not been modified in any way
 *@return the error code indicating success or the error encountered when validating the calendar
 *@param obj - a pointer to a Calendar struct
 **/
ICalErrorCode validateCalendar(const Calendar* obj){
    
    if (obj == NULL){
        return INV_CAL;
    }

    /* make sure product id exists, and lists are not null*/
    if (strcmp(obj->prodID, "") == 0 || obj->events == NULL || obj->properties == NULL){
        return INV_CAL;
    }

    /* make sure events list is not empty, must have at least one event */
    if (obj->events->length < 1){
        return INV_CAL;
    }

    /* validate calendar properties */
    if (validateCalendarProperties(obj->properties) == INV_CAL){
        return INV_CAL;
    }

    /* validate events */
    return validateEvents(obj->events);
}

/** Function to validate events list of a Calendar object
 */
ICalErrorCode validateEvents(List *events){
    ICalErrorCode eToReturn = OK;
    ListIterator iterate;
    Event *event;
    int i;

    iterate = createIterator(events);
    event = nextElement(&iterate);


    for (i = 0; i < events->length; i++){
        /* validate hardcoded properties first */
        if (strcmp(event->UID, "") == 0){
            eToReturn = INV_EVENT;
            return eToReturn;
        }
        if (validateDateTime(event->creationDateTime) != OK){
            eToReturn = INV_EVENT;
            return eToReturn;
        }
        if (validateDateTime(event->startDateTime) != OK){
            eToReturn = INV_EVENT;
            return eToReturn;
        }

        /* validate event properties */
        if (validateEventProperties(event->properties) != OK){
            eToReturn = INV_EVENT;
            return eToReturn;
        }

        /* validate event alarms */
        if (validateAlarms(event->alarms) != OK){
            switchError(INV_ALARM, &eToReturn);
        }
        event = nextElement(&iterate);
    }
    return eToReturn;
}

/** Function to change an ical error code
 */
void switchError(ICalErrorCode errorFound, ICalErrorCode *toChange){

    if (*toChange == INV_EVENT){
        return;
    } else {
        *toChange = errorFound;
    }
    return;
}

/** Function to validate properties list in an event
 */
ICalErrorCode validateEventProperties(List *properties){
    ListIterator iterate;
    Property *prop;
    int i;
    char *name, *desc;

    /* EVENT PROPERTIES THAT MUST NOT OCCUR MORE THAN ONCE */
    /* THESE PROPERTIES ARE OPTIONAL */
    bool class = false, created = false, description = false;
    bool geo = false, lastmod = false, location = false;
    bool organizer = false, priority = false, seq = false;
    bool status = false, summary = false, transp = false;
    bool url = false, recurid = false;

    /* OPTIONAL BUT MUST NOT APPEAR TOGETHER */
    bool dtend = false, duration = false;

    /* OPTIONAL AND CAN OCCUR MORE THAN ONCE */
    /*
    RRULE, ATTACH, ATTENDEE, CATEGORIES, COMMENT, CONTACT,
    EXDATE, RSTATUS, RELATED, RESOURCES, RDATE
    */

   if (properties == NULL){
       return INV_EVENT;
   }


    iterate = createIterator(properties);
    prop = nextElement(&iterate);

    for (i = 0; i < properties->length; i++){
        name = prop->propName;
        desc = prop->propDescr;

        if (strcmp(desc, "") == 0){ /* make sure the description exists */
            return INV_EVENT;
        }

        if (strcmp(name, "CLASS") == 0){
            if (class == false){
                class = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "CREATED") == 0){
            if (created == false){
                created = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "DESCRIPTION") == 0){
            if (description == false){
                description = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "GEO") == 0){
            if (geo == false){
                geo = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "LAST-MODIFIED") == 0){
            if (lastmod == false){
                lastmod = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "LOCATION") == 0){
            if (location == false){
                location = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "ORGANIZER") == 0){
            if (organizer == false){
                organizer = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "PRIORITY") == 0){
            if (priority == false){
                priority = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "SEQUENCE") == 0){
            if (seq == false){
                seq = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "STATUS") == 0){
            if (status == false){
                status = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "SUMMARY") == 0){
            if (summary == false){
                summary = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "TRANSP") == 0){
            if (transp == false){
                transp = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "URL") == 0){
            if (url == false){
                url = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "RECURRENCE-ID") == 0){
            if (recurid == false){
                recurid = true;
            } else {
                return INV_EVENT;
            }
        } else
        if (strcmp(name, "DTEND") == 0){
            if (dtend == false){
                dtend = true;
            }
            if (duration == true){
                return INV_EVENT;
            }
        } else 
        if (strcmp(name, "DURATION") == 0){
            if (duration == false){
                duration = true;
            }
            if (dtend == true){
                return INV_EVENT;
            }
        } else
        /* OPTIONAL PROPERTIES THAT MAY OCCUR MORE THAN ONCE */
        if (strcmp(name, "RRULE") == 0){
        } else
        if (strcmp(name, "ATTACH") == 0){
        } else
        if (strcmp(name, "ATTENDEE") == 0){
        } else
        if (strcmp(name, "CATEGORIES") == 0){
        } else
        if (strcmp(name, "COMMENT") == 0){
        } else
        if (strcmp(name, "CONTACT") == 0){
        } else
        if (strcmp(name, "EXDATE") == 0){
        } else
        if (strcmp(name, "REQUEST-STATUS") == 0){
        } else
        if (strcmp(name, "RELATED-TO") == 0){
        } else
        if (strcmp(name, "RESOURCES") == 0){
        } else
        if (strcmp(name, "RDATE") == 0){
        } else {
            return INV_EVENT;
        }

        prop = nextElement(&iterate);
    }

    return OK;
}


/** Function to validate an alarm
 */
ICalErrorCode validateAlarms(List *alarms){
    ListIterator iterate;
    Alarm *alarm;
    int i;
    anAlarm alarmType;

    iterate = createIterator(alarms);
    alarm = nextElement(&iterate);

    for (i = 0; i < alarms->length; i++){
        /* verify action */
        if (strcmp(alarm->action, "AUDIO") == 0){
            alarmType = AUDIO; /* can only be of type audio */
        } else {
            return INV_ALARM;
        }

        /* verify trigger */
        if (alarm->trigger == NULL){
            return INV_ALARM;
        }
        if (strlen(alarm->trigger) == 0 || strcmp(alarm->trigger, "") == 0){
            return INV_ALARM;
        }

        /* verify alarm properties */
        if (alarm->properties == NULL){
            return INV_ALARM;
        }
        if (validateAlarmProperties(alarm->properties, alarmType) != OK){
            return INV_ALARM;
        }

        alarm = nextElement(&iterate);
    }
    return OK;
}

/** Function to verify alarm properties
 */
ICalErrorCode validateAlarmProperties(List *properties, anAlarm alarmType){
    ListIterator iterate;
    Property *prop;
    int i;
    char *name, *desc;

    /* ALARM PROPERTY COMPONENTS */
    /* these properties are optional but must occur if the other occurs, can only occur once */
    bool duration = false, repeat = false;

    /* for AUDIO, is OPTIONAL but MUST NOT occur more than once */
    bool attach = false;

    /* for DISPLAY, is REQUIRED but MUST NOT occur more than once */
    bool description = false;

    /* for EMAIL, is REQUIRED but MUST NOT occur more than once */
    /* DESCRIPTION */
    bool summary = false;

    /* for EMAIL, is REQUIRED and CAN occur more than once */
    bool attendee = false;

    /* for EMAIL, is OPTIONAL and MAY occur more than once */
    /* ATTACH */

    if (properties == NULL){
        return INV_ALARM;
    }


    iterate = createIterator(properties);
    prop = nextElement(&iterate);

    for(i = 0; i < properties->length; i++){
        name = prop->propName;
        desc = prop->propDescr;

        /* make sure description isnt empty */
        if (strcmp(desc, "") == 0){
            return INV_ALARM;
        }

        /* verify property names for each audio item */
        if (alarmType == AUDIO){
            /* audio can only have duration, repeat, attach */
            if (strcmp(name, "DURATION") == 0){
                if (duration == false){
                    duration = true;
                } else {
                    return INV_ALARM;
                }
            } else
            if (strcmp(name, "REPEAT") == 0){
                if (repeat == false){
                    repeat = true;
                } else {
                    return INV_ALARM;
                }
            } else
            if (strcmp(name, "ATTACH") == 0){
                if (attach == false){
                    attach = true;
                } else {
                    return INV_ALARM;
                }
            } else {
                return INV_ALARM;
            }
            
        } else
        if (alarmType == DISPLAY){
            /* display can only have duration, repeat, description */
            if (strcmp(name, "DURATION") == 0){
                if (duration == false){
                    duration = true;
                } else {
                    return INV_ALARM;
                }
            } else
            if (strcmp(name, "REPEAT") == 0){
                if (repeat == false){
                    repeat = true;
                } else {
                    return INV_ALARM;
                }
            } else
            if (strcmp(name, "DESCRIPTION") == 0){
                if (description == false){
                    description = true;
                } else {
                    return INV_ALARM;
                }
            } else {
                return INV_ALARM;
            }
        } else
        if (alarmType == EMAIL){
            /* email can only have description, summary, attendee, duration, repeat, attach */
            if (strcmp(name, "DURATION") == 0){
                if (duration == false){
                    duration = true;
                } else {
                    return INV_ALARM;
                }
            } else
            if (strcmp(name, "REPEAT") == 0){
                if (repeat == false){
                    repeat = true;
                } else {
                    return INV_ALARM;
                }
            } else
            if (strcmp(name, "DESCRIPTION") == 0){
                if (description == false){
                    description = true;
                } else {
                    return INV_ALARM;
                }
            } else
            if (strcmp(name, "SUMMARY") == 0){
                if (summary == false){
                    summary = true;
                } else {
                    return INV_ALARM;
                }
            } else
            if (strcmp(name, "ATTENDEE") == 0){
                if (attendee == false){
                    attendee = true;
                }
            } else
            if (strcmp(name, "ATTACH") == 0){
            } else {
                return INV_ALARM;
            }

        } else {
            return INV_ALARM;
        }

        prop = nextElement(&iterate);
    }

    /* verify duration and repeat */
    if (duration != repeat){
        return INV_ALARM;
    }

    /* verify DISPLAY */
    if (alarmType == DISPLAY){
        if (description == false){
            return INV_ALARM;
        }
    } else
    /* verify EMAIL */
    if (alarmType == EMAIL){
        if (description == false){
            return INV_ALARM;
        } else
        if (summary == false){
            return INV_ALARM;
        } else 
        if (attendee == false){
            return INV_ALARM;
        }
    }


    return OK;
}

/** Function to validate a datetime
 */
ICalErrorCode validateDateTime(DateTime v){
    int i;
    /* validate DATE */
    if (strcmp(v.date, "") == 0){ /* make sure not empty */
        return INV_EVENT;
    }
    if (strlen(v.date) != 8){ /* must be in format YYYYMMDD */
        return INV_EVENT;
    }
    for (i = 0; i < 8; i++){
        if (!isdigit(v.date[i])){ /* all chars must be digits */
            return INV_EVENT;
        }
    }

    /* validate TIME */
    if (strcmp(v.time, "") == 0){ /* make sure not empty */
        return INV_EVENT;
    }
    if (strlen(v.time) != 6){ /* must be in format HHMMSS */
        return INV_EVENT;
    }
    for (i = 0; i < 6; i++){
        if (!isdigit(v.time[i])){
            return INV_EVENT; /* all chars must be digits */
        }
    }

    return OK;
}

/** Function to validate properties list of a Calendar object
 */
ICalErrorCode validateCalendarProperties(List *properties){
    ListIterator iterate;
    Property *prop;
    int i;
    char *name, *desc;
    bool calscale = false, method = false;

    iterate = createIterator(properties);
    prop = nextElement(&iterate);

    for (i = 0; i < properties->length; i++){
        name = prop->propName;
        desc = prop->propDescr;

        /* check to make sure name and desc are valid */
        if (strcmp(desc, "") == 0){
            return INV_CAL;
        }

        if (strcmp(name, "CALSCALE") == 0){
            if (calscale == false){ /* calscale can only occur once */
                calscale = true;
            } else {
                return INV_CAL;
            }
            prop = nextElement(&iterate);
            continue;
        } else
        if (strcmp(name, "METHOD") == 0){
            if (method == false){ /* method can only occur once */
                method = true;
            } else {
                return INV_CAL;
            }
            prop = nextElement(&iterate);
            continue;
        } else
        if (strcmp(name, "PRODID") == 0){
            return INV_CAL; /* prodid is hardcoded, cannot be a property of it */
        } else
        if (strcmp(name, "VERSION") == 0){
            return INV_CAL; /* version is hardcoded, cannot be a property of it */
        }

        return INV_CAL;
    }

    return OK;
}

/** Function to writing a Calendar object into a file in iCalendar format.
 *@pre Calendar object exists, is not null, and is valid
 *@post Calendar has not been modified in any way, and a file representing the
 Calendar contents in iCalendar format has been created
 *@return the error code indicating success or the error encountered when parsing the calendar
 *@param obj - a pointer to a Calendar struct
 **/
ICalErrorCode writeCalendar(char* fileName, const Calendar* obj){

    FILE *fp = NULL;

    if (fileName == NULL || obj == NULL){
        /* make sure filename and calendar object are valid */
        return WRITE_ERROR;
    }

    fp = fopen(fileName, "w");
    if (fp == NULL){
        /* make sure file can be opened */
        return WRITE_ERROR;
    }

    /* BEGIN CALENDAR */
    fputs("BEGIN:VCALENDAR\r\n", fp);

    /* PRINT CALENDAR VERSION */
    fprintf(fp, "VERSION:%f\r\n", obj->version);
    /* PRINT CALENDAR PRODUCT ID */
    fprintf(fp, "PRODID:%s\r\n", obj->prodID);

    /* PRINT ALL CALENDAR PROPRTIES */
    if (writeProperties(fp, obj->properties) == BAD){
        remove(fileName);
        return WRITE_ERROR;
    }

    /* PRINT ALL EVENTS */
    if (writeEvents(fp, obj->events) == BAD){
        remove(fileName);
        return WRITE_ERROR;
    }

    /* END CALENDAR */
    fputs("END:VCALENDAR\r\n", fp);

    fclose(fp);
    return OK;
}

/** Function to write a list of events to a file
 * 
 */
otherError writeEvents(FILE *fp, List *events){
    ListIterator iterate;
    Event *element;
    int i;

    iterate = createIterator(events);
    element = nextElement(&iterate);
    
    for (i = 0; i < events->length; i++){
        if (element == NULL){
            return BAD;
        }
        fprintf(fp, "BEGIN:VEVENT\r\n");
        /* print UID */
        fprintf(fp, "UID:%s\r\n", element->UID);
        /* print creation Date */
        if (writeDate(fp, element->creationDateTime, 0) == BAD){
            return BAD;
        }
        /* print start Date */
        if (writeDate(fp, element->startDateTime, 1) == BAD){
            return BAD;
        }
        /* print all event properties */
        if (writeProperties(fp, element->properties) == BAD){
            return BAD;
        }
        /* print all alarms */
        if (writeAlarms(fp, element->alarms) == BAD){
            return BAD;
        }
        fprintf(fp, "END:VEVENT\r\n");
        element = nextElement(&iterate);
    }

    return GOOD;
}

otherError writeAlarms(FILE *fp, List *alarms){
    ListIterator iterate;
    Alarm *element;
    int i;

    iterate = createIterator(alarms);
    element = nextElement(&iterate);

    for (i = 0; i < alarms->length; i++){
        if (element == NULL){
            return BAD;
        }
        fprintf(fp, "BEGIN:VALARM\r\n");

        /* print action */
        fprintf(fp, "ACTION:%s\r\n", element->action);
        /* print trigger */
        fprintf(fp, "TRIGGER:%s\r\n", element->trigger);

        /* print all alarm properties */
        if (writeProperties(fp, element->properties) == BAD){
            return BAD;
        }

        fprintf(fp, "END:VALARM\r\n");

        element = nextElement(&iterate);
    }
    return GOOD;
}

otherError writeDate(FILE *fp, DateTime date, int creationOrStart){
    char *name;
    /* 0 for creation date, 1 for start date */
    name = malloc(8);
    if (name == NULL){
        return BAD;
    }
    name[7] = '\0';
    if (creationOrStart == 0){
        strcpy(name, "DTSTAMP");
    } else {
        strcpy(name, "DTSTART");
    }

    /* creationDate: DTSTAMP */
    /* startDate: DTSTART */

    if (date.UTC == true){
        /* last letter will be Z */
        fprintf(fp, "%s:%sT%sZ\r\n", name, date.date, date.time);
    } else {
        /* last letter will NOT be Z */
        fprintf(fp, "%s:%sT%s\r\n", name, date.date, date.time);
    }
    free(name);

    return GOOD;
}

/** Function to write a list of properties to a file
 */
otherError writeProperties(FILE *fp, List *properties){
    ListIterator iterate;
    Property *element;
    int i;

    iterate = createIterator(properties);
    element = nextElement(&iterate);

    for (i = 0; i < properties->length; i++){
        if (element == NULL){
            return BAD;
        }
        fprintf(fp, "%s:%s\r\n", element->propName, element->propDescr);
        element = nextElement(&iterate);
    }
    return GOOD;
}

/** Function to create a Calendar object based on the contents of an iCalendar file.
 *@pre File name cannot be an empty string or NULL.  File name must have the .ics extension.
 File represented by this name must exist and must be readable.
 *@post Either:
 A valid calendar has been created, its address was stored in the variable obj, and OK was returned
 or
 An error occurred, the calendar was not created, all temporary memory was freed, obj was set to NULL, and the
 appropriate error code was returned
 *@return the error code indicating success or the error encountered when parsing the calendar
 *@param fileName - a string containing the name of the iCalendar file
 *@param a double pointer to a Calendar struct that needs to be allocated
 **/
ICalErrorCode createCalendar(char* fileName, Calendar** obj){
    
    Calendar *new = NULL;
    char *buffer, *line, *desc;
    FILE *fp;
    int size;
    int startChar, endChar;
    bool finished = false;
    
    char delimiter;
    bool versionFound = false, productidFound = false;
    bool calendarEnded = false, calendarStarted = false;
    
    /* calendar optional properties - optional but cannot occur more than once*/
    bool calendarCalScale = false, calendarMethod = false;
    
    /********************************/
    /* EVENT PROPERTIES START */
    bool eventPresent = false; /* there must be at least one event */
    bool addEvents = false, createNewEvent = false;
    
    /* event properties that are required but must not occur more than once */
    bool eventDTSTAMP = false, eventUID = false, eventDTSTART = false;
    
    /* event properties that are optional but must only occur once */
    bool eventClass = false, eventCreated = false, eventDescription = false, eventGeo = false, eventLastMod = false;
    bool eventLocation = false, eventOrganizer = false, eventPriority = false, eventSeq = false, eventStatus = false;
    bool eventSummary = false, eventTransp = false, eventURL = false, eventRecurid = false;
    
    /* event - DTEND or DURATION may appear in event, but only one can occur! */
    bool eventDTENDorDURATION = false;
    /* EVENT PROPERTIES END */
    /********************************/
    
    /********************************/
    /* ALARM PROPERTIES START */
    anAlarm alarmType = NONE;
    bool addAlarms = false, createNewAlarm = false;
    
    bool alarmAction = false, alarmTrigger = false;
    bool alarmDuration = false, alarmRepeat = false, alarmAttendee = false;
    
    int alarmDescription = 0, alarmSummary = 0, alarmAttach = 0;
    
    /* ALARM AUDIO
     REQUIRED, CAN ONLY OCCUR ONCE: ACTION, TRIGGER
     DURATION AND REPEAT MUST BE SPECIFIED SAME NUMBER OF TIMES (O OR 1) (CAN ONLY OCCUR ONCE EACH)
     ATTACH MUST NOT OCCUR MORE THAN ONCE
     */
    
    /* ALARM DISPLAY
     REQUIRED, CAN ONLY OCCUR ONCE: ACTION, DESCRIPTION, TRIGGER
     DURATION AND REPEAT MUST BE SPECIFIED SAME NUMBER OF TIMES (O OR 1) (CAN ONLY OCCUR ONCE EACH)
     */
    
    /* ALARM EMAIL
     REQUIRED, CAN ONLY OCCUR ONCE: ACTION, DESCRIPTION, TRIGGER, SUMMARY
     REQUIRED, CAN OCCUR MORE THAN ONCE: ATTENDEE
     DURATION AND REPEAT MUST BE SPECIFIED SAME NUMBER OF TIMES (O OR 1) (CAN ONLY OCCUR ONCE EACH)
     */
    /* ALARM PROPERTIES END */
    /********************************/
    
    
    Property *createProperty = NULL;
    Event *createEvent = NULL;
    Alarm *createAlarm = NULL;
    char *oldBuffer = NULL;
    
    
    if (fileName == NULL){
        return INV_FILE;
    }
    if (obj == NULL){
        return OTHER_ERROR;
    }
    
    
    *obj = NULL; /* we do this here so that if the file is invalid we don't have to set it to NULL every time*/
    
    /* check filename extension */
    if (strrchr(fileName, '.') == NULL){
        return INV_FILE; //no extension
    }
    if (strcmp(strrchr(fileName, '.'), ".ics") != 0){
        return INV_FILE; //extension isn't '.ics'
    }
    fp = fopen(fileName, "r");
    if (fp == NULL){
        //file not found
        return INV_FILE;
    }
    
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    buffer = malloc(size+1); //allocate space for entire file
    if (buffer == NULL){
        return OTHER_ERROR;
    }
    fseek(fp, 0, SEEK_SET);
    buffer[size] = '\0';
    
    if (buffer){
        fread(buffer, 1, size, fp);
        fclose(fp);
    } else {
        free(buffer);
        fclose(fp);
        return OTHER_ERROR;
    }
    if (strcmp(buffer, "") == 0){
        return INV_FILE; //empty string
    }
    
    new = malloc(sizeof(Calendar));
    if (new == NULL){
        free(buffer);
        return OTHER_ERROR;
    }

    /* first we create the properties and events list */
    new->properties = initializeList(printProperty, deleteProperty, compareProperties);
    new->events = initializeList(printEvent, deleteEvent, compareEvents);

    if (new->properties == NULL || new->events == NULL){
        return OTHER_ERROR;
    }
    
    
    startChar = endChar = 0;
    
    /* remove comments */
    removeComments(&buffer);
    size = strlen(buffer)+1;
    
    
    /* handle folded lines
     we will unfold the entire file at once
     what we do is look for a CRLF followed by a tab or space, then we will remove it
     */
    while (!finished){
        while (buffer[endChar] != '\r'){
            if (buffer[endChar] == '\0' || endChar >= size){
                //reached the end of the file (we're done unfolding)
                finished = true;
                break;
            }
            endChar++;
        }
        
        if (endChar <= strlen(buffer)-2){
            if (buffer[endChar+1] == '\n'){
                if (buffer[endChar+2] == '\t' || buffer[endChar+2] == ' '){
                    oldBuffer = buffer;
                    buffer = malloc(size-3);
                    buffer[size-4] = '\0';
                    if (buffer == NULL){
                        return OTHER_ERROR;
                    }
                    memcpy(buffer, oldBuffer, endChar);
                    memcpy(&buffer[endChar], &oldBuffer[endChar+3], size-(endChar+3));
                    free(oldBuffer);
                    size -= 3;
                } else {
                    endChar++;
                }
            }
        } else {
            finished = true;
        }
    }
    
    
    /*split lines into property name and property description
     check for each case: product id, version, begin, utc comment
     */
    finished = false;
    startChar = endChar = 0;
    while (finished == false){
        while (buffer[endChar] != ':' && buffer[endChar] != ';'){ //go to the first delimiter
            if (buffer[endChar] == '\0'){
                finished = true;
                if (eventPresent == false){
                    free(buffer);
                    deleteCalendar(new);
                    return INV_CAL;
                }
                if (calendarStarted == false || calendarEnded == false || versionFound == false || productidFound == false){
                    free(buffer);
                    deleteCalendar(new);
                    return INV_CAL;
                }
                //reached end of the file
                *obj = new;
                free(buffer);
                return OK;
            }
            if (buffer[endChar] == '\r'){
                //there is a line with no delimiter (invalid)
                free(buffer);
                deleteCalendar(new);
                return INV_CAL;
            }
            if (endChar > size){
                free(buffer);
                deleteCalendar(new);
                return INV_FILE;
            }
            endChar++;
        }
        
        delimiter = buffer[endChar];
        
        //when at first delimiter, copy the first word
        line = malloc(endChar - startChar + 1);
        if (line == NULL){
            free(buffer);
            deleteCalendar(new);
            return OTHER_ERROR;
        }
        line[endChar-startChar] = '\0';
        memcpy(line, &buffer[startChar], endChar-startChar);
        
        /* change line from lowercase to uppercase */
        toUpperCase(&line);
        
        //go to end of line, then copy the second word
        endChar++;
        startChar = endChar;
        while (buffer[endChar] != '\r'){
            if (endChar > size){
                free(buffer);
                deleteCalendar(new);
                free(line);
                return INV_FILE;
            }
            endChar++;
        }
        
        //when at end of line, copy description into desc
        desc = malloc(endChar - startChar + 1);
        if (desc == NULL){
            free(buffer);
            free(line);
            deleteCalendar(new);
            return OTHER_ERROR;
        }
        desc[endChar-startChar] = '\0';
        memcpy(desc, &buffer[startChar], endChar-startChar);
        
        if (calendarEnded == true){
            free(buffer);
            deleteCalendar(new);
            free(line);
            free(desc);
            return INV_CAL;
        }
        
        /* CHECK THE CONTENTS OF 'LINE' HERE AND MAKE CASES FOR EACH */
        if (strcmp(line, "BEGIN") == 0){
            
            if (delimiter == ';'){ /* delimiter MUST be ":" in BEGIN */
                free(line);
                free(desc);
                free(buffer);
                deleteCalendar(new);
                return INV_CAL;
            }
            if (strcmp(desc,"VCALENDAR") == 0){
                if (calendarStarted == true){
                    free(line);
                    free(desc);
                    free(buffer);
                    deleteCalendar(new);
                    return INV_CAL;
                }
                calendarStarted = true;
            } else
                if (strcmp(desc,"VEVENT") == 0){
                    if (addEvents == true){
                        free(line);
                        free(desc);
                        free(buffer);
                        deleteCalendar(new);
                        return INV_EVENT;
                    }
                    eventPresent = true;
                    createNewEvent = true;
                    addEvents = true;
                } else
                    if (strcmp(desc, "VALARM") == 0){
                        addAlarms = true;
                        createNewAlarm = true;
                        if (addEvents == false){
                            /* alarm must be in event component */
                            free(line);
                            free(desc);
                            deleteCalendar(new);
                            free(buffer);
                            return INV_ALARM;
                        }
                    }
        } else
            if (calendarStarted == false){
                free(line);
                free(desc);
                deleteCalendar(new);
                free(buffer);
                return INV_CAL;
            }
            else
                if (strcmp(line, "END") == 0){
                    if (delimiter == ';'){ /* delimiter MUST be ":" in END */
                        free(line);
                        free(desc);
                        free(buffer);
                        deleteCalendar(new);
                        return INV_CAL;
                    }
                    if (strcmp(desc, "VCALENDAR") == 0){
                        if (addEvents == true){
                            free(line);
                            free(desc);
                            deleteCalendar(new);
                            free(buffer);
                            return INV_EVENT;
                        } else
                            if (addAlarms == true){
                                free(line);
                                free(desc);
                                deleteCalendar(new);
                                free(buffer);
                                return INV_ALARM;
                            }
                        calendarEnded = true;
                    } else
                        if (strcmp(desc, "VEVENT") == 0){
                            if (addEvents == false){
                                free(line);
                                free(desc);
                                deleteCalendar(new);
                                free(buffer);
                                return INV_EVENT;
                            } else
                            if (addAlarms == true){
                                free(line);
                                free(desc);
                                deleteCalendar(new);
                                free(buffer);
                                return INV_ALARM;
                            } else
                            if (eventUID == false || eventDTSTAMP == false || eventDTSTART == false) {
                                free(line);
                                free(desc);
                                deleteCalendar(new);
                                free(buffer);
                                return INV_EVENT;
                            }
                            
                            /* reset event parameters */
                            eventClass = eventCreated = eventDescription = eventGeo = eventLastMod = false;
                            eventLocation = eventOrganizer = eventPriority = eventSeq = eventStatus = false;
                            eventSummary = eventTransp = eventURL = eventRecurid = false;
                            eventDTENDorDURATION = eventDTSTAMP = eventUID = eventDTSTART = false;
                            addEvents = createNewEvent = false;
                            createEvent = NULL;
                        } else
                            if (strcmp(desc,"VALARM") == 0){
                                if (addAlarms == false){
                                    free(line);
                                    free(desc);
                                    free(buffer);
                                    deleteCalendar(new);
                                    return INV_ALARM;
                                }
                                addAlarms = false;
                                
                                /* check to make sure alarm has been ended correctly */
                                if (alarmAction == false || alarmTrigger == false){
                                    free(line);
                                    free(desc);
                                    deleteCalendar(new);
                                    free(buffer);
                                    return INV_ALARM;
                                }
                                
                                if (alarmDuration != alarmRepeat){
                                    free(line);
                                    free(desc);
                                    deleteCalendar(new);
                                    free(buffer);
                                    return INV_ALARM;
                                }
                                
                                if (alarmType == AUDIO){
                                    /* make sure audio was done properly
                                     attach is optional, but must not occur more than once
                                     */
                                    if (alarmAttach > 1){
                                        free(line);
                                        free(desc);
                                        deleteCalendar(new);
                                        free(buffer);
                                        return INV_ALARM;
                                    }
                                    
                                } else
                                    if (alarmType == EMAIL){
                                        /* make sure email was done properly
                                         description is required, must not occur more than once
                                         attendee is required, can occur > 1
                                         summary is required, must not occur more than once
                                         */
                                        if (alarmDescription != 1 || alarmSummary != 1 || alarmAttendee == false){
                                            free(line);
                                            free(desc);
                                            deleteCalendar(new);
                                            free(buffer);
                                            return INV_ALARM;
                                        }
                                        
                                    } else
                                        if (alarmType == DISPLAY){
                                            /* make sure display was done properly
                                             description is required, must not occur more than once
                                             */
                                            if (alarmDescription != 1){
                                                free(line);
                                                free(desc);
                                                deleteCalendar(new);
                                                free(buffer);
                                                return INV_ALARM;
                                            }
                                        }
                                
                                /* reset variables */
                                alarmType = NONE;
                                addAlarms = createNewAlarm = alarmAction = alarmTrigger = alarmDuration = alarmRepeat = alarmAttendee = false;
                                alarmDescription = alarmSummary = alarmAttach = 0;
                            }
                } else
                    if (strcmp(line, "VERSION") == 0){
                        if (versionFound == false){
                            if (checkVersion(desc) == false){
                                deleteCalendar(new);
                                free(buffer);
                                free(line);
                                free(desc);
                                return INV_VER;
                            }
                            new->version = atof(desc);
                            versionFound = true;
                        } else {
                            free(buffer);
                            deleteCalendar(new);
                            free(line);
                            free(desc);
                            return DUP_VER; //if version was already found, there can't be 2 versions!
                        }
                    } else
                        if (strcmp(line, "PRODID") == 0){
                            if (productidFound == false){
                                if (strcmp(desc, "") == 0){
                                    deleteCalendar(new);
                                    free(line);
                                    free(desc);
                                    free(buffer);
                                    return INV_PRODID;
                                }
                                memcpy(new->prodID, desc, strlen(desc)+1);
                                productidFound = true;
                            } else {
                                deleteCalendar(new);
                                free(line);
                                free(desc);
                                free(buffer);
                                return DUP_PRODID; //if product id was already found, there can't be 2 product ids!
                            }
                        } else
                            if (addEvents == true){
                                /* add events */
                                
                                /* if we are adding a new event then create a new event, otherwise just add properties / alarms and stuff to the list head */
                                if (createNewEvent == true){
                                    /* we create an event */
                                    createEvent = malloc(sizeof(Event));
                                    if (createEvent == NULL){
                                        free(line);
                                        free(desc);
                                        deleteCalendar(new);
                                        free(buffer);
                                        return OTHER_ERROR;
                                    }
                                    createEvent->properties = initializeList(printProperty, deleteProperty, compareProperties);
                                    createEvent->alarms = initializeList(printAlarm, deleteAlarm, compareAlarms);
                                    insertFront(new->events, createEvent);
                                    createNewEvent = false;
                                }
                                
                                if (addAlarms == true){
                                    /* add alarms */

                                    if (strcmp(line, "") == 0){
                                        free(buffer);
                                        free(line);
                                        free(desc);
                                        deleteCalendar(new);
                                        return INV_ALARM;
                                    }
                                    if (strcmp(desc, "") == 0){
                                        free(buffer);
                                        free(line);
                                        free(desc);
                                        deleteCalendar(new);
                                        return INV_ALARM;
                                    }
                                    
                                    if (createNewAlarm == true){
                                        /* we create an alarm */
                                        createAlarm = malloc(sizeof(Alarm));
                                        if (createAlarm == NULL){
                                            free(line);
                                            free(buffer);
                                            free(desc);
                                            deleteCalendar(new);
                                            return OTHER_ERROR;
                                        }
                                        createAlarm->properties = initializeList(printProperty, deleteProperty, compareProperties);
                                        createAlarm->trigger = NULL;
                                        insertFront(createEvent->alarms, createAlarm);
                                        createNewAlarm = false;
                                    }
                                    
                                    if (strcmp(line, "ACTION") == 0){
                                        
                                        /* make sure action has not already been specified */
                                        if (alarmAction == true){
                                            free(line);
                                            free(desc);
                                            free(buffer);
                                            deleteCalendar(new);
                                            return INV_ALARM;
                                        }
                                        
                                        /* we are going to extract the actual type (AUDIO/DISPLAY/EMAIL) */
                                        alarmType = findAlarmType(desc, delimiter);
                                        
                                        if (alarmType == ERROR){
                                            free(line);
                                            free(desc);
                                            free(buffer);
                                            deleteCalendar(new);
                                            return OTHER_ERROR;
                                        } else
                                            if (alarmType == NONE){
                                                /* invalid alarm type */
                                                free(line);
                                                free(desc);
                                                free(buffer);
                                                deleteCalendar(new);
                                                return INV_ALARM;
                                            }
                                        
                                        alarmAction = true;
                                        strcpy(createAlarm->action, desc);
                                    } else
                                        if (strcmp(line, "TRIGGER") == 0){
                                            /* make sure trigger has not already been specified */
                                            if (alarmTrigger == true){
                                                free(line);
                                                free(desc);
                                                free(buffer);
                                                deleteCalendar(new);
                                                return INV_ALARM;
                                            }
                                            
                                            /* store everything in trigger */
                                            createAlarm->trigger = malloc(strlen(desc)+1);
                                            if (createAlarm->trigger == NULL){
                                                free(line);
                                                free(desc);
                                                free(buffer);
                                                deleteCalendar(new);
                                                return OTHER_ERROR;
                                            }
                                            createAlarm->trigger[strlen(desc)] = '\0';
                                            strcpy(createAlarm->trigger, desc);
                                            alarmTrigger = true;
                                        } else {
                                            
                                            /* update variables depending on the property name */
                                            if (strcmp(line, "DURATION") == 0){
                                                if (alarmDuration == true){
                                                    free(line);
                                                    free(desc);
                                                    free(buffer);
                                                    deleteCalendar(new);
                                                    return INV_ALARM;
                                                }
                                                alarmDuration = true;
                                            } else
                                                if (strcmp(line, "REPEAT") == 0){
                                                    if (alarmRepeat == true){
                                                        free(line);
                                                        free(desc);
                                                        free(buffer);
                                                        deleteCalendar(new);
                                                        return INV_ALARM;
                                                    }
                                                    alarmRepeat = true;
                                                } else
                                                    if (strcmp(line, "ATTACH") == 0){
                                                        alarmAttach++;
                                                    } else
                                                        if (strcmp(line, "DESCRIPTION") == 0){
                                                            alarmDescription++;
                                                        } else
                                                            if (strcmp(line, "SUMMARY") == 0){
                                                                alarmSummary++;
                                                            } else
                                                                if (strcmp(line, "ATTENDEE") == 0){
                                                                    alarmAttendee = true;
                                                                }
                                            
                                            /* ADD TO ALARM PROPERTIES */
                                            /* first we create a property */
                                            createProperty = malloc(sizeof(Property) + strlen(desc)+1); //hold property and description
                                            if (createProperty == NULL){
                                                free(line);
                                                free(desc);
                                                free(buffer);
                                                deleteCalendar(new);
                                                return OTHER_ERROR;
                                            }
                                            
                                            /* now we add the contents of line and desc to the property */
                                            strcpy(createProperty->propName, line);
                                            strcpy(createProperty->propDescr, desc);
                                            
                                            /* finally we add the property to the back of the properties list */
                                            insertBack(createAlarm->properties, createProperty);
                                            
                                            /* and point new property to NULL */
                                            createProperty = NULL;
                                            /* end of adding alarms to event */
                                        }
                                } else
                                    if (strcmp(line, "") == 0){
                                        free(line);
                                        free(desc);
                                        deleteCalendar(new);
                                        free(buffer);
                                        return INV_EVENT;
                                    } else
                                    if (strcmp(desc, "") == 0){
                                        free(line);
                                        free(desc);
                                        deleteCalendar(new);
                                        free(buffer);
                                        return INV_EVENT;
                                    } else
                                    if (strcmp(line, "UID") == 0){
                                        if (eventUID == true){
                                            free(line);
                                            free(desc);
                                            deleteCalendar(new);
                                            free(buffer);
                                            return INV_EVENT;
                                        } else {
                                            if (strcmp(desc, "") == 0){
                                                free(line);
                                                free(desc);
                                                free(buffer);
                                                deleteCalendar(new);
                                                return INV_EVENT;
                                            }
                                            eventUID = true;
                                        }
                                        
                                        /* add user id to the event */
                                        strcpy(createEvent->UID, desc);
                                    } else
                                        if (strcmp(line, "DTSTAMP") == 0){
                                            /* add creation date to event*/
                                            
                                            //make sure creation date has not already been added
                                            if (eventDTSTAMP == true){
                                                free(line);
                                                free(desc);
                                                deleteCalendar(new);
                                                free(buffer);
                                                return INV_EVENT;
                                            } else {
                                                eventDTSTAMP = true;
                                            }
                                            
                                            /* EXTRACT DATE AND TIME from DTSTAMP */
                                            if (extractDateTime(&(createEvent->creationDateTime), desc) != OK){
                                                free(line);
                                                free(desc);
                                                deleteCalendar(new);
                                                free(buffer);
                                                return INV_DT;
                                            }
                                        } else
                                            if (strcmp(line,"DTSTART") == 0){
                                                /* add date start tp event */
                                                //make sure start date has not already been added
                                                if (eventDTSTART == true){
                                                    free(line);
                                                    free(desc);
                                                    deleteCalendar(new);
                                                    free(buffer);
                                                    return INV_DT;
                                                } else {
                                                    eventDTSTART = true;
                                                }
                                                
                                                /* EXTRACT DATE AND TIME from DTSTART */
                                                if (extractDateTime(&(createEvent->startDateTime), desc) != OK){
                                                    free(line);
                                                    free(desc);
                                                    deleteCalendar(new);
                                                    free(buffer);
                                                    return INV_DT;
                                                }
                                                
                                            }
                                            else {
                                                
                                                /* check line and update 'optional but can only occur once' parameters */
                                                if (strcmp(line, "CLASS") == 0){
                                                    if (eventClass == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventClass = true;
                                                    }
                                                } else if (strcmp(line, "CREATED") == 0){
                                                    if (eventCreated == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventCreated = true;
                                                    }
                                                } else if (strcmp(line, "DESCRIPTION") == 0){
                                                    if (eventDescription == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventDescription = true;
                                                    }
                                                } else if (strcmp(line, "GEO") == 0){
                                                    if (eventGeo == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventGeo = true;
                                                    }
                                                } else if (strcmp(line, "LAST-MODIFIED") == 0){
                                                    if (eventLastMod == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventLastMod = true;
                                                    }
                                                } else if (strcmp(line, "LOCATION") == 0){
                                                    if (eventLocation == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventLocation = true;
                                                    }
                                                } else if (strcmp(line, "ORGANIZER") == 0){
                                                    if (eventOrganizer == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventOrganizer = true;
                                                    }
                                                } else if (strcmp(line, "PRIORITY") == 0){
                                                    if (eventPriority == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventPriority = true;
                                                    }
                                                } else if (strcmp(line, "SEQUENCE") == 0){
                                                    if (eventSeq == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventSeq = true;
                                                    }
                                                } else if (strcmp(line, "STATUS") == 0){
                                                    if (eventStatus == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventStatus = true;
                                                    }
                                                } else if (strcmp(line, "SUMMARY") == 0){
                                                    if (eventSummary == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventSummary = true;
                                                    }
                                                } else if (strcmp(line, "TRANSP") == 0){
                                                    if (eventTransp == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventTransp = true;
                                                    }
                                                } else if (strcmp(line, "URL") == 0){
                                                    if (eventURL == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventURL = true;
                                                    }
                                                } else if (strcmp(line, "RECURRENCE-ID") == 0){
                                                    if (eventRecurid == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventRecurid = true;
                                                    }
                                                } else if (strcmp(line, "DTEND") == 0 || strcmp(line, "DURATION") == 0){
                                                    if (eventDTENDorDURATION == true){
                                                        free(line);
                                                        free(desc);
                                                        deleteCalendar(new);
                                                        free(buffer);
                                                        return INV_EVENT;
                                                    } else {
                                                        eventDTENDorDURATION = true;
                                                    }
                                                }
                                                
                                                /* now add property to event properties */
                                                createProperty = malloc(sizeof(Property) + strlen(desc)+1); //hold property and description
                                                if (createProperty == NULL){
                                                    free(line);
                                                    free(desc);
                                                    free(buffer);
                                                    deleteCalendar(new);
                                                    return OTHER_ERROR;
                                                }
                                                /* now we add the contents of line and desc to the property */
                                                strcpy(createProperty->propName, line);
                                                strcpy(createProperty->propDescr, desc);
                                                /* finally we add the property to the back of the properties list */
                                                insertBack(createEvent->properties, createProperty);
                                                /* and point new property to NULL */
                                                createProperty = NULL;
                                            }
                            }
                            else {
                                /* handle calendar properties */
                                
                                /* optional but can only occur once */
                                if (strcmp(line, "CALSCALE") == 0){
                                    if (calendarCalScale == true){
                                        free(line);
                                        free(desc);
                                        deleteCalendar(new);
                                        free(buffer);
                                        return INV_CAL;
                                    } else {
                                        calendarCalScale = true;
                                    }
                                } else
                                    if (strcmp(line, "METHOD") == 0){
                                        if (calendarMethod == true){
                                            free(line);
                                            free(desc);
                                            deleteCalendar(new);
                                            free(buffer);
                                            return INV_CAL;
                                        } else {
                                            calendarMethod = true;
                                        }
                                    }
                                
                                /* then we create a property */
                                createProperty = malloc(sizeof(Property) + strlen(desc)+1); //hold property and description
                                if (createProperty == NULL){
                                    free(line);
                                    free(desc);
                                    free(buffer);
                                    deleteCalendar(new);
                                    return OTHER_ERROR;
                                }
                                
                                /* now we add the contents of line and desc to the property */
                                strcpy(createProperty->propName, line);
                                strcpy(createProperty->propDescr, desc);
                                
                                /* finally we add the property to the back of the properties list */
                                insertBack(new->properties, createProperty);
                                
                                /* and point new property to NULL */
                                createProperty = NULL;
                            }
        
        /* go to the next line and start parsing */
        endChar+=2;
        startChar = endChar;
        free(line);
        free(desc);
    }
    free(buffer);
    deleteCalendar(new);
    return INV_FILE;
}


/** Function to find the alarm type of ACTION description
 * returns the alarm type
 */

anAlarm findAlarmType(char *desc, char delimiter){
    int i, j, length;
    char *actionparam;
    
    length = strlen(desc);
    i = 0;
    
    if (delimiter == ';'){
        while (desc[i] != ':'){
            i++;
            if (i > length){
                return NONE;
            }
        }
        actionparam = malloc(length - i);
        if (actionparam == NULL){
            return ERROR;
        }
        actionparam[length-i-1] = '\0';
        
        for (j = 0; j < length-i-1; j++){
            actionparam[j] = desc[j+i+1];
        }
        if (strcmp(actionparam, "AUDIO") == 0){
            free(actionparam);
            return AUDIO;
        } else
            if (strcmp(actionparam, "EMAIL") == 0){
                free(actionparam);
                return EMAIL;
            } else
                if (strcmp(actionparam, "DISPLAY") == 0){
                    free(actionparam);
                    return DISPLAY;
                } else {
                    free(actionparam);
                    return NONE;
                }
    } else {
        //when delimiter is ':' just look at the description
        if (strcmp(desc, "AUDIO") == 0){
            return AUDIO;
        } else
            if (strcmp(desc, "EMAIL") == 0){
                return EMAIL;
            } else
                if (strcmp(desc, "DISPLAY") == 0){
                    return DISPLAY;
                } else {
                    return NONE;
                }
    }
    
}

/** Function extracts the date and time from a description and returns an error code
 *
 */
ICalErrorCode extractDateTime(DateTime *toCreate, char *desc){
    
    int i, j, length;
    length = strlen(desc);
    i = length - 1;
    
    int testing = -1;
    char year[5];
    char month[3];
    char day[3];
    char hour[3];
    char minute[3];
    char second[3];
    
    year[4] = month[2] = day[2] = hour[2] = minute[2] = second[2] = '\0';
    
    if (length < 15){
        /* invalid format, must be at MINIMUM: YYYYMMDDTHHMMSS, so 15 chars*/
        toCreate = NULL;
        return INV_DT;
    }
    /* we are going to start from the end of the string and work backwards to extract the date and time */
    
    if (desc[i] == 'Z'){
        /* if last letter is Z, then it is UTC time */
        toCreate->UTC = true;
        i--;
    } else {
        toCreate->UTC = false;
    }
    
    for (j = 1; j >= 0; j--){
        /* allocate SECONDS */
        second[j] = desc[i];
        
        if (!isdigit(desc[i])){ //make sure char is a digit
            return INV_DT;
        }
        i--;
    }
    /* check seconds for range */
    testing = atoi(second);
    if (testing < 0 || testing > 60){
        return INV_DT;
    }
    
    for (j = 1; j >= 0; j--){
        /* allocate minutes */
        minute[j] = desc[i];
        if (!isdigit(desc[i])){ //make sure char is a digit
            return INV_DT;
        }
        i--;
    }
    /* check minutes for range */
    testing = atoi(minute);
    if (testing < 0 || testing > 59){
        return INV_DT;
    }
    
    for (j = 1; j >= 0; j--){
        /* allocate hour */
        hour[j] = desc[i];
        if (!isdigit(desc[i])){ //make sure char is a digit
            return INV_DT;
        }
        i--;
    }
    /* check hour for range */
    testing = atoi(hour);
    if (testing < 0 || testing > 23){
        return INV_DT;
    }
    
    if (desc[i] != 'T'){
        /* make sure the T exists */
        return INV_DT;
    }
    i--;
    for (j = 1; j >= 0; j--){
        /* allocate day */
        day[j] = desc[i];
        if (!isdigit(desc[i])){ //make sure char is a digit
            return INV_DT;
        }
        i--;
    }
    /* check day for range */
    testing = atoi(day);
    if (testing < 1 || testing > 31){
        return INV_DT;
    }
    
    for (j = 1; j >= 0; j--){
        /* allocate month */
        month[j] = desc[i];
        if (!isdigit(desc[i])){ //make sure char is a digit
            return INV_DT;
        }
        i--;
    }
    /* check month for range */
    testing = atoi(month);
    if (testing < 1 || testing > 12){
        return INV_DT;
    }
    for (j = 3; j >= 0; j--){
        /* allocate year */
        year[j] = desc[i];
        if (!isdigit(desc[i])){ //make sure char is a digit
            return INV_DT;
        }
        i--;
    }
    
    //now make sure there is a ':' or we are at the start of desc
    if (i >= 0){
        if (desc[i] != ':'){
            return INV_DT;
        }
    }
    
    /* create the date */
    strcpy((*toCreate).date, "");
    strcat((*toCreate).date, year);
    strcat((*toCreate).date, month);
    strcat((*toCreate).date, day);
    
    /* create the time */
    strcpy((*toCreate).time, "");
    strcat((*toCreate).time, hour);
    strcat((*toCreate).time, minute);
    strcat((*toCreate).time, second);
    
    return OK;
}

/** Function to check if a version is a number
 *
 */
bool checkVersion(char *version){
    int i;
    int numOfDots = 0;
    int length = strlen(version);
    if (version == NULL){
        return false;
    } else
        if (strcmp(version, "") == 0){
            return false;
        }
    
    //check to see if it is a valid number
    for (i = 0; i < length; i++){
        if (isdigit(version[i]) == false){
            if (version[i] == '.' && numOfDots < 1){
                numOfDots++;
            } else {
                return false;
            }
        }
    }
    return true;
}


/** Function to delete all calendar content and free all the memory.
 *@pre Calendar object exists, is not null, and has not been freed
 *@post Calendar object had been freed
 *@return none
 *@param obj - a pointer to a Calendar struct
 **/
void deleteCalendar(Calendar* obj){
    
    Property *propToDel = NULL;
    Event *eventToDel = NULL;
    
    if (obj == NULL){
        return; //nothing to free
    }
    
    /* DELETION OF LIST EVENTS */
    if (obj->events != NULL){
        while ( (eventToDel = deleteDataFromList(obj->events, getFromFront(obj->events))) != NULL){
            deleteEvent(eventToDel);
        }
        free(obj->events);
    }
    
    /* DELETION OF LIST PROPERTIES */
    if (obj->properties != NULL){
        while ( (propToDel = deleteDataFromList(obj->properties, getFromFront(obj->properties))) != NULL){
            deleteProperty(propToDel);
        }
        free(obj->properties);
    }
    
    
    free(obj);
}


/** Function to create a string representation of a Calendar object.
 *@pre Calendar object exists, is not null, and is valid
 *@post Calendar has not been modified in any way, and a string representing the Calndar contents has been created
 *@return a string contaning a humanly readable representation of a Calendar object
 *@param obj - a pointer to a Calendar struct
 **/
char* printCalendar(const Calendar* obj){
    
    char *string = malloc(20);
    if (string == NULL){
        return NULL;
    }
    string[19] = '\0';
    
    if (obj == NULL){
        free(string);
        return NULL;
    }
    
    strcpy(string, "This is a calendar!");
    
    return string;
}


/** Function to "convert" the ICalErrorCode into a humanly redabale string.
 *@return a string contaning a humanly readable representation of the error code by indexing into
 the descr array using rhe error code enum value as an index
 *@param err - an error code
 **/
char* printError(ICalErrorCode err){
    
    char *string = malloc(50);
    if (string == NULL){
        return NULL;
    }
    string[49] = '\0';
    
    switch(err){
        case(0):
            strcpy(string, "OK");
            break;
            
        case(1):
            strcpy(string, "Invalid File");
            break;
            
        case(2):
            strcpy(string, "Invalid Calendar");
            break;
            
        case(3):
            strcpy(string, "Invalid Version");
            break;
            
        case(4):
            strcpy(string, "Duplicate Version");
            break;
            
        case(5):
            strcpy(string, "Invalid Product ID");
            break;
            
        case(6):
            strcpy(string, "Duplicate Product ID");
            break;
            
        case(7):
            strcpy(string, "Invalid Event");
            break;
            
        case(8):
            strcpy(string, "Invalid Date");
            break;
            
        case(9):
            strcpy(string, "Invalid Alarm");
            break;
            
        case(10):
            strcpy(string, "Write Error");
            break;
            
        case(11):
            strcpy(string, "Other Error");
            break;
            
        default:
            strcpy(string, "Error not found");
            break;
            
    }
    
    return string;
}

/** Function to convert a string to upper case
 * modifies 'line' to be upper case
 */
void toUpperCase(char **line){
    int i;
    int length = strlen(*line);
    
    for (i = 0; i < length; i++){
        (*line)[i] = toupper((*line)[i]);
    }
}

/** Function to remove comments from the buffer
 *
 */
void removeComments(char **buffer){
    int i, j, length;
    
    if (*buffer == NULL){
        return;
    }
    
    length = strlen(*buffer);
    
    i = j = 0;
    
    while (i < length){
        /* check first letter of each line */
        if ((*buffer)[i] == ';'){
            j = i; //save j (start of comment)
            /* go to where \n is OR go to end of file '\0'*/
            while ((*buffer)[i] != '\n' && (*buffer)[i+1] != '\0'){
                i++;
            }
            
            //now remove the comment
            memcpy(&(*buffer)[j], &(*buffer)[i+1], length-i);
            length = strlen(*buffer);
            //(*buffer) = realloc(*buffer, length+1);
            i = j; //go back to j incase the next line is also a comment
            continue;
        }
        
        /* go to next line */
        while ((*buffer)[i] != '\n'){
            i++;
            if (i > length || (*buffer)[i] == '\0'){
                return; //reached end of file, we're done
            }
        }
        //when we get to \n
        i++;
    }
}


// ************* List helper functions - MUST be implemented ***************
void deleteEvent(void* toBeDeleted){
    freeList(((Event*)toBeDeleted)->properties);
    freeList(((Event*)toBeDeleted)->alarms);
    free(toBeDeleted);
}
int compareEvents(const void* first, const void* second){
    
    return 0;
}
char* printEvent(void* toBePrinted){
    char *string;
    
    if (toBePrinted == NULL){
        return NULL;
    } else {
        string = malloc(6);
        if (string == NULL){
            return NULL;
        }
        string[5] = '\0';
        strcpy(string, "event");
        return string;
    }
}

void deleteAlarm(void* toBeDeleted){
    if (toBeDeleted != NULL){
        if (((Alarm*) toBeDeleted)->trigger != NULL){
            free(((Alarm*) toBeDeleted)->trigger);
        }
        freeList(((Alarm*) toBeDeleted)->properties);
        free(toBeDeleted);
    }
}
int compareAlarms(const void* first, const void* second){
    return 0;
}
char* printAlarm(void* toBePrinted){
    char *string;
    if (toBePrinted == NULL){
        return NULL;
    }
    
    string = malloc(6);
    if (string == NULL){
        return NULL;
    }
    strcpy(string, "Alarm");
    
    return string;
}

void deleteProperty(void* toBeDeleted){
    if (toBeDeleted != NULL){
        free(toBeDeleted);
    }
}
int compareProperties(const void* first, const void* second){
    return 0;
}
char* printProperty(void* toBePrinted){
    char *string;
    
    if (toBePrinted == NULL){
        return NULL;
    }
    string = malloc(9);
    string[8] = '\0';
    strcpy(string, "Property");
    return string;
}

void deleteDate(void* toBeDeleted){
    if (toBeDeleted != NULL){
        free(toBeDeleted);
    }
}
int compareDates(const void* first, const void* second){
    return 0;
}
char* printDate(void* toBePrinted){
    char *string;
    if (toBePrinted == NULL){
        return NULL;
    }
    string = malloc(5);
    if (string == NULL){
        return NULL;
    }
    string[4] = '\0';
    strcpy(string, "Date");
    return string;
}
// **************************************************************************

