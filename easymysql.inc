#if defined _easymysql_included
    #endinput
#endif

#define _easymysql_included

#if !defined EZMYSQL_MAX_QUERY_BUFFER_SIZE
    #define EZMYSQL_MAX_QUERY_BUFFER_SIZE (4096)
#endif

#if !defined EZMYSQL_MAX_ERROR_BUFFER_SIZE
    #define EZMYSQL_MAX_ERROR_BUFFER_SIZE (1024)
#endif

#if !defined EZMYSQL_MAX_CACHE_SIZE
    #define EZMYSQL_MAX_CACHE_SIZE (8192)
#endif

static s_QueryBuffer[EZMYSQL_MAX_QUERY_BUFFER_SIZE];
static s_ErrorBuffer[EZMYSQL_MAX_ERROR_BUFFER_SIZE];
static s_CacheBuffer[EZMYSQL_MAX_CACHE_SIZE];

#if !defined EZMYSQL_MAX_CONNECTIONS
    #define EZMYSQL_MAX_CONNECTIONS (10)
#endif

#if !defined EZMYSQL_MAX_PASS_SIZE
    #define EZMYSQL_MAX_PASS_SIZE (64)
#endif

#if !defined EZMYSQL_MAX_DBNAME_SIZE
    #define EZMYSQL_MAX_DBNAME_SIZE (64)
#endif

#if !defined EZMYSQL_MAX_USER_SIZE
    #define EZMYSQL_MAX_USER_SIZE (64)
#endif

#if !defined EZMYSQL_MAX_HOST_SIZE
    #define EZMYSQL_MAX_HOST_SIZE (128)
#endif

#if !defined EZMYSQL_MAX_PORT_SIZE
    #define EZMYSQL_MAX_PORT_SIZE (6)
#endif

#if !defined EZMYSQL_AUTO_RECONNECT
    #define EZMYSQL_AUTO_RECONNECT (true)
#endif

#if !defined EZMYSQL_RECONNECT_ATTEMPTS
    #define EZMYSQL_RECONNECT_ATTEMPTS (5)
#endif

#if !defined EZMYSQL_RECONNECT_DELAY
    #define EZMYSQL_RECONNECT_DELAY (1000)
#endif

#if !defined EZMYSQL_CONNECTION_TIMEOUT
    #define EZMYSQL_CONNECTION_TIMEOUT (10000)
#endif

#if !defined EZMYSQL_QUERY_TIMEOUT
    #define EZMYSQL_QUERY_TIMEOUT (30000)
#endif

#if !defined EZMYSQL_DEBUG_MODE
    #define EZMYSQL_DEBUG_MODE (false)
#endif

enum EZMYSQL_QUERY_TYPE {
    EZMYSQL_QUERY_NONE,
    EZMYSQL_QUERY_INSERT,
    EZMYSQL_QUERY_UPDATE,
    EZMYSQL_QUERY_DELETE,
    EZMYSQL_QUERY_SELECT,
    EZMYSQL_QUERY_ALTER,
    EZMYSQL_QUERY_CREATE,
    EZMYSQL_QUERY_DROP,
    EZMYSQL_QUERY_CUSTOM
}

enum E_MYSQL_CONNECTION_DATA {
    MySQL:E_MYSQL_CONNECTION_HANDLE,
    E_MYSQL_CONNECTION_DBNAME[EZMYSQL_MAX_DBNAME_SIZE char],
    E_MYSQL_CONNECTION_PASSWORD[EZMYSQL_MAX_PASS_SIZE char],
    E_MYSQL_CONNECTION_USER[EZMYSQL_MAX_USER_SIZE char],
    E_MYSQL_CONNECTION_HOST[EZMYSQL_MAX_HOST_SIZE char],
    E_MYSQL_CONNECTION_PORT[EZMYSQL_MAX_PORT_SIZE char],
    bool:E_MYSQL_CONNECTION_ACTIVE,
    E_MYSQL_RECONNECT_ATTEMPTS,
    E_MYSQL_LAST_PING_TIME,
    E_MYSQL_TOTAL_QUERIES,
    E_MYSQL_FAILED_QUERIES,
    E_MYSQL_CONNECTION_TIME,
    bool:E_MYSQL_IN_TRANSACTION
}

enum E_MYSQL_QUERY_CACHE {
    E_MYSQL_QUERY_CACHE_ID,
    E_MYSQL_QUERY_CACHE_HANDLE,
    E_MYSQL_QUERY_CACHE_DATA[2048 char],
    E_MYSQL_QUERY_CACHE_TIMESTAMP,
    E_MYSQL_QUERY_CACHE_HITS
}

