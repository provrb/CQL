#include "hdr/cql_api.h"

std::string     CQLAPI::RowDataToString( CQLRow row )
{
    // Data inside the row
    std::any rowData = row.rowData;
    std::string rowDataAsString = "";

    // Check the type id of rowData, convert it
    // if possible and return it as a string
    if (rowData.type() == typeid(CQLDataTypes::CQL_TEXT))
        rowDataAsString = std::any_cast<std::string>(rowData);
    else if (rowData.type() == typeid(CQLDataTypes::CQL_INT))
        rowDataAsString = std::to_string(std::any_cast<int>(rowData));
    else if (rowData.type() == typeid(CQLDataTypes::CQL_DECIMNAL))
        rowDataAsString = std::to_string(std::any_cast<float>(rowData));
    else if (rowData.type() == typeid(CQLDataTypes::CQL_BIG_INT))
        rowDataAsString = std::to_string(std::any_cast<long>(rowData));

    return rowDataAsString; // Default
}

CQLDocument     CQLAPI::MakeNewDocument( std::string CQLFileName, std::string CQLFilePath )
{
    printf("Making new document...\n");
    
    // Make a new CQLDocument struct
    CQLDocument newDocument;

    // Check if theres a cql document already in use
    if (this->CQLFile.inUse)
    {
        printf("Warning, Document already in use. Taking and saving snapshot...\n");
        
        // take snapshot of old document, put it in new document
        this->CQLFile.inUse = false; // set old document to not in use
        // Log changes
        this->CQLFile.Log("Document No Longer Being Used");

        newDocument.documentSnapshots.push_back(this->CQLFile);
    }

    // Fill out new document properties
    newDocument.inUse                        = true;  // Document is in use
    newDocument.CQLTableID                   = std::rand(); // Generate random table id
    newDocument.documentMetadata.timeCreated = time(NULL); // Set .timeCreated to current time

    // Set current document the one new one created
    // memcpy(&this->CQLFile, &newDocument, sizeof(newDocument));
    this->CQLFile = newDocument;
    this->CQLFile.MakeCQLFile(CQLFileName, CQLFilePath);
    
    printf("- New Document Made.\n");

    return this->CQLFile;
}

void            CQLAPI::ConnectToDatabase( std::string file ) 
{

}

void            CQLAPI::CreateTable( std::vector<CQLColumn> columns ) 
{

}


/*

    Functions to interact with columns and rows.

    Columns are stored in a hashmap and in a vector in a CQLTable struct.
    The columns are hashed by their unique column id which makes
    retreving information fast. After a column is no longer needed, it is erased.

    To remove and retrieve columns, either the name of a column or 
    the unique id of a column can be used as parameters to complete
    the operation.

    Whenever a column is inserted into the hashmap and current documents
    CQL table, a unique column id is automatically generated and the
    'inUse' field is set to true. Column id is generated using mt19937 <random>
    to provide a random number in a range from 0 - 10,000 + the size of the column hashmap.

*/

void CQLAPI::InsertColumn(CQLColumn column) 
{
    std::random_device seed;                           // seed generator
    std::mt19937 generator(seed());                      // random number generator with seed
    std::uniform_int_distribution<size_t> dist(1, 10000 + this->CQLFile.columnHashmaps.size()); // use generator to get random number, then make sure its in the range

    column.inUse = true;
    column.columnId = dist(generator);

    // Repeat if the column id has been found in the hasmap to avoid collisions
    while (this->CQLFile.columnHashmaps.find(column.columnId) != this->CQLFile.columnHashmaps.end())
        column.columnId = dist(generator);

    /*
        Add column to vector and map to access later on
    */
    this->CQLFile.savedTable.columns[column.columnName] = &column;
    this->CQLFile.columnHashmaps.insert(std::make_pair(column.columnId, column));
}

void CQLAPI::RemoveColumn(std::string columnName)
{
    CQLColumn* col = this->GetColumn(columnName);
    if (col->columnName == _BAD_COLUMN)
        return;

    /*
        Get column from saved table columns using 'columnName'
        if exists, remove it from hashmap using column id
        and remove it from saved table columns
    */
    this->CQLFile.columnHashmaps.erase(col->columnId);
    this->CQLFile.savedTable.columns.erase(col->columnName);
    printf("Removed column.\n");
}

void CQLAPI::RemoveColumn(int columnID)
{
    CQLColumn col = this->GetColumn(columnID);
    if (col.columnName == _BAD_COLUMN)
        return;

    /*
        Get column from hashmap using 'columnID'
        if exists, remove it from hasmap and saved table columns
    */
    this->CQLFile.columnHashmaps.erase(col.columnId);
    this->CQLFile.savedTable.columns.erase(col.columnName);
    printf("Removed column.\n");
}

