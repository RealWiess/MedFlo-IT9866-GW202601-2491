/******************************************************************************
 *
 *  Copyright (C) 2014 The Android Open Source Project

 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#ifndef __PB_API_H__
#define __PB_API_H__

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
** Constants and types
*****************************************************************************/
#define MAX_PHONE_NUMBER_SIZE  32
#define MAX_NAME_SIZE          64
#define MAX_PHONE_NUMBER_COUNT 4
#define MAX_PHONEBOOK_USER     512

#define STATE_WAIT_VCARD_BEGIN 0
#define STATE_PARSE_VCARD_INFO 1

typedef struct {
    uint8_t pNumber[MAX_PHONE_NUMBER_SIZE];
} PhoneNumber;

typedef struct {
    uint8_t     pName[MAX_NAME_SIZE];
    PhoneNumber ptPhoneNumber[MAX_PHONE_NUMBER_COUNT];
    int phoneNumCount;
} PB_User;

typedef struct {
    PB_User* ptPhoneBookUser[MAX_PHONEBOOK_USER];
    int      userCount;
} PhoneBook;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

/*******************************************************************************
**
** Function         PB_GeneratePhonebook
**
** Description      This function is used to parse vcard buffer then generate phonebook.
**
**                  Input Parameters:
**                      pVcfBuffer:  input vcard buffer 
**
**                      bufferSize:  size of the vcard buffer                      
**                      
**                  Output Parameters:
**                      None
**
**
** Returns          None.
**
*******************************************************************************/
void PB_GeneratePhonebook(uint8_t* pVcBuffer, uint32_t bufferSize);

/*******************************************************************************
**
** Function         PB_SearchPhonebookByNumber
**
** Description      This function is used to serach phonebook by phone number.
**
**                  Input Parameters:
**                      pPhoneNum:  input of phone number buffer 
**
**                      bufferSize: size of input phone number buffer                   
**                      
**                  Output Parameters:
**                      None
**
**
** Returns          NULL: Not hit user data
**                  PB_User* : Address of user data.
**
*******************************************************************************/
PB_User* PB_SearchPhonebookByNumber(uint8_t* pPhoneNum, int bufferSize);

/*******************************************************************************
**
** Function         PB_DestroyPhonebook
**
** Description      This function is used to destroy existed phonebook.
**
**                  Input Parameters:
**                      None
**                      
**                  Output Parameters:
**                      None
**
**
** Returns          None.
**
*******************************************************************************/
void PB_DestroyPhonebook(void);

/*******************************************************************************
**
** Function         PB_GetPhonebook
**
** Description      This function is used to get existed phonebook.
**
**                  Input Parameters:
**                      None
**
**                  Output Parameters:
**                      PhoneBook
**
**
** Returns          None.
**
*******************************************************************************/
PhoneBook PB_GetPhonebook(void);

#ifdef __cplusplus
}
#endif

#endif ///__PB_API_H__