static s_ConnectionData[EZMYSQL_MAX_CONNECTIONS][E_MYSQL_CONNECTION_DATA];
static s_ConnectionCount = 0;
static s_QueryCache[32][E_MYSQL_QUERY_CACHE];
static s_CacheIndex = 0;
static s_GlobalQueryCounter = 0;
static bool:s_SystemInitialized = false;

stock ezmysql_init() {
    s_SystemInitialized = true;
    s_ConnectionCount = 0;
    s_CacheIndex = 0;
    s_GlobalQueryCounter = 0;
    
    for(new i = 0; i < EZMYSQL_MAX_CONNECTIONS; i++) {
        s_ConnectionData[i][E_MYSQL_CONNECTION_HANDLE] = MYSQL_INVALID_HANDLE;
        s_ConnectionData[i][E_MYSQL_CONNECTION_ACTIVE] = false;
        s_ConnectionData[i][E_MYSQL_RECONNECT_ATTEMPTS] = 0;
        s_ConnectionData[i][E_MYSQL_LAST_PING_TIME] = 0;
        s_ConnectionData[i][E_MYSQL_TOTAL_QUERIES] = 0;
        s_ConnectionData[i][E_MYSQL_FAILED_QUERIES] = 0;
        s_ConnectionData[i][E_MYSQL_CONNECTION_TIME] = 0;
        s_ConnectionData[i][E_MYSQL_IN_TRANSACTION] = false;
    }
    
    for(new i = 0; i < 32; i++) {
        s_QueryCache[i][E_MYSQL_QUERY_CACHE_ID] = -1;
        s_QueryCache[i][E_MYSQL_QUERY_CACHE_HITS] = 0;
        s_QueryCache[i][E_MYSQL_QUERY_CACHE_TIMESTAMP] = 0;
    }
    
    if(EZMYSQL_DEBUG_MODE) {
        printf("[EasyMySQL] Sistema inicializado correctamente");
    }
    
    return true;
}

stock ezmysql_shutdown() {
    for(new i = 0; i < EZMYSQL_MAX_CONNECTIONS; i++) {
        if(s_ConnectionData[i][E_MYSQL_CONNECTION_ACTIVE]) {
            ezmysql_close(MySQL:i);
        }
    }
    s_SystemInitialized = false;
    printf("[EasyMySQL] Sistema apagado correctamente");
}

stock MySQL:ezmysql_connect(const host[], const user[], const password[], const dbname[], const port[] = "3306", MySQLOpt:option_id = MySQLOpt:0) {
    if(!s_SystemInitialized) {
        printf("[EasyMySQL] ERROR: Sistema no inicializado. Llamar a ezmysql_init() primero");
        return MYSQL_INVALID_HANDLE;
    }

    if(s_ConnectionCount >= EZMYSQL_MAX_CONNECTIONS) {
        printf("[EasyMySQL] ERROR: Limite de conexiones alcanzado (%d/%d)", s_ConnectionCount, EZMYSQL_MAX_CONNECTIONS);
        return MYSQL_INVALID_HANDLE;
    }

    if(strlen(host) == 0 || strlen(user) == 0 || strlen(dbname) == 0) {
        printf("[EasyMySQL] ERROR: Parametros de conexion invalidos (host/user/dbname vacios)");
        return MYSQL_INVALID_HANDLE;
    }

    new MySQL:handle = mysql_connect(host, user, password, dbname, option_id);

    if(handle == MYSQL_INVALID_HANDLE) {
        printf("[EasyMySQL] ERROR: No se pudo obtener manejador de conexion");
        return MYSQL_INVALID_HANDLE;
    }

    if(mysql_errno(handle) != 0) {
        mysql_error(s_ErrorBuffer, .handle = handle);
        printf("[EasyMySQL] ERROR de conexion a %s@%s: %s", user, host, s_ErrorBuffer);
        mysql_close(handle);
        return MYSQL_INVALID_HANDLE;
    }

    new connection_index = _:handle;
    
    if(connection_index < 0 || connection_index >= EZMYSQL_MAX_CONNECTIONS) {
        printf("[EasyMySQL] ERROR: Indice de conexion fuera de rango");
        mysql_close(handle);
        return MYSQL_INVALID_HANDLE;
    }

    strpack(s_ConnectionData[connection_index][E_MYSQL_CONNECTION_DBNAME], dbname);
    strpack(s_ConnectionData[connection_index][E_MYSQL_CONNECTION_PASSWORD], password);
    strpack(s_ConnectionData[connection_index][E_MYSQL_CONNECTION_USER], user);
    strpack(s_ConnectionData[connection_index][E_MYSQL_CONNECTION_HOST], host);
    strpack(s_ConnectionData[connection_index][E_MYSQL_CONNECTION_PORT], port);
    s_ConnectionData[connection_index][E_MYSQL_CONNECTION_HANDLE] = handle;
    s_ConnectionData[connection_index][E_MYSQL_CONNECTION_ACTIVE] = true;
    s_ConnectionData[connection_index][E_MYSQL_RECONNECT_ATTEMPTS] = 0;
    s_ConnectionData[connection_index][E_MYSQL_LAST_PING_TIME] = GetTickCount();
    s_ConnectionData[connection_index][E_MYSQL_TOTAL_QUERIES] = 0;
    s_ConnectionData[connection_index][E_MYSQL_FAILED_QUERIES] = 0;
    s_ConnectionData[connection_index][E_MYSQL_CONNECTION_TIME] = GetTickCount();
    s_ConnectionData[connection_index][E_MYSQL_IN_TRANSACTION] = false;
    
    s_ConnectionCount++;
    
    if(EZMYSQL_DEBUG_MODE) {
        printf("[EasyMySQL] Conexion exitosa: %s@%s:%s (Handle: %d) [%d/%d]", user, host, port, _:handle, s_ConnectionCount, EZMYSQL_MAX_CONNECTIONS);
    }
    
    return handle;
}

