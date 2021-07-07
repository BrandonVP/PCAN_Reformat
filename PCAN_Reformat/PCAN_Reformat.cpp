/*
 Name:		PCAN_Reformat.ino
 Created:	6/22/2021 2:35:46 PM
 Author:	Brandon Van Pelt
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
#define DEVMODE 

// Pick a capture software
#define PCAN_View
//#define ScanTool
//#define PCAN_Explorer

// Pad frames shorter than 8 with zeros
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
            buffer.messageNum = messageNumber++;
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
        fprintf(fp, "% 7s   %9.3f   %04x   %d ", buffer.messageNum, buffer.time, buffer.id, buffer.length);

#if defined PAD_ZERO
        for (int i = 0; i < 8; i++)
        {
            (i < buffer.length) ? (fscanf(ptr, "%x", &buffer.data[i])) : (buffer.data[i] = 0);
            fprintf(fp, "  %02x", buffer.data[i]);
        }
#else
        for (int i = 0; i < buffer.length; i++)
        {
            fscanf(ptr, "%x", &buffer.data[i]);
            //printf("%02x ", buffer.data[i]);

            fprintf(fp, "  %02x", buffer.data[i]);
        }
#endif
        //printf("\n");
        fprintf(fp, "\n");
    }
}

// TODO(Brandon) Convert to use with multiple capture software
// Find all unique IDs and add them to list
void formatTRC(char* filename, char* fileOut, int id)
{

    FILE* ptr = fopen(filename, "r");
    FILE* fp = fopen(fileOut, "w+");

    if (ptr == NULL)
    {
        printf("no such file.");
        return;
    }

    char line_in[MAX_STRING_LENGTH];

    for (int i = 0; i < FILE_HEADER_SIZE; i++)
    {
        fgets(line_in, MAX_STRING_LENGTH, ptr);
    }

    // Msg Num, Time, ID, Length
    while (fscanf(ptr, "%s %f %*s %*s %x %*s %d", &buffer.messageNum, &buffer.time, &buffer.id, &buffer.length) != EOF)
    {
        for (int i = 0; i < MAX_CHAR_LENGTH; i++)
        {
            if (buffer.messageNum[i] == ')')
            {
                buffer.messageNum[i] = ' ';
                i = MAX_CHAR_LENGTH;
            }
        }
        //printf("ID: %0x\n", buffer.id);
        if (buffer.id == id)
        {
            fprintf(fp, "% 7s   %9.3f   %04x   %d ", buffer.messageNum, buffer.time, buffer.id, buffer.length);
        }

        for (int i = 0; i < buffer.length; i++)
        {
            fscanf(ptr, "%x", &buffer.data[i]);
            //printf("%02x ", buffer.data[i]);

            if (buffer.id == id)
            {
                fprintf(fp, "  %02x", buffer.data[i]);
            }
        }
        //printf("\n");
        if (buffer.id == id)
        {
            fprintf(fp, "\n");
        }
    }
}

// Call with filename to filter
int main(int argc, char* argv[])
{
#if defined DEVMODE
    char filename[] = "rogue.trc";
    char fileOut[] = "rogue.txt";
#else 
    char filename[] = argv[1];
    char fileOut[] = argv[2];
#endif

    //formatTRC(filename, fileOut, 0x137);
    formatTRC(filename, fileOut);

    printf("Task Complete\n");
}