CQLColumn* CQLAPI::GetColumn(std::string columnName)
{
    CQLColumn* col = {0};
    col->columnName = _BAD_COLUMN;
    try {
        col = this->CQLFile.savedTable.columns[columnName];
    }
    catch (std::out_of_range) {
        printf("Column '%s' not found.\n", columnName.c_str());
    }
    
    return col;
}

CQLColumn CQLAPI::GetColumn(int columnId)
{
    CQLColumn col = {};
    col.columnName = _BAD_COLUMN;
    try {
        col = this->CQLFile.columnHashmaps.at(columnId);
    }
    catch (std::out_of_range) {
        printf("Column not found.\n");
    }

    return col;
}

template <typename CQLType>
void CQLAPI::SetRowData(CQLType data, std::string columnName, int rowId)
{
    CQLColumn* col = this->GetColumn(columnName);
    if (col->columnName == _BAD_COLUMN)
        return;

    // Check if the row exists in the column
    if (col->rows.find(rowId) == col->rows.end()) // row DOES NOT exist
    {
        printf("Row %i does not exist.\n", rowId);
        return;
    }

    // set row data 
    col.rows.at(rowId) = std::make_any<CQLType>(data);
}

void CQLAPI::AddRow(std::string columnName, CQLRow row)
{
    CQLColumn* col = GetColumn(columnName);
    if (col->columnName == _BAD_COLUMN)
        return;
    
    // Get the index of the last row in column
    int lastExistingRowIndex = col->rows.end()->first;

    row.parentColumn = col;
    row.inUse = true;
    row.rowIndex = lastExistingRowIndex + 1; // row will be the next after the last one
    
    // Generate random row id
    std::random_device seed;
    std::mt19937 generator(seed());
    std::uniform_int_distribution<int> dist(0, 10000 + row.rowIndex);
    row.rowID = dist(generator);

    col->rows[row.rowIndex] = &row;
}

void CQLAPI::DeleteRow(std::string columnName, std::string rowName)
{
}

bool CQLAPI::RowDataIsNumeric(CQLRow row)
{
    // check if there is a value stored
    if (!row.rowData.has_value())
        return false;

    // Check if row.rowData type is a numeric type
    // Checks if its a: float, long, or int
    if (row.rowData.type() == typeid(CQLDataTypes::CQL_BIG_INT)
        || row.rowData.type() == typeid(CQLDataTypes::CQL_INT)
        || row.rowData.type() == typeid(CQLDataTypes::CQL_DECIMNAL)
        )
    {
        return true;
    }

    return false; // default
}

template <typename T>
void CQLAPI::DeleteRows(std::function<bool(T&)> condition)
{

}


/*

    Aggregate functions for rows or columns.

    These functions work by iterating through a columns rows,
    checking if its numeric, then performing the desired operation on them. I.e: sum.
    These aggregate functions supporting floating point numbers and return
    a double.

    Count returns the number of rows in a column. 
*/

size_t CQLAPI::Count(CQLColumn column)
{
    return column.rows.size();
}

double CQLAPI::Sum(CQLColumn column)
{
    double sum = 0;
    for (auto& index_row : column.rows)
    {
        // Check if the row data is an integar
        if (RowDataIsNumeric(*index_row.second))
            sum += std::any_cast<double>(index_row.second->rowData);
    }

    return sum;
} // return the sum of rowData if its numeri

double CQLAPI::Avg(CQLColumn column)
{
    // Firstly get the sum of all numeric values in the columns rows
    double sum = Sum(column);
    
    // Divide sum by the amount of rows there are
    // TODO: Use count() with condition if numeric
    long len = 0;
    
    for (auto& index_row : column.rows)
        if (RowDataIsNumeric(*index_row.second))
            len += 1;
    
    // sum / how many numeric values are in column
    return sum / len;
}

double CQLAPI::Max(CQLColumn column) 
{
    double max = 0;
    for (auto& index_row : column.rows)
    {
        // if row data isnt numeric than continue
        if (!RowDataIsNumeric(*index_row.second))
            continue;

        // Check if row.rowData is greater than max
        if (std::any_cast<double>(index_row.second->rowData) > max)
            max = std::any_cast<double>(index_row.second->rowData);
    }

    return max;
}

double CQLAPI::Min(CQLColumn column) 
{
    double min = 0;
    for (auto& index_row : column.rows)
    {
        // if row data is not numeric, continue
        if (!RowDataIsNumeric(*index_row.second))
            continue;

        // check if row.rowData is less than min
        if (std::any_cast<double>(index_row.second->rowData) < min)
            min = std::any_cast<double>(index_row.second->rowData);
    }

    return min;
}