stock bool:ezmysql_is_connected(MySQL:handle) {
    if(handle == MYSQL_INVALID_HANDLE) {
        return false;
    }

    new connection_index = _:handle;
    
    if(connection_index < 0 || connection_index >= EZMYSQL_MAX_CONNECTIONS) {
        return false;
    }
    
    if(!s_ConnectionData[connection_index][E_MYSQL_CONNECTION_ACTIVE]) {
        return false;
    }
    
    if(mysql_errno(handle) != 0) {
        s_ConnectionData[connection_index][E_MYSQL_CONNECTION_ACTIVE] = false;
        return false;
    }
    
    return true;
}

stock ezmysql_close(MySQL:handle) {
    if(handle == MYSQL_INVALID_HANDLE) {
        return;
    }

    new connection_index = _:handle;
    
    if(connection_index < 0 || connection_index >= EZMYSQL_MAX_CONNECTIONS) {
        return;
    }

    if(s_ConnectionData[connection_index][E_MYSQL_CONNECTION_IN_TRANSACTION]) {
        ezmysql_rollback(handle);
    }

    s_ConnectionData[connection_index][E_MYSQL_CONNECTION_ACTIVE] = false;
    
    mysql_close(handle);
    
    if(s_ConnectionCount > 0) {
        s_ConnectionCount--;
    }

    if(EZMYSQL_DEBUG_MODE) {
        printf("[EasyMySQL] Conexion cerrada (Handle: %d) [%d/%d conexiones activas]", connection_index, s_ConnectionCount, EZMYSQL_MAX_CONNECTIONS);
    }
}

