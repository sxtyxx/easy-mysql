new MySQL:connection;

public OnGameModeInit() {
    ezmysql_init();
    
    connection = ezmysql_connect("localhost", "root", "password", "database", "3306");
    
    if(ezmysql_is_connected(connection)) {
        ezmysql_create_table(connection, "usuarios", "id INT AUTO_INCREMENT PRIMARY KEY, nombre VARCHAR(32)");
    }
}

public OnGameModeExit() {
    ezmysql_shutdown();
}
