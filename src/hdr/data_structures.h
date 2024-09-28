/*
    Data structures used throughout CQL which hold information.
    
    Each row and column has a fixed data type it can store.
    
    The types that can be comprehended are in
    'CQLDataTypes'

    A 'CQLDocument' is a file where columns and rows are saved to.
    Each CQLDocument has metadata where the time created, modified,
    logs, etc are saved
        - All document and table snapshots are stored in the CQLDocument struct.

    A 'CQLTable' is a structure with information including the 
    table name and all the columns belonging to the table. Each
    CQLDocument will have a CQLTable.
*/

#ifndef _CQL_DATA_STRUCTURES_H_
#define _CQL_DATA_STRUCTURES_H_

#include <string>
#include <vector>
#include <ctime>
#include <unordered_map>
#include <map>
#include <any>
#include <fstream>
#include <iostream>

#include "json.h"

using JSON = nlohmann::json;

// Forward decl's
struct CQLColumn;
struct CQLDocument;

/*
    A row contained inside of a CQLColumn.
    The row can hold any type of data
*/
struct CQLRow 
{
    std::any    rowData;       // The data inside the row
    CQLColumn*  parentColumn;  // A pointer to the parent column, sort of like a linked list
    int         rowID;         // Unique number to identify the row
    bool        inUse = false; // Determine if the row is used, if not delete it
    int         rowIndex;      // The index of the row in the column
};

/*
    Column that contains rows,
    identified by its columnName
*/
struct CQLColumn 
{
    std::map<int, CQLRow*>          rows;          // Row index and row.
    std::string                     columnName;    // The name of a column, present at the top of the column
    int                             columnId;      // unique number to id column
    bool                            inUse;         // determine if column is in use
};

/*
    The CQL Table that holds all of your 
    columns which holds all of your rows

    Present inside of a CQLDocument.
*/
struct CQLTable
{
    std::string                                 tableName; // The name of the table
    std::unordered_map<std::string, CQLColumn*> columns;   // All columns in the table with the name as the key
    std::vector<CQLRow*>                        rows;      // Pointers to all rows in this column
};

/*
    Meta data for a cql document
*/
struct CQLMetaData
{
    time_t                        timeCreated; // The time this meta data was generated
    std::string                   fileName;    // The name of the file this meta data is about
    std::string                   filePath;    // The path of the file this meta data is about
    std::map<time_t, std::string> logs;        // All logs for the parent file. Can be saved to file

    void SetFilePath(std::string newFilePath)
    {
        // Check if a file is already made and has a path.
        if (!filePath.empty())
            printf("File Already Created at %s\n", filePath.c_str());
        else
            this->filePath = newFilePath;
    }

    void SetFileName(std::string newFileName)
    {
        // Already a named file. Make sure there not a path for it
        if (!fileName.empty() && !filePath.empty())
            printf("File Path and Name Already Filled Out. 'Name: %s' 'Path: %s'\n", fileName.c_str(), filePath.c_str());
        else
            this->fileName = newFileName;
    }

    /*
        Save all logs put in 'logs' map.
        Put all information into log file that is
        created from MakeLogFile()
    */
    void SaveLogs(std::string logFilePath)
    {
        std::fstream logFile(logFilePath); // File

        // Log file likely was not created, create the log file
        if (!logFile.is_open()) {
            std::string path = MakeLogFile(); // Path to log file
            if (path == "Error") // Bad file
                return;
            
            SaveLogs(path); // save logs on new log file that was created
            
            return;
        }

        // Put all logs from 'logs' into the log file
        // [time_logged] log_message
        for (auto log : logs)
            logFile << "[" << log.first << "] " << log.second << "\n";
    }

    /*
        Make a .log file where all contents of 'logs'
        are saved to.
        Default path is current directory.
    */
    std::string MakeLogFile()
    {
        // Path to save file to
        std::string path     = filePath;
        std::string fileName = fileName;

        // Full path with file name and file extension
        std::string  logFilePath = path + fileName + ".log";
        std::fstream file(logFilePath);

        // If an error did not occur, return the path to the file
        if (file.is_open()) {
            file.close();
            return logFilePath;
        }

        // Return the string 'Error' letting functions know the file isnt created
        return "Error";    
    }
};

/*
    A document containing a table which is a
    list of columns, each document has a name and id

    Use the CQL API to interact with this
*/
struct CQLDocument
{
    CQLMetaData documentMetadata; // Meta data for the current document
    CQLTable    savedTable;       // Saved table with information about columns and rows
    bool        inUse = false;    // If the current document is in use
    int         CQLTableID;       // Unique number to indentify the table

    /*
        Snapshots that can be used to restore
        data. Could take a lot of space so be weary
        when taking snapshots of your work.

        TODO: Implement a better way for document snapshots to be saved.
    */
    std::vector<CQLTable>    tableSnapshots;
    std::vector<CQLDocument> documentSnapshots;

    std::map<int, CQLColumn> columnHashmaps; // hashed by column id

    /*
        File path to save a cql document's data to
    */
    std::string cqlFilePath = "None";
      

    /*
        Write text to a map of logs formatted
        with the timestamp at the time of the log.
        Saved in the documents metadata
    */
    void Log(std::string text) 
    {
        documentMetadata.logs.insert(std::make_pair(time(NULL), text));
    }

    /*
        Make a text file with extension '.cql'.
        Where all table information from 'savedTable'-
        goes.
    */
    std::string MakeCQLFile(std::string CQLFileName="Untitled Document", std::string CQLFilePath="./")
    {
        // Path to save file to
        std::string path = CQLFilePath;
        std::string fileName = CQLFileName;

        documentMetadata.SetFileName(CQLFileName);
        documentMetadata.SetFilePath(CQLFilePath);

        // Full path with file name and file extension
        std::string fullFilePath = path + fileName + ".cql";

        // File object
        std::fstream file(fullFilePath);
        if (!file.is_open()) // Bad path or error
            return "Error";

        file.close(); // Make sure file is clsoed after opening
        this->cqlFilePath = fullFilePath;
        return this->cqlFilePath;
    }
};

/*
    CQL Data types that can be used 
    in columns or rows
*/
namespace CQLDataTypes
{
    // Numerica data types
    typedef int   CQL_INT;
    typedef long  CQL_BIG_INT;
    typedef float CQL_DECIMNAL;

    // String data types
    typedef std::string CQL_TEXT;

    // Boolean types
    typedef bool CQL_BOOLEAN;

    // TODO: BLOB, JSON, ARRAY
}

#endif