stock bool:ezmysql_reconnect(MySQL:handle) {
    if(handle == MYSQL_INVALID_HANDLE) {
        return false;
    }

    new connection_index = _:handle;
    
    if(connection_index < 0 || connection_index >= EZMYSQL_MAX_CONNECTIONS) {
        return false;
    }

    new host[EZMYSQL_MAX_HOST_SIZE];
    new user[EZMYSQL_MAX_USER_SIZE];
    new password[EZMYSQL_MAX_PASS_SIZE];
    new dbname[EZMYSQL_MAX_DBNAME_SIZE];
    
    strunpack(host, s_ConnectionData[connection_index][E_MYSQL_CONNECTION_HOST]);
    strunpack(user, s_ConnectionData[connection_index][E_MYSQL_CONNECTION_USER]);
    strunpack(password, s_ConnectionData[connection_index][E_MYSQL_CONNECTION_PASSWORD]);
    strunpack(dbname, s_ConnectionData[connection_index][E_MYSQL_CONNECTION_DBNAME]);

    if(mysql_errno(handle) != 0) {
        if(s_ConnectionData[connection_index][E_MYSQL_RECONNECT_ATTEMPTS] < EZMYSQL_RECONNECT_ATTEMPTS) {
            s_ConnectionData[connection_index][E_MYSQL_RECONNECT_ATTEMPTS]++;
            
            if(EZMYSQL_DEBUG_MODE) {
                printf("[EasyMySQL] Reintentando conexion... Intento %d/%d para %s@%s", 
                    s_ConnectionData[connection_index][E_MYSQL_RECONNECT_ATTEMPTS], 
                    EZMYSQL_RECONNECT_ATTEMPTS, 
                    user, 
                    host);
            }
            
            mysql_error(s_ErrorBuffer, .handle = handle);
            printf("[EasyMySQL] Error: %s", s_ErrorBuffer);
            
            return false;
        } else {
            s_ConnectionData[connection_index][E_MYSQL_CONNECTION_ACTIVE] = false;
            printf("[EasyMySQL] ERROR: No se pudo reconectar despues de %d intentos a %s@%s", 
                EZMYSQL_RECONNECT_ATTEMPTS, user, host);
            return false;
        }
    }

    s_ConnectionData[connection_index][E_MYSQL_CONNECTION_ACTIVE] = true;
    s_ConnectionData[connection_index][E_MYSQL_RECONNECT_ATTEMPTS] = 0;
    
    if(EZMYSQL_DEBUG_MODE) {
        printf("[EasyMySQL] Reconexion exitosa: %s@%s", user, host);
    }
    
    return true;
}

stock bool:ezmysql_ping(MySQL:handle) {
    if(!ezmysql_is_connected(handle)) {
        return false;
    }

    new connection_index = _:handle;
    
    if(connection_index < 0 || connection_index >= EZMYSQL_MAX_CONNECTIONS) {
        return false;
    }

    mysql_query(handle, "SELECT 1", true);
    
    if(mysql_errno(handle) != 0) {
        s_ConnectionData[connection_index][E_MYSQL_CONNECTION_ACTIVE] = false;
        return false;
    }
    
    s_ConnectionData[connection_index][E_MYSQL_LAST_PING_TIME] = GetTickCount();
    
    return true;
}

stock ezmysql_get_error(MySQL:handle, error[], size = sizeof(error)) {
    if(handle == MYSQL_INVALID_HANDLE) {
        format(error, size, "Conexion invalida");
        return;
    }
    mysql_error(error, size, .handle = handle);
}

stock ezmysql_get_errno(MySQL:handle) {
    if(handle == MYSQL_INVALID_HANDLE) {
        return -1;
    }
    return mysql_errno(handle);
}

stock MySQL:ezmysql_get_handle(index = 0) {
    if(index < 0 || index >= s_ConnectionCount) {
        return MYSQL_INVALID_HANDLE;
    }
    return MySQL:index;
}

stock ezmysql_get_connection_count() {
    return s_ConnectionCount;
}

stock ezmysql_get_max_connections() {
    return EZMYSQL_MAX_CONNECTIONS;
}

stock bool:ezmysql_is_system_initialized() {
    return s_SystemInitialized;
}

stock bool:ezmysql_table_exists(MySQL:handle, const tableName[]) {
    if(!ezmysql_is_connected(handle)) {
        return false;
    }

    new connection_index = _:handle;
    
    if(connection_index < 0 || connection_index >= EZMYSQL_MAX_CONNECTIONS) {
        return false;
    }

    new dbname[EZMYSQL_MAX_DBNAME_SIZE];
    strunpack(dbname, s_ConnectionData[connection_index][E_MYSQL_CONNECTION_DBNAME]);

    mysql_format(handle, s_QueryBuffer, sizeof(s_QueryBuffer),
        "SELECT TABLE_NAME FROM information_schema.TABLES WHERE TABLE_SCHEMA = '%e' AND TABLE_NAME = '%e' LIMIT 1;",
        dbname, tableName);

    mysql_query(handle, s_QueryBuffer, true);
    
    new result = cache_num_rows() > 0;
    
    if(EZMYSQL_DEBUG_MODE) {
        printf("[EasyMySQL] Verificacion de tabla '%s': %s", tableName, result ? "EXISTE" : "NO EXISTE");
    }
    
    return result;
}

