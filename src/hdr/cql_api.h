#ifndef _CQL_API_H_
#define _CQL_API_H_

#include "data_structures.h" // CQL data structures
#include "json.h"            // Parsing

#include <functional> // for WHERE condition implementation
#include <stdlib.h>   // printing
#include <cstdlib>    // generate random numbers
#include <iomanip>    // std::setw
#include <random>     // Random numbers

#ifdef _WIN32
#include <Windows.h> // HANDLE, lock file functions
#endif

// ANCHOR - CQL API Implementation

constexpr char* _BAD_COLUMN = "INVALID_COLUMN"; // Column is invalid. Generally used when retreival of column fails

/*
    CQLAPI Class:

    API to interact with an SQL-like database system from scratch within a C++ environment.

    Core Functionalities:
    - Document and table management: creation, snapshots, and manipulation.
    - Column operations: addition, removal, updates, and aggregate functions (e.g., COUNT, SUM, AVG).
    - Database operations: connection, printing tables, and rollback functionalities.
    - Row-level operations: addition, deletion, and updates based on specified conditions.
    - Internal utilities for file handling and table parsing.

    Member Variables:
    - CQLFile: Represents the current CQL document.
    - columnMap: Hashmap storing columns indexed by their IDs.

    Note: This class serves as the fundamental engine for constructing a SQL-like database system from scratch in C++, 
    providing essential functionalities for managing documents, tables, columns, and rows.
*/

class CQLAPI
{
public:

    /*
        Make a new document to interact with
        Contains a table, meta data, etc.

        Add an optional name for the CQL table file to be called.
        Also choose a path to save the file. (Default is the current directory)
        When a new document is made it is automatically stored inside of the CQLAPI instance.
        You can operate on multiple documents by create multiple instances of CQLAPI
        and calling 'MakeNewDocument'
    */
    CQLDocument                     MakeNewDocument( std::string CQLFileName = "Untitled Document",
                                                     std::string CQLFilePath = "./" );

    /*
        Inline function implementations.
        Getters and setters for data snap shots,
        your cql table, document, and more.
    */
    inline void                     TakeTableSnapshot()                          { this->CQLFile.tableSnapshots.push_back(this->CQLFile.savedTable); }
    inline void                     TakeDocumentSnapshot()                       { this->CQLFile.documentSnapshots.push_back(this->CQLFile); }
    inline std::vector<CQLTable>    GetTableSnapshots()                          { return this->CQLFile.tableSnapshots; }
    inline std::vector<CQLDocument> GetDocumentSnapshots()                       { return this->CQLFile.documentSnapshots; }
    inline CQLTable                 GetCQLTable()                                { return this->CQLFile.savedTable; }
    inline void                     SetCurrentDocument( CQLDocument NewCQLFile ) { this->CQLFile = NewCQLFile; }
    inline void                     SetTableName( std::string name )             { this->CQLFile.savedTable.tableName = name; }
    
    /*
        CQL Column implementation.
        Functions used to interact with the columns
        in a CQL table.

        In 'GetColumn' if a column doesnt exist it will return
        an empty CQLColumn with 'columnName' as _BAD_COLUMN instead of
        return 'nullptr' because the different return types of each version
        of the function.
    */
    void                            RemoveColumn( std::string columnName );
    void                            RemoveColumn( int columnID );
    CQLColumn*                      GetColumn( std::string columnName );
    CQLColumn                       GetColumn( int columnId );
    void                            InsertColumn( CQLColumn column );

    /*
        Aggregate SQL-CQL equivalant functions.
        Count() - Return the number of rows in a column with an optional condition
        Sum() - Return the sum of a columns row data, if numeric
        Avg() - Return the average of a columns row data, if numeric
        Max() - Return the max value from a columns row data, if numeric
        Min() - Return the minimum value from a columns row data, if numeric
    */
    //template <typename T>
    //int                             Count( CQLColumn column, bool (*condition)(T&) );
    size_t                          Count( CQLColumn column );
    double                          Sum( CQLColumn column );
    double                          Avg( CQLColumn column );
    double                          Max( CQLColumn column );
    double                          Min( CQLColumn column );

    /*
        Important misc functions.
        
        ConnectToDatabase() - Open and connect to an existing .cql file
        CreateTable() - Create a blank new table
        RollbackTable() and RollbackDocument() - Rollback data to a snapshot passed in a
        parameter or the latest snapshot
        PrintTable() - Print out your table
        WriteTableToFile() - Write the contents of your table to the file path in the classes current 'CQLDocument' file path
        - NOTE: A file will be created if it does not exist
    */
    void                            ConnectToDatabase( std::string file );
    void                            CreateTable( std::vector<CQLColumn> columns );
    void                            RollbackTable();                          // Rollback table to latest snapshot
    void                            RollbackTable( CQLTable table ); 
    void                            RollbackDocument( CQLDocument document ); // make sure 'document' is in document snapshots
    void                            RollbackDocument();                       // Rollback document to latest snapshot
    void                            PrintTable();
    std::string                     WriteTableToFile();
    
    /*
        Functions to interact with the rows
        in a column from the column name, or to add a row.
    */
    template <typename T>
    void                            DeleteRows( std::function<bool(T&)> condition );
    template <typename CQLType>
    void                            SetRowData( CQLType data, std::string columnName, int rowId );
    void                            AddRow( std::string columnName, CQLRow row );
    void                            DeleteRow( std::string columnName, std::string rowName );
    bool                            RowDataIsNumeric( CQLRow row );  // Return a boolean value depending if the row.data is numeric
protected:
    void                            YieldIfFileInUse();              // Yield if the current file is in use. e.g wait until file is closed to log
    
    /*
        Parsing CQL and formatting it to text
        Taking data from a CQL table to make it readable
    */
    JSON                            ParseTableAsJson();                // Parse columns and rows as json table
    void                            ImportJsonTable( JSON jsonTable ); // Import json and use it as a table
    static std::string              RowDataToString( CQLRow row );     // Turn 'rowData' field of a row to a string

private:
    CQLDocument                        CQLFile;   // Current document functions will perform operations on
    std::unordered_map<int, CQLColumn> columnMap; // Column hashmap, hashed by col id
};


/*
    Functions to create, remove, and get file locks.

    This is useful if you have more than one query on the same file since
    you always want to make sure you are operating on the most recent
    version of the file. So the solution to this would be to lock the current
    file that you're working on, and unlock it after you're done saving it.
    Have any querys involving that file yield until the operation is complete
    and the file lock is gone.

    TODO: Fix cases a file is accidentally locked.
    This is an anonymous namespace to prevent a file being accidentally locked.
    These functions work across Windows and Linux platforms where Windows
    utlizes the fileapi.h 'LockFileEx', 'UnlockFileEx' and etc. On Linux
    'flock' is used with 'LOCK_EX' to lock the file. To unlock 'LOCK_UN' is used.
*/
namespace FILE_LOCKING 
{

    // WINDOWS FUNTIONS. USE A FILE HANDLE INSTEAD OF FILE PATH
#ifdef _WIN32
    HANDLE   GetFileHandle( std::string path );
    bool     LockFile( HANDLE file );
    bool     UnlockFile( HANDLE file );
#endif

    bool     LockFile( std::string path );
    bool     UnlockFile( std::string path );
    bool     IsFileLocked( std::string path );
};

#endif // _CQL_API_H_