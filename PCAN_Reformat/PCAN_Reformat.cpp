/*
 Name:		  PCAN_Reformat.ino
 Created:	  6/22/2021 2:35:46 PM
 Author:	  Brandon Van Pelt
 Description: Formats data from multiple capture software to a single standard format
*/

// Disable Visual Studio's deprecated code warning
#define _CRT_SECURE_NO_WARNINGS

#include <algorithm>
#include <iostream>
#include <iostream>
#include <list>
#include <stdio.h>
#include <stdlib.h> 
#include <string>

/*=========================================================
    START SETTINGS
===========================================================*/
// Switches between hardwired filename to CMD argv[1] filename
// TODO: Finish command line function
#define DEVMODE // Leave this on until CMD is finished

// *** Pick a capture software ***
#define PCAN_View
//#define ScanTool
//#define PCAN_Explorer

// *** Filenames ***
#define FILE_IN "sgdm_15_40_20.trc"
#define FILE_OUT "sgdm_15_40_20(2A).txt"

// *** Filter by ID ***
#define FILTER_BY_ID
#define ID1 0x2A
//#define ID2 0x340
//#define ID3 0x3DE
//#define ID4 0x122
//#define ID5 0x3E8
//#define ID6 0x2FF
//#define ID7 0x00
//#define ID8 0x00
//#define ID9 0x00
//#define ID10 0x00

// *** Pad frames shorter than 8 with zeros ***
//#define PAD_ZERO

/*=========================================================
    END SETTINGS
===========================================================*/

#if defined PCAN_View
#define FILE_HEADER_SIZE 15
#endif
#if defined ScanTool
#define FILE_HEADER_SIZE 0
#endif
#if defined PCAN_Explorer
#define FILE_HEADER_SIZE 20
#endif

#define MAX_STRING_LENGTH 100
#define MAX_CHAR_LENGTH 20
#define DATA_SIZE 8

using namespace std;

typedef struct
{
    char messageNum[MAX_CHAR_LENGTH];
    float time;
    int id;
    int length;
    int data[DATA_SIZE];
} CANMSG;

CANMSG buffer;

int count = 0;