stock bool:ezmysql_column_exists(MySQL:handle, const tableName[], const columnName[]) {
    if(!ezmysql_is_connected(handle)) {
        return false;
    }

    new connection_index = _:handle;
    
    if(connection_index < 0 || connection_index >= EZMYSQL_MAX_CONNECTIONS) {
        return false;
    }

    new dbname[EZMYSQL_MAX_DBNAME_SIZE];
    strunpack(dbname, s_ConnectionData[connection_index][E_MYSQL_CONNECTION_DBNAME]);

    mysql_format(handle, s_QueryBuffer, sizeof(s_QueryBuffer),
        "SELECT COLUMN_NAME FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = '%e' AND TABLE_NAME = '%e' AND COLUMN_NAME = '%e' LIMIT 1;",
        dbname, tableName, columnName);

    mysql_query(handle, s_QueryBuffer, true);
    
    new result = cache_num_rows() > 0;
    
    if(EZMYSQL_DEBUG_MODE) {
        printf("[EasyMySQL] Verificacion de columna '%s' en tabla '%s': %s", columnName, tableName, result ? "EXISTE" : "NO EXISTE");
    }
    
    return result;
}

stock ezmysql_create_table(MySQL:handle, const tableName[], const tableStructure[] = "id INT AUTO_INCREMENT PRIMARY KEY, created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP") {
    if(!ezmysql_is_connected(handle)) {
        printf("[EasyMySQL] ERROR: Conexion inactiva para crear tabla '%s'", tableName);
        return;
    }

    new connection_index = _:handle;
    
    if(connection_index < 0 || connection_index >= EZMYSQL_MAX_CONNECTIONS) {
        return;
    }

    mysql_format(handle, s_QueryBuffer, sizeof(s_QueryBuffer),
        "CREATE TABLE IF NOT EXISTS `%s` (%s);", tableName, tableStructure);
    
    s_ConnectionData[connection_index][E_MYSQL_TOTAL_QUERIES]++;
    s_GlobalQueryCounter++;
    
    if(EZMYSQL_DEBUG_MODE) {
        printf("[EasyMySQL] Query #%d: CREATE TABLE `%s`", s_GlobalQueryCounter, tableName);
    }
    
    mysql_tquery(handle, s_QueryBuffer, "ezmysql_cb_create_table", "is", _:handle, tableName);
}

forward ezmysql_cb_create_table(MySQL:handle, const tableName[]);
public ezmysql_cb_create_table(MySQL:handle, const tableName[]) {
    new connection_index = _:handle;
    
    if(mysql_errno(handle) != 0) {
        mysql_error(s_ErrorBuffer, .handle = handle);
        printf("[EasyMySQL] ERROR: Fallo al crear tabla '%s': %s", tableName, s_ErrorBuffer);
        
        if(connection_index >= 0 && connection_index < EZMYSQL_MAX_CONNECTIONS) {
            s_ConnectionData[connection_index][E_MYSQL_FAILED_QUERIES]++;
        }
    } else {
        if(EZMYSQL_DEBUG_MODE) {
            printf("[EasyMySQL] Tabla '%s' creada/verificada correctamente", tableName);
        }
    }
}

stock ezmysql_drop_table(MySQL:handle, const tableName[]) {
    if(!ezmysql_is_connected(handle)) {
        printf("[EasyMySQL] ERROR: Conexion inactiva para eliminar tabla '%s'", tableName);
        return;
    }

    new connection_index = _:handle;
    
    if(connection_index < 0 || connection_index >= EZMYSQL_MAX_CONNECTIONS) {
        return;
    }

    mysql_format(handle, s_QueryBuffer, sizeof(s_QueryBuffer),
        "DROP TABLE IF EXISTS `%s`;", tableName);
    
    s_ConnectionData[connection_index][E_MYSQL_TOTAL_QUERIES]++;
    s_GlobalQueryCounter++;
    
    if(EZMYSQL_DEBUG_MODE) {
        printf("[EasyMySQL] Query #%d: DROP TABLE `%s`", s_GlobalQueryCounter, tableName);
    }
    
    mysql_tquery(handle, s_QueryBuffer, "ezmysql_cb_drop_table", "is", _:handle, tableName);
}

forward ezmysql_cb_drop_table(MySQL:handle, const tableName[]);
public ezmysql_cb_drop_table(MySQL:handle, const tableName[]) {
    new connection_index = _:handle;
    
    if(mysql_errno(handle) != 0) {
        mysql_error(s_ErrorBuffer, .handle = handle);
        printf("[EasyMySQL] ERROR: Fallo al eliminar tabla '%s': %s", tableName, s_ErrorBuffer);
        
        if(connection_index >= 0 && connection_index < EZMYSQL_MAX_CONNECTIONS) {
            s_ConnectionData[connection_index][E_MYSQL_FAILED_QUERIES]++;
        }
    } else {
        if(EZMYSQL_DEBUG_MODE) {
            printf("[EasyMySQL] Tabla '%s' eliminada correctamente", tableName);
        }
    }
}

