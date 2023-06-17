#ifndef NVS_FUNCTIONS_H
#define NVS_FUNCTIONS_H

// Include the necessary header files


// Declare the function prototype for
// initialise -non volatile storage-
void init_nvs(void);
void store_string_in_nvs(char* variable_namespace, char* variable_name, char* variable_value);
void log_as_info_a_string_value_from_nvs(char* variable_namespace, char* variable_name);
char* get_string_value_from_nvs(char* variable_namespace, char* variable_name);

#endif // NVS_FUNCTIONS_H