// Convert capture to standard output text file
void formatTRC(char* filename, char* fileOut)
{
    char line_in[MAX_STRING_LENGTH];

    FILE* ptr = fopen(filename, "r");
    FILE* fp = fopen(fileOut, "w+");

    if (ptr == NULL)
    {
        printf("File not found!");
        return;
    }

    // Remove the header from the file
    for (int i = 0; i < FILE_HEADER_SIZE; i++)
    {
        fgets(line_in, MAX_STRING_LENGTH, ptr);
    }

#if defined PCAN_View
    /*
    ;-------------------------------------------------------------------------------
    ;   Message   Time    Type ID     Rx/Tx
    ;   Number    Offset  |    [hex]  |  Data Length
    ;   |         [ms]    |    |      |  |  Data [hex] ...
    ;   |         |       |    |      |  |  |
    ;---+-- ------+------ +- --+----- +- +- +- +- -- -- -- -- -- -- --
        1     13212.973  DT 18DB33F1 Rx  8  02 01 00 00 00 00 00 00 
    */
    while (fscanf(ptr, "%s %f %*s %XS %*s %d", &buffer.messageNum, &buffer.time, &buffer.id, &buffer.length) != EOF)
    {
#endif

#if defined ScanTool
    /*
    ;-------------------------------------------------------------------------------
    ;   Message   Time     ID [hex]     
    ;   Number    Offset   |     Data Length
    ;   |         [ms]     |     |   Data [hex] ...
    ;   |         |        |     |   |
    ;---+---------+--------+-----+-  +- -- -- -- -- -- -- --
        1       108408    020D   6   AA  05  60  00  82  6A  00  00
    */
        int messageNumber = 0;
        while (fscanf(ptr, "%d %d %x %d", &buffer.messageNum, &buffer.time, &buffer.id, &buffer.length) != EOF)
        {
            
#endif

#if defined PCAN_Explorer
    /*
    ;-------------------------------------------------------------------------------
    ;   Message Number
    ;   |         Time Offset (ms)
    ;   |         |       Bus
    ;   |         |       |    Type
    ;   |         |       |    |       ID (hex)
    ;   |         |       |    |       |    Reserved
    ;   |         |       |    |       |    |   Data Length Code
    ;   |         |       |    |       |    |   |    Data Bytes (hex) ...
    ;   |         |       |    |       |    |   |    |
    ;   |         |       |    |       |    |   |    |
    ;---+-- ------+------ +- --+-- ----+--- +- -+-- -+ -- -- -- -- -- -- --
        1)         0.279 1  Rx        018E -  8    00 00 00 64 16 42 07 82 
    */
    while (fscanf(ptr, "%s %f %*s %*s %x %*s %d", &buffer.messageNum, &buffer.time, &buffer.id, &buffer.length) != EOF)
    {
        // Remove the ")" from the message number
        for (int i = 0; i < MAX_CHAR_LENGTH; i++)
        {
            if (buffer.messageNum[i] == ')')
            {
                buffer.messageNum[i] = ' ';
                i = MAX_CHAR_LENGTH;
            }
        }
#endif

        //printf("ID: %0x\n", buffer.id);
        fprintf(fp, "% 8s   % 11.3f   %04X   %d ", buffer.messageNum, buffer.time, buffer.id, buffer.length);

#if defined PAD_ZERO
        for (int i = 0; i < 8; i++)
        {
            (i < buffer.length) ? (fscanf(ptr, "%x", &buffer.data[i])) : (buffer.data[i] = 0);
            fprintf(fp, "  %02X", buffer.data[i]);
        }
#else
        for (int i = 0; i < buffer.length; i++)
        {
            fscanf(ptr, "%X", &buffer.data[i]);
            //printf("%02x ", buffer.data[i]);

            fprintf(fp, "  %02X", buffer.data[i]);
        }
#endif
        fprintf(fp, "\n");
    }
}

// Find onlys messages with passed ID
void formatTRC(char* filename, char* fileOut, int * filterIDs)
{
    char line_in[MAX_STRING_LENGTH];

    FILE* ptr = fopen(filename, "r");
    FILE* fp = fopen(fileOut, "w+");

    if (ptr == NULL)
    {
        printf("File not found!");
        return;
    }

    // Remove the header from the file
    for (int i = 0; i < FILE_HEADER_SIZE; i++)
    {
        fgets(line_in, MAX_STRING_LENGTH, ptr);
    }

    bool hasID = false;

#if defined PCAN_View
    /*
    ;-------------------------------------------------------------------------------
    ;   Message   Time    Type ID     Rx/Tx
    ;   Number    Offset  |    [hex]  |  Data Length
    ;   |         [ms]    |    |      |  |  Data [hex] ...
    ;   |         |       |    |      |  |  |
    ;---+-- ------+------ +- --+----- +- +- +- +- -- -- -- -- -- -- --
        1     13212.973  DT 18DB33F1 Rx  8  02 01 00 00 00 00 00 00
    */
    while (fscanf(ptr, "%s %f %*s %x %*s %d", &buffer.messageNum, &buffer.time, &buffer.id, &buffer.length) != EOF)
    {
#endif

#if defined ScanTool
    /*
    ;-------------------------------------------------------------------------------
    ;   Message   Time     ID
    ;   Number    Offset   [hex]  Data Length
    ;   |         [ms]     |      |  Data [hex] ...
    ;   |         |        |      |  |
    ;---+---------+--------+------+- +- -- -- -- -- -- -- --
                00059172  0233     8  00 57 6F 72 6C 64 00 00
    */
    int messageNumber = 0;
    while (fscanf(ptr, "%f %x %d", &buffer.time, &buffer.id, &buffer.length) != EOF)
    {
        sprintf(buffer.messageNum, "  %06d", messageNumber++);
#endif

#if defined PCAN_Explorer
    /*
    ;-------------------------------------------------------------------------------
    ;   Message Number
    ;   |         Time Offset (ms)
    ;   |         |       Bus
    ;   |         |       |    Type
    ;   |         |       |    |       ID (hex)
    ;   |         |       |    |       |    Reserved
    ;   |         |       |    |       |    |   Data Length Code
    ;   |         |       |    |       |    |   |    Data Bytes (hex) ...
    ;   |         |       |    |       |    |   |    |
    ;   |         |       |    |       |    |   |    |
    ;---+-- ------+------ +- --+-- ----+--- +- -+-- -+ -- -- -- -- -- -- --
        1)         0.279 1  Rx        018E -  8    00 00 00 64 16 42 07 82
    */
    while (fscanf(ptr, "%s %f %*s %*s %x %*s %d", &buffer.messageNum, &buffer.time, &buffer.id, &buffer.length) != EOF)
    {
        // Remove the ")" from the message number
        for (int i = 0; i < MAX_CHAR_LENGTH; i++)
        {
            if (buffer.messageNum[i] == ')')
            {
                buffer.messageNum[i] = ' ';
                i = MAX_CHAR_LENGTH;
            }
        }
#endif
        
        //for (int i = 0; i < sizeof(filterIDs) / sizeof(filterIDs[0]); i++)
        for (int i = 0; i < 8; i++)
        {
            if (filterIDs[i] == buffer.id)
            {
                hasID = true;
                break;
            }
        }

        //if (buffer.id == 0x42F || buffer.id == 0x326)
        if (hasID)
        {
            fprintf(fp, "% 8s   % 11.3f   %04x   %d ", buffer.messageNum, buffer.time, buffer.id, buffer.length);
        }

        for (int i = 0; i < buffer.length; i++)
        {
            fscanf(ptr, "%x", &buffer.data[i]);
            //printf("%02x ", buffer.data[i]);
            if (hasID)
            {
                fprintf(fp, "  %02x", buffer.data[i]);
            }
        }

        //printf("\n");
        //if (buffer.id == 0x42F || buffer.id == 0x326)
        if (hasID)
        {
            fprintf(fp, "\n");
        }
        hasID = false;
    }
}

// Call with filename to filter
int main(int argc, char* argv[])
{
#if defined DEVMODE
    char filename[] = FILE_IN;
    char fileOut[] = FILE_OUT;
#else 
    char filename[] = argv[1];
    char fileOut[] = argv[2];
#endif

#if defined FILTER_BY_ID
    int filterIDs[10];
    #if defined ID1
    filterIDs[0] = ID1;
    #else
    filterIDs[0] = 0x00;
    #endif

    #if defined ID2
    filterIDs[1] = ID2;
    #else
    filterIDs[1] = 0x00;
    #endif

    #if defined ID3
    filterIDs[2] = ID3;
    #else
    filterIDs[2] = 0x00;
    #endif

    #if defined ID4
    filterIDs[3] = ID4;
    #else
    filterIDs[3] = 0x00;
    #endif

    #if defined ID5
    filterIDs[4] = ID5;
    #else
    filterIDs[4] = 0x00;
    #endif

    #if defined ID6
    filterIDs[5] = ID6;
    #else
    filterIDs[5] = 0x00;
    #endif

    #if defined ID7
    filterIDs[6] = ID7;
    #else
    filterIDs[6] = 0x00;
    #endif

    #if defined ID8
    filterIDs[7] = ID8;
    #else
    filterIDs[7] = 0x00;
    #endif

    #if defined ID9
    filterIDs[8] = ID9;
    #else
    filterIDs[8] = 0x00;
    #endif

    #if defined ID10
    filterIDs[9] = ID10;
    #else
    filterIDs[9] = 0x00;
    #endif

    formatTRC(filename, fileOut, filterIDs);
#else
    formatTRC(filename, fileOut);
#endif

    printf("File format complete\n");
}