stock ezmysql_add_column(MySQL:handle, const tableName[], const columnName[], const columnType[] = "VARCHAR(255)") {
    if(!ezmysql_is_connected(handle)) {
        printf("[EasyMySQL] ERROR: Conexion inactiva para agregar columna");
        return;
    }

    new connection_index = _:handle;
    
    if(connection_index < 0 || connection_index >= EZMYSQL_MAX_CONNECTIONS) {
        return;
    }

    if(ezmysql_column_exists(handle, tableName, columnName)) {
        if(EZMYSQL_DEBUG_MODE) {
            printf("[EasyMySQL] Columna '%s' ya existe en tabla '%s'", columnName, tableName);
        }
        return;
    }

    mysql_format(handle, s_QueryBuffer, sizeof(s_QueryBuffer),
        "ALTER TABLE `%s` ADD COLUMN `%s` %s;", tableName, columnName, columnType);

    s_ConnectionData[connection_index][E_MYSQL_TOTAL_QUERIES]++;
    s_GlobalQueryCounter++;
    
    if(EZMYSQL_DEBUG_MODE) {
        printf("[EasyMySQL] Query #%d: ALTER TABLE `%s` ADD COLUMN `%s` %s", s_GlobalQueryCounter, tableName, columnName, columnType);
    }

    mysql_tquery(handle, s_QueryBuffer, "ezmysql_cb_add_column", "iss", _:handle, tableName, columnName);
}

forward ezmysql_cb_add_column(MySQL:handle, const tableName[], const columnName[]);
public ezmysql_cb_add_column(MySQL:handle, const tableName[], const columnName[]) {
    new connection_index = _:handle;
    
    if(mysql_errno(handle) != 0) {
        mysql_error(s_ErrorBuffer, .handle = handle);
        printf("[EasyMySQL] ERROR: Fallo al agregar columna '%s' a tabla '%s': %s", columnName, tableName, s_ErrorBuffer);
        
        if(connection_index >= 0 && connection_index < EZMYSQL_MAX_CONNECTIONS) {
            s_ConnectionData[connection_index][E_MYSQL_FAILED_QUERIES]++;
        }
    } else {
        if(EZMYSQL_DEBUG_MODE) {
            printf("[EasyMySQL] Columna '%s' agregada a tabla '%s' correctamente", columnName, tableName);
        }
    }
}

stock ezmysql_remove_column(MySQL:handle, const tableName[], const columnName[]) {
    if(!ezmysql_is_connected(handle)) {
        printf("[EasyMySQL] ERROR: Conexion inactiva para eliminar columna");
        return;
    }

    new connection_index = _:handle;
    
    if(connection_index < 0 || connection_index >= EZMYSQL_MAX_CONNECTIONS) {
        return;
    }

    if(!ezmysql_column_exists(handle, tableName, columnName)) {
        if(EZMYSQL_DEBUG_MODE) {
            printf("[EasyMySQL] Columna '%s' no existe en tabla '%s'", columnName, tableName);
        }
        return;
    }

    mysql_format(handle, s_QueryBuffer, sizeof(s_QueryBuffer),
        "ALTER TABLE `%s` DROP COLUMN `%s`;", tableName, columnName);

    s_ConnectionData[connection_index][E_MYSQL_TOTAL_QUERIES]++;
    s_GlobalQueryCounter++;
    
    if(EZMYSQL_DEBUG_MODE) {
        printf("[EasyMySQL] Query #%d: ALTER TABLE `%s` DROP COLUMN `%s`", s_GlobalQueryCounter, tableName, columnName);
    }

    mysql_tquery(handle, s_QueryBuffer, "ezmysql_cb_remove_column", "iss", _:handle, tableName, columnName);
}

forward ezmysql_cb_remove_column(MySQL:handle, const tableName[], const columnName[]);
public ezmysql_cb_remove_column(MySQL:handle, const tableName[], const columnName[]) {
    new connection_index = _:handle;
    
    if(mysql_errno(handle) != 0) {
        mysql_error(s_ErrorBuffer, .handle = handle);
        printf("[EasyMySQL] ERROR: Fallo al eliminar columna '%s' de tabla '%s': %s", columnName, tableName, s_ErrorBuffer);
        
  