JSON CQLAPI::ParseTableAsJson() // Parse columns and rows
{
    // Empty json table
    nlohmann::json tableInfo = {};

    /*
        Lambda to get row data from a row
        as a string.
    */
    auto GetRowData = [](const std::pair<int, CQLRow*> pair) -> std::string { return RowDataToString(*pair.second); };

    /*
        Insert a blank array with the column name as a key.
        After insert all a columns rows into the blank array
        using std::tranform. Use std::back_insertor to insert
        the string from GetRowData into the array created for the column
    */
    for (auto& keyValuePair : this->GetCQLTable().columns)
    {
        CQLColumn* col = keyValuePair.second;
        tableInfo[col->columnName] = nlohmann::json::array();
        std::transform(col->rows.cbegin(), col->rows.cend(), std::back_inserter(tableInfo[col->columnName]), GetRowData);
    }

    return tableInfo;
}

void CQLAPI::YieldIfFileInUse()
{

}

std::string CQLAPI::WriteTableToFile()
{
    // CQL file not made. Make a cql file.
    if (this->CQLFile.cqlFilePath == "None")
    {
        this->CQLFile.MakeCQLFile();
    }

    std::fstream file(this->CQLFile.cqlFilePath);
    
    if (!file.is_open())
        return "Error";

    // Write column headers
    for (auto& keyValuePair : this->GetCQLTable().columns)
    {
        std::string columnName = keyValuePair.first;
        file << std::setw(10) << columnName << "\t";
    }
    
    // Write rows
    for (CQLRow* row : this->GetCQLTable().rows)
        file << std::setw(10) << RowDataToString(*row) << "\t";

    file.close();
    return "Done";
}




/*
    File locking implementation.
    Works across different platforms.

    On linux it uses flock() and on windows
    it utilizes windows.h LockFilEx and UnlockFileEx
*/

/*
    Windows overload functions that allow the user
    to speciy a HANDLE to a file as a function parameter
    instead of passing the file's path as a string.
*/
#ifdef _WIN32

HANDLE FILE_LOCKING::GetFileHandle(std::string path) 
{
    // Get the handle of the file from the path.
    // Quickly read it if file exists.
    // If file exists than return null
    HANDLE fileHandle = CreateFile(
        path.c_str(),          // File path
        GENERIC_READ,          // Open for reading
        FILE_SHARE_READ,       // Allow other files to read
        NULL,                  // Default security attributes
        OPEN_EXISTING,         // Only open if file exists
        FILE_ATTRIBUTE_NORMAL, // Normal file attributes
        NULL                   // Empty template file
    );

    // Check if file handle is invalid. error occured
    if (fileHandle == INVALID_HANDLE_VALUE)
        return nullptr;

    return fileHandle;
}

bool FILE_LOCKING::LockFile(HANDLE file) 
{
    OVERLAPPED overlapped = { 0 };

    // Lock file using 'file' handle
    BOOL lockFileResult = LockFileEx(
        file,                                                // file handle to lock
        LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, // How to lock the file, what flags to set
        0,                                                   // reserved
        MAXDWORD,                                            // Lock all bytes low
        MAXDWORD,                                            // Lock all bytes high
        &overlapped                                          // Put other info about file into overlapped struct
    );

    if (lockFileResult) {
        printf("Locked File Successfully\n");
    } else {
        printf("LockFile Failed to Lock File\n");
    }
    
    CloseHandle(file);

    return lockFileResult;
}

bool FILE_LOCKING::UnlockFile(HANDLE file) 
{
    OVERLAPPED overlapped = { 0 };
    BOOL unlockFileResult = UnlockFileEx(
        file,
        0,
        MAXDWORD,
        MAXDWORD,
        &overlapped
    );
    
    if (unlockFileResult) {
        printf("Unlocked File Successfully\n");
    } else {
        printf("UnlockFile Failed to Unlock File\n");
    }

    CloseHandle(file);

    return unlockFileResult;
}

#endif

bool FILE_LOCKING::LockFile(std::string path) 
{
#ifdef _WIN32
    HANDLE file = FILE_LOCKING::GetFileHandle(path);
    if (file == nullptr) {
        printf("Error getting file handle. Check the file or file path.");
        return false;
    }

    return FILE_LOCKING::LockFile(file);
#elif __unix__
#endif
}

bool FILE_LOCKING::UnlockFile(std::string path) 
{
#ifdef _WIN32
    HANDLE file = FILE_LOCKING::GetFileHandle(path);
    if (file == nullptr) {
        printf("Error getting file handle. Check the file or file path.");
        return false;
    }

    return FILE_LOCKING::UnlockFile(file);
#elif __unix__
#endif

}

bool FILE_LOCKING::IsFileLocked(std::string path) 
{

#ifdef _WIN32
#elif __unix__
#endif
    return true;
}
