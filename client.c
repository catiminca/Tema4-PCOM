#include <arpa/inet.h>
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <unistd.h>     /* read, write, close */
#include "helpers.h"
#include "parson.h"
#include "requests.h"

char host[13] = "34.254.242.81";
int user_connected = 0;
char *user;
char **cookies = NULL;
int entered_lib = 0;
char *token = NULL;

void authentification_user(char buffer[100], int command, int sockfd) {
    char username[100], password[100];
    JSON_Value *value_json = json_value_init_object();
    JSON_Object *object_json = json_value_get_object(value_json);

    printf("username = ");
    scanf("%s", username);

    printf("password = ");
    scanf("%s", password);

    json_object_set_string(object_json, "username", username);
    json_object_set_string(object_json, "password", password);

    user = json_serialize_to_string_pretty(value_json);
    char *msg;
    char *response;
    if (command == 0) {
        if (user_connected == 0) {
            msg = compute_post_request("host", REGISTER, "application/json", &user, 1, NULL, 0, NULL);
            send_to_server(sockfd, msg);
            response = receive_from_server(sockfd);
            printf("%s\n", response);
            if (strstr(response, "is taken") != NULL) {
                printf("The %s is taken\n", username);
            } else {
                printf("New user added %s\n", username);
            }
            json_free_serialized_string(user);
        } else {
            printf("Can't register because user connected\n");
            return;
        }
    } else if (command == 1) {
        msg = compute_post_request("host", LOGIN, "application/json", &user, 1, NULL, 0, NULL);
        send_to_server(sockfd, msg);
        response = receive_from_server(sockfd);
        printf("%s\n", response);
        char *cookie = strstr(response, "Set-Cookie: ");
        if (cookie != NULL) {
            cookies = (char **)malloc(sizeof(char *));  
            char *aux_ptr = strtok(cookie, ";");
            aux_ptr = strstr(aux_ptr , "connect.sid");
            cookies[0] = aux_ptr;
            printf("User %s connected.\n", username);
            user_connected = 1;
        } else {
            printf("Failed login\n");
        }
        json_free_serialized_string(user);
    }
    free(msg);
    free(response);
}

void enter_library(int sockfd) {
    char *msg = compute_get_request(host, ACCESS, NULL, cookies, 1, NULL);
    send_to_server(sockfd, msg);
    char *response = receive_from_server(sockfd);
    printf("%s\n", response);
    if (strstr(response, "token") != NULL) {
        char* aux = strstr(response, "token");
        token = (char *)malloc(40960 * sizeof(char));
        token = aux + strlen("token") + 3;
        token[strlen(token) - 2] = '\0';
        printf("Entered with succes\n");
        entered_lib = 1;
    } else {
        printf("No acces to enter\n");
    }
    free(msg);
    free(response);
}

void logout_user(int sockfd) {
    char *msg = compute_get_request(host, LOGOUT, NULL, cookies, 1, NULL);
    send_to_server(sockfd, msg);
    char *response = receive_from_server(sockfd);
    printf("%s\n", response);
    user_connected = 0;
    entered_lib = 0;
    if (cookies != NULL) {
        free(cookies);
        cookies = NULL;
    }
    free(msg);
    free(response);
    printf("Logout succesfully\n");

}

void get_books(int sockfd) {
    if (entered_lib == 1) {
        char *msg = compute_get_request(host, BOOKS, NULL, cookies, 1, token);
        send_to_server(sockfd, msg);
        char *response = receive_from_server(sockfd);
        printf("%s\n", response);

        char *aux = strstr(response, "[");
        if (aux == NULL) {
            printf("Library empty\n");
        } else {
            printf("Books are %s\n", aux);
        }
    } else {
        printf("No access to lib - get_books\n");
    }
}

void get_book_details(int sockfd) {
    if (entered_lib == 1) {
        int id;
        char url_book[100];
        printf("id=");
        scanf("%d", &id);
        sprintf(url_book, "/api/v1/tema/library/books/%d", id);
        char *msg = compute_get_request(host, url_book, NULL, cookies, 1, token);
        send_to_server(sockfd, msg);
        char *response = receive_from_server(sockfd);
        printf("%s\n", response);
        char * aux = strstr(response, "{");
        if (strstr(response, "No book was found!")) {
            printf("No book was found at %d\n", id);
        } else {
            printf("Book at id %s\n", aux);
            return;
        }
        
    } else {
        printf("No access to lib - get_book\n");
    }
}

void add_book(int sockfd) {
    if (entered_lib == 1) {
        char *book_details;
        char title[1000], author[1000], publisher[1000], genre[1000], page_count[1000]; 
        JSON_Value *value_json = json_value_init_object();
        JSON_Object *object_json = json_value_get_object(value_json);
        printf("title = ");
        scanf("%s", title);

        printf("author = ");
        scanf("%s", author);

        printf("publisher = "); 
        scanf("%s", publisher);

        printf("genre = ");
        scanf("%s", genre);

        printf("page_count = ");
        scanf("%s", page_count);    

        json_object_set_string(object_json, "title", title);
        json_object_set_string(object_json, "author", author);
        json_object_set_string(object_json, "publisher", publisher);
        json_object_set_string(object_json, "genre", genre);
        json_object_set_number(object_json, "page_count", atoi(page_count));  

        book_details = json_serialize_to_string_pretty(value_json);
        printf("%s\n", book_details);
        char *msg = compute_post_request(host, BOOKS, "application/json", &book_details, 1, NULL, 0, token);
        send_to_server(sockfd, msg);
        char *response = receive_from_server(sockfd);
        printf("%s\n", response);

        printf("Book added successfully\n");
        json_free_serialized_string(book_details);
    } else {
        printf("No access to lib - add_book\n");
    }
}

void delete_book(int sockfd) {
    if (entered_lib == 1) {
        int id;
        char url_book[100];
        printf("id=");
        scanf("%d", &id);
        sprintf(url_book, "/api/v1/tema/library/books/%d", id);
        char *msg = delete_request(host, url_book, NULL, cookies, 1, token);
        send_to_server(sockfd, msg);
        char *response = receive_from_server(sockfd);
        printf("%s\n", response);
        if (strstr(response, "No book") == NULL) {
            printf("Book %d deleted\n", id);
        } else {
            printf("No book deleted\n");
        }
    } else {
        printf("No access to lib - delete_book\n");
    }
}

int main(int argc, char *argv[]) {
    int sockfd;
    char buffer[100];

    while(1) {
        sockfd = open_connection(host, 8080, AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            return -1;
        }
        scanf("%s", buffer);
        if (strcmp(buffer, "register") == 0) {
            authentification_user(buffer, 0, sockfd);
        } else if (strcmp(buffer, "login") == 0) {
            authentification_user(buffer, 1, sockfd);
        } else if (strcmp(buffer, "enter_library") == 0) {
            if (user_connected == 1) {
                enter_library(sockfd);
            } else {
                printf("User not connected-enter_library\n");
            }
        } else if (strcmp(buffer, "get_books") == 0) {
            get_books(sockfd);
        } else if (strcmp(buffer, "get_book") == 0) {
            get_book_details(sockfd);
        } else if (strcmp(buffer, "add_book") == 0) {
            add_book(sockfd);
        } else if (strcmp(buffer, "delete_book") == 0) {
            delete_book(sockfd);
        } else if (strcmp(buffer, "logout") == 0) {
            if (user_connected == 1) {
                logout_user(sockfd);
            } else {
                printf("User not connected-logout");
            }
        } else if (strcmp(buffer, "exit") == 0) {
            break;
        }
    }

    close_connection(sockfd);
    return 0;   
}