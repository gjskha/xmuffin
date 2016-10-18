#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

int main(int argc, char *argv[]) {

    if(argc != 3) {
        fprintf(stderr, "Usage: foo bar baz\n");
        return EXIT_FAILURE;
    }
    
    if(curl_global_init(CURL_GLOBAL_ALL)) {
        fprintf(stderr, "Fatal: The initialization of libcurl has failed.\n");
        return EXIT_FAILURE;
    }
    
    if(atexit(curl_global_cleanup)) {
        fprintf(stderr, "Fatal: atexit failed to register curl_global_cleanup.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    if(!getmsgcount()) {
        fprintf(stderr, "Fatal: getmsgcount failed.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}

int getmsgcount() {
    curl = curl_easy_init();
    if(!curl)  {
        fprintf(stderr, "Error: curl_easy_init failed.\n");
    }

    curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");

    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_IMAPS);


    curl_easy_setopt(curl, CURLOPT_USERNAME, username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
    curl_easy_setopt(curl, CURLOPT_URL, "imaps://imap.gmail.com:993/");

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "LIST \"\" *");

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);


    curl_easy_cleanup(curl);
    return 